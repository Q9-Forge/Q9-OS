/*
 * test_q9kernel_procsleep.c -- Regressionstest fuer q9kernel_procsleep.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. Alle
 * externen Abhaengigkeiten (Q9K_SchedInsert/SleepQInsert/SchedFirstPick)
 * werden hier durch einfache, aufrufzaehlende Stubs ersetzt -- dieser
 * Test prueft NUR die Ticks-Umrechnung/Verzweigungslogik von
 * q9kernel_procsleep.c selbst.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_procsleep test_q9kernel_procsleep.c && \
 *       ./test_q9kernel_procsleep
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x2000];

#define Q9_D_PROC ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9K_SIGMASK_SCRATCH_LEVEL   ((unsigned long)(g_fakeGlobals + 0x1A00))
#define Q9K_SIGMASK_SCRATCH_ERROR   ((unsigned long)(g_fakeGlobals + 0x1A20))
#define Q9K_SIGMASK_SCRATCH_SUCCESS ((unsigned long)(g_fakeGlobals + 0x1A40))
/* P$SigVec ($28) und P$SigDat ($2C) liegen real genau 4 Byte
 * auseinander. Auf diesem Host ist Q9_u32 8 Byte breit, ein Zugriff
 * auf $28 reichte also bis $2F und ueberschriebe P$SigDat -- gleiche
 * Grosszuegigkeit wie bei P$User in den anderen Tests, betrifft NUR
 * diesen Test. */
#define Q9K_PROCDESC_SIGVEC_OFF 0x100UL
#define Q9K_PROCDESC_SIGDAT_OFF 0x120UL
#define Q9K_ICPT_SCRATCH_VEC        ((unsigned long)(g_fakeGlobals + 0x1A60))
#define Q9K_ICPT_SCRATCH_DAT        ((unsigned long)(g_fakeGlobals + 0x1A80))
#define Q9K_ICPT_SCRATCH_PENDING    ((unsigned long)(g_fakeGlobals + 0x1AA0))

/* Real nur wenige Byte auseinander -- hier grosszuegig auf 8-Byte-
 * Schritte gelegt, gleiches Muster wie ueberall (Q9_u32 = 8 Byte auf
 * diesem Testhost). */
#define Q9K_PROCDESC_STATE_OFF      0x00UL   /* bleibt real -- nur 1 Byte */
#define Q9K_PROCDESC_SLEEPTICKS_OFF 0x08UL
#define Q9K_PROCDESC_SAVEDSP_OFF    0x10UL

#define Q9K_TEST_DESC_SIZE 64UL

/* Aufrufzaehlende Stubs. */
static int g_schedInsertCalls = 0;
static unsigned long g_schedInsertLastDesc = 0;
void Q9K_SchedInsert(unsigned long desc)
{
    g_schedInsertCalls++;
    g_schedInsertLastDesc = desc;
}

static int g_sleepQInsertCalls = 0;
static unsigned long g_sleepQInsertLastDesc = 0;
void Q9K_SleepQInsert(unsigned long desc)
{
    g_sleepQInsertCalls++;
    g_sleepQInsertLastDesc = desc;
}

static unsigned long g_schedFirstPickReturn = 0xDEADBEEFUL;
/* Seit 2026-09-04 ruft diese Uebersetzungseinheit ausserdem F$Send-Bausteine
   auf (Q9K_ProcSend -> Q9K_ProcLookup/Q9K_SchedWake). Beide sind hier reine
   Stubs -- der Test deckt Q9K_ProcSleep ab, nicht die Weckwirkung. */
static unsigned long g_lookupResult;
unsigned long Q9K_ProcLookup(unsigned short pid) { (void)pid; return g_lookupResult; }
static int g_wakeCalls;
static unsigned long g_wakeLast;
void Q9K_SchedWake(unsigned long desc) { g_wakeCalls++; g_wakeLast = desc; }

unsigned long Q9K_SchedFirstPick(void)
{
    return g_schedFirstPickReturn;
}

#include "q9kernel_procsleep.c"

static int failures = 0;

static void checkU32(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-65s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-65s = %lu (erwartet %lu)\n", label, (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

/* Byteweise Big-Endian-Lesen -- gleiches Muster wie test_q9kernel_procend.c
 * (Q9K_SetFrameReg schreibt absichtlich immer Big-Endian, s. dortigen
 * Kopfkommentar). */
static Q9_u32 getBE32(Q9_u32 addr)
{
    const Q9_u8 *p = (const Q9_u8 *)addr;
    return ((Q9_u32)p[0] << 24) | ((Q9_u32)p[1] << 16) | ((Q9_u32)p[2] << 8) | (Q9_u32)p[3];
}

static void resetStubs(void)
{
    g_schedInsertCalls = 0;
    g_schedInsertLastDesc = 0;
    g_sleepQInsertCalls = 0;
    g_sleepQInsertLastDesc = 0;
    g_schedFirstPickReturn = 0xDEADBEEFUL;
}

int main(void)
{
    static unsigned char pool[Q9K_TEST_DESC_SIZE];
    static unsigned char frame[64];
    Q9_u32 desc = (Q9_u32)(unsigned long)pool;
    Q9_u32 frameBase = (Q9_u32)(unsigned long)frame;
    Q9_u32 next;

    memset(g_fakeGlobals, 0, sizeof(g_fakeGlobals));

    /* Fall 1: Sleep(0) -- unendlich. State='s', SleepTicks==Sentinel,
     * Q9K_SleepQInsert aufgerufen, NICHT Q9K_SchedInsert. */
    {
        memset(pool, 0xCC, sizeof(pool));
        memset(frame, 0xCC, sizeof(frame));
        Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, frameBase);
        resetStubs();
        g_schedFirstPickReturn = 0x1234;

        next = Q9K_ProcSleep(desc, 0);
        checkU32("F1: Rueckgabe == Q9K_SchedFirstPick()-Ergebnis", next, 0x1234);
        checkU32("F1: State == 's' (schlafend)", (Q9_u32)Q9K_GetU8(desc + Q9K_PROCDESC_STATE_OFF), (Q9_u32)'s');
        checkU32("F1: SleepTicks == Q9K_SLEEP_INFINITE", Q9K_GetU32(desc + Q9K_PROCDESC_SLEEPTICKS_OFF), Q9K_SLEEP_INFINITE);
        checkU32("F1: Q9K_SleepQInsert wurde aufgerufen", (Q9_u32)g_sleepQInsertCalls, 1);
        checkU32("F1: ... mit dem echten Deskriptor", g_sleepQInsertLastDesc, desc);
        checkU32("F1: Q9K_SchedInsert NICHT aufgerufen (kein Zeitscheiben-Verzicht)", (Q9_u32)g_schedInsertCalls, 0);
        checkU32("F1: Rahmen-D0 == 0 (keine vorzeitige Aktivierung moeglich)", getBE32(frameBase + 0 * 4), 0);
    }

    /* Fall 2: Sleep(1) -- reiner Zeitscheiben-Verzicht. State bleibt
     * unangetastet (kein 's'), Q9K_SchedInsert aufgerufen, NICHT
     * Q9K_SleepQInsert. */
    {
        memset(pool, 0xCC, sizeof(pool));
        memset(frame, 0xCC, sizeof(frame));
        Q9K_SetU8(desc + Q9K_PROCDESC_STATE_OFF, 'a');
        Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, frameBase);
        resetStubs();
        g_schedFirstPickReturn = 0x5678;

        next = Q9K_ProcSleep(desc, 1);
        checkU32("F2: Rueckgabe == Q9K_SchedFirstPick()-Ergebnis", next, 0x5678);
        checkU32("F2: State bleibt 'a' (nie wirklich geschlafen, laut Manual)",
                 (Q9_u32)Q9K_GetU8(desc + Q9K_PROCDESC_STATE_OFF), (Q9_u32)'a');
        checkU32("F2: Q9K_SchedInsert wurde aufgerufen", (Q9_u32)g_schedInsertCalls, 1);
        checkU32("F2: ... mit dem echten Deskriptor", g_schedInsertLastDesc, desc);
        checkU32("F2: Q9K_SleepQInsert NICHT aufgerufen", (Q9_u32)g_sleepQInsertCalls, 0);
        checkU32("F2: Rahmen-D0 == 0", getBE32(frameBase + 0 * 4), 0);
    }

    /* Fall 3: Sleep(5) -- echter Countdown (n-1 = 4 gespeichert). */
    {
        memset(pool, 0xCC, sizeof(pool));
        memset(frame, 0xCC, sizeof(frame));
        Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, frameBase);
        resetStubs();

        next = Q9K_ProcSleep(desc, 5);
        (void)next;
        checkU32("F3: State == 's' (schlafend)", (Q9_u32)Q9K_GetU8(desc + Q9K_PROCDESC_STATE_OFF), (Q9_u32)'s');
        checkU32("F3: SleepTicks == 4 (n-1, reale Konvention)", Q9K_GetU32(desc + Q9K_PROCDESC_SLEEPTICKS_OFF), 4);
        checkU32("F3: Q9K_SleepQInsert wurde aufgerufen", (Q9_u32)g_sleepQInsertCalls, 1);
    }

    /* Fall 4: Sleep(2) -- Randfall, kleinster echter Countdown (n-1=1). */
    {
        memset(pool, 0xCC, sizeof(pool));
        memset(frame, 0xCC, sizeof(frame));
        Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, frameBase);
        resetStubs();

        Q9K_ProcSleep(desc, 2);
        checkU32("F4: SleepTicks == 1 (n-1 fuer n=2)", Q9K_GetU32(desc + Q9K_PROCDESC_SLEEPTICKS_OFF), 1);
    }

    /* Fall 5: High-Bit gesetzt -- 256stel-Sekunden -> Ticks.
     * 256 (=1.0 Sekunde in 256steln) -> 256*25/64 = 100 Ticks (= 1
     * Sekunde bei 100 Ticks/Sekunde) -- exakt, kein Rundungsfehler. */
    {
        Q9_u32 val256 = 0x80000000UL | 256UL;

        memset(pool, 0xCC, sizeof(pool));
        memset(frame, 0xCC, sizeof(frame));
        Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, frameBase);
        resetStubs();

        Q9K_ProcSleep(desc, val256);
        checkU32("F5: 256 Zweihundertsechsundfuenfzigstel-Sekunden -> 99 Ticks-Countdown (100-1)",
                 Q9K_GetU32(desc + Q9K_PROCDESC_SLEEPTICKS_OFF), 99);
    }

    /* Fall 6: High-Bit gesetzt, kleiner Wert -- rundet auf Sleep(1)
     * (Zeitscheiben-Verzicht) ab, wenn die Umrechnung exakt 1 ergibt. */
    {
        Q9_u32 val256 = 0x80000000UL | 3UL;   /* 3*25/64 = 1 (abgerundet) */

        memset(pool, 0xCC, sizeof(pool));
        memset(frame, 0xCC, sizeof(frame));
        Q9K_SetU8(desc + Q9K_PROCDESC_STATE_OFF, 'a');
        Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, frameBase);
        resetStubs();

        Q9K_ProcSleep(desc, val256);
        checkU32("F6: kleine 256stel-Werte koennen auf ticks==1 abrunden -> Zeitscheiben-Verzicht",
                 (Q9_u32)g_schedInsertCalls, 1);
        checkU32("F6: ... NICHT Sleep-Queue", (Q9_u32)g_sleepQInsertCalls, 0);
    }


    /* F$SigMask (Callcode 0x57) und sein Zusammenspiel mit F$Send.
     * Die Maske ist ein ZAEHLER, damit verschachtelte kritische
     * Abschnitte sich nicht gegenseitig die Maske wegnehmen. */
    {
        static unsigned char sigDesc[0x400];
        Q9_u32 desc = (Q9_u32)(unsigned long)sigDesc;
        Q9_u16 err;

        printf("\n--- F$SigMask ---\n");
        memset(sigDesc, 0, sizeof(sigDesc));
        Q9K_SetU32(Q9_D_PROC, desc);
        g_lookupResult = desc;

        checkU32("F$SigMask setzt die Maske", (Q9_u32)Q9K_ProcSigMask(1, &err), 1);
        checkU32("Maskenzaehler steht auf 1",
              (Q9_u32)Q9K_GetU8(desc + Q9K_PROCDESC_SIGLVL_OFF), 1);
        Q9K_ProcSigMask(1, &err);
        checkU32("zweites Setzen erhoeht auf 2 (verschachtelt)",
              (Q9_u32)Q9K_GetU8(desc + Q9K_PROCDESC_SIGLVL_OFF), 2);
        Q9K_ProcSigMask(0xFFFFFFFFUL, &err);
        checkU32("-1 senkt wieder auf 1",
              (Q9_u32)Q9K_GetU8(desc + Q9K_PROCDESC_SIGLVL_OFF), 1);

        /* Bei gesetzter Maske wird ein normales Signal abgelegt, aber
         * der Prozess NICHT geweckt. */
        g_wakeCalls = 0;
        Q9K_SetU16(desc + Q9K_PROCDESC_SIGNAL_OFF, 0);
        checkU32("F$Send meldet auch bei maskiertem Empfaenger Erfolg",
              (Q9_u32)Q9K_ProcSend(1, 42, &err), 1);
        checkU32("das Signal steht im Deskriptor an",
              (Q9_u32)Q9K_GetU16(desc + Q9K_PROCDESC_SIGNAL_OFF), 42);
        checkU32("der maskierte Prozess wurde NICHT geweckt", (Q9_u32)g_wakeCalls, 0);

        /* S$Kill durchbricht die Maske. */
        g_wakeCalls = 0;
        Q9K_ProcSend(1, Q9K_SIGNAL_KILL, &err);
        checkU32("S$Kill durchbricht die Maske und weckt doch",
              (Q9_u32)g_wakeCalls, 1);

        /* S$Wake ebenfalls -- und wird dabei nicht abgelegt. */
        g_wakeCalls = 0;
        Q9K_SetU16(desc + Q9K_PROCDESC_SIGNAL_OFF, 0);
        Q9K_ProcSend(1, Q9K_SIGNAL_WAKE, &err);
        checkU32("S$Wake durchbricht die Maske ebenfalls", (Q9_u32)g_wakeCalls, 1);
        checkU32("S$Wake wird dabei nicht als Signal abgelegt",
              (Q9_u32)Q9K_GetU16(desc + Q9K_PROCDESC_SIGNAL_OFF), 0);

        /* Maske oeffnen -> ein anstehendes Signal wird jetzt zugestellt. */
        Q9K_SetU16(desc + Q9K_PROCDESC_SIGNAL_OFF, 42);
        Q9K_SetU8(desc + Q9K_PROCDESC_SIGLVL_OFF, 1);
        g_wakeCalls = 0;
        Q9K_ProcSigMask(0, &err);
        checkU32("Maske loeschen setzt den Zaehler auf 0",
              (Q9_u32)Q9K_GetU8(desc + Q9K_PROCDESC_SIGLVL_OFF), 0);
        checkU32("und stellt das aufgelaufene Signal zu", (Q9_u32)g_wakeCalls, 1);

        /* Ohne anstehendes Signal weckt das Oeffnen niemanden. */
        Q9K_SetU16(desc + Q9K_PROCDESC_SIGNAL_OFF, 0);
        Q9K_SetU8(desc + Q9K_PROCDESC_SIGLVL_OFF, 1);
        g_wakeCalls = 0;
        Q9K_ProcSigMask(0, &err);
        checkU32("ohne anstehendes Signal weckt das Oeffnen niemanden",
              (Q9_u32)g_wakeCalls, 0);

        /* -1 unterhalb von 0 bleibt bei 0, statt umzulaufen. */
        Q9K_ProcSigMask(0xFFFFFFFFUL, &err);
        checkU32("-1 auf offener Maske laeuft nicht unter",
              (Q9_u32)Q9K_GetU8(desc + Q9K_PROCDESC_SIGLVL_OFF), 0);

        /* Ohne aktuellen Prozess sauberer Fehlschlag. */
        Q9K_SetU32(Q9_D_PROC, 0);
        err = 0;
        checkU32("F$SigMask ohne aktuellen Prozess schlaegt fehl",
              (Q9_u32)Q9K_ProcSigMask(1, &err), 0);
        checkU32("und meldet E$PrcID", (Q9_u32)err, 0x00E0UL);

        /* Bridge: volle Zellbreite. */
        Q9K_SetU32(Q9_D_PROC, desc);
        Q9K_SetU32(Q9K_SIGMASK_SCRATCH_LEVEL, 1);
        Q9K_SysSigMaskImpl();
        checkU32("F$SigMask-Bridge meldet Erfolg in voller Zellbreite",
              Q9K_GetU32(Q9K_SIGMASK_SCRATCH_SUCCESS), 1);
        Q9K_SetU32(Q9_D_PROC, 0);
        Q9K_SysSigMaskImpl();
        checkU32("F$SigMask-Bridge meldet den Fehlschlag in voller Zellbreite",
              Q9K_GetU32(Q9K_SIGMASK_SCRATCH_SUCCESS), 0);
        checkU32("F$SigMask-Bridge legt E$PrcID in voller Zellbreite ab",
              Q9K_GetU32(Q9K_SIGMASK_SCRATCH_ERROR), 0x00E0UL);
    }


    /* F$Icpt (Callcode 0x09): traegt Abfangroutine und Datenzeiger in die
     * dafuer vorgesehenen Deskriptorfelder ein und meldet, wie viele
     * Signale anstehen. Laut Beschreibung kann der Aufruf nicht
     * scheitern. */
    {
        static unsigned char icptDesc[0x400];
        Q9_u32 desc = (Q9_u32)(unsigned long)icptDesc;

        printf("\n--- F$Icpt ---\n");
        memset(icptDesc, 0, sizeof(icptDesc));
        Q9K_SetU32(Q9_D_PROC, desc);

        checkU32("F$Icpt meldet ohne anstehendes Signal 0",
                 Q9K_ProcIcpt(0x11223344UL, 0x55667788UL), 0);
        checkU32("F$Icpt legt die Abfangroutine in P$SigVec ab",
                 Q9K_GetU32(desc + Q9K_PROCDESC_SIGVEC_OFF), 0x11223344UL);
        checkU32("F$Icpt legt den Datenzeiger in P$SigDat ab",
                 Q9K_GetU32(desc + Q9K_PROCDESC_SIGDAT_OFF), 0x55667788UL);

        Q9K_SetU16(desc + Q9K_PROCDESC_SIGNAL_OFF, 42);
        checkU32("F$Icpt meldet ein anstehendes Signal",
                 Q9K_ProcIcpt(1, 2), 1);

        Q9K_SetU32(Q9_D_PROC, 0);
        checkU32("F$Icpt ohne aktuellen Prozess meldet 0 statt abzustuerzen",
                 Q9K_ProcIcpt(1, 2), 0);

        Q9K_SetU32(Q9_D_PROC, desc);
        Q9K_SetU16(desc + Q9K_PROCDESC_SIGNAL_OFF, 0);
        Q9K_SetU32(Q9K_ICPT_SCRATCH_VEC, 0xAABBCCDDUL);
        Q9K_SetU32(Q9K_ICPT_SCRATCH_DAT, 0xEEFF0011UL);
        Q9K_SysIcptImpl();
        checkU32("F$Icpt-Bridge legt die Anzahl in der Zelle ab",
                 Q9K_GetU32(Q9K_ICPT_SCRATCH_PENDING), 0);
        checkU32("F$Icpt-Bridge hat den Vektor wirklich gesetzt",
                 Q9K_GetU32(desc + Q9K_PROCDESC_SIGVEC_OFF), 0xAABBCCDDUL);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
