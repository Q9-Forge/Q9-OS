/*
 * q9trace.c -- Implementierung, s. q9trace.h.
 */
#include "q9trace.h"

void Q9TraceFilterInit(Q9TraceFilter *f)
{
    unsigned i;
    for (i = 0; i < Q9TRACE_MASK_BYTES; i++) {
        f->syscallMask[i] = 0xff;   /* alle Syscalls an (Default: alles sehen) */
        f->internalMask[i] = 0x00;  /* interne Funktionen DEFAULT AUS -- Detailstufe steuert das zusaetzlich */
    }
    f->pid = 0;
    f->user = 0;
    f->path = 0xffff;
    f->fileId = 0;
    f->errorOnly = 0;
    f->detail = 1;
}

void Q9TraceMaskSet(Q9_u8 mask[Q9TRACE_MASK_BYTES], unsigned code)
{
    mask[code >> 3] = (Q9_u8)(mask[code >> 3] | (1u << (code & 7)));
}

void Q9TraceMaskClear(Q9_u8 mask[Q9TRACE_MASK_BYTES], unsigned code)
{
    mask[code >> 3] = (Q9_u8)(mask[code >> 3] & ~(1u << (code & 7)));
}

int Q9TraceMaskTest(const Q9_u8 mask[Q9TRACE_MASK_BYTES], unsigned code)
{
    return (mask[code >> 3] & (1u << (code & 7))) != 0;
}

int Q9TraceShouldLog(const Q9TraceFilter *f, const Q9TraceEvent *ev)
{
    int isIntern = (ev->recType == Q9TRACE_REC_INTERN_ENTRY ||
                    ev->recType == Q9TRACE_REC_INTERN_RETURN);

    /* Detailstufe: 1 = nur Code/Prozess/Ergebnis (keine internen
     * Funktionen ueberhaupt, Parameter spielen in Paket 2 noch keine
     * Rolle -- die Satzform traegt ohnehin noch keine Syscall-
     * Parameter, s. Kopfkommentar Q9TraceBuildRecord), 3 = + interne
     * Funktionen. Stufe 2 verhaelt sich in Paket 2 wie Stufe 1 (echte
     * Parameterkodierung kommt erst mit der Beschreibungstabelle,
     * Paket 4) -- kein stilles Weglassen, nur noch nicht umgesetzt. */
    if (isIntern && f->detail < 3)
        return 0;

    if (isIntern) {
        if (!Q9TraceMaskTest(f->internalMask, ev->code))
            return 0;
    } else {
        if (!Q9TraceMaskTest(f->syscallMask, ev->code))
            return 0;
    }

    if (f->pid != 0 && f->pid != ev->pid)
        return 0;

    if (f->user != 0 && f->user != ev->user)
        return 0;

    if (f->path != 0xffff && ev->path != 0xffff && f->path != ev->path)
        return 0;

    if (f->fileId != 0 && ev->fileId != 0 && f->fileId != ev->fileId)
        return 0;

    if (f->errorOnly) {
        /* "nur Fehler" haelt Rueckkehrsaetze mit Carry zurueck UND den
         * zugehoerigen Eintrittssatz" (Konzeptdokument 2.4) -- das
         * Zusammenhalten von Eintritt+Rueckkehr ist Sache des AUFRUFERS
         * (er kennt den Ausgang erst bei der Rueckkehr); diese Funktion
         * kann nur pro Satz entscheiden. Ein Eintrittssatz wird deshalb
         * HIER nie unterdrueckt -- der Aufrufer muss ihn zwischenhalten
         * (z.B. im letzten Satz je Tiefe) und erst beim Verwerfen der
         * zugehoerigen Rueckkehr auch ihn verwerfen. Rueckkehr-/intern-
         * Rueckkehr-Saetze ohne Carry werden hier direkt gefiltert. */
        if ((ev->recType == Q9TRACE_REC_RETURN ||
             ev->recType == Q9TRACE_REC_INTERN_RETURN) && !ev->carrySet)
            return 0;
    }

    return 1;
}

static Q9_u32 PutHeader(Q9_u8 *out, Q9_u8 recType, Q9_u8 recLen, Q9_u8 code,
                        Q9_u8 flags, Q9_u16 pid, Q9_u32 tick, Q9_u16 fineTime)
{
    out[0] = recType;
    out[1] = recLen;
    out[2] = code;
    out[3] = flags;
    out[4] = (Q9_u8)(pid >> 8);
    out[5] = (Q9_u8)(pid & 0xff);
    out[6] = (Q9_u8)(tick >> 24);
    out[7] = (Q9_u8)(tick >> 16);
    out[8] = (Q9_u8)(tick >> 8);
    out[9] = (Q9_u8)(tick & 0xff);
    out[10] = (Q9_u8)(fineTime >> 8);
    out[11] = (Q9_u8)(fineTime & 0xff);
    return Q9TRACE_HDR_LEN;
}

Q9_u32 Q9TraceBuildRecord(Q9_u8 *out, Q9_u8 recType, Q9_u8 code,
                           unsigned depth, int carrySet,
                           Q9_u16 pid, Q9_u32 tick, Q9_u16 fineTime,
                           const Q9_u8 *payload, Q9_u32 payloadLen)
{
    Q9_u8 flags;
    Q9_u32 maxPayload;
    Q9_u32 i;

    if (recType != Q9TRACE_REC_ENTRY && recType != Q9TRACE_REC_RETURN &&
        recType != Q9TRACE_REC_INTERN_ENTRY && recType != Q9TRACE_REC_INTERN_RETURN)
        return 0;

    flags = (Q9_u8)(depth & Q9TRACE_FLAG_DEPTH_MASK);
    if (carrySet)
        flags = (Q9_u8)(flags | Q9TRACE_FLAG_CARRY);

    maxPayload = Q9TRACE_MAX_REC - Q9TRACE_HDR_LEN;
    if (payloadLen > maxPayload) {
        payloadLen = maxPayload;
        flags = (Q9_u8)(flags | Q9TRACE_FLAG_TRUNC);
    }

    PutHeader(out, recType, (Q9_u8)(Q9TRACE_HDR_LEN + payloadLen), code,
              flags, pid, tick, fineTime);

    for (i = 0; i < payloadLen; i++)
        out[Q9TRACE_HDR_LEN + i] = payload[i];

    return Q9TRACE_HDR_LEN + payloadLen;
}

Q9_u32 Q9TraceBuildLostRecord(Q9_u8 *out, Q9_u16 pid, Q9_u32 tick,
                               Q9_u16 fineTime, Q9_u32 lostCount)
{
    PutHeader(out, Q9TRACE_REC_LOST, Q9TRACE_HDR_LEN + 4, 0, 0, pid, tick, fineTime);
    out[Q9TRACE_HDR_LEN + 0] = (Q9_u8)(lostCount >> 24);
    out[Q9TRACE_HDR_LEN + 1] = (Q9_u8)(lostCount >> 16);
    out[Q9TRACE_HDR_LEN + 2] = (Q9_u8)(lostCount >> 8);
    out[Q9TRACE_HDR_LEN + 3] = (Q9_u8)(lostCount & 0xff);
    return Q9TRACE_HDR_LEN + 4;
}

Q9_u32 Q9TraceBuildProcInfoRecord(Q9_u8 *out, Q9_u16 pid, Q9_u32 tick,
                                   Q9_u16 fineTime, Q9_u16 user)
{
    PutHeader(out, Q9TRACE_REC_PROCINFO, Q9TRACE_HDR_LEN + 2, 0, 0, pid, tick, fineTime);
    out[Q9TRACE_HDR_LEN + 0] = (Q9_u8)(user >> 8);
    out[Q9TRACE_HDR_LEN + 1] = (Q9_u8)(user & 0xff);
    return Q9TRACE_HDR_LEN + 2;
}

Q9_u32 Q9TraceBuildTimeBaseRecord(Q9_u8 *out, Q9_u32 tick, Q9_u16 fineTime,
                                   Q9_u32 timeRaw)
{
    PutHeader(out, Q9TRACE_REC_TIMEBASE, Q9TRACE_HDR_LEN + 4, 0, 0, 0, tick, fineTime);
    out[Q9TRACE_HDR_LEN + 0] = (Q9_u8)(timeRaw >> 24);
    out[Q9TRACE_HDR_LEN + 1] = (Q9_u8)(timeRaw >> 16);
    out[Q9TRACE_HDR_LEN + 2] = (Q9_u8)(timeRaw >> 8);
    out[Q9TRACE_HDR_LEN + 3] = (Q9_u8)(timeRaw & 0xff);
    return Q9TRACE_HDR_LEN + 4;
}

void Q9TraceDecodeHeader(const Q9_u8 *rec, Q9TraceRecHdr *hdr)
{
    hdr->recType = rec[0];
    hdr->recLen = rec[1];
    hdr->code = rec[2];
    hdr->flags = rec[3];
    hdr->pid = (Q9_u16)(((Q9_u16)rec[4] << 8) | rec[5]);
    hdr->tick = ((Q9_u32)rec[6] << 24) | ((Q9_u32)rec[7] << 16) |
                ((Q9_u32)rec[8] << 8) | (Q9_u32)rec[9];
    hdr->fineTime = (Q9_u16)(((Q9_u16)rec[10] << 8) | rec[11]);
}

Q9_u32 Q9TraceLostRecordFormatter(Q9_u8 *out, Q9_u32 lostCount)
{
    return Q9TraceBuildLostRecord(out, 0, 0, 0, lostCount);
}
