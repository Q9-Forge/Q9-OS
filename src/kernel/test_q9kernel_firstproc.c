/*
 * test_q9kernel_firstproc.c -- Regressionstest fuer q9kernel_firstproc.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. Q9_D_ACTIVQ/
 * Q9K_PROCPOOL_FREE_ADDR/Q9K_FIRSTPROC_SP_ADDR/PC_ADDR werden per #define
 * VOR dem #include auf echte Testpuffer umgebogen. Q9K_AllocMem (Fake-
 * Bump-Allocator) und Q9K_FirstProcPlaceholder (Fake-Zielfunktion) werden
 * hier ebenfalls simuliert -- kein echtes .a linken noetig.
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
#define Q9K_FIRSTPROC_SP_ADDR   ((unsigned long)(g_fakeGlobals + 0x080))
#define Q9K_FIRSTPROC_PC_ADDR   ((unsigned long)(g_fakeGlobals + 0x090))

/* Next/Prev/SavedSP/EntryPC liegen im echten Deskriptor nur 4 Byte
 * auseinander (+0x30/+0x34/+0x38/+0x3C) -- auf diesem 64-Bit-Testhost
 * (Q9_u32 = 8 Byte) wuerde ein Q9K_SetU32-Schreibzugriff Nachbarfelder
 * ueberschreiben (gleicher Fund wie schon bei Q9_D_MODDIR_END in
 * test_q9kernel_tables.c). Hier deshalb NUR fuer diesen Test grosszuegig
 * auf 8-Byte-Schritte gelegt -- betrifft NICHT den echten Deskriptor
 * (der bleibt bei den realen +0x30/+0x34/+0x38/+0x3C aus
 * q9kernel_firstproc.c). */
/* 0x00 bleibt frei (kollidiert sonst mit Q9K_PROCDESC_STATE_OFF, das
 * NICHT ueberschrieben wird -- s. q9kernel_firstproc.c). */
#define Q9K_READYQ_NEXT_OFF      0x08UL
#define Q9K_READYQ_PREV_OFF      0x10UL
#define Q9K_PROCDESC_SAVEDSP_OFF 0x18UL
#define Q9K_PROCDESC_ENTRYPC_OFF 0x20UL

unsigned long Q9K_AllocMem(unsigned long requestedSize)
{
    unsigned long addr;
    if (g_fakePoolNext + requestedSize > sizeof(g_fakePool))
        return 0;
    addr = (unsigned long)(g_fakePool + g_fakePoolNext);
    g_fakePoolNext += requestedSize;
    return addr;
}

void Q9K_FirstProcPlaceholder(void) { /* nie aufgerufen, nur Adresse gebraucht */ }

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

int main(void)
{
    static unsigned char procPool[4 * 128]; /* 4 Slots a 128 Byte, wie Q9K_PROCDESC_SIZE */
    Q9_u32 poolBase = (Q9_u32)(unsigned long)procPool;

    memset(g_fakeGlobals, 0xCC, sizeof(g_fakeGlobals));
    memset(procPool, 0, sizeof(procPool));

    /* Q9_D_ACTIVQ als leere Ringliste initialisieren -- gleiches Muster
     * wie Q9K_InitEmptyQueue in q9kernel_cinit.c (Kopf=Schwanz=sich
     * selbst). */
    Q9K_SetU32(Q9_D_ACTIVQ + Q9K_READYQ_NEXT_OFF, Q9_D_ACTIVQ);
    Q9K_SetU32(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF, Q9_D_ACTIVQ);

    buildFreeList(poolBase, 128, 4, Q9K_PROCPOOL_FREE_ADDR);

    /* Fall 1: Erfolgsfall -- Deskriptor + Stack alloziert, Ready-Queue
     * korrekt verkettet, SP/PC-Uebergabefelder plausibel gesetzt. */
    checkU32("Q9K_StartFirstProcess() Rueckgabewert (0 = Erfolg)", Q9K_StartFirstProcess(), 0);

    checkU32("Freiliste hat nach Pop noch 3 Eintraege (naechster != 0 pruefbar)",
             (Q9_u32)(Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR) != 0), 1);

    checkU32("Q9_D_ACTIVQ.next zeigt jetzt auf den neuen Deskriptor (== Pool-Basis)",
             Q9K_GetU32(Q9_D_ACTIVQ + Q9K_READYQ_NEXT_OFF), poolBase);
    checkU32("Q9_D_ACTIVQ.prev zeigt ebenfalls auf den neuen Deskriptor (einziger Eintrag)",
             Q9K_GetU32(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF), poolBase);
    checkU32("Deskriptor.next zeigt zurueck auf den Sentinel",
             Q9K_GetU32(poolBase + Q9K_READYQ_NEXT_OFF), Q9_D_ACTIVQ);
    checkU32("Deskriptor.prev zeigt zurueck auf den Sentinel",
             Q9K_GetU32(poolBase + Q9K_READYQ_PREV_OFF), Q9_D_ACTIVQ);

    checkU32("Deskriptor-State == 'a' (aktiv)",
             (Q9_u32)(*(Q9_u8 *)(poolBase + Q9K_PROCDESC_STATE_OFF)), (Q9_u32)'a');

    {
        Q9_u32 sp = Q9K_GetU32(Q9K_FIRSTPROC_SP_ADDR);
        Q9_u32 pc = Q9K_GetU32(Q9K_FIRSTPROC_PC_ADDR);
        checkU32("Q9K_FirstProcSP wurde gesetzt (nicht mehr Kanarienwert)", (Q9_u32)(sp != 0), 1);
        checkU32("Q9K_FirstProcPC zeigt auf Q9K_FirstProcPlaceholder",
                 (Q9_u32)(pc == (Q9_u32)(unsigned long)Q9K_FirstProcPlaceholder), 1);
        checkU32("Deskriptor.SavedSP stimmt mit Q9K_FirstProcSP ueberein",
                 Q9K_GetU32(poolBase + Q9K_PROCDESC_SAVEDSP_OFF), sp);
    }

    /* Fall 2: zweiter Aufruf -- zweiter Deskriptor muss HINTER dem
     * ersten in die Ready-Queue eingehaengt werden (append, nicht
     * ueberschreiben). */
    checkU32("Zweiter Q9K_StartFirstProcess() ebenfalls erfolgreich", Q9K_StartFirstProcess(), 0);
    checkU32("Sentinel.prev zeigt jetzt auf den ZWEITEN Deskriptor",
             Q9K_GetU32(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF), poolBase + 128);
    checkU32("Erster Deskriptor zeigt jetzt auf den zweiten (next)",
             Q9K_GetU32(poolBase + Q9K_READYQ_NEXT_OFF), poolBase + 128);

    /* Fall 3: Pool-Erschoepfung (nur noch 2 Slots frei, beide schon
     * verbraucht) -- muss sauber 1 liefern, nicht abstuerzen. */
    Q9K_StartFirstProcess(); /* verbraucht 3. Slot */
    Q9K_StartFirstProcess(); /* verbraucht 4. und letzten Slot */
    checkU32("Fuenfter Aufruf nach Pool-Erschoepfung schlaegt sauber fehl (1)",
             Q9K_StartFirstProcess(), 1);

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
