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

/* F$RetPD gibt einen Deskriptor an den Pool zurueck. Der Zeiger dorthin
 * stammt aus einem DBT-Slot (echte 4 Byte, 68k-Zeigerbreite) und ist auf
 * einem 64-Bit-Host nicht dereferenzierbar -- die Testpuffer liegen ueber
 * 4 GB, und niedrigen Speicher zu mappen verhindert macOS (__PAGEZERO).
 * Deshalb wird die Pool-Rueckgabe hier abgefangen und nur protokolliert. */
static unsigned long g_freedDesc;
static int           g_freedCount;
static void testPathPoolFree(unsigned long desc)
{
    g_freedDesc = desc;
    g_freedCount++;
}
#define Q9K_TEST_PATHPOOL_FREE_HOOK testPathPoolFree

static unsigned char g_pathPool[4 * 256];   /* 4 Slots a 256 Byte = PDSIZE, wie real */
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
    checkU32("I$Open traegt die Pfadnummer in PD_PD ein",
             (Q9_u32)Q9K_ReadU16BE(poolBase + Q9K_PATHDESC_NUM_OFF), 3);
    checkU32("I$Open traegt den Zugriffsmodus in PD_MOD ein -- ohne ihn verweigert IOMan jeden Zugriff",
             (Q9_u32)*(volatile unsigned char *)(unsigned long)(poolBase + Q9K_PATHDESC_MODE_OFF), 3);

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
        buildFreeList(poolBase, 256, 4, Q9K_PATHPOOL_FREE_ADDR);

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
        buildFreeList(poolBase, 256, 4, Q9K_PATHPOOL_FREE_ADDR);
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

    /* --- F$PrsNam (Callcode $10), 2026-09-02 -------------------------
     * Konvention aus dem File-Manager scf abgelesen, s. Kopfkommentar
     * von Q9K_ProcPrsNam. Geprueft: fuehrende '/' werden uebersprungen,
     * a1/a0/Laenge/Trennzeichen stimmen, mehrteilige Pfade liefern das
     * ERSTE Element, und die Fehlerfaelle melden E_BPNAM. */
    {
        static const char p1[] = "/term";
        static const char p2[] = "/dd/SYS/motd";
        static const char p3[] = "term ";
        static const char p4[] = "///";
        static const char p5[] = "";
        Q9_u32 nameStart = 0, past = 0;
        Q9_u16 len = 0, delim = 0, err = 0;
        int ok;

        printf("\n--- F$PrsNam ---\n");

        ok = Q9K_ProcPrsNam((Q9_u32)(unsigned long)p1, &nameStart, &past, &len, &delim, &err);
        checkU32("F$PrsNam \"/term\" Erfolg", (Q9_u32)ok, 1);
        checkU32("F$PrsNam \"/term\" Name beginnt bei 't'", nameStart, (Q9_u32)(unsigned long)(p1 + 1));
        checkU32("F$PrsNam \"/term\" Laenge 4", (Q9_u32)len, 4);
        checkU32("F$PrsNam \"/term\" Trennzeichen 0", (Q9_u32)delim, 0);
        checkU32("F$PrsNam \"/term\" a0 hinter dem Namen", past, (Q9_u32)(unsigned long)(p1 + 5));

        ok = Q9K_ProcPrsNam((Q9_u32)(unsigned long)p2, &nameStart, &past, &len, &delim, &err);
        checkU32("F$PrsNam \"/dd/SYS/motd\" liefert erstes Element", (Q9_u32)len, 2);
        checkU32("F$PrsNam \"/dd/...\" Trennzeichen '/'", (Q9_u32)delim, '/');
        /* ECHTER BUG GEFUNDEN + GEFIXT (2026-09-08): a0 muss HINTER dem
         * Trenner stehen (p2+4, beim 'S' von "SYS"), nicht AUF ihm
         * (p2+3) -- real per Live-Messung nachgewiesen, dass RBF diesen
         * Zeiger direkt als naechsten Suchnamen weiterverwendet, ohne
         * selbst noch einen Trenner zu ueberspringen. Mit dem alten
         * Verhalten (a0 auf dem '/') verglich RBF beim naechsten Namen
         * faelschlich gegen "/startup" statt "startup" -- das war die
         * wahre Ursache des $D8-Fehlers bei I$Open("/dd/startup"), s.
         * docs/OWN_KERNEL_STATUS.md. */
        checkU32("F$PrsNam \"/dd/...\" a0 HINTER dem '/' (beim naechsten Namen)",
                 past, (Q9_u32)(unsigned long)(p2 + 4));

        /* Kettentest: der zurueckgegebene a0-Zeiger muss sich OHNE
         * weitere Anpassung direkt als naechster Eingabezeiger eignen
         * (genau die Verkettung, die RBF laut obigem Fund tatsaechlich
         * nutzt) und "SYS" liefern. */
        {
            Q9_u32 nameStart2 = 0, past2 = 0;
            Q9_u16 len2 = 0, delim2 = 0, err2 = 0;
            int ok2 = Q9K_ProcPrsNam(past, &nameStart2, &past2, &len2, &delim2, &err2);
            checkU32("F$PrsNam Kettenaufruf liefert \"SYS\" (Laenge 3)", (Q9_u32)ok2, 1);
            checkU32("F$PrsNam Kettenaufruf Laenge 3", (Q9_u32)len2, 3);
            checkU32("F$PrsNam Kettenaufruf Name beginnt bei 'S'", nameStart2, past);
        }

        ok = Q9K_ProcPrsNam((Q9_u32)(unsigned long)p3, &nameStart, &past, &len, &delim, &err);
        checkU32("F$PrsNam ohne fuehrenden '/' Erfolg", (Q9_u32)ok, 1);
        checkU32("F$PrsNam \"term \" Trennzeichen Leerzeichen", (Q9_u32)delim, ' ');

        ok = Q9K_ProcPrsNam((Q9_u32)(unsigned long)p4, &nameStart, &past, &len, &delim, &err);
        checkU32("F$PrsNam nur Trenner meldet Fehlschlag", (Q9_u32)ok, 0);
        checkU32("F$PrsNam nur Trenner meldet E_BPNAM", (Q9_u32)err, 0x00D7);

        ok = Q9K_ProcPrsNam((Q9_u32)(unsigned long)p5, &nameStart, &past, &len, &delim, &err);
        checkU32("F$PrsNam leerer Pfad meldet Fehlschlag", (Q9_u32)ok, 0);

        ok = Q9K_ProcPrsNam(0, &nameStart, &past, &len, &delim, &err);
        checkU32("F$PrsNam Nullzeiger meldet Fehlschlag", (Q9_u32)ok, 0);
        checkU32("F$PrsNam Nullzeiger meldet E_BPNAM", (Q9_u32)err, 0x00D7);
    }

    /* --- F$RetPD (Callcode $31), 2026-09-05 -------------------------
     * Gegenstueck zu F$AllPD. Konvention aus IOMans Aufrufstelle
     * abgelesen (d0.w = Nummer, a0 = DBT), s. Q9K_ProcRetPD. Geprueft
     * werden: Freigeben raeumt den DBT-Slot, der Deskriptor kehrt in die
     * Freiliste zurueck (naechstes Allozieren bekommt ihn wieder), und
     * alle drei Fehlerfaelle. */
    {
        static unsigned char dbt[4 + 8 * 4];
        Q9_u32 dbtAddr = (Q9_u32)(unsigned long)dbt;
        Q9_u32 desc = 0;
        Q9_u16 num = 0, err = 0;
        int ok;

        printf("\n--- F$RetPD ---\n");

        memset(dbt, 0, sizeof dbt);
        dbt[0] = 0; dbt[1] = 8;
        buildFreeList(poolBase, 256, 4, Q9K_PATHPOOL_FREE_ADDR);
        g_freedDesc = 0; g_freedCount = 0;

        ok = Q9K_ProcAllPD(dbtAddr, &desc, &num, &err);
        checkU32("F$RetPD Vorbereitung: AllPD liefert Nummer 1", (Q9_u32)num, 1);

        ok = Q9K_ProcRetPD(dbtAddr, num, &err);
        checkU32("F$RetPD Erfolg", (Q9_u32)ok, 1);
        checkU32("F$RetPD DBT-Slot ist wieder frei", Q9K_ReadU32BE_At(dbtAddr + 4), 0);
        checkU32("F$RetPD gibt genau einen Deskriptor zurueck", (Q9_u32)g_freedCount, 1);
        checkU32("F$RetPD gibt den Deskriptor aus dem DBT-Slot zurueck",
                 (Q9_u32)(g_freedDesc & 0xFFFFFFFFUL), desc & 0xFFFFFFFFUL);

        ok = Q9K_ProcRetPD(dbtAddr, 0, &err);
        checkU32("F$RetPD Nummer 0 meldet Fehlschlag", (Q9_u32)ok, 0);
        checkU32("F$RetPD Nummer 0 meldet E_BPNUM", (Q9_u32)err, 0x00C9);

        ok = Q9K_ProcRetPD(dbtAddr, 9, &err);
        checkU32("F$RetPD Nummer ueber dem Hoechstindex meldet Fehlschlag", (Q9_u32)ok, 0);
        checkU32("F$RetPD Nummer ueber dem Hoechstindex meldet E_BPNUM", (Q9_u32)err, 0x00C9);

        ok = Q9K_ProcRetPD(dbtAddr, 5, &err);
        checkU32("F$RetPD schon freier Slot meldet Fehlschlag", (Q9_u32)ok, 0);
        checkU32("F$RetPD schon freier Slot meldet E_BPNUM", (Q9_u32)err, 0x00C9);

        ok = Q9K_ProcRetPD(0, 1, &err);
        checkU32("F$RetPD Null-DBT meldet Fehlschlag", (Q9_u32)ok, 0);
        checkU32("F$RetPD Null-DBT meldet E_BPADDR", (Q9_u32)err, 0x00D2);

        checkU32("F$RetPD gibt in keinem Fehlerfall etwas frei", (Q9_u32)g_freedCount, 1);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
