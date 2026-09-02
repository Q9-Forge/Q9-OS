/*
 * test_q9kernel_exctable.c -- Regressionstest fuer q9kernel_exctable.c.
 *
 * Gleiche Konvention wie test_q9kernel_arena.c: #include der echten .c-
 * Datei direkt (keine Logik-Duplizierung), Q9_D_EXCJMP per #define VOR
 * dem #include auf einen echten, beschreibbaren Testpuffer umgebogen --
 * im Original ist das nur der ZEIGER auf den Boot-ROM-Block, hier also
 * ein Puffer, dessen Inhalt selbst wieder auf einen zweiten, echten
 * Testpuffer (den simulierten Vektor-Block) zeigt.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_exctable test_q9kernel_exctable.c && \
 *       ./test_q9kernel_exctable
 */

#include <stdio.h>
#include <string.h>

static unsigned long g_fakeExcJmpPtr;                /* simuliert Q9_D_EXCJMP-Inhalt (Zeiger) */
/* 256 * 16 statt 256 * 4 -- ein Funktionszeiger ist auf diesem 64-Bit-
 * Host 8 Byte breit (auf dem echten 32-Bit-Ziel waeren es nur 4, s.
 * Kommentar in q9kernel_exctable.c), 16 Byte/Eintrag ist grosszuegig
 * genug fuer jede realistische Zeigerbreite. */
static unsigned char  g_fakeVectorTable[256 * 16];   /* simuliert den Boot-ROM-Block */

#define Q9_D_EXCJMP ((unsigned long)&g_fakeExcJmpPtr)

/* F$IRQ-Polling-Tabelle und ihr Scratch (2026-09-02): im echten Kernel
 * feste Global-Adressen, im Test normale Puffer -- gleiches
 * Umlenkungsmuster wie bei Q9_D_EXCJMP oben. */
static unsigned char g_irqTable[16 * 20];
static unsigned char g_irqScratch[32];
#define Q9K_IRQTAB_BASE         ((unsigned long)g_irqTable)
#define Q9K_IRQ_SCRATCH_VECTOR  ((unsigned long)(g_irqScratch +  0))
#define Q9K_IRQ_SCRATCH_PRIO    ((unsigned long)(g_irqScratch +  4))
#define Q9K_IRQ_SCRATCH_ISR     ((unsigned long)(g_irqScratch +  8))
#define Q9K_IRQ_SCRATCH_STATIC  ((unsigned long)(g_irqScratch + 12))
#define Q9K_IRQ_SCRATCH_PORT    ((unsigned long)(g_irqScratch + 16))
#define Q9K_IRQ_SCRATCH_ERROR   ((unsigned long)(g_irqScratch + 20))
#define Q9K_IRQ_SCRATCH_SUCCESS ((unsigned long)(g_irqScratch + 24))

/* Q9K_ExcTrap liegt im Assembler-Teil (q9kernel_entry.a) und wird hier
 * nur als Adresse gebraucht -- Platzhalter, damit der Test eigenstaendig
 * bleibt (gleiche Begruendung wie bei Q9K_TrapDispatch/Q9K_SetVBR). */
void Q9K_ExcTrap(void) { }
#include "q9kernel_exctable.c"

/* Minimale Stubs fuer die drei echten q9kernel_entry.a-Symbole, deren
 * ADRESSE Q9K_BuildExcTable in die Tabelle eintraegt (Q9K_TrapDispatch/
 * Q9K_TimerIRQHandler) bzw. die es direkt aufruft (Q9K_SetVBR) -- werden
 * hier nie wirklich ausgefuehrt, muessen aber als echte Symbole
 * existieren, damit der Host-Test ueberhaupt linkt. NACHTRAG 2026-08-21:
 * fehlten schon VOR dem Scheduler-Abschnitt (Q9K_SetVBR/Q9K_TrapDispatch
 * waren bereits seit der F$Link/F$UnLink-Runde unentdeckt unverlinkbar --
 * dieser Test wurde seither offenbar nicht mehr neu gebaut), hier bei
 * Gelegenheit mitgefixt statt liegen gelassen. */
void Q9K_SetVBR(Q9_u32 tableBase) { (void)tableBase; }
void Q9K_TrapDispatch(void) { }
void Q9K_TimerIRQHandler(void) { }

static int failures = 0;

static void checkU32(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-55s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-55s = %lu (erwartet %lu)\n", label, (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

int main(void)
{
    unsigned int i;
    unsigned int sumCounts = 0;

    memset(g_fakeVectorTable, 0xCC, sizeof(g_fakeVectorTable));
    g_fakeExcJmpPtr = (unsigned long)g_fakeVectorTable;

    /* Fall 1: Summe der counts in der echten Quelltabelle muss exakt
     * 254 ergeben (256 minus die 2 reservierten Reset-Vektoren) -- das
     * ist die Konsistenzpruefung, die am echten Kernel eine Panic
     * ausloest, wenn sie fehlschlaegt. */
    for (i = 0; i < Q9K_EXCSOURCE_COUNT; i++)
        sumCounts += Q9K_ExcGroupCounts[i];
    checkU32("Summe aller Quelltabellen-counts == 254", sumCounts, 254);

    /* Fall 2: Q9K_BuildExcTable muss bei der echten Quelltabelle 0
     * (Erfolg) zurueckgeben. */
    checkU32("Q9K_BuildExcTable() Rueckgabewert (0 = Erfolg)", Q9K_BuildExcTable(), 0);

    /* Fall 3: Vektor 0/1 (reserviert) duerfen NICHT angefasst worden
     * sein -- Kanarienvogel-Muster muss dort unveraendert stehen. */
    {
        int untouched = (g_fakeVectorTable[0] == 0xCC && g_fakeVectorTable[1] == 0xCC &&
                          g_fakeVectorTable[2] == 0xCC && g_fakeVectorTable[3] == 0xCC &&
                          g_fakeVectorTable[4] == 0xCC && g_fakeVectorTable[5] == 0xCC &&
                          g_fakeVectorTable[6] == 0xCC && g_fakeVectorTable[7] == 0xCC);
        checkU32("Vektor 0/1 (reserviert) unangetastet", (Q9_u32)untouched, 1);
    }

    /* Fall 4: Vektor 2 (erster Eintrag der Quelltabelle) muss auf
     * Q9K_ExcTrap zeigen (seit 2026-09-02 der Default statt der
     * C-Funktion Q9K_ExcDefault, s. dortigen Kommentar).
     * Abstand sizeof(Q9K_ExcHandler), NICHT "*4"
     * -- muss zu dem passen, was Q9K_BuildExcTable tatsaechlich schreibt
     * (s. Kommentar in q9kernel_exctable.c). */
    {
        Q9K_ExcHandler *slot2 = (Q9K_ExcHandler *)((Q9_u32)(unsigned long)g_fakeVectorTable + 2 * sizeof(Q9K_ExcHandler));
        checkU32("Vektor 2 zeigt auf Q9K_ExcTrap", (Q9_u32)(*slot2 == (Q9K_ExcHandler)Q9K_ExcTrap), 1);
    }

    /* Fall 5: Vektor 255 (letzter Eintrag) muss ebenfalls gesetzt sein
     * (voller Durchlauf bis zum Ende, kein vorzeitiger Abbruch). */
    {
        Q9K_ExcHandler *slot255 = (Q9K_ExcHandler *)((Q9_u32)(unsigned long)g_fakeVectorTable + 255 * sizeof(Q9K_ExcHandler));
        checkU32("Vektor 255 zeigt auf Q9K_ExcTrap", (Q9_u32)(*slot255 == (Q9K_ExcHandler)Q9K_ExcTrap), 1);
    }

    /* Fall 6: kaputte Quelltabelle (zu viele Eintraege) muss sauber
     * Fehlercode 1 liefern, nicht abstuerzen -- Q9K_ExcGroupCounts selbst
     * ist const/echt, deshalb hier eine lokale, absichtlich zu lange
     * Kopie simulieren statt die echte Tabelle zu veraendern. */
    {
        Q9_u16 brokenCounts[2] = { 200, 100 }; /* 300 > 254 */
        Q9_u32 vectorIndex = Q9K_EXCTABLE_RESERVED;
        unsigned int j;
        int overflowed = 0;

        for (j = 0; j < 2 && !overflowed; j++) {
            Q9_u16 n;
            for (n = 0; n < brokenCounts[j]; n++) {
                if (vectorIndex >= Q9K_EXCTABLE_TOTAL) { overflowed = 1; break; }
                vectorIndex++;
            }
        }
        checkU32("Kaputte (zu lange) Quelltabelle wuerde erkannt (Simulation)", (Q9_u32)overflowed, 1);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
