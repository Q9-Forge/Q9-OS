/*
 * test_q9kernel_dbg.c -- Hosttests fuer den Ringpuffer-Baustein
 * (q9ringbuf.c) und das Trace-Satzformat/die Filterlogik (q9trace.c),
 * Debug-Konzept Arbeitspaket 2 (docs/DEBUG_KONZEPT_de.md Abschnitt 2).
 *
 *   gcc -Wall -Wextra -o test_dbg test_q9kernel_dbg.c && ./test_dbg
 */

#include <stdio.h>
#include <string.h>

#include "q9ringbuf.c"
#include "q9trace.c"

static int failures;

static void check(const char *label, unsigned long got, unsigned long want)
{
    if (got == want) {
        printf("[OK]   %-62s = %lu\n", label, got);
    } else {
        printf("[FAIL] %-62s = %lu (erwartet %lu)\n", label, got, want);
        failures++;
    }
}

static void checkInt(const char *label, int got, int want)
{
    check(label, (unsigned long)got, (unsigned long)want);
}

/* ---------- Ringpuffer ---------- */

static void test_ringbuf_basic(void)
{
    Q9RingBuf rb;
    Q9_u8 mem[64];
    Q9_u8 rec[12] = { 1, 12, 0x03, 0, 0, 7, 0, 0, 0, 100, 0, 5 };
    Q9_u8 out[64];
    Q9_u32 n;

    Q9RingBufInit(&rb, mem, sizeof mem, 0, "trace", Q9RB_MODE_OVERWRITE);
    checkInt("ringbuf: init used=0", (int)rb.used, 0);

    checkInt("ringbuf: write ok", Q9RingBufWrite(&rb, rec, sizeof rec), 1);
    check("ringbuf: used nach einem Satz", rb.used, sizeof rec);
    check("ringbuf: writtenCount", rb.writtenCount, 1);

    n = Q9RingBufRead(&rb, out, sizeof out, 0);
    check("ringbuf: gelesene Byte", n, sizeof rec);
    checkInt("ringbuf: Inhalt stimmt", memcmp(out, rec, sizeof rec) == 0, 1);
    check("ringbuf: used nach Lesen=0", rb.used, 0);
}

static void test_ringbuf_wrap_overwrite(void)
{
    Q9RingBuf rb;
    Q9_u8 mem[20]; /* passt genau fuer zwei 12-Byte-Saetze NICHT -- erzwingt Ueberschreiben */
    Q9_u8 recA[12] = { 1, 12, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0 };
    Q9_u8 recB[12] = { 1, 12, 2, 0, 0, 2, 0, 0, 0, 2, 0, 0 };
    Q9_u8 recC[12] = { 1, 12, 3, 0, 0, 3, 0, 0, 0, 3, 0, 0 };
    Q9_u8 out[64];
    Q9_u32 lost = 0;
    Q9TraceRecHdr hdr;

    Q9RingBufInit(&rb, mem, sizeof mem, 0, "trace", Q9RB_MODE_OVERWRITE);
    Q9RingBufWrite(&rb, recA, sizeof recA);
    Q9RingBufWrite(&rb, recB, sizeof recB); /* 24 Byte noetig, Puffer hat 20 -> recA faellt weg */
    check("ringbuf overwrite: lostCount nach zweitem Schreiben", rb.lostCount, 1);

    Q9RingBufWrite(&rb, recC, sizeof recC);
    check("ringbuf overwrite: lostCount nach drittem Schreiben", rb.lostCount, 2);

    Q9RingBufRead(&rb, out, sizeof out, &lost);
    check("ringbuf overwrite: Lesestand meldet verlorene Saetze", lost, 2);
    Q9TraceDecodeHeader(out, &hdr);
    check("ringbuf overwrite: aeltester verbliebener Satz ist C (nicht A/B)", hdr.code, 3);
}

static void test_ringbuf_halt_mode(void)
{
    Q9RingBuf rb;
    Q9_u8 mem[20];
    Q9_u8 recA[12] = { 1, 12, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0 };
    Q9_u8 recB[12] = { 1, 12, 2, 0, 0, 2, 0, 0, 0, 2, 0, 0 };

    Q9RingBufInit(&rb, mem, sizeof mem, 0, "trace", Q9RB_MODE_HALT);
    checkInt("ringbuf halt: erster Satz wird angenommen", Q9RingBufWrite(&rb, recA, sizeof recA), 1);
    checkInt("ringbuf halt: zweiter Satz (zu gross) wird verworfen", Q9RingBufWrite(&rb, recB, sizeof recB), 0);
    check("ringbuf halt: lostCount zaehlt den Verwurf", rb.lostCount, 1);
    check("ringbuf halt: writtenCount bleibt bei 1", rb.writtenCount, 1);
}

static void test_ringbuf_wraparound_offset(void)
{
    Q9RingBuf rb;
    Q9_u8 mem[16];
    Q9_u8 rec[8] = { 1, 8, 9, 0, 0, 9, 0, 0 };
    Q9_u8 out[64];
    int i;

    Q9RingBufInit(&rb, mem, sizeof mem, 0, "trace", Q9RB_MODE_OVERWRITE);
    /* mehrfach schreiben/lesen, damit write/readOff tatsaechlich ueber
     * die Puffergrenze hinaus wandern (reiner Zirkularitaetstest). */
    for (i = 0; i < 5; i++) {
        Q9RingBufWrite(&rb, rec, sizeof rec);
        Q9RingBufRead(&rb, out, sizeof out, 0);
    }
    checkInt("ringbuf wraparound: nach mehreren Runden intakt", memcmp(out, rec, sizeof rec) == 0, 1);
    check("ringbuf wraparound: writtenCount=5", rb.writtenCount, 5);
}

/* ---------- Satzformat ---------- */

static void test_record_roundtrip(void)
{
    Q9_u8 buf[Q9TRACE_MAX_REC];
    Q9_u8 payload[2] = { 0xAA, 0xBB };
    Q9_u32 len;
    Q9TraceRecHdr hdr;

    len = Q9TraceBuildRecord(buf, Q9TRACE_REC_ENTRY, 0x03, 2, 0,
                              7, 0x00000064, 0x0005, payload, sizeof payload);
    check("record: Laenge = Kopf+Nutzdaten", len, Q9TRACE_HDR_LEN + 2);

    Q9TraceDecodeHeader(buf, &hdr);
    checkInt("record: Satztyp", hdr.recType, Q9TRACE_REC_ENTRY);
    check("record: Satzlaenge im Kopf", hdr.recLen, len);
    checkInt("record: Callcode", hdr.code, 0x03);
    checkInt("record: Tiefe in Flags", hdr.flags & Q9TRACE_FLAG_DEPTH_MASK, 2);
    checkInt("record: Carry nicht gesetzt", (hdr.flags & Q9TRACE_FLAG_CARRY) != 0, 0);
    check("record: Prozess-ID", hdr.pid, 7);
    check("record: Tick-Zaehler", hdr.tick, 0x64);
    check("record: Feinzeit", hdr.fineTime, 5);
    checkInt("record: Nutzdaten Byte 0", buf[Q9TRACE_HDR_LEN + 0], 0xAA);
    checkInt("record: Nutzdaten Byte 1", buf[Q9TRACE_HDR_LEN + 1], 0xBB);
}

static void test_record_return_carry(void)
{
    Q9_u8 buf[Q9TRACE_MAX_REC];
    Q9TraceRecHdr hdr;

    Q9TraceBuildRecord(buf, Q9TRACE_REC_RETURN, 0x03, 0, 1, 7, 1, 0, 0, 0);
    Q9TraceDecodeHeader(buf, &hdr);
    checkInt("record return: Carry-Bit gesetzt", (hdr.flags & Q9TRACE_FLAG_CARRY) != 0, 1);
}

static void test_record_truncation(void)
{
    Q9_u8 buf[Q9TRACE_MAX_REC];
    Q9_u8 bigPayload[Q9TRACE_MAX_REC]; /* garantiert zu gross */
    Q9_u32 len;
    Q9TraceRecHdr hdr;

    memset(bigPayload, 0x7e, sizeof bigPayload);
    len = Q9TraceBuildRecord(buf, Q9TRACE_REC_ENTRY, 1, 0, 0, 1, 0, 0,
                              bigPayload, sizeof bigPayload);
    check("record truncation: Laenge gekappt auf MAX_REC", len, Q9TRACE_MAX_REC);
    Q9TraceDecodeHeader(buf, &hdr);
    checkInt("record truncation: Bit 5 gesetzt", (hdr.flags & Q9TRACE_FLAG_TRUNC) != 0, 1);
}

static void test_record_lost_procinfo_timebase(void)
{
    Q9_u8 buf[Q9TRACE_MAX_REC];
    Q9_u32 len;
    Q9TraceRecHdr hdr;

    len = Q9TraceBuildLostRecord(buf, 0, 42, 1, 3);
    check("lost: Laenge", len, Q9TRACE_HDR_LEN + 4);
    Q9TraceDecodeHeader(buf, &hdr);
    checkInt("lost: Satztyp", hdr.recType, Q9TRACE_REC_LOST);
    check("lost: Anzahl im Nutzdatenfeld", ((Q9_u32)buf[12] << 24) | ((Q9_u32)buf[13] << 16) |
          ((Q9_u32)buf[14] << 8) | buf[15], 3);

    len = Q9TraceBuildProcInfoRecord(buf, 9, 1, 0, 0x0105);
    check("procinfo: Laenge", len, Q9TRACE_HDR_LEN + 2);
    Q9TraceDecodeHeader(buf, &hdr);
    checkInt("procinfo: Satztyp", hdr.recType, Q9TRACE_REC_PROCINFO);
    check("procinfo: Prozess-ID", hdr.pid, 9);
    check("procinfo: Benutzer gepackt", ((Q9_u16)buf[12] << 8) | buf[13], 0x0105);

    len = Q9TraceBuildTimeBaseRecord(buf, 1000, 7, 0x12345678UL);
    check("timebase: Laenge", len, Q9TRACE_HDR_LEN + 4);
    Q9TraceDecodeHeader(buf, &hdr);
    checkInt("timebase: Satztyp", hdr.recType, Q9TRACE_REC_TIMEBASE);
    check("timebase: Tick", hdr.tick, 1000);
}

/* ---------- Filter ---------- */

static void test_filter_default_allows_all(void)
{
    Q9TraceFilter f;
    Q9TraceEvent ev;

    Q9TraceFilterInit(&f);
    memset(&ev, 0, sizeof ev);
    ev.recType = Q9TRACE_REC_ENTRY;
    ev.code = 0x03;
    ev.pid = 7;
    ev.path = 0xffff;

    checkInt("filter default: Syscall-Eintritt erlaubt", Q9TraceShouldLog(&f, &ev), 1);
}

static void test_filter_syscall_mask(void)
{
    Q9TraceFilter f;
    Q9TraceEvent ev;

    Q9TraceFilterInit(&f);
    Q9TraceMaskClear(f.syscallMask, 0x03);
    memset(&ev, 0, sizeof ev);
    ev.recType = Q9TRACE_REC_ENTRY;
    ev.code = 0x03;
    ev.path = 0xffff;

    checkInt("filter mask: ausgeblendeter Callcode wird unterdrueckt", Q9TraceShouldLog(&f, &ev), 0);

    ev.code = 0x04;
    checkInt("filter mask: anderer Callcode bleibt erlaubt", Q9TraceShouldLog(&f, &ev), 1);
}

static void test_filter_pid_user_path(void)
{
    Q9TraceFilter f;
    Q9TraceEvent ev;

    Q9TraceFilterInit(&f);
    f.pid = 5;
    memset(&ev, 0, sizeof ev);
    ev.recType = Q9TRACE_REC_ENTRY;
    ev.code = 1;
    ev.path = 0xffff;
    ev.pid = 7;
    checkInt("filter pid: falscher Prozess unterdrueckt", Q9TraceShouldLog(&f, &ev), 0);
    ev.pid = 5;
    checkInt("filter pid: richtiger Prozess erlaubt", Q9TraceShouldLog(&f, &ev), 1);

    Q9TraceFilterInit(&f);
    f.user = 0x0005;
    ev.pid = 0;
    ev.user = 0x0007;
    checkInt("filter user: falscher Benutzer unterdrueckt", Q9TraceShouldLog(&f, &ev), 0);
    ev.user = 0x0005;
    checkInt("filter user: richtiger Benutzer erlaubt", Q9TraceShouldLog(&f, &ev), 1);

    Q9TraceFilterInit(&f);
    f.path = 3;
    ev.user = 0;
    ev.path = 4;
    checkInt("filter path: falscher Pfad unterdrueckt", Q9TraceShouldLog(&f, &ev), 0);
    ev.path = 3;
    checkInt("filter path: richtiger Pfad erlaubt", Q9TraceShouldLog(&f, &ev), 1);
    ev.path = 0xffff; /* kein Pfad bekannt (z.B. interner Satz) -> Filter greift nicht */
    checkInt("filter path: unbekannter Pfad wird nicht gefiltert", Q9TraceShouldLog(&f, &ev), 1);
}

static void test_filter_error_only(void)
{
    Q9TraceFilter f;
    Q9TraceEvent ev;

    Q9TraceFilterInit(&f);
    f.errorOnly = 1;
    memset(&ev, 0, sizeof ev);
    ev.path = 0xffff;

    ev.recType = Q9TRACE_REC_RETURN;
    ev.carrySet = 0;
    checkInt("filter error-only: erfolgreiche Rueckkehr unterdrueckt", Q9TraceShouldLog(&f, &ev), 0);
    ev.carrySet = 1;
    checkInt("filter error-only: fehlerhafte Rueckkehr erlaubt", Q9TraceShouldLog(&f, &ev), 1);

    ev.recType = Q9TRACE_REC_ENTRY;
    checkInt("filter error-only: Eintrittssatz selbst nie unterdrueckt", Q9TraceShouldLog(&f, &ev), 1);
}

static void test_filter_detail_level(void)
{
    Q9TraceFilter f;
    Q9TraceEvent ev;

    Q9TraceFilterInit(&f);
    Q9TraceMaskSet(f.internalMask, 5); /* Maske erlaubt es, Detailstufe entscheidet trotzdem */
    memset(&ev, 0, sizeof ev);
    ev.recType = Q9TRACE_REC_INTERN_ENTRY;
    ev.code = 5;
    ev.path = 0xffff;

    f.detail = 1;
    checkInt("filter detail 1: interne Funktion unterdrueckt", Q9TraceShouldLog(&f, &ev), 0);
    f.detail = 2;
    checkInt("filter detail 2: interne Funktion weiterhin unterdrueckt", Q9TraceShouldLog(&f, &ev), 0);
    f.detail = 3;
    checkInt("filter detail 3: interne Funktion erlaubt", Q9TraceShouldLog(&f, &ev), 1);
}

static void test_mask_helpers(void)
{
    Q9_u8 mask[Q9TRACE_MASK_BYTES];
    memset(mask, 0, sizeof mask);
    checkInt("mask: frisch genullt -> Bit 0x7f aus", Q9TraceMaskTest(mask, 0x7f), 0);
    Q9TraceMaskSet(mask, 0x7f);
    checkInt("mask: nach Set -> Bit 0x7f an", Q9TraceMaskTest(mask, 0x7f), 1);
    checkInt("mask: Nachbarbit 0x7e unberuehrt", Q9TraceMaskTest(mask, 0x7e), 0);
    Q9TraceMaskClear(mask, 0x7f);
    checkInt("mask: nach Clear -> Bit 0x7f wieder aus", Q9TraceMaskTest(mask, 0x7f), 0);
    Q9TraceMaskSet(mask, 255);
    checkInt("mask: hoechster Callcode (255) erreichbar", Q9TraceMaskTest(mask, 255), 1);
}

int main(void)
{
    test_ringbuf_basic();
    test_ringbuf_wrap_overwrite();
    test_ringbuf_halt_mode();
    test_ringbuf_wraparound_offset();
    test_record_roundtrip();
    test_record_return_carry();
    test_record_truncation();
    test_record_lost_procinfo_timebase();
    test_filter_default_allows_all();
    test_filter_syscall_mask();
    test_filter_pid_user_path();
    test_filter_error_only();
    test_filter_detail_level();
    test_mask_helpers();

    if (failures == 0) {
        printf("\nALLE TESTS OK\n");
        return 0;
    }
    printf("\n%d FEHLER\n", failures);
    return 1;
}
