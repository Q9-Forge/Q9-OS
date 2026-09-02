/*
 * test_q9kernel_iopath.c -- Regressionstest fuer q9kernel_iopath.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. Pool-
 * Adressen werden per #define VOR dem #include auf echte Testpuffer
 * umgebogen.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_iopath test_q9kernel_iopath.c && \
 *       ./test_q9kernel_iopath
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_pathPool[4 * 32];    /* 4 Slots a 32 Byte, wie real */
static unsigned char g_poolGlobals[16];     /* nur BASE/FREE-Zeigerfelder */

#define Q9K_PATHPOOL_BASE_ADDR ((unsigned long)(g_poolGlobals + 0x00))
#define Q9K_PATHPOOL_FREE_ADDR ((unsigned long)(g_poolGlobals + 0x08))

#include "q9kernel_iopath.c"

static int failures = 0;

static void checkU32(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-70s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-70s = %lu (erwartet %lu)\n", label, (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

/* Baut die Freiliste manuell auf (normalerweise Q9K_BuildFreeList,
 * q9kernel_tables.c -- hier bewusst eigenstaendig nachgebildet, um
 * diesen Test unabhaengig von jener Datei zu halten). */
static void buildFreeList(Q9_u32 base, Q9_u32 slotSize, Q9_u32 count, Q9_u32 freeHeadAddr)
{
    Q9_u32 i;
    for (i = 0; i < count; i++) {
        Q9_u32 slot = base + i * slotSize;
        Q9_u32 next = (i + 1 < count) ? (base + (i + 1) * slotSize) : 0;
        Q9K_SetU32(slot, next);
    }
    Q9K_SetU32(freeHeadAddr, base);
}

int main(void)
{
    Q9_u32 poolBase = (Q9_u32)(unsigned long)g_pathPool;
    static const char name1[] = "/term";
    static const char name2[] = "/dd/SYS/motd";
    Q9_u32 name1Addr = (Q9_u32)(unsigned long)name1;
    Q9_u32 name2Addr = (Q9_u32)(unsigned long)name2;
    Q9_u32 past = 0;
    Q9_u32 num1, num2, num3, num4, num5;

    memset(g_poolGlobals, 0, sizeof(g_poolGlobals));
    memset(g_pathPool, 0xCC, sizeof(g_pathPool));

    Q9K_SetU32(Q9K_PATHPOOL_BASE_ADDR, poolBase);
    buildFreeList(poolBase, Q9K_PATHDESC_SIZE, 4, Q9K_PATHPOOL_FREE_ADDR);

    /* Fall 1: "/term" oeffnen -- erwartete erste Pfadnummer = 3
     * (Slot 0, s. Kopfkommentar "Offset 3"). */
    num1 = Q9K_ProcIOpen(0, name1Addr, &past);
    checkU32("F1: erste Pfadnummer == 3", num1, 3);
    checkU32("F1: past-Zeiger zeigt hinter das NUL-Byte",
             past, name1Addr + (Q9_u32)sizeof(name1));
    checkU32("F1: Slot 0 als Konsole markiert (TYPE==1)",
             Q9K_GetU32(poolBase + Q9K_PATHDESC_TYPE_OFF), Q9K_PATHDESC_TYPE_CONSOLE);

    /* Fall 2: zweiter, unabhaengiger Pfad -- naechste Pfadnummer = 4. */
    num2 = Q9K_ProcIOpen(1, name2Addr, &past);
    checkU32("F2: zweite Pfadnummer == 4", num2, 4);
    checkU32("F2: past-Zeiger fuer den LAENGEREN Namen korrekt",
             past, name2Addr + (Q9_u32)sizeof(name2));

    /* Fall 3+4: Pool hat noch 2 freie Slots -- beide erfolgreich. */
    num3 = Q9K_ProcIOpen(0, name1Addr, &past);
    num4 = Q9K_ProcIOpen(0, name1Addr, &past);
    checkU32("F3: dritte Pfadnummer == 5", num3, 5);
    checkU32("F4: vierte Pfadnummer == 6", num4, 6);

    /* Fall 5: Pool erschoepft (alle 4 Slots vergeben) -- muss sauber
     * mit 0 fehlschlagen, kein Absturz. */
    num5 = Q9K_ProcIOpen(0, name1Addr, &past);
    checkU32("F5: Pool erschoepft -- Rueckgabe 0", num5, 0);

    /* --- F$AllPD (Callcode $30), 2026-09-02 -------------------------
     * Konvention und DBT-Aufbau sind aus IOMans Disassemblierung
     * abgelesen, s. Kopfkommentar von Q9K_ProcAllPD. Geprueft werden:
     * Index 0 wird uebersprungen, der Deskriptor traegt seine eigene
     * Nummer big-endian an Offset 0 (IOMan vergleicht das), der Zeiger
     * landet im richtigen DBT-Slot, belegte Slots werden uebersprungen,
     * und die beiden Fehlerfaelle liefern die richtigen Codes. */
    {
        static unsigned char dbt[4 + 8 * 4];
        Q9_u32 dbtAddr = (Q9_u32)(unsigned long)dbt;
        Q9_u32 desc = 0;
        Q9_u16 num = 0, err = 0;
        int ok;

        printf("\n--- F$AllPD ---\n");

        memset(dbt, 0, sizeof dbt);
        dbt[0] = 0; dbt[1] = 8;                 /* hoechster Index = 8, big-endian */
        buildFreeList(poolBase, 32, 4, Q9K_PATHPOOL_FREE_ADDR);

        ok = Q9K_ProcAllPD(dbtAddr, &desc, &num, &err);
        checkU32("F$AllPD Erfolg", (Q9_u32)ok, 1);
        checkU32("F$AllPD erste Nummer ist 1 (Index 0 uebersprungen)", (Q9_u32)num, 1);
        checkU32("F$AllPD Deskriptor = erster Pool-Slot", desc, poolBase);
        checkU32("F$AllPD Nummer big-endian im Deskriptor", (Q9_u32)Q9K_ReadU16BE(desc), 1);
        /* Nur die unteren 32 Bit vergleichen: die DBT-Slots sind 4 Byte breit
         * (68k-Zeigerbreite), auf dem 64-Bit-Hosttest passt ein echter
         * Zeiger dort nicht vollstaendig hinein. */
        checkU32("F$AllPD DBT-Slot 1 zeigt auf den Deskriptor", Q9K_ReadU32BE_At(dbtAddr + 4), desc & 0xFFFFFFFFUL);

        ok = Q9K_ProcAllPD(dbtAddr, &desc, &num, &err);
        checkU32("F$AllPD zweite Nummer ist 2", (Q9_u32)num, 2);
        checkU32("F$AllPD DBT-Slot 2 zeigt auf den Deskriptor", Q9K_ReadU32BE_At(dbtAddr + 8), desc & 0xFFFFFFFFUL);

        /* belegter Slot wird uebersprungen */
        memset(dbt, 0, sizeof dbt);
        dbt[0] = 0; dbt[1] = 8;
        Q9K_WriteU32BE_At(dbtAddr + 4, 0xDEADBEEFUL);   /* Slot 1 belegt */
        buildFreeList(poolBase, 32, 4, Q9K_PATHPOOL_FREE_ADDR);
        ok = Q9K_ProcAllPD(dbtAddr, &desc, &num, &err);
        checkU32("F$AllPD ueberspringt belegten Slot 1", (Q9_u32)num, 2);

        /* DBT voll */
        memset(dbt, 0, sizeof dbt);
        dbt[0] = 0; dbt[1] = 2;
        Q9K_WriteU32BE_At(dbtAddr + 4, 0x11111111UL);
        Q9K_WriteU32BE_At(dbtAddr + 8, 0x22222222UL);
        ok = Q9K_ProcAllPD(dbtAddr, &desc, &num, &err);
        checkU32("F$AllPD volle DBT meldet Fehlschlag", (Q9_u32)ok, 0);
        checkU32("F$AllPD volle DBT meldet E_PTHFUL", (Q9_u32)err, 0x00C8);

        /* DBT-Zeiger 0 */
        ok = Q9K_ProcAllPD(0, &desc, &num, &err);
        checkU32("F$AllPD DBT-Zeiger 0 meldet Fehlschlag", (Q9_u32)ok, 0);
        checkU32("F$AllPD DBT-Zeiger 0 meldet E_BPADDR", (Q9_u32)err, 0x00D2);

        /* Pool erschoepft */
        memset(dbt, 0, sizeof dbt);
        dbt[0] = 0; dbt[1] = 8;
        Q9K_SetU32(Q9K_PATHPOOL_FREE_ADDR, 0);
        ok = Q9K_ProcAllPD(dbtAddr, &desc, &num, &err);
        checkU32("F$AllPD erschoepfter Pool meldet Fehlschlag", (Q9_u32)ok, 0);
        checkU32("F$AllPD erschoepfter Pool meldet E_PTHFUL", (Q9_u32)err, 0x00C8);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
