/*
 * test_q9kernel_sched.c -- Regressionstest fuer q9kernel_sched.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. Q9_D_PROC/
 * Q9K_READYQ_SENTINEL_ADDR/Q9K_SCHED_SLICE_ADDR werden per #define VOR dem #include
 * auf echte Testpuffer umgebogen.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_sched test_q9kernel_sched.c && \
 *       ./test_q9kernel_sched
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x2000];

#define Q9_D_PROC               ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9K_READYQ_SENTINEL_ADDR             ((unsigned long)(g_fakeGlobals + 0x008))
/* Systemweite Mindestprioritaet -- im Test in den Fake-Speicher
 * umgebogen, sonst greift der Scheduler auf die echte Adresse $55E zu. */
#define Q9K_SCHED_MINPTY_ADDR                ((unsigned long)(g_fakeGlobals + 0x1F00))
#define Q9K_SCHED_SLICE_ADDR    ((unsigned long)(g_fakeGlobals + 0x010))
#define Q9K_WAITQ_SENTINEL_ADDR ((unsigned long)(g_fakeGlobals + 0x018))   /* NACHTRAG 2026-08-22 */
#define Q9K_SLEEPQ_SENTINEL_ADDR ((unsigned long)(g_fakeGlobals + 0x020))  /* NACHTRAG 2026-08-30 */
/* Real nur 2 Byte (Deskriptor-Offset 0x0E) -- grosszuegig auf einen
 * eigenen, von Q9K_READYQ_NEXT_OFF/PREV_OFF (0x40/0x48) weit entfernten
 * Testoffset gelegt, damit Q9K_SetU32 (8 Byte auf diesem Host) sie nicht
 * ueberlappt. */
/* NACHTRAG 2026-09-04: von $18 nach $20 verschoben. Seit der Angleichung des
   Deskriptor-Layouts an das echte OS-9 (P$Prior $18, P$Age $1A, P$State $1C)
   liegen die Byte-Felder PRIORITY/AGE/STATE im Bereich $18..$1D -- der alte
   Testwert haette sie ueberschrieben. Die Testpuffer bilden das Layout nur
   verkuerzt nach, deshalb hier ein eigener, kollisionsfreier Wert. */
#define Q9K_PROCDESC_SLEEPTICKS_OFF 0x20UL

/* Real nur 0x30/0x34 auseinander -- auf diesem 64-Bit-Testhost grosszuegig
 * auf 8-Byte-Schritte gelegt, gleiches Muster wie in den anderen Tests. */
#define Q9K_READYQ_NEXT_OFF     0x40UL
#define Q9K_READYQ_PREV_OFF     0x48UL

#define Q9K_TEST_DESC_SIZE      96UL   /* muss > Q9K_READYQ_PREV_OFF+8 sein */

/* Q9K_AlarmTick lebt in q9kernel_alarm.c (dort eigenstaendig getestet)
 * und wird vom Scheduler einmal pro Tick gerufen. Hier ein zaehlender
 * Stub -- dieser Test prueft die Ready-Queue, nicht die Alarme. */
static int g_alarmTicks;
unsigned long Q9K_AlarmTick(void) { g_alarmTicks++; return 0; }

/* Dasselbe fuer die Systemuhr (q9kernel_clock.c, eigene Testreihe in
 * test_q9kernel_clock.c) -- der Scheduler ruft sie ebenfalls pro Tick. */
static int g_clockTicks;
unsigned long Q9K_ClockTick(void) { g_clockTicks++; return 0; }

#include "q9kernel_sched.c"

static int failures = 0;

static void checkU32(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-60s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-60s = %lu (erwartet %lu)\n", label, (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

static void checkU16(const char *label, Q9_u16 got, Q9_u16 want)
{
    if (got == want) {
        printf("[OK]   %-60s = %u\n", label, (unsigned)got);
    } else {
        printf("[FAIL] %-60s = %u (erwartet %u)\n", label, (unsigned)got, (unsigned)want);
        failures++;
    }
}

int main(void)
{
    static unsigned char pool[3 * Q9K_TEST_DESC_SIZE];
    Q9_u32 base = (Q9_u32)(unsigned long)pool;
    Q9_u32 p1 = base + 0 * Q9K_TEST_DESC_SIZE;
    Q9_u32 p2 = base + 1 * Q9K_TEST_DESC_SIZE;
    Q9_u32 p3 = base + 2 * Q9K_TEST_DESC_SIZE;
    Q9_u32 picked;

    memset(g_fakeGlobals, 0, sizeof(g_fakeGlobals));
    memset(pool, 0, sizeof(pool));

    /* NACHTRAG 2026-08-30: Q9K_SchedReschedule ruft seit "F$Sleep"
     * UNBEDINGT Q9K_SleepQDecrementAll auf (einmal pro Tick, s.
     * q9kernel_sched.c) -- die Sleep-Queue-Sentinel MUSS deshalb schon
     * VOR dem ersten Q9K_SchedReschedule-Aufruf als leere Ringliste
     * initialisiert sein (nicht erst in Fall 11 weiter unten), sonst
     * dereferenziert der erste Schleifendurchlauf einen ungueltigen
     * Zeiger (real per AddressSanitizer gefunden: SEGV in Q9K_GetU32 <-
     * Q9K_SleepQDecrementAll <- Q9K_SchedReschedule, ausgeloest durch
     * einen der FRUEHEREN Testfaelle, die von der neuen Sleep-Queue
     * noch nichts wissen). */
    Q9K_SetU32(Q9K_SLEEPQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF, Q9K_SLEEPQ_SENTINEL_ADDR);
    Q9K_SetU32(Q9K_SLEEPQ_SENTINEL_ADDR + Q9K_READYQ_PREV_OFF, Q9K_SLEEPQ_SENTINEL_ADDR);

    /* Q9K_READYQ_SENTINEL_ADDR als leere Ringliste initialisieren -- gleiches Muster
     * wie Q9K_InitEmptyQueue (q9kernel_cinit.c). */
    Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF, Q9K_READYQ_SENTINEL_ADDR);
    Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_PREV_OFF, Q9K_READYQ_SENTINEL_ADDR);

    /* Drei Prozesse mit unterschiedlicher Prioritaet: p1=5, p2=1, p3=10. */
    *(volatile Q9_u8 *)(p1 + Q9K_PROCDESC_PRIORITY_OFF) = 5;
    *(volatile Q9_u8 *)(p2 + Q9K_PROCDESC_PRIORITY_OFF) = 1;
    *(volatile Q9_u8 *)(p3 + Q9K_PROCDESC_PRIORITY_OFF) = 10;

    /* Fall 1: Einfuegen setzt Age=Prioritaet. */
    Q9K_SchedInsert(p1);
    checkU16("Q9K_SchedInsert(p1, Prio 5) setzt Age=5", Q9K_GetU16(p1 + Q9K_PROCDESC_AGE_OFF), 5);
    Q9K_SchedInsert(p2);
    checkU16("Q9K_SchedInsert(p2, Prio 1) setzt Age=1", Q9K_GetU16(p2 + Q9K_PROCDESC_AGE_OFF), 1);
    Q9K_SchedInsert(p3);
    checkU16("Q9K_SchedInsert(p3, Prio 10) setzt Age=10", Q9K_GetU16(p3 + Q9K_PROCDESC_AGE_OFF), 10);

    /* Fall 2: Q9K_SchedPickHighestAge waehlt den hoechsten Age-Wert
     * (p3=10), unabhaengig von der Einfuegereihenfolge, UND entfernt ihn
     * aus der Queue. */
    picked = Q9K_SchedPickHighestAge();
    checkU32("Q9K_SchedPickHighestAge waehlt p3 (hoechstes Alter)", picked, p3);
    checkU32("p2 ist jetzt der Nachfolger des Sentinels (p3 entfernt)",
             Q9K_GetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF), p1);

    /* p3 wieder einfuegen fuer die naechsten Faelle -- Age faellt dabei
     * auf seine Prioritaet (10) zurueck, wie bei einer echten
     * Wiedereinfuegung nach einer Zeitscheibe. */
    Q9K_SchedInsert(p3);

    /* Fall 3: Q9K_SchedAgeAll erhoeht ALLE Eintraege in der Queue (p1,
     * p2, p3 -- kein "aktueller Prozess" ausgenommen, da Q9_D_PROC hier
     * noch 0 ist). */
    Q9K_SchedAgeAll();
    checkU16("Nach Q9K_SchedAgeAll: p1.Age = 6 (5+1)", Q9K_GetU16(p1 + Q9K_PROCDESC_AGE_OFF), 6);
    checkU16("Nach Q9K_SchedAgeAll: p2.Age = 2 (1+1)", Q9K_GetU16(p2 + Q9K_PROCDESC_AGE_OFF), 2);
    checkU16("Nach Q9K_SchedAgeAll: p3.Age = 11 (10+1)", Q9K_GetU16(p3 + Q9K_PROCDESC_AGE_OFF), 11);

    /* Fall 4: Q9K_SchedReschedule mit voller Zeitscheibe (Slice>0) --
     * KEIN Wechsel, Slice dekrementiert. */
    Q9K_SetU16(Q9K_SCHED_SLICE_ADDR, 2);
    Q9K_SetU32(Q9_D_PROC, 0); /* noch kein laufender Prozess in diesem Testschritt */
    checkU32("Reschedule mit Slice=2 -> kein Wechsel (0)", Q9K_SchedReschedule(), 0);
    checkU16("Slice danach = 1", Q9K_GetU16(Q9K_SCHED_SLICE_ADDR), 1);
    checkU32("Reschedule mit Slice=1 -> kein Wechsel (0)", Q9K_SchedReschedule(), 0);
    checkU16("Slice danach = 0", Q9K_GetU16(Q9K_SCHED_SLICE_ADDR), 0);

    /* Fall 5: Slice ist jetzt 0 -- naechster Reschedule-Aufruf waehlt
     * den Prozess mit dem hoechsten Alter (p3, aktuell 12 nach der
     * AgeAll in diesem Aufruf) und setzt Q9_D_PROC darauf. */
    picked = Q9K_SchedReschedule();
    checkU32("Reschedule bei Slice=0 waehlt p3 (hoechstes Alter)", picked, p3);
    checkU32("Q9_D_PROC zeigt jetzt auf p3", Q9K_GetU32(Q9_D_PROC), p3);
    checkU16("Slice nach dem Wechsel neu aufgeladen (2)", Q9K_GetU16(Q9K_SCHED_SLICE_ADDR), Q9K_SCHED_TSLICE);

    /* Fall 6: der vorherige "aktuelle" Prozess war 0 (Testaufbau) --
     * jetzt echtes Szenario: Q9_D_PROC=p3 laeuft, Slice erschoepft sich,
     * p1/p2 sind die einzigen Wartenden -- p1 (Age 7 nach dieser Runde)
     * gewinnt gegen p2 (Age 3). p3 muss dabei SELBST wieder in die
     * Queue eingefuegt werden (mit Age=Prioritaet=10). */
    Q9K_SetU16(Q9K_SCHED_SLICE_ADDR, 0);
    picked = Q9K_SchedReschedule();
    checkU32("Reschedule waehlt p1 (Age 7 > p2s Age 3)", picked, p1);
    checkU32("Q9_D_PROC zeigt jetzt auf p1", Q9K_GetU32(Q9_D_PROC), p1);
    checkU16("p3 wurde mit Age=Prioritaet (10) zurueck in die Queue gelegt",
             Q9K_GetU16(p3 + Q9K_PROCDESC_AGE_OFF), 10);

    /* Fall 7: nur EIN Prozess insgesamt bereit (Queue leer nach dem
     * Herausnehmen) -- Reschedule liefert 0 (kein Wechsel), Slice wird
     * trotzdem neu aufgeladen. */
    {
        Q9_u32 solo = base; /* p1 wiederverwenden als einzigen Testprozess */
        Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF, Q9K_READYQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_PREV_OFF, Q9K_READYQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9_D_PROC, solo);
        Q9K_SetU16(Q9K_SCHED_SLICE_ADDR, 0);
        checkU32("Reschedule bei leerer Queue -> kein Wechsel (0)", Q9K_SchedReschedule(), 0);
        checkU32("Q9_D_PROC bleibt unveraendert (derselbe Prozess laeuft weiter)",
                 Q9K_GetU32(Q9_D_PROC), solo);
        checkU16("Slice trotzdem neu aufgeladen (2)", Q9K_GetU16(Q9K_SCHED_SLICE_ADDR), Q9K_SCHED_TSLICE);
    }

    /* Fall 8: Q9K_SchedFirstPick -- der Sonder-Einstiegspunkt fuers
     * allererste Boot (kein "aktueller" Prozess existiert, den es
     * zurueck in die Queue zu legen gaebe). Drei frisch eingefuegte
     * Prozesse, hoechstes Alter (=Prioritaet, da eben erst eingefuegt)
     * gewinnt und wird in Q9_D_PROC eingetragen. */
    {
        Q9_u32 picked8;

        Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF, Q9K_READYQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_PREV_OFF, Q9K_READYQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9_D_PROC, 0);

        Q9K_SchedInsert(p1); /* Prio 5 */
        Q9K_SchedInsert(p2); /* Prio 1 */
        Q9K_SchedInsert(p3); /* Prio 10 */

        Q9K_SetU16(Q9K_SCHED_SLICE_ADDR, 0); /* simuliert die genullte Boot-Zeit-Ausgangslage */

        picked8 = Q9K_SchedFirstPick();
        checkU32("Q9K_SchedFirstPick waehlt p3 (hoechste Prioritaet/Alter)", picked8, p3);
        checkU32("Q9_D_PROC zeigt danach auf p3", Q9K_GetU32(Q9_D_PROC), p3);
        checkU32("p3 wurde aus der Ready-Queue entfernt (Sentinel.next == p1)",
                 Q9K_GetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF), p1);
        checkU16("Q9K_SchedFirstPick laedt die Zeitscheibe mit auf (sonst sofortiger Wechsel beim 1. Tick)",
                 Q9K_GetU16(Q9K_SCHED_SLICE_ADDR), Q9K_SCHED_TSLICE);
    }

    /* Fall 9: Q9K_SchedFirstPick bei leerer Queue -- liefert 0, Q9_D_PROC
     * bleibt unangetastet (kein Fake-Erfolg). */
    {
        Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF, Q9K_READYQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_PREV_OFF, Q9K_READYQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9_D_PROC, 0);

        checkU32("Q9K_SchedFirstPick bei leerer Queue -> 0", Q9K_SchedFirstPick(), 0);
        checkU32("Q9_D_PROC bleibt 0", Q9K_GetU32(Q9_D_PROC), 0);
    }

    /* Fall 10 (NACHTRAG 2026-08-22, Abschnitt "F$Exit/F$Wait"):
     * Q9K_WaitQInsert/Q9K_WaitQRemove -- eigene, von der Ready-Queue
     * komplett getrennte Warteschlange, gleiches Next/Prev-Grundmuster.
     * p1/p2 hier zweckentfremdet (Ready-Queue-Faelle sind bereits
     * abgeschlossen), rein als generische Deskriptor-Slots. */
    {
        Q9K_SetU32(Q9K_WAITQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF, Q9K_WAITQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9K_WAITQ_SENTINEL_ADDR + Q9K_READYQ_PREV_OFF, Q9K_WAITQ_SENTINEL_ADDR);

        Q9K_WaitQInsert(p1);
        checkU32("Q9K_WaitQInsert(p1): Sentinel.next == p1",
                 Q9K_GetU32(Q9K_WAITQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF), p1);

        Q9K_WaitQInsert(p2);
        checkU32("Q9K_WaitQInsert(p2): Sentinel.prev == p2 (hinten angehaengt)",
                 Q9K_GetU32(Q9K_WAITQ_SENTINEL_ADDR + Q9K_READYQ_PREV_OFF), p2);
        checkU32("Q9K_WaitQInsert(p2): p1.next == p2 (verkettet)",
                 Q9K_GetU32(p1 + Q9K_READYQ_NEXT_OFF), p2);

        Q9K_WaitQRemove(p1);
        checkU32("Q9K_WaitQRemove(p1): Sentinel.next == p2 (p1 entfernt)",
                 Q9K_GetU32(Q9K_WAITQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF), p2);

        Q9K_WaitQRemove(p2);
        checkU32("Q9K_WaitQRemove(p2): Warteschlange wieder leer (Sentinel.next == Sentinel)",
                 Q9K_GetU32(Q9K_WAITQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF), Q9K_WAITQ_SENTINEL_ADDR);
        checkU32("Ready-Queue bleibt von der Wait-Queue unberuehrt (Sentinel.next == Sentinel)",
                 Q9K_GetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF), Q9K_READYQ_SENTINEL_ADDR);
    }

    /* Fall 11 (NACHTRAG 2026-08-30, Abschnitt "F$Sleep"):
     * Q9K_SleepQInsert/Q9K_SleepQDecrementAll -- eigene, von Ready-/
     * Wait-Queue komplett getrennte Warteschlange. p1/p2/p3 hier erneut
     * zweckentfremdet als generische Deskriptor-Slots. */
    {
        Q9K_SetU32(Q9K_SLEEPQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF, Q9K_SLEEPQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9K_SLEEPQ_SENTINEL_ADDR + Q9K_READYQ_PREV_OFF, Q9K_SLEEPQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF, Q9K_READYQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_PREV_OFF, Q9K_READYQ_SENTINEL_ADDR);

        /* p1: Countdown 3 -- braucht 3 Dekrement-Aufrufe bis zum Wecken. */
        *(unsigned char *)(p1 + Q9K_PROCDESC_STATE_OFF) = 's';
        Q9K_SetU32(p1 + Q9K_PROCDESC_SLEEPTICKS_OFF, 3);
        /* p2: Sentinel Q9K_SLEEP_INFINITE -- darf NIE geweckt werden. */
        *(unsigned char *)(p2 + Q9K_PROCDESC_STATE_OFF) = 's';
        Q9K_SetU32(p2 + Q9K_PROCDESC_SLEEPTICKS_OFF, Q9K_SLEEP_INFINITE);
        /* p3: Countdown 1 -- wacht bereits beim ERSTEN Dekrement auf. */
        *(unsigned char *)(p3 + Q9K_PROCDESC_STATE_OFF) = 's';
        Q9K_SetU32(p3 + Q9K_PROCDESC_SLEEPTICKS_OFF, 1);

        Q9K_SleepQInsert(p1);
        Q9K_SleepQInsert(p2);
        Q9K_SleepQInsert(p3);

        Q9K_SleepQDecrementAll();   /* Tick 1: p3 wacht auf (1->0), p1: 3->2, p2 unangetastet */
        checkU32("Tick1: p3 wurde in die Ready-Queue verschoben (Sentinel.next == p3)",
                 Q9K_GetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF), p3);
        checkU32("Tick1: p3.State zurueck auf 'a'",
                 (Q9_u32)*(unsigned char *)(p3 + Q9K_PROCDESC_STATE_OFF), (Q9_u32)'a');
        checkU32("Tick1: p1.SleepTicks == 2 (3->2)", Q9K_GetU32(p1 + Q9K_PROCDESC_SLEEPTICKS_OFF), 2);
        checkU32("Tick1: p2.SleepTicks bleibt Q9K_SLEEP_INFINITE (nie dekrementiert)",
                 Q9K_GetU32(p2 + Q9K_PROCDESC_SLEEPTICKS_OFF), Q9K_SLEEP_INFINITE);
        checkU32("Tick1: p1 noch in der Sleep-Queue (Sentinel.next == p1)",
                 Q9K_GetU32(Q9K_SLEEPQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF), p1);

        Q9K_SleepQDecrementAll();   /* Tick 2: p1: 2->1 */
        Q9K_SleepQDecrementAll();   /* Tick 3: p1: 1->0, wacht auf */
        checkU32("Tick3: p1 wurde ebenfalls in die Ready-Queue verschoben",
                 (Q9_u32)*(unsigned char *)(p1 + Q9K_PROCDESC_STATE_OFF), (Q9_u32)'a');
        checkU32("Tick3: Sleep-Queue enthaelt nur noch p2 (Sentinel.next == p2)",
                 Q9K_GetU32(Q9K_SLEEPQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF), p2);

        Q9K_SleepQDecrementAll();   /* Tick 4: p2 (unendlich) bleibt unberuehrt */
        checkU32("Tick4: p2 bleibt in der Sleep-Queue (nie geweckt)",
                 Q9K_GetU32(Q9K_SLEEPQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF), p2);
        checkU32("Tick4: p2.State bleibt 's'",
                 (Q9_u32)*(unsigned char *)(p2 + Q9K_PROCDESC_STATE_OFF), (Q9_u32)'s');
    }


    /* Mindestprioritaet (D_MinPty): der vom Handbuch bei F$SSpd genannte
     * Weg, einen Prozess anzuhalten -- seine Prioritaet unter die
     * Untergrenze senken. Bei 0 gibt es keine Untergrenze, das Verhalten
     * ist dann unveraendert (so bootet das System auch). */
    {
        static unsigned char mpPool[2 * Q9K_TEST_DESC_SIZE];
        Q9_u32 mpBase = (Q9_u32)(unsigned long)mpPool;
        Q9_u32 lo = mpBase;
        Q9_u32 hi = mpBase + Q9K_TEST_DESC_SIZE;

        printf("\n--- Mindestprioritaet (F$SSpd-Ersatzweg) ---\n");
        memset(mpPool, 0, sizeof(mpPool));
        Q9K_SetU8(lo + Q9K_PROCDESC_PRIORITY_OFF, 3);   /* niedrig */
        Q9K_SetU8(hi + Q9K_PROCDESC_PRIORITY_OFF, 9);   /* hoch    */

        Q9K_SetU16(Q9K_SCHED_MINPTY_ADDR, 0);
        Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF, Q9K_READYQ_SENTINEL_ADDR);
        Q9K_SetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_PREV_OFF, Q9K_READYQ_SENTINEL_ADDR);
        Q9K_SchedInsert(lo);
        Q9K_SchedInsert(hi);
        checkU32("ohne Untergrenze gewinnt die hoehere Prioritaet",
                 Q9K_SchedPickHighestAge(), hi);

        Q9K_SetU16(Q9K_SCHED_MINPTY_ADDR, 5);
        checkU32("unter der Untergrenze wird niemand gewaehlt",
                 Q9K_SchedPickHighestAge(), 0);

        /* Angehalten heisst nicht vergessen: Untergrenze zuruecknehmen,
         * und derselbe Prozess ist wieder waehlbar -- er lag die ganze
         * Zeit noch in der Ready-Queue. */
        Q9K_SetU16(Q9K_SCHED_MINPTY_ADDR, 0);
        checkU32("nach Zuruecknehmen der Untergrenze laeuft er wieder",
                 Q9K_SchedPickHighestAge(), lo);

        /* Genau auf der Grenze zaehlt als lauffaehig (>=, nicht >). */
        Q9K_SchedInsert(lo);
        Q9K_SetU16(Q9K_SCHED_MINPTY_ADDR, 3);
        checkU32("genau auf der Untergrenze laeuft weiter",
                 Q9K_SchedPickHighestAge(), lo);

        Q9K_SetU16(Q9K_SCHED_MINPTY_ADDR, 0);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
