/*
 * test_q9kernel_ssvc.c -- Regressionstest fuer q9kernel_ssvc.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. Q9_D_SYSDIS/
 * Q9_D_USRDIS werden per #define VOR dem #include auf echte Testpuffer
 * umgebogen.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_ssvc test_q9kernel_ssvc.c && \
 *       ./test_q9kernel_ssvc
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x40];   /* nur Q9_D_SYSDIS/USRDIS-Zeigerfelder */
static unsigned char g_sysdis[0x800];       /* je 0x800 Byte, wie real */
static unsigned char g_usrdis[0x800];

/* Markierungstabelle "extern per F$SSvc registriert" (2026-09-02): im
 * echten Kernel eine feste Global-Adresse, im Test ein normaler Puffer --
 * gleiches Umlenkungsmuster wie bei Q9_D_SYSDIS/Q9_D_USRDIS. */
static unsigned char g_ssvcExternal[256];
#define Q9K_SSVC_EXTERNAL_BASE ((unsigned long)g_ssvcExternal)

#define Q9_D_SYSDIS ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9_D_USRDIS ((unsigned long)(g_fakeGlobals + 0x008))

#include "q9kernel_ssvc.c"

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

/* Schreibt ein reales, grossgeschriebenes (Big-Endian) Codewort/Offset-
 * Wort-Paar an addr -- gleiche Konvention wie die Tabelle im Manual. */
static void putEntry(Q9_u32 addr, Q9_u16 codeword, Q9_u16 offsetword)
{
    unsigned char *p = (unsigned char *)addr;
    p[0] = (unsigned char)(codeword >> 8);
    p[1] = (unsigned char)codeword;
    p[2] = (unsigned char)(offsetword >> 8);
    p[3] = (unsigned char)offsetword;
}

static void putEnd(Q9_u32 addr)
{
    unsigned char *p = (unsigned char *)addr;
    p[0] = 0xFF;
    p[1] = 0xFF;
}

int main(void)
{
    static unsigned char table[64];
    Q9_u32 tableBase = (Q9_u32)(unsigned long)table;
    Q9_u32 sysdisBase = (Q9_u32)(unsigned long)g_sysdis;
    Q9_u32 usrdisBase = (Q9_u32)(unsigned long)g_usrdis;
    Q9_u32 dataPtr = 0xDEADBEEFUL;
    /* Kleine, feste Offset-Werte statt echter Host-Zeiger-Differenzen --
     * eine Differenz zwischen zwei echten Host-Adressen koennte den
     * 16-Bit-Bereich ueberschreiten (nicht garantiert klein genug), das
     * waere ein Testfehler, kein echter Bug. Q9K_ProcSSvc dereferenziert
     * die berechnete Routine-Adresse ohnehin nie -- geprueft wird nur
     * die ARITHMETIK (entryAddr + Offset + 4 == gespeicherter Wert). */
    Q9_u32 entryAddrA, entryAddrB;
    Q9_u32 routineA;

    Q9_u32 usrdisSlot7Before;

    memset(g_fakeGlobals, 0, sizeof(g_fakeGlobals));
    memset(g_sysdis, 0xCC, sizeof(g_sysdis));
    memset(g_usrdis, 0xCC, sizeof(g_usrdis));
    memset(table, 0xCC, sizeof(table));

    Q9K_SetU32(Q9_D_SYSDIS, sysdisBase);
    Q9K_SetU32(Q9_D_USRDIS, usrdisBase);

    /* Vorher-Schnappschuss fuer den "bleibt unveraendert"-Vergleich unten
     * -- ein hartkodiertes Hex-Literal ODER ein Vergleich gegen einen
     * NACHBAR-Slot waeren beide durch dieselbe Host-Breiten-Falle
     * gefaehrdet (Q9K_SetU32 schreibt hier 8 statt 4 Byte, ueberlappt
     * also benachbarte "Slots" bei dichter 4-Byte-Indizierung -- bereits
     * bei Fall 3 unten dokumentiert). */
    usrdisSlot7Before = Q9K_GetU32(usrdisBase + 7UL * 4UL);

    /* Fall 1: normaler Eintrag (Code 5, KEIN SysTrap-Bit) -- muss in
     * BEIDE Tabellen (SysDis und UsrDis) eingetragen werden. Offset =
     * +100 (klein, garantiert 16-Bit-sicher), reale Manual-Konvention
     * "Routine - EintragAdresse - 4" nachgebildet. */
    entryAddrA = tableBase;
    routineA = entryAddrA + 100UL + 4UL;
    putEntry(entryAddrA, 5, 100);

    /* Fall 2 (SysTrap-Bit gesetzt, Code 7): NUR SysDis. Negativer Offset
     * (-50), testet die Vorzeichenerweiterung -- erwarteter Wert wird
     * weiter unten separat (host-breitenkorrekt) berechnet, s. dort. */
    entryAddrB = tableBase + 4UL;
    putEntry(entryAddrB, (Q9_u16)(0x8000U | 7U), (Q9_u16)(unsigned short)(-50));

    putEnd(entryAddrB + 4UL);

    Q9K_ProcSSvc(tableBase, dataPtr);

    checkU32("F1: Code 5 (kein SysTrap) -- SysDis-Primaerarray == RoutineA",
             Q9K_GetU32(sysdisBase + 5UL * 4UL), routineA);
    checkU32("F1: Code 5 -- SysDis-Sekundaerarray == dataPtr",
             Q9K_GetU32(sysdisBase + 0x400UL + 5UL * 4UL), dataPtr);
    checkU32("F1: Code 5 -- UsrDis-Primaerarray == RoutineA (auch dort, kein SysTrap)",
             Q9K_GetU32(usrdisBase + 5UL * 4UL), routineA);
    checkU32("F1: Code 5 -- UsrDis-Sekundaerarray == dataPtr",
             Q9K_GetU32(usrdisBase + 0x400UL + 5UL * 4UL), dataPtr);

    /* NACHTRAG: Q9_u32 ist auf diesem 64-Bit-Testhost 8 statt 4 Byte
     * breit (gleiches, bereits mehrfach in diesem Projekt dokumentiertes
     * Limit) -- die Vorzeichenerweiterung in Q9K_ProcSSvc fuellt
     * ABSICHTLICH nur die oberen 16 der unteren 32 Bit (korrekt fuers
     * echte 32-Bit-Ziel, wo Q9K_SetU32 dort ohnehin nur 4 Byte
     * schreibt/die Arithmetik nativ 32-Bit-modular ist). Auf DIESEM Host
     * addiert Q9K_ProcSSvc den daraus resultierenden 32-Bit-Bitmuster-
     * Wert aber in NATIVER 64-Bit-Breite zu routineB -- kein echter
     * Ueberlauf/Wraparound wie auf dem Ziel. Der erwartete Wert wird
     * deshalb hier bewusst genauso (32-Bit-Bitmuster, 64-Bit-Addition)
     * nachgerechnet statt eine echte "-50"-Subtraktion zu erwarten --
     * pruefte sonst nur einen Host-Host-Vergleich, keine echte
     * Code-Eigenschaft. */
    {
        Q9_u32 hostSignExt = 0xFFFF0000UL | (Q9_u32)(unsigned short)(-50);
        Q9_u32 expectedRoutineB = entryAddrB + hostSignExt + 4UL;
        checkU32("F2: Code 7 (SysTrap gesetzt) -- SysDis-Primaerarray == RoutineB",
                 Q9K_GetU32(sysdisBase + 7UL * 4UL), expectedRoutineB);
    }
    checkU32("F2: Code 7 -- UsrDis-Primaerarray bleibt UNVERAENDERT (nur Supervisor-Tabelle)",
             Q9K_GetU32(usrdisBase + 7UL * 4UL), usrdisSlot7Before);

    /* Fall 3: leere Tabelle (sofortiges Ende) -- darf nichts veraendern,
     * kein Absturz. Vergleich gegen eine VORHER gelesene Kopie statt
     * eines hartkodierten Hex-Literals -- vermeidet dieselbe
     * Host-Breiten-Falle wie oben (ein hartkodiertes "0xAAAAAAAA" waere
     * auf diesem 8-Byte-Q9_u32-Host nicht dasselbe wie 8 wiederholte
     * 0xAA-Bytes). */
    {
        static unsigned char emptyTable[4];
        Q9_u32 before;

        putEnd((Q9_u32)(unsigned long)emptyTable);
        memset(g_sysdis, 0xAA, sizeof(g_sysdis));
        before = Q9K_GetU32(sysdisBase);

        Q9K_ProcSSvc((Q9_u32)(unsigned long)emptyTable, dataPtr);
        checkU32("F3: leere Tabelle laesst SysDis-Anfang unveraendert",
                 Q9K_GetU32(sysdisBase), before);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
