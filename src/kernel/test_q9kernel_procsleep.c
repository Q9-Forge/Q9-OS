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

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
