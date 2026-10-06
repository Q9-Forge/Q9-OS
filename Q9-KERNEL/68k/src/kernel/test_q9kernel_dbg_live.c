/*
 * test_q9kernel_dbg_live.c -- Hosttest fuer die Live-Kernel-Anbindung
 * (q9kernel_dbg.c): F$Q9Dbg-Unterfunktionen, Eintritts-/Rueckkehr-
 * Haken, Boot-Initialisierung -- mit allen Adresszellen in einen
 * Testpuffer umgeleitet (exakt das Muster aus test_q9kernel_bitmap.c).
 *
 *   gcc -std=c89 -pedantic -Wall -Wextra -o test_dbg_live test_q9kernel_dbg_live.c && ./test_dbg_live
 */

#include <stdio.h>
#include <string.h>

#include "q9ringbuf.h" /* nur fuer die Q9_u32/u16/u8-Typedefs, vor den Fake-Prototypen unten benoetigt */

static unsigned char g_cells[0x400];
#define CELL(off) ((unsigned long)(g_cells + (off)))

/* WICHTIG: auf diesem (64-Bit-)Testhost ist "unsigned long" (= Q9_u32,
 * s. q9ringbuf.h) 8 Byte breit, nicht 4 wie auf dem echten m68k-Ziel.
 * Jede Zelle braucht deshalb mindestens 8 Byte Abstand zur naechsten,
 * sonst ueberlappt ein Q9K_PutU32-Schreibzugriff die Nachbarzelle --
 * exakt dieselbe Vorsicht wie bei den bestehenden
 * Q9K_BITMAP_SCRATCH_*-Zellen (dort 0x20 Byte Abstand). Hier 0x10
 * (16 Byte) Abstand je Skalarzelle, die drei Tiefen-Arrays
 * entsprechend 8 x 0x10 = 0x80 Byte. Die eigentlichen, produktiven
 * Adressen in q9kernel_dbg.c bleiben davon unberuehrt (dort ist
 * Q9_u32 echte 4 Byte, 4-Byte-Abstand also korrekt) -- diese Umleitung
 * gilt NUR fuer den Hosttest. */
#define Q9DBG_A_BUF_ADDR          CELL(0x000)
#define Q9DBG_A_BUF_SIZE          CELL(0x010)
#define Q9DBG_A_WRITE_OFF         CELL(0x020)
#define Q9DBG_A_READ_OFF          CELL(0x030)
#define Q9DBG_A_USED              CELL(0x040)
#define Q9DBG_A_WRITTEN           CELL(0x050)
#define Q9DBG_A_LOST              CELL(0x060)
#define Q9DBG_A_MODE              CELL(0x070)
#define Q9DBG_A_ENABLED           CELL(0x080)
#define Q9DBG_A_OUTMODE           CELL(0x090)
#define Q9DBG_A_SYSCALL_MASK      CELL(0x0A0) /* 32 Byte, bis 0x0BF */
#define Q9DBG_A_INTERN_MASK       CELL(0x0C0) /* 32 Byte, bis 0x0DF */
#define Q9DBG_A_FILT_PID          CELL(0x0E0)
#define Q9DBG_A_FILT_USER         CELL(0x0F0)
#define Q9DBG_A_FILT_PATH         CELL(0x100)
#define Q9DBG_A_FILT_FILEID       CELL(0x110)
#define Q9DBG_A_FILT_ERRONLY      CELL(0x120)
#define Q9DBG_A_FILT_DETAIL       CELL(0x130)
#define Q9DBG_A_TRAP_DEPTH        CELL(0x140)
#define Q9DBG_A_TRAP_CODE         CELL(0x150) /* 8 x 0x10 = 0x80 Byte, bis 0x1CF */
#define Q9DBG_A_TRAP_PID          CELL(0x1D0) /* 8 x 0x10 = 0x80 Byte, bis 0x24F */
#define Q9DBG_A_TRAP_TICK         CELL(0x250) /* 8 x 0x10 = 0x80 Byte, bis 0x2CF */
#define Q9DBG_A_RET_CARRY         CELL(0x2D0)
#define Q9DBG_A_SVC_FN            CELL(0x2E0)
#define Q9DBG_A_SVC_D1            CELL(0x2F0)
#define Q9DBG_A_SVC_D2            CELL(0x300)
#define Q9DBG_A_SVC_D3            CELL(0x310)
#define Q9DBG_A_SVC_A0            CELL(0x320)
#define Q9DBG_A_SVC_OUT_D0        CELL(0x330)
#define Q9DBG_A_SVC_OUT_D1        CELL(0x340)
#define Q9DBG_A_SVC_OUT_D2        CELL(0x350)
#define Q9DBG_A_SVC_OUT_ERR       CELL(0x360)
#define Q9DBG_A_DISPATCH_CODE_LOW CELL(0x370)
#define Q9DBG_A_CUR_PROC          CELL(0x380) /* Fortsetzung 119: D_Proc, 0 = unbekannt */
#define Q9DBG_A_TICKS             CELL(0x390) /* Fortsetzung 119: Tickzaehler */

/* Fake-Allokator: liefert IMMER den angeforderten Puffer aus einem
 * statischen Testarray -- reicht fuer diesen Hosttest, der nur die
 * q9kernel_dbg.c-Logik pruefen will, nicht Q9K_ProcSRqMem selbst (das
 * hat eigene Tests, s. test_q9kernel_sysmem.c). */
static unsigned char g_fakeTraceBuf[65536];
static int g_allocShouldFail;

int Q9K_ProcSRqMem(Q9_u32 requestedSize, Q9_u32 *outAddr, Q9_u32 *outSize, Q9_u16 *outError)
{
    if (g_allocShouldFail) {
        *outError = 0xED; /* E$NoRAM, Platzhalterwert */
        return 0;
    }
    *outAddr = (Q9_u32)(unsigned long)g_fakeTraceBuf;
    *outSize = requestedSize < sizeof g_fakeTraceBuf ? requestedSize : (Q9_u32)sizeof g_fakeTraceBuf;
    return 1;
}

#include "q9ringbuf.c"
#include "q9trace.c"
#include "q9kernel_dbg.c"

static int failures;

static void checkInt(const char *label, int got, int want)
{
    if (got == want) {
        printf("[OK]   %-62s = %d\n", label, got);
    } else {
        printf("[FAIL] %-62s = %d (erwartet %d)\n", label, got, want);
        failures++;
    }
}

static void checkU32(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-62s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-62s = %lu (erwartet %lu)\n", label, (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

static void test_init_allocates_buffer(void)
{
    g_allocShouldFail = 0;
    Q9K_DbgInit();
    checkU32("init: Puffergroesse gewaehrt (65536)", Q9K_GetU32(Q9DBG_A_BUF_SIZE), 65536UL);
    checkInt("init: Puffer initial nicht aktiviert", (int)Q9K_GetU32(Q9DBG_A_ENABLED), 0);
    checkInt("init: Tiefe initial 0", (int)Q9K_GetU32(Q9DBG_A_TRAP_DEPTH), 0);
}

static void test_init_handles_alloc_failure_gracefully(void)
{
    g_allocShouldFail = 1;
    Q9K_DbgInit();
    checkU32("init (Allokation schlaegt fehl): Puffergroesse 0", Q9K_GetU32(Q9DBG_A_BUF_SIZE), 0);
    g_allocShouldFail = 0;
    Q9K_DbgInit(); /* fuer die folgenden Tests wieder einen echten Puffer */
}

static void test_svc_version(void)
{
    int ok;
    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_VERSION);
    ok = Q9K_SysFQ9DbgImpl();
    checkInt("F$Q9Dbg Version: Erfolg", ok, 1);
    checkU32("F$Q9Dbg Version: d0 = Versionsnummer 1", Q9K_GetU32(Q9DBG_A_SVC_OUT_D0), 1);
    checkU32("F$Q9Dbg Version: d1 = Puffergroesse", Q9K_GetU32(Q9DBG_A_SVC_OUT_D1), 65536UL);
}

static void test_svc_trace_onoff_and_logging(void)
{
    int ok;
    Q9_u8 buf[256];
    Q9_u32 n;
    Q9TraceRecHdr hdr;

    /* Trace zunaechst AUS -- ein simulierter Syscall darf NICHTS loggen. */
    Q9K_PutU32(Q9DBG_A_DISPATCH_CODE_LOW, 0x03); /* F$Fork */
    Q9K_DbgLogEntryImpl();
    Q9K_PutU32(Q9DBG_A_RET_CARRY, 0);
    Q9K_DbgLogReturnImpl();
    checkU32("Trace aus: kein Satz geschrieben", Q9K_GetU32(Q9DBG_A_WRITTEN), 0);

    /* Jetzt einschalten. */
    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_TRACE_ONOFF);
    Q9K_PutU32(Q9DBG_A_SVC_D1, 1);
    ok = Q9K_SysFQ9DbgImpl();
    checkInt("F$Q9Dbg Trace an: Erfolg", ok, 1);

    Q9K_PutU32(Q9DBG_A_DISPATCH_CODE_LOW, 0x03); /* F$Fork */
    Q9K_DbgLogEntryImpl();
    Q9K_PutU32(Q9DBG_A_RET_CARRY, 0);
    Q9K_DbgLogReturnImpl();
    checkU32("Trace an: zwei Saetze geschrieben (Eintritt+Rueckkehr)", Q9K_GetU32(Q9DBG_A_WRITTEN), 2);
    checkU32("Trace an: Tiefe nach Rueckkehr wieder 0", Q9K_GetU32(Q9DBG_A_TRAP_DEPTH), 0);

    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_RING_READ);
    Q9K_PutU32(Q9DBG_A_SVC_D2, sizeof buf);
    Q9K_PutU32(Q9DBG_A_SVC_D3, Q9DBG_RING_TRACE);
    Q9K_PutU32(Q9DBG_A_SVC_A0, (Q9_u32)(unsigned long)buf);
    ok = Q9K_SysFQ9DbgImpl();
    checkInt("F$Q9Dbg Ringpuffer lesen: Erfolg", ok, 1);
    n = Q9K_GetU32(Q9DBG_A_SVC_OUT_D0);

    Q9TraceDecodeHeader(buf, &hdr);
    checkInt("gelesener Satz 1: Eintritt", hdr.recType, Q9TRACE_REC_ENTRY);
    checkInt("gelesener Satz 1: Callcode F$Fork (0x03)", hdr.code, 0x03);
    Q9TraceDecodeHeader(buf + hdr.recLen, &hdr);
    checkInt("gelesener Satz 2: Rueckkehr", hdr.recType, Q9TRACE_REC_RETURN);
    checkInt("gelesener Satz 2: Carry nicht gesetzt", (hdr.flags & Q9TRACE_FLAG_CARRY) != 0, 0);
    (void)n;
}

static void test_nested_calls_depth_and_matching(void)
{
    Q9_u8 buf[256];
    Q9_u32 n;
    Q9TraceRecHdr hdr;

    Q9K_DbgInit();
    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_TRACE_ONOFF);
    Q9K_PutU32(Q9DBG_A_SVC_D1, 1);
    Q9K_SysFQ9DbgImpl();

    /* Aeusserer Aufruf (F$Fork, 0x03) ruft intern einen weiteren Trap
     * auf (F$Load, 0x01), bevor er selbst zurueckkehrt -- simuliert
     * exakt das im Konzeptdokument gezeigte Verschachtelungsbeispiel. */
    Q9K_PutU32(Q9DBG_A_DISPATCH_CODE_LOW, 0x03);
    Q9K_DbgLogEntryImpl();
    checkU32("verschachtelt: Tiefe nach aeusserem Eintritt = 1", Q9K_GetU32(Q9DBG_A_TRAP_DEPTH), 1);

    Q9K_PutU32(Q9DBG_A_DISPATCH_CODE_LOW, 0x01);
    Q9K_DbgLogEntryImpl();
    checkU32("verschachtelt: Tiefe nach innerem Eintritt = 2", Q9K_GetU32(Q9DBG_A_TRAP_DEPTH), 2);

    Q9K_PutU32(Q9DBG_A_RET_CARRY, 0);
    Q9K_DbgLogReturnImpl(); /* innerer Rueckkehr -- muss F$Load (0x01) betreffen, nicht F$Fork */
    checkU32("verschachtelt: Tiefe nach innerer Rueckkehr = 1", Q9K_GetU32(Q9DBG_A_TRAP_DEPTH), 1);

    Q9K_PutU32(Q9DBG_A_RET_CARRY, 1); /* aeusserer Rueckkehr mit Fehler */
    Q9K_DbgLogReturnImpl();
    checkU32("verschachtelt: Tiefe nach aeusserer Rueckkehr = 0", Q9K_GetU32(Q9DBG_A_TRAP_DEPTH), 0);

    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_RING_READ);
    Q9K_PutU32(Q9DBG_A_SVC_D2, sizeof buf);
    Q9K_PutU32(Q9DBG_A_SVC_D3, Q9DBG_RING_TRACE);
    Q9K_PutU32(Q9DBG_A_SVC_A0, (Q9_u32)(unsigned long)buf);
    Q9K_SysFQ9DbgImpl();
    n = Q9K_GetU32(Q9DBG_A_SVC_OUT_D0);

    Q9TraceDecodeHeader(buf, &hdr);
    checkInt("verschachtelt, Satz 1: Eintritt F$Fork (Tiefe 0)", hdr.code, 0x03);
    checkInt("verschachtelt, Satz 1: Tiefe 0", hdr.flags & Q9TRACE_FLAG_DEPTH_MASK, 0);
    Q9TraceDecodeHeader(buf + hdr.recLen, &hdr);
    {
        Q9_u32 off2 = hdr.recLen;
        Q9TraceDecodeHeader(buf + off2, &hdr);
        checkInt("verschachtelt, Satz 2: Eintritt F$Load (Tiefe 1)", hdr.code, 0x01);
        checkInt("verschachtelt, Satz 2: Tiefe 1", hdr.flags & Q9TRACE_FLAG_DEPTH_MASK, 1);
        off2 += hdr.recLen;
        Q9TraceDecodeHeader(buf + off2, &hdr);
        checkInt("verschachtelt, Satz 3: Rueckkehr F$Load (Tiefe 1)", hdr.code, 0x01);
        off2 += hdr.recLen;
        Q9TraceDecodeHeader(buf + off2, &hdr);
        checkInt("verschachtelt, Satz 4: Rueckkehr F$Fork (Tiefe 0)", hdr.code, 0x03);
        checkInt("verschachtelt, Satz 4: Carry gesetzt (Fehlerfall)", (hdr.flags & Q9TRACE_FLAG_CARRY) != 0, 1);
    }
    (void)n;
}

static void test_svc_mask_set_get(void)
{
    Q9_u8 maskIn[32];
    Q9_u8 maskOut[32];
    int i;

    Q9K_DbgInit();
    memset(maskIn, 0, sizeof maskIn);
    Q9TraceMaskSet(maskIn, 0x50);

    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_MASK_SET);
    Q9K_PutU32(Q9DBG_A_SVC_A0, (Q9_u32)(unsigned long)maskIn);
    Q9K_SysFQ9DbgImpl();

    memset(maskOut, 0xAA, sizeof maskOut);
    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_MASK_GET);
    Q9K_PutU32(Q9DBG_A_SVC_A0, (Q9_u32)(unsigned long)maskOut);
    Q9K_SysFQ9DbgImpl();

    for (i = 0; i < 32; i++) {
        if (maskIn[i] != maskOut[i]) {
            printf("[FAIL] mask set/get: Byte %d weicht ab (%d != %d)\n", i, maskIn[i], maskOut[i]);
            failures++;
        }
    }
    printf("[OK]   mask set/get: alle 32 Byte stimmen ueberein\n");
}

static void test_svc_filter_set(void)
{
    int ok;
    Q9K_DbgInit();
    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_FILTER_SET);
    Q9K_PutU32(Q9DBG_A_SVC_D1, Q9DBG_FILTER_PID);
    Q9K_PutU32(Q9DBG_A_SVC_D2, 7);
    ok = Q9K_SysFQ9DbgImpl();
    checkInt("F$Q9Dbg Filter setzen (PID): Erfolg", ok, 1);
    checkU32("F$Q9Dbg Filter setzen (PID): Wert gespeichert", Q9K_GetU32(Q9DBG_A_FILT_PID), 7);

    Q9K_PutU32(Q9DBG_A_SVC_D1, 99); /* unbekannte Filterart */
    ok = Q9K_SysFQ9DbgImpl();
    checkInt("F$Q9Dbg Filter setzen (unbekannt): Fehlschlag", ok, 0);
}

static void test_svc_ring_clear(void)
{
    Q9K_DbgInit();
    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_TRACE_ONOFF);
    Q9K_PutU32(Q9DBG_A_SVC_D1, 1);
    Q9K_SysFQ9DbgImpl();
    Q9K_PutU32(Q9DBG_A_DISPATCH_CODE_LOW, 0x03);
    Q9K_DbgLogEntryImpl();
    checkU32("vor Leeren: Schreibzaehler > 0", Q9K_GetU32(Q9DBG_A_WRITTEN) > 0, 1);

    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_RING_CLEAR);
    Q9K_PutU32(Q9DBG_A_SVC_D3, Q9DBG_RING_TRACE);
    Q9K_SysFQ9DbgImpl();
    checkU32("nach Leeren: used = 0", Q9K_GetU32(Q9DBG_A_USED), 0);
}

static void test_trace_fn_internal(void)
{
    Q9_u8 buf[64];
    Q9TraceRecHdr hdr;

    Q9K_DbgInit();
    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_TRACE_ONOFF);
    Q9K_PutU32(Q9DBG_A_SVC_D1, 1);
    Q9K_SysFQ9DbgImpl();

    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_LEVEL_SET);
    Q9K_PutU32(Q9DBG_A_SVC_D1, 3); /* Detailstufe 3 -- interne Funktionen sichtbar */
    Q9K_PutU32(Q9DBG_A_SVC_D2, Q9DBG_OUT_RINGBUF);
    Q9K_SysFQ9DbgImpl();

    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_IFUNC_MASK);
    {
        Q9_u8 allOn[32];
        memset(allOn, 0xff, sizeof allOn);
        Q9K_PutU32(Q9DBG_A_SVC_A0, (Q9_u32)(unsigned long)allOn);
        Q9K_SysFQ9DbgImpl();
    }

    Q9K_DbgTraceFnEntry(42, 0, 0);
    Q9K_DbgTraceFnReturn(42);

    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_RING_READ);
    Q9K_PutU32(Q9DBG_A_SVC_D2, sizeof buf);
    Q9K_PutU32(Q9DBG_A_SVC_D3, Q9DBG_RING_TRACE);
    Q9K_PutU32(Q9DBG_A_SVC_A0, (Q9_u32)(unsigned long)buf);
    Q9K_SysFQ9DbgImpl();

    Q9TraceDecodeHeader(buf, &hdr);
    checkInt("Q9K_TRACE_FN: Eintritt intern, Funktions-ID 42", hdr.recType == Q9TRACE_REC_INTERN_ENTRY && hdr.code == 42, 1);
    Q9TraceDecodeHeader(buf + hdr.recLen, &hdr);
    checkInt("Q9K_TRACE_FN: Rueckkehr intern, Funktions-ID 42", hdr.recType == Q9TRACE_REC_INTERN_RETURN && hdr.code == 42, 1);
}

/* Fortsetzung 119: P$ID aus dem aktuellen Deskriptor, Tiefenkorrektur bei
 * aeusserem Trap ($3AC = 0), F$Q9Dbg ($7F) wird nie protokolliert, Tick. */
static void test_pid_depthreset_selfskip(void)
{
    static unsigned char desc[0x400];
    Q9_u8 buf[256];
    Q9TraceRecHdr hdr;

    Q9K_DbgInit();
    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_TRACE_ONOFF);
    Q9K_PutU32(Q9DBG_A_SVC_D1, 1);
    Q9K_SysFQ9DbgImpl();

    memset(desc, 0, sizeof desc);
    desc[0] = 0x00; desc[1] = 0x05;                 /* P$ID = 5 (Big Endian) */
    Q9K_PutU32(Q9DBG_A_CUR_PROC, (Q9_u32)(unsigned long)desc);
    Q9K_PutU32(Q9DBG_A_TICKS, 1234);

    /* Tiefe pro Prozess (Deskriptor $3E0): F$ID hinein ... */
    Q9K_PutU32(Q9DBG_A_DISPATCH_CODE_LOW, 0x0C);    /* F$ID */
    Q9K_DbgLogEntryImpl();
    checkInt("Prozesstiefe nach Eintritt = 1", desc[0x3E0], 1);
    checkInt("Callcode auf dem Prozessstapel", desc[0x3E1], 0x0C);

    /* ... F$Q9Dbg selbst (verschachtelt) taucht nicht auf ... */
    Q9K_PutU32(Q9DBG_A_DISPATCH_CODE_LOW, 0x7F);
    Q9K_DbgLogEntryImpl();
    Q9K_PutU32(Q9DBG_A_RET_CARRY, 0);
    Q9K_DbgLogReturnImpl();
    Q9K_DbgLogReturnImpl();                         /* Rueckkehr von F$ID */
    checkInt("Prozesstiefe nach beiden Rueckkehren = 0", desc[0x3E0], 0);
    checkU32("F$Q9Dbg ($7F) wird nicht protokolliert: nur 2 Saetze", Q9K_GetU32(Q9DBG_A_WRITTEN), 2);

    /* ... und F$Sleep kehrt nie ueber den Haken zurueck: keine Tiefe. */
    Q9K_PutU32(Q9DBG_A_DISPATCH_CODE_LOW, 0x0A);
    Q9K_DbgLogEntryImpl();
    checkInt("F$Sleep erhoeht die Prozesstiefe nicht", desc[0x3E0], 0);
    checkU32("F$Sleep-Eintritt wird protokolliert", Q9K_GetU32(Q9DBG_A_WRITTEN), 3);

    Q9K_PutU32(Q9DBG_A_SVC_FN, Q9DBG_FN_RING_READ);
    Q9K_PutU32(Q9DBG_A_SVC_D1, 0);
    Q9K_PutU32(Q9DBG_A_SVC_D2, sizeof buf);
    Q9K_PutU32(Q9DBG_A_SVC_D3, Q9DBG_RING_TRACE);
    Q9K_PutU32(Q9DBG_A_SVC_A0, (Q9_u32)(unsigned long)buf);
    Q9K_SysFQ9DbgImpl();
    Q9TraceDecodeHeader(buf, &hdr);
    checkInt("Satz 1: Callcode F$ID", hdr.code, 0x0C);
    checkInt("Satz 1: PID aus P$ID", hdr.pid, 5);
    checkU32("Satz 1: Tick aus dem Tickzaehler", hdr.tick, 1234);
    Q9TraceDecodeHeader(buf + hdr.recLen, &hdr);
    checkInt("Satz 2: Rueckkehr von F$ID", hdr.code, 0x0C);
    checkInt("Satz 2: Typ Rueckkehr", hdr.recType, Q9TRACE_REC_RETURN);
    checkInt("Satz 2: PID", hdr.pid, 5);

    Q9K_PutU32(Q9DBG_A_CUR_PROC, 0);
    Q9K_PutU32(Q9DBG_A_TICKS, 0);
}

int main(void)
{
    test_init_allocates_buffer();
    test_init_handles_alloc_failure_gracefully();
    test_svc_version();
    test_svc_trace_onoff_and_logging();
    test_nested_calls_depth_and_matching();
    test_svc_mask_set_get();
    test_svc_filter_set();
    test_svc_ring_clear();
    test_trace_fn_internal();
    test_pid_depthreset_selfskip();

    if (failures == 0) {
        printf("\nALLE TESTS OK\n");
        return 0;
    }
    printf("\n%d FEHLER\n", failures);
    return 1;
}
