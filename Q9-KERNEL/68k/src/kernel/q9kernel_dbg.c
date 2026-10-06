/*
 * q9kernel_dbg.c -- F$Q9Dbg (Debug-Konzept, Arbeitspaket 2 Live-
 * Anbindung): Trace-Ringpuffer-Instanz, Filter, Eintritts-/Rueckkehr-
 * Haken, Konfigurations-Syscall.
 *
 * SPEICHERSTRATEGIE (Entwurfsentscheidung, docs/OWN_KERNEL_STATUS.md
 * Fortsetzung 115/117): der 64-KByte-Rohpuffer wird beim Booten per
 * Q9K_ProcSRqMem angefordert (derselbe, laengst bewaehrte Allokator wie
 * fuer jeden anderen Speicheranforderer) -- KEIN statisches C-Array
 * (nie durch diese Toolchain verifiziert, s. Fortsetzung 115). Die
 * Buchfuehrung (Lese-/Schreibzeiger, Zaehler, Filter, Trap-
 * Verschachtelungskontext) liegt in einzeln benannten, FESTEN
 * Adresszellen ab $2000 -- VERIFIZIERT frei: der letzte existierende
 * Eintrag vor dieser Aenderung (`Q9K_SysStartParam`, q9kernel_entry.a)
 * traegt selbst den Kommentar "naechste freie Adresse $2000"; die
 * Kernel-Globals-Region reicht laut q9kernel_cinit.c (Kommentar bei
 * der Arena-Verschiebung, Fortsetzung "Q9K_PROCDESC_SIZE 128->512")
 * bis $8000, bevor der Stack beginnt -- reichlich Abstand zu beiden
 * historisch dokumentierten Kollisionen (Arena, Stack). Dieselbe,
 * bereits ueberall im Kernel bewaehrte Technik (volatile Zugriff auf
 * eine feste Adresse ueber Q9K_GetU32/Q9K_PutU32 & Co.), keine neue
 * Speicherkategorie -- bewusst auch fuer die beiden 32-Byte-Filter-
 * masken, statt eines C-Arrays (genau dieselbe Technik wie das
 * bestehende `Q9K_MgrRoutineTable`, 19 x Q9_u32 an einer festen
 * Adresse, s. q9kernel_entry.a).
 */
#include "q9kernel_config.h"
#include "q9ringbuf.h"
#include "q9trace.h"
#include "q9dbg.h"

/* Nur im Developer-Kernel uebersetzt -- im Atom-Kernel bleibt diese
 * Uebersetzungseinheit leer (echte "keine Laufzeitkosten", nicht nur
 * unerreichbarer Code). q9ringbuf.c/q9trace.c selbst sind reine,
 * kernel-unabhaengige Logikbausteine ohne Q9K_DEBUG-Bezug und werden
 * bewusst immer mituebersetzt (s. build.sh) -- ungenutzt im
 * Atom-Kernel, aber harmlos. */
#ifdef Q9K_DEBUG

extern int Q9K_ProcSRqMem(Q9_u32 requestedSize, Q9_u32 *outAddr,
                           Q9_u32 *outSize, Q9_u16 *outError);

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_PutU32(Q9_u32 addr, Q9_u32 v) { *(volatile Q9_u32 *)addr = v; }
static Q9_u8  Q9K_GetU8(Q9_u32 addr) { return *(volatile Q9_u8 *)addr; }
static void   Q9K_PutU8(Q9_u32 addr, Q9_u8 v) { *(volatile Q9_u8 *)addr = v; }

/* Adresszellen, s. Kopfkommentar. Jede Zelle einzeln benannt und
 * dokumentiert, exakt im Stil der bestehenden q9kernel_entry.a-Zellen
 * (naechste freie Adresse am Ende vermerkt).
 *
 * Jede Zelle per #ifndef/#define ueberschreibbar -- exakt dieselbe
 * Technik wie Q9K_BITMAP_SCRATCH_* in q9kernel_bitmap.c & Co., damit
 * ein Hosttest sie vor dem #include in einen Testpuffer umleiten kann
 * (ein Dereferenzieren echter Hardware-Adressen wie $2000 wuerde auf
 * dem Host sonst abstuerzen). */
#ifndef Q9DBG_A_BUF_ADDR
#define Q9DBG_A_BUF_ADDR     0x2000UL /* Q9_u32, Adresse des per Q9K_ProcSRqMem reservierten Puffers (0 = keiner) */
#endif
#ifndef Q9DBG_A_BUF_SIZE
#define Q9DBG_A_BUF_SIZE     0x2004UL /* Q9_u32, tatsaechlich gewaehrte Groesse */
#endif
#ifndef Q9DBG_A_WRITE_OFF
#define Q9DBG_A_WRITE_OFF    0x2008UL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_READ_OFF
#define Q9DBG_A_READ_OFF     0x200CUL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_USED
#define Q9DBG_A_USED         0x2010UL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_WRITTEN
#define Q9DBG_A_WRITTEN      0x2014UL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_LOST
#define Q9DBG_A_LOST         0x2018UL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_MODE
#define Q9DBG_A_MODE         0x201CUL /* Q9_u32, nur unteres Byte benutzt */
#endif
#ifndef Q9DBG_A_ENABLED
#define Q9DBG_A_ENABLED      0x2020UL /* Q9_u32, 0/1 -- Trace an/aus */
#endif
#ifndef Q9DBG_A_OUTMODE
#define Q9DBG_A_OUTMODE      0x2024UL /* Q9_u32, Q9DBG_OUT_* */
#endif
#ifndef Q9DBG_A_SYSCALL_MASK
#define Q9DBG_A_SYSCALL_MASK 0x2028UL /* 32 Byte, bis 0x2047 */
#endif
#ifndef Q9DBG_A_INTERN_MASK
#define Q9DBG_A_INTERN_MASK  0x2048UL /* 32 Byte, bis 0x2067 */
#endif
#ifndef Q9DBG_A_FILT_PID
#define Q9DBG_A_FILT_PID     0x2068UL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_FILT_USER
#define Q9DBG_A_FILT_USER    0x206CUL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_FILT_PATH
#define Q9DBG_A_FILT_PATH    0x2070UL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_FILT_FILEID
#define Q9DBG_A_FILT_FILEID  0x2074UL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_FILT_ERRONLY
#define Q9DBG_A_FILT_ERRONLY 0x2078UL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_FILT_DETAIL
#define Q9DBG_A_FILT_DETAIL  0x207CUL /* Q9_u32 */
#endif
#ifndef Q9DBG_A_TRAP_DEPTH
#define Q9DBG_A_TRAP_DEPTH   0x2080UL /* Q9_u32, aktuelle Verschachtelungstiefe von Q9K_TrapDispatch */
#endif
#ifndef Q9DBG_A_TRAP_CODE
#define Q9DBG_A_TRAP_CODE    0x2084UL /* Q9DBG_TRAP_MAXDEPTH x Q9_u32, bis 0x20A3 */
#endif
#ifndef Q9DBG_A_TRAP_PID
#define Q9DBG_A_TRAP_PID     0x20A4UL /* Q9DBG_TRAP_MAXDEPTH x Q9_u32, bis 0x20C3 */
#endif
#ifndef Q9DBG_A_TRAP_TICK
#define Q9DBG_A_TRAP_TICK    0x20C4UL /* Q9DBG_TRAP_MAXDEPTH x Q9_u32, bis 0x20E3 */
#endif
/* naechste freie Adresse $20E4 */

#define Q9DBG_TRAP_MAXDEPTH 8
#define Q9DBG_BUF_REQUEST   65536UL

/* Bestehende Dispatcher-Zelle (q9kernel_entry.a, Q9K_TrapDispatch) --
 * traegt den aktuellen Callcode als Wort, unteres Byte bei +1
 * (big-endian). NICHT meine eigene Zelle, nur hier gelesen -- ebenfalls
 * ueberschreibbar fuer Hosttests. */
#ifndef Q9DBG_A_DISPATCH_CODE_LOW
#define Q9DBG_A_DISPATCH_CODE_LOW 0x1371UL
#endif

/* Von den Assembler-Stubs gesetzte Ein-/Ausgabezellen (gleiche
 * Technik wie Q9K_SendScratch_... / Q9K_RetPDScratch_... & Co. in
 * q9kernel_entry.a) -- je EINE Gruppe fuer die Eintritts-/Rueckkehr-
 * Haken und eine fuer F$Q9Dbg selbst. */
#ifndef Q9DBG_A_RET_CARRY
#define Q9DBG_A_RET_CARRY    0x20E4UL /* Q9_u32, vom Rueckkehr-Stub gesetzt: 0/1 */
#endif

#ifndef Q9DBG_A_SVC_FN
#define Q9DBG_A_SVC_FN       0x20E8UL /* Q9_u32, d0.w des F$Q9Dbg-Aufrufers */
#endif
#ifndef Q9DBG_A_SVC_D1
#define Q9DBG_A_SVC_D1       0x20ECUL
#endif
#ifndef Q9DBG_A_SVC_D2
#define Q9DBG_A_SVC_D2       0x20F0UL
#endif
#ifndef Q9DBG_A_SVC_D3
#define Q9DBG_A_SVC_D3       0x20F4UL
#endif
#ifndef Q9DBG_A_SVC_A0
#define Q9DBG_A_SVC_A0       0x20F8UL /* Zeigerwert als Q9_u32 */
#endif
#ifndef Q9DBG_A_SVC_OUT_D0
#define Q9DBG_A_SVC_OUT_D0   0x20FCUL
#endif
#ifndef Q9DBG_A_SVC_OUT_D1
#define Q9DBG_A_SVC_OUT_D1   0x2100UL
#endif
#ifndef Q9DBG_A_SVC_OUT_D2
#define Q9DBG_A_SVC_OUT_D2   0x2104UL
#endif
#ifndef Q9DBG_A_SVC_OUT_ERR
#define Q9DBG_A_SVC_OUT_ERR  0x2108UL /* 0 = Erfolg, sonst Fehlercode */
#endif
/* naechste freie Adresse $210C */

/* ---------- Hilfsfunktionen: Ringpuffer/Filter aus den Zellen bauen ---------- */

static Q9_u32 LostFormatterBridge(Q9_u8 *out, Q9_u32 lostCount)
{
    return Q9TraceLostRecordFormatter(out, lostCount);
}

static void LoadRingBuf(Q9RingBuf *rb)
{
    rb->data = (Q9_u8 *)(unsigned long)Q9K_GetU32(Q9DBG_A_BUF_ADDR);
    rb->size = Q9K_GetU32(Q9DBG_A_BUF_SIZE);
    rb->writeOff = Q9K_GetU32(Q9DBG_A_WRITE_OFF);
    rb->readOff = Q9K_GetU32(Q9DBG_A_READ_OFF);
    rb->used = Q9K_GetU32(Q9DBG_A_USED);
    rb->writtenCount = Q9K_GetU32(Q9DBG_A_WRITTEN);
    rb->lostCount = Q9K_GetU32(Q9DBG_A_LOST);
    rb->mode = (Q9_u8)Q9K_GetU32(Q9DBG_A_MODE);
    rb->id = Q9DBG_RING_TRACE;
    rb->name[0] = 't'; rb->name[1] = 'r'; rb->name[2] = 'a';
    rb->name[3] = 'c'; rb->name[4] = 'e'; rb->name[5] = '\0';
    rb->lostFmt = LostFormatterBridge;
}

static void StoreRingBuf(const Q9RingBuf *rb)
{
    Q9K_PutU32(Q9DBG_A_WRITE_OFF, rb->writeOff);
    Q9K_PutU32(Q9DBG_A_READ_OFF, rb->readOff);
    Q9K_PutU32(Q9DBG_A_USED, rb->used);
    Q9K_PutU32(Q9DBG_A_WRITTEN, rb->writtenCount);
    Q9K_PutU32(Q9DBG_A_LOST, rb->lostCount);
}

static void LoadFilter(Q9TraceFilter *f)
{
    int i;
    for (i = 0; i < Q9TRACE_MASK_BYTES; i++)
        f->syscallMask[i] = Q9K_GetU8(Q9DBG_A_SYSCALL_MASK + (Q9_u32)i);
    for (i = 0; i < Q9TRACE_MASK_BYTES; i++)
        f->internalMask[i] = Q9K_GetU8(Q9DBG_A_INTERN_MASK + (Q9_u32)i);
    f->pid = (Q9_u16)Q9K_GetU32(Q9DBG_A_FILT_PID);
    f->user = (Q9_u16)Q9K_GetU32(Q9DBG_A_FILT_USER);
    f->path = (Q9_u16)Q9K_GetU32(Q9DBG_A_FILT_PATH);
    f->fileId = Q9K_GetU32(Q9DBG_A_FILT_FILEID);
    f->errorOnly = (Q9_u8)Q9K_GetU32(Q9DBG_A_FILT_ERRONLY);
    f->detail = (Q9_u8)Q9K_GetU32(Q9DBG_A_FILT_DETAIL);
}

static void StoreFilter(const Q9TraceFilter *f)
{
    int i;
    for (i = 0; i < Q9TRACE_MASK_BYTES; i++)
        Q9K_PutU8(Q9DBG_A_SYSCALL_MASK + (Q9_u32)i, f->syscallMask[i]);
    for (i = 0; i < Q9TRACE_MASK_BYTES; i++)
        Q9K_PutU8(Q9DBG_A_INTERN_MASK + (Q9_u32)i, f->internalMask[i]);
    Q9K_PutU32(Q9DBG_A_FILT_PID, f->pid);
    Q9K_PutU32(Q9DBG_A_FILT_USER, f->user);
    Q9K_PutU32(Q9DBG_A_FILT_PATH, f->path);
    Q9K_PutU32(Q9DBG_A_FILT_FILEID, f->fileId);
    Q9K_PutU32(Q9DBG_A_FILT_ERRONLY, f->errorOnly);
    Q9K_PutU32(Q9DBG_A_FILT_DETAIL, f->detail);
}

/* ---------- Boot-Initialisierung ---------- */

/* Von q9kernel_cinit.c NACH Q9K_ArenaInit() aufgerufen (Abschnitt 2,
 * Punkt 3) -- der Allokator muss bereits laufen. Boot-Abbruch bei
 * Fehlschlag waere falsch: das Debug-Feature ist optional, ein voller
 * Arena fuehrt nur zu einem wirkungslosen Trace (Puffergroesse 0, ueber
 * Unterfunktion 0 erkennbar), nicht zu einem kaputten Kernel. */
void Q9K_DbgInit(void)
{
    Q9_u32 addr = 0, size = 0;
    Q9_u16 err = 0;
    Q9TraceFilter f;

    if (Q9K_ProcSRqMem(Q9DBG_BUF_REQUEST, &addr, &size, &err)) {
        Q9K_PutU32(Q9DBG_A_BUF_ADDR, addr);
        Q9K_PutU32(Q9DBG_A_BUF_SIZE, size);
    } else {
        Q9K_PutU32(Q9DBG_A_BUF_ADDR, 0);
        Q9K_PutU32(Q9DBG_A_BUF_SIZE, 0);
    }

    Q9K_PutU32(Q9DBG_A_WRITE_OFF, 0);
    Q9K_PutU32(Q9DBG_A_READ_OFF, 0);
    Q9K_PutU32(Q9DBG_A_USED, 0);
    Q9K_PutU32(Q9DBG_A_WRITTEN, 0);
    Q9K_PutU32(Q9DBG_A_LOST, 0);
    Q9K_PutU32(Q9DBG_A_MODE, Q9RB_MODE_OVERWRITE);
    Q9K_PutU32(Q9DBG_A_ENABLED, 0); /* Standard AUS, Konzeptdokument 2.5 Unterfunktion 1 */
    Q9K_PutU32(Q9DBG_A_OUTMODE, Q9DBG_OUT_RINGBUF);

    Q9TraceFilterInit(&f);
    StoreFilter(&f);

    Q9K_PutU32(Q9DBG_A_TRAP_DEPTH, 0);
}

/* ---------- Eintritts-/Rueckkehr-Haken (aus Q9K_TrapDispatch/
 * Q9K_TrapAfterCall per bsr aufgerufen, s. q9kernel_entry.a) ---------- */

/* Arbeitsbereich der Haken (Fortsetzung 119). Die Haken laufen auf dem
 * Stack des AUFRUFENDEN Prozesses, oft tief verschachtelt (F$Fork ->
 * F$Load -> IOMan -> RBF). Die gut 200 Byte lokaler Strukturen liessen dort
 * sysgos kleinen Stack ueberlaufen: Q9TraceMaskTest kehrte nach Adresse 2
 * zurueck (gemessen, Start mit Q9K_TRACEBOOT=1). Auf dem Ziel liegt der
 * Bereich deshalb fest bei $2200 (512 Byte, hinter den Buchfuehrungszellen),
 * auf dem Host als statisches Feld. Die Haken sperren waehrenddessen die
 * Interrupts (q9kernel_entry.a), damit kein zweiter Prozess ihn mitbenutzt. */
typedef struct Q9DbgScratch {
    Q9RingBuf     rb;
    Q9TraceFilter f;
    Q9TraceEvent  ev;
    Q9_u8         rec[Q9TRACE_MAX_REC];
} Q9DbgScratch;
typedef char Q9DbgScratchFits[(sizeof(Q9DbgScratch) <= 0x200) ? 1 : -1];
#if defined(_OSK)
#define Q9DBG_SCRATCH ((Q9DbgScratch *)0x2200UL)
#else
static Q9DbgScratch g_q9dbgScratch;
#define Q9DBG_SCRATCH (&g_q9dbgScratch)
#endif

static void LogCommon(Q9_u8 recType, Q9_u8 code, unsigned depth,
                       int carrySet, Q9_u16 pid, Q9_u32 tick)
{
    Q9DbgScratch *w = Q9DBG_SCRATCH;
    Q9_u32 recLen;

    if (Q9K_GetU32(Q9DBG_A_ENABLED) == 0)
        return;
    if (Q9K_GetU32(Q9DBG_A_BUF_ADDR) == 0)
        return; /* Boot-Allokation schlug fehl -- Trace bleibt wirkungslos */

    LoadFilter(&w->f);
    w->ev.recType = recType;
    w->ev.code = code;
    w->ev.pid = pid;
    w->ev.user = 0;       /* Benutzer-Tracking ist Paket 4 (Syscall-Beschreibungstabelle) */
    w->ev.path = 0xffff;  /* dito Pfadnummer -- noch keine Quelle dafuer verdrahtet */
    w->ev.fileId = 0;
    w->ev.isReturn = (recType == Q9TRACE_REC_RETURN || recType == Q9TRACE_REC_INTERN_RETURN);
    w->ev.carrySet = carrySet;

    if (!Q9TraceShouldLog(&w->f, &w->ev))
        return;

    recLen = Q9TraceBuildRecord(w->rec, recType, code, depth, carrySet,
                                 pid, tick, 0, 0, 0);

    LoadRingBuf(&w->rb);
    Q9RingBufWrite(&w->rb, w->rec, recLen);
    StoreRingBuf(&w->rb);
}

/* Eintritt: vom Assembler-Stub direkt nach dem Ermitteln des Callcodes
 * aufgerufen (Q9K_TrapDispatch, s. dortigen Kopfkommentar der
 * Einfuegestelle) -- noch VOR der Handler-Suche, Callcode liest diese
 * Funktion selbst aus der bestehenden Scratchzelle $1370 (vom
 * Dispatcher dort bereits abgelegt, zu diesem Zeitpunkt garantiert
 * aktuell -- s. Kopfkommentar der Einfuegestelle in q9kernel_entry.a). */
#ifndef Q9DBG_A_TICKS
#define Q9DBG_A_TICKS        0x1BE0UL /* Q9K_TickCount, Timer-Interrupts seit Boot */
#endif
#define Q9DBG_SLOT(d) ((d) * (Q9_u32)sizeof(Q9_u32))

/* Fortsetzung 119: aktueller Prozessdeskriptor (D_Proc) fuer P$ID und die
 * prozesseigene Verschachtelungstiefe $3AC (s. Q9K_TrapExtInvoke). Fuer
 * Hosttests umlenkbar; ein Wert 0 heisst "unbekannt" (keine Korrektur). */
#ifndef Q9DBG_A_CUR_PROC
#define Q9DBG_A_CUR_PROC     0x004CUL /* Q9_D_Proc */
#endif
#define Q9DBG_PD_ID          0x00UL   /* P$ID, Wort */
#define Q9DBG_CALLCODE_SELF  0x7FU    /* F$Q9Dbg -- wird nie protokolliert (Konzept 2.5) */

/* Fortsetzung 119, zweite Fassung: Tiefe und Callcode-Stapel PRO PROZESS
 * im Deskriptor (Byte $3F0 = Tiefe, $3F1-$3F8 = Callcodes je Tiefe; in
 * F$Fork-Slots genullt, s. Q9K_ProcPoolAlloc). Der globale Zaehler war
 * falsch, sobald ein Prozess mitten im Aufruf wechselte: F$Sleep/F$Wait/
 * F$Exit/F$Chain/F$RTE/F$NProc kehren nie ueber den Rueckkehr-Haken zurueck
 * und erhoehen die Tiefe deshalb nicht; jeder andere Aufruf kommt im
 * SELBEN Prozess zurueck, in dem er begann. Live gemessen vorher: nach
 * F$Fork->F$Load erreichte der globale Zaehler 8, danach blieb der Trace
 * fuer den Rest des Starts stumm. Ohne bekannten Prozess (D_Proc = 0, frueher
 * Boot, Hosttests) gilt die globale Fassung weiter. */
#define Q9DBG_PD_DEPTH       0x3F0UL  /* Byte */
#define Q9DBG_PD_CODES       0x3F1UL  /* Q9DBG_TRAP_MAXDEPTH Byte */

static int Q9K_DbgNoReturn(Q9_u8 code)
{
    return code == 0x04 || code == 0x05 || code == 0x06 || code == 0x0A ||
           code == 0x1E || code == 0x2D;   /* Wait, Chain, Exit, Sleep, RTE, NProc */
}

static Q9_u16 Q9K_DbgPid(Q9_u32 cur)
{
    return (Q9_u16)((Q9K_GetU8(cur + Q9DBG_PD_ID) << 8) | Q9K_GetU8(cur + Q9DBG_PD_ID + 1));
}

void Q9K_DbgLogEntryImpl(void)
{
    Q9_u8 code = Q9K_GetU8(Q9DBG_A_DISPATCH_CODE_LOW); /* ($1370).w, unteres Byte */
    Q9_u32 cur = Q9K_GetU32(Q9DBG_A_CUR_PROC);
    Q9_u32 depth;

    if (cur != 0) {
        Q9_u16 pid = Q9K_DbgPid(cur);
        depth = Q9K_GetU8(cur + Q9DBG_PD_DEPTH);
        if (depth >= Q9DBG_TRAP_MAXDEPTH)
            depth = Q9DBG_TRAP_MAXDEPTH - 1;   /* gegen Altlasten: nie verstummen */
        if (code != Q9DBG_CALLCODE_SELF)
            LogCommon(Q9TRACE_REC_ENTRY, code, (unsigned)depth, 0, pid, Q9K_GetU32(Q9DBG_A_TICKS));
        if (!Q9K_DbgNoReturn(code) && depth + 1 < Q9DBG_TRAP_MAXDEPTH) {
            Q9K_PutU8(cur + Q9DBG_PD_CODES + depth, code);
            Q9K_PutU8(cur + Q9DBG_PD_DEPTH, (Q9_u8)(depth + 1));
        }
        return;
    }

    depth = Q9K_GetU32(Q9DBG_A_TRAP_DEPTH);
    if (depth < Q9DBG_TRAP_MAXDEPTH) {
        Q9K_PutU32(Q9DBG_A_TRAP_CODE + Q9DBG_SLOT(depth), code);
        Q9K_PutU32(Q9DBG_A_TRAP_PID + Q9DBG_SLOT(depth), 0);
        Q9K_PutU32(Q9DBG_A_TRAP_TICK + Q9DBG_SLOT(depth), 0);
        if (code != Q9DBG_CALLCODE_SELF)
            LogCommon(Q9TRACE_REC_ENTRY, code, (unsigned)depth, 0, 0, Q9K_GetU32(Q9DBG_A_TICKS));
    }
    Q9K_PutU32(Q9DBG_A_TRAP_DEPTH, depth + 1);
}

void Q9K_DbgLogReturnImpl(void)
{
    int carrySet = (int)Q9K_GetU32(Q9DBG_A_RET_CARRY);
    Q9_u32 cur = Q9K_GetU32(Q9DBG_A_CUR_PROC);
    Q9_u32 depth;

    if (cur != 0) {
        Q9_u8 code;
        depth = Q9K_GetU8(cur + Q9DBG_PD_DEPTH);
        if (depth == 0 || depth > Q9DBG_TRAP_MAXDEPTH)
            return;
        depth--;
        Q9K_PutU8(cur + Q9DBG_PD_DEPTH, (Q9_u8)depth);
        code = Q9K_GetU8(cur + Q9DBG_PD_CODES + depth);
        if (code != Q9DBG_CALLCODE_SELF)
            LogCommon(Q9TRACE_REC_RETURN, code, (unsigned)depth, carrySet, Q9K_DbgPid(cur),
                      Q9K_GetU32(Q9DBG_A_TICKS));
        return;
    }

    depth = Q9K_GetU32(Q9DBG_A_TRAP_DEPTH);
    if (depth == 0)
        return; /* unbalanciert -- sicherheitshalber kein Unterlauf */
    depth--;
    Q9K_PutU32(Q9DBG_A_TRAP_DEPTH, depth);
    if (depth < Q9DBG_TRAP_MAXDEPTH) {
        Q9_u8 code = (Q9_u8)Q9K_GetU32(Q9DBG_A_TRAP_CODE + Q9DBG_SLOT(depth));
        if (code != Q9DBG_CALLCODE_SELF)
            LogCommon(Q9TRACE_REC_RETURN, code, (unsigned)depth, carrySet, 0, Q9K_GetU32(Q9DBG_A_TICKS));
    }
}

/* ---------- F$Q9Dbg ($7F) ---------- */

/* Vom Assembler-Stub aufgerufen, nachdem er d0-d3/a0 in die
 * Q9DBG_A_SVC_*-Zellen gelegt hat (gleiche Technik wie
 * Q9K_SysSRqMemImpl & Co.). Setzt Q9DBG_A_SVC_OUT_* und meldet per
 * Rueckgabewert Erfolg/Fehlschlag (Stub setzt daraus Carry + d1). */
int Q9K_SysFQ9DbgImpl(void)
{
    Q9_u32 fn = Q9K_GetU32(Q9DBG_A_SVC_FN);
    Q9_u32 d1 = Q9K_GetU32(Q9DBG_A_SVC_D1);
    Q9_u32 d2 = Q9K_GetU32(Q9DBG_A_SVC_D2);
    Q9_u32 d3 = Q9K_GetU32(Q9DBG_A_SVC_D3);
    Q9_u32 a0 = Q9K_GetU32(Q9DBG_A_SVC_A0);
    Q9RingBuf rb;
    Q9TraceFilter f;

    Q9K_PutU32(Q9DBG_A_SVC_OUT_D0, 0);
    Q9K_PutU32(Q9DBG_A_SVC_OUT_D1, 0);
    Q9K_PutU32(Q9DBG_A_SVC_OUT_D2, 0);

    switch (fn) {
    case Q9DBG_FN_VERSION:
        LoadRingBuf(&rb);
        Q9K_PutU32(Q9DBG_A_SVC_OUT_D0, 1);       /* Version 1 */
        Q9K_PutU32(Q9DBG_A_SVC_OUT_D1, rb.size); /* Puffergroesse (0 = Allokation schlug fehl) */
        Q9K_PutU32(Q9DBG_A_SVC_OUT_D2, rb.writtenCount);
        return 1;

    case Q9DBG_FN_TRACE_ONOFF:
        Q9K_PutU32(Q9DBG_A_ENABLED, (d1 != 0) ? 1 : 0);
        return 1;

    case Q9DBG_FN_MASK_SET:
        {
            int i;
            for (i = 0; i < Q9TRACE_MASK_BYTES; i++)
                Q9K_PutU8(Q9DBG_A_SYSCALL_MASK + (Q9_u32)i,
                          *(volatile Q9_u8 *)(a0 + (Q9_u32)i));
        }
        return 1;

    case Q9DBG_FN_MASK_GET:
        {
            int i;
            for (i = 0; i < Q9TRACE_MASK_BYTES; i++)
                *(volatile Q9_u8 *)(a0 + (Q9_u32)i) =
                    Q9K_GetU8(Q9DBG_A_SYSCALL_MASK + (Q9_u32)i);
        }
        return 1;

    case Q9DBG_FN_FILTER_SET:
        LoadFilter(&f);
        switch (d1) {
        case Q9DBG_FILTER_PID:       f.pid = (Q9_u16)d2; break;
        case Q9DBG_FILTER_USER:      f.user = (Q9_u16)d2; break;
        case Q9DBG_FILTER_PATH:      f.path = (Q9_u16)d2; break;
        case Q9DBG_FILTER_FILEID:    f.fileId = d2; break;
        case Q9DBG_FILTER_ERRORONLY: f.errorOnly = (Q9_u8)(d2 != 0); break;
        case Q9DBG_FILTER_DETAIL:    f.detail = (Q9_u8)d2; break;
        default:
            Q9K_PutU32(Q9DBG_A_SVC_OUT_ERR, 1);
            return 0;
        }
        StoreFilter(&f);
        return 1;

    case Q9DBG_FN_LEVEL_SET:
        LoadFilter(&f);
        f.detail = (Q9_u8)d1;
        StoreFilter(&f);
        Q9K_PutU32(Q9DBG_A_OUTMODE, d2);
        return 1;

    case Q9DBG_FN_RING_READ:
        if (d3 != Q9DBG_RING_TRACE) {
            Q9K_PutU32(Q9DBG_A_SVC_OUT_ERR, 1);
            return 0;
        }
        {
            Q9_u32 lost = 0;
            Q9_u32 n;
            LoadRingBuf(&rb);
            n = Q9RingBufRead(&rb, (Q9_u8 *)(unsigned long)a0, d2, &lost);
            StoreRingBuf(&rb);
            Q9K_PutU32(Q9DBG_A_SVC_OUT_D0, n);
            Q9K_PutU32(Q9DBG_A_SVC_OUT_D1, rb.readOff);
            Q9K_PutU32(Q9DBG_A_SVC_OUT_D2, lost);
        }
        return 1;

    case Q9DBG_FN_RING_CLEAR:
        if (d3 != Q9DBG_RING_TRACE) {
            Q9K_PutU32(Q9DBG_A_SVC_OUT_ERR, 1);
            return 0;
        }
        LoadRingBuf(&rb);
        Q9RingBufClear(&rb);
        StoreRingBuf(&rb);
        return 1;

    case Q9DBG_FN_IFUNC_MASK:
        {
            int i;
            for (i = 0; i < Q9TRACE_MASK_BYTES; i++)
                Q9K_PutU8(Q9DBG_A_INTERN_MASK + (Q9_u32)i,
                          *(volatile Q9_u8 *)(a0 + (Q9_u32)i));
        }
        return 1;

    default:
        Q9K_PutU32(Q9DBG_A_SVC_OUT_ERR, 1);
        return 0;
    }
}

/* ---------- Q9K_TRACE_FN: interne Funktionsverfolgung ---------- */

/* Minimal-Umsetzung des in Abschnitt 2.1/2.5 vorgesehenen Makros fuer
 * interne Kernfunktionen -- bewusst NICHT an Dutzenden Stellen verteilt
 * (das waere Paket 4-Umfang und ungetestbar ohne die
 * Syscall-/Funktions-Beschreibungstabelle), sondern als funktionierender
 * Mechanismus bereitgestellt und an EINER Stelle demonstriert/verifiziert
 * (s. Q9K_DbgTraceFnDemoEntry/-Return unten, aus einem Hosttest
 * aufgerufen). Weitere Instrumentierung ist eine reine Ein-Zeilen-
 * Ergaenzung je Funktion, kein Umbau. */
void Q9K_DbgTraceFnEntry(Q9_u8 funcId, Q9_u32 v1, Q9_u32 v2)
{
    Q9_u32 depth = Q9K_GetU32(Q9DBG_A_TRAP_DEPTH);
    Q9_u8 payload[8];
    (void)v1; (void)v2; /* Paket 2: nur Funktions-ID + Tiefe, volle Werte sind Paket 4 */
    LogCommon(Q9TRACE_REC_INTERN_ENTRY, funcId,
              (unsigned)(depth > 0 ? depth - 1 : 0), 0, 0, 0);
    (void)payload;
}

void Q9K_DbgTraceFnReturn(Q9_u8 funcId)
{
    Q9_u32 depth = Q9K_GetU32(Q9DBG_A_TRAP_DEPTH);
    LogCommon(Q9TRACE_REC_INTERN_RETURN, funcId,
              (unsigned)(depth > 0 ? depth - 1 : 0), 0, 0, 0);
}

#endif /* Q9K_DEBUG */
