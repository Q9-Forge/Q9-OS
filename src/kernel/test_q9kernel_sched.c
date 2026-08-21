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
#define Q9K_SCHED_SLICE_ADDR    ((unsigned long)(g_fakeGlobals + 0x010))

/* Real nur 0x30/0x34 auseinander -- auf diesem 64-Bit-Testhost grosszuegig
 * auf 8-Byte-Schritte gelegt, gleiches Muster wie in den anderen Tests. */
#define Q9K_READYQ_NEXT_OFF     0x40UL
#define Q9K_READYQ_PREV_OFF     0x48UL

#define Q9K_TEST_DESC_SIZE      96UL   /* muss > Q9K_READYQ_PREV_OFF+8 sein */

/* Stub fuer die TEMPORAERE Diagnose-Instrumentierung (Abschnitt F$Fork,
 * Bug-Suche "kein Prozesswechsel nach F$Fork mehr", noch NICHT geloest,
 * s. [[project_q9os_own_kernel_design]]) -- nur damit dieser Test trotz
 * der temporaeren Q9K_DiagPrintU32-Aufrufe in q9kernel_sched.c
 * weiterhin linkt. Kein Verhalten, reiner No-op. */
void Q9K_DiagPrintU32(unsigned long value) { (void)value; }

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

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
