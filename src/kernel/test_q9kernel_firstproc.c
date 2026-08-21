/*
 * test_q9kernel_firstproc.c -- Regressionstest fuer q9kernel_firstproc.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. NACHTRAG
 * 2026-08-21 (Abschnitt "Scheduler"): komplett auf Q9K_ProcCreate(entryPC,
 * priority) umgestellt, ersetzt das fruehere parameterlose
 * Q9K_StartFirstProcess. Q9K_SchedInsert (extern, real in q9kernel_sched.c)
 * wird hier durch einen einfachen lokalen Stub ersetzt (Append + Age=
 * Prioritaet, gleiche minimale Nachbildung wie schon Q9K_AllocMem als
 * Fake-Bump-Allocator) -- kein echtes q9kernel_sched.c wird eingebunden,
 * damit dieser Test unabhaengig von dessen eigenem Test bleibt (der prueft
 * Q9K_SchedInsert selbst bereits ausfuehrlich, s. test_q9kernel_sched.c).
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_firstproc test_q9kernel_firstproc.c && \
 *       ./test_q9kernel_firstproc
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x2000];
static unsigned char g_fakePool[1 << 16];
static unsigned long g_fakePoolNext;

/* Grosszuegige, getrennte Testadressen -- gleiche Begruendung wie in
 * test_q9kernel_tables.c (echte Nachbar-Offsets koennten beim 8-Byte-
 * breiten Q9_u32 auf diesem Host ueberlappen, hier aber ohnehin nicht
 * Gegenstand des Tests). */
#define Q9_D_ACTIVQ             ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9K_PROCPOOL_FREE_ADDR  ((unsigned long)(g_fakeGlobals + 0x040))

/* State/Priority/Age/Next/Prev/SavedSP/EntryPC liegen im echten Deskriptor
 * nur wenige Byte auseinander (+0x00/+0x01/+0x02/+0x30/+0x34/+0x38/+0x3C)
 * -- auf diesem 64-Bit-Testhost (Q9_u32 = 8 Byte) wuerde ein
 * Q9K_SetU32-Schreibzugriff Nachbarfelder ueberschreiben (gleicher Fund
 * wie schon bei Q9_D_MODDIR_END in test_q9kernel_tables.c). Hier deshalb
 * NUR fuer diesen Test grosszuegig auf 8-Byte-Schritte gelegt -- betrifft
 * NICHT den echten Deskriptor (der bleibt bei den realen Offsets aus
 * q9kernel_firstproc.c). */
#define Q9K_READYQ_NEXT_OFF      0x08UL
#define Q9K_READYQ_PREV_OFF      0x10UL
#define Q9K_PROCDESC_PRIORITY_OFF 0x18UL
#define Q9K_PROCDESC_SAVEDSP_OFF 0x20UL
#define Q9K_PROCDESC_ENTRYPC_OFF 0x28UL

/* Minimaler Stub fuer das echte Q9K_GetA6 (q9kernel_entry.a) -- liefert
 * hier einen erfundenen, aber erkennbaren "a6-waere-hier"-Kanarienwert
 * statt eines echten Registerinhalts (den es auf dem Testhost gar nicht
 * gibt), damit Q9K_ProcCreate ihn unveraendert in den Fake-Rahmen
 * uebernimmt und der Test ihn dort wiederfinden kann. */
#define Q9K_FAKE_A6_CANARY 0xCAFEUL
unsigned long Q9K_GetA6(void) { return Q9K_FAKE_A6_CANARY; }

unsigned long Q9K_AllocMem(unsigned long requestedSize)
{
    unsigned long addr;
    if (g_fakePoolNext + requestedSize > sizeof(g_fakePool))
        return 0;
    addr = (unsigned long)(g_fakePool + g_fakePoolNext);
    g_fakePoolNext += requestedSize;
    return addr;
}

/* Minimaler Stub fuer das echte Q9K_SchedInsert (q9kernel_sched.c) --
 * reale Funktion wird DORT bereits ausfuehrlich getestet (s. Kopf-
 * kommentar). Hier nur genug, um Q9K_ProcCreates Aufruf nachzubilden:
 * Age=Prioritaet setzen, hinten an Q9_D_ACTIVQ anhaengen. */
#define Q9K_PROCDESC_AGE_OFF 0x30UL
static void Q9K_SchedInsert(unsigned long desc)
{
    unsigned long tail = *(unsigned long *)(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF);
    unsigned char priority = *(unsigned char *)(desc + Q9K_PROCDESC_PRIORITY_OFF);

    *(unsigned short *)(desc + Q9K_PROCDESC_AGE_OFF) = (unsigned short)priority;

    *(unsigned long *)(desc + Q9K_READYQ_NEXT_OFF) = Q9_D_ACTIVQ;
    *(unsigned long *)(desc + Q9K_READYQ_PREV_OFF) = tail;
    *(unsigned long *)(tail + Q9K_READYQ_NEXT_OFF) = desc;
    *(unsigned long *)(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF) = desc;
}

#include "q9kernel_firstproc.c"

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

/* Baut einen einfachen Pool aus count Slots (je slotSize Byte) ab base
 * zu einer Freiliste zusammen -- eigene Kopie der Logik aus
 * q9kernel_tables.c (dort nicht exportiert, bewusst schlank gehalten),
 * damit dieser Test unabhaengig von q9kernel_tables.c bleibt. */
static void buildFreeList(Q9_u32 base, Q9_u32 slotSize, Q9_u32 count, Q9_u32 freeHeadAddr)
{
    Q9_u32 i;
    for (i = 0; i < count - 1; i++)
        Q9K_SetU32(base + i * slotSize, base + (i + 1) * slotSize);
    Q9K_SetU32(base + (count - 1) * slotSize, 0);
    Q9K_SetU32(freeHeadAddr, base);
}

static void FakeEntryA(void) { /* nie aufgerufen, nur Adresse gebraucht */ }
static void FakeEntryB(void) { /* nie aufgerufen, nur Adresse gebraucht */ }

int main(void)
{
    static unsigned char procPool[4 * 128]; /* 4 Slots a 128 Byte, wie Q9K_PROCDESC_SIZE */
    Q9_u32 poolBase = (Q9_u32)(unsigned long)procPool;
    Q9_u32 entryA = (Q9_u32)(unsigned long)FakeEntryA;
    Q9_u32 desc1, desc2;

    memset(g_fakeGlobals, 0xCC, sizeof(g_fakeGlobals));
    memset(procPool, 0, sizeof(procPool));

    /* Q9_D_ACTIVQ als leere Ringliste initialisieren -- gleiches Muster
     * wie Q9K_InitEmptyQueue in q9kernel_cinit.c (Kopf=Schwanz=sich
     * selbst). */
    Q9K_SetU32(Q9_D_ACTIVQ + Q9K_READYQ_NEXT_OFF, Q9_D_ACTIVQ);
    Q9K_SetU32(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF, Q9_D_ACTIVQ);

    buildFreeList(poolBase, 128, 4, Q9K_PROCPOOL_FREE_ADDR);

    /* Fall 1: Erfolgsfall -- Deskriptor + Stack alloziert, Ready-Queue
     * korrekt verkettet (via Q9K_SchedInsert-Stub), Fake-Rahmen plausibel
     * aufgebaut. */
    desc1 = Q9K_ProcCreate(entryA, 7);
    checkU32("Q9K_ProcCreate() liefert einen Deskriptor (!= 0)", (Q9_u32)(desc1 != 0), 1);
    checkU32("Deskriptor == Pool-Basis (erster Slot)", desc1, poolBase);

    checkU32("Freiliste hat nach Pop noch 3 Eintraege (naechster != 0 pruefbar)",
             (Q9_u32)(Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR) != 0), 1);

    checkU32("Q9_D_ACTIVQ.next zeigt jetzt auf den neuen Deskriptor",
             Q9K_GetU32(Q9_D_ACTIVQ + Q9K_READYQ_NEXT_OFF), desc1);
    checkU32("Q9_D_ACTIVQ.prev zeigt ebenfalls auf den neuen Deskriptor (einziger Eintrag)",
             Q9K_GetU32(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF), desc1);
    checkU32("Deskriptor.next zeigt zurueck auf den Sentinel",
             Q9K_GetU32(desc1 + Q9K_READYQ_NEXT_OFF), Q9_D_ACTIVQ);
    checkU32("Deskriptor.prev zeigt zurueck auf den Sentinel",
             Q9K_GetU32(desc1 + Q9K_READYQ_PREV_OFF), Q9_D_ACTIVQ);

    checkU32("Deskriptor-State == 'a' (aktiv)",
             (Q9_u32)(*(Q9_u8 *)(desc1 + Q9K_PROCDESC_STATE_OFF)), (Q9_u32)'a');
    checkU32("Deskriptor-Priority == 7 (uebergebener Wert)",
             (Q9_u32)(*(Q9_u8 *)(desc1 + Q9K_PROCDESC_PRIORITY_OFF)), 7);
    checkU32("Deskriptor.EntryPC == entryA",
             Q9K_GetU32(desc1 + Q9K_PROCDESC_ENTRYPC_OFF), entryA);

    {
        Q9_u32 sp = Q9K_GetU32(desc1 + Q9K_PROCDESC_SAVEDSP_OFF);
        checkU32("Deskriptor.SavedSP wurde gesetzt (nicht mehr Kanarienwert)", (Q9_u32)(sp != 0), 1);

        /* Fake-Rahmen-Inhalt pruefen: SR=$2000 bei +0x3C, PC=entryA bei
         * +0x3E, Format/Vektor-Wort=0 bei +0x42 (s. Kopfkommentar
         * q9kernel_firstproc.c). Register D0-D7/A0-A6 (+0x00..+0x3B)
         * muessen genullt sein.
         *
         * WICHTIG: PC/A6 hier bewusst NICHT ueber Q9K_GetU32 pruefen --
         * dieser Fake-Rahmen ist ein byte-genau gepackter ECHTER
         * Hardware-Frame (68030-Kurzformat), auf dem echten 32-Bit-Ziel
         * ist Q9_u32 exakt 4 Byte breit und die Felder liegen deshalb
         * luecklos hintereinander (PC direkt hinter SR, Format-Wort
         * direkt hinter PC). Auf DIESEM 64-Bit-Testhost ist Q9_u32 aber
         * 8 Byte breit (unsigned long) -- ein Q9K_GetU32-Lesezugriff auf
         * PC (+0x3E) wuerde deshalb 2 Byte ueber das Format-Wort hinaus
         * lesen, ein Zugriff auf A6 (+0x38) 2 Byte in SR hinein --
         * gleicher, bereits an anderer Stelle dokumentierter Fund
         * (Q9_u32 4 vs. 8 Byte, s. Kopfkommentar test_q9kernel_sched.c)
         * -- HIER kein Testfehler in Q9K_ProcCreate, sondern eine
         * Eigenschaft des Testhelfers selbst. Deshalb hier bewusst
         * schmale, hostunabhaengige 4-Byte-Zugriffe (unsigned int). */
        checkU32("Fake-Rahmen: SR == $2000",
                 (Q9_u32)*(unsigned short *)(sp + 0x3C), 0x2000);
        checkU32("Fake-Rahmen: PC == entryA",
                 (Q9_u32)*(unsigned int *)(sp + 0x3E), (Q9_u32)(unsigned int)entryA);
        checkU32("Fake-Rahmen: Format/Vektor-Wort == 0",
                 (Q9_u32)*(unsigned short *)(sp + 0x42), 0);
        checkU32("Fake-Rahmen: D0-Slot (erstes Registerfeld) genullt",
                 (Q9_u32)*(unsigned int *)(sp + 0x00), 0);
        checkU32("Fake-Rahmen: A6-Slot (+0x38) traegt den echten a6-Wert (Q9K_GetA6), NICHT 0",
                 (Q9_u32)*(unsigned int *)(sp + 0x38), (Q9_u32)Q9K_FAKE_A6_CANARY);
    }

    /* Fall 2: zweiter Aufruf -- zweiter Deskriptor muss HINTER dem
     * ersten in die Ready-Queue eingehaengt werden (append, nicht
     * ueberschreiben). */
    desc2 = Q9K_ProcCreate((Q9_u32)(unsigned long)FakeEntryB, 3);
    checkU32("Zweiter Q9K_ProcCreate() ebenfalls erfolgreich", (Q9_u32)(desc2 != 0), 1);
    checkU32("Sentinel.prev zeigt jetzt auf den ZWEITEN Deskriptor",
             Q9K_GetU32(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF), desc2);
    checkU32("Erster Deskriptor zeigt jetzt auf den zweiten (next)",
             Q9K_GetU32(desc1 + Q9K_READYQ_NEXT_OFF), desc2);

    /* Fall 3: Pool-Erschoepfung (nur noch 2 Slots frei, beide schon
     * verbraucht) -- muss sauber 0 liefern, nicht abstuerzen. */
    Q9K_ProcCreate(entryA, 1); /* verbraucht 3. Slot */
    Q9K_ProcCreate(entryA, 1); /* verbraucht 4. und letzten Slot */
    checkU32("Fuenfter Aufruf nach Pool-Erschoepfung schlaegt sauber fehl (0)",
             Q9K_ProcCreate(entryA, 1), 0);

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
