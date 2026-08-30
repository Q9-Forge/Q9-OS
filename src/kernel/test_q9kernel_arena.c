/*
 * test_q9kernel_arena.c -- Regressionstest fuer q9kernel_arena.c.
 *
 * Reines portables C (Host-gcc). Anders als test_q9kernel_modsearch.c
 * ist dieser Test tatsaechlich end-zu-Ende lauffaehig: q9kernel_arena.c
 * liest/schreibt ueber direkte Pointer-Casts (*(Q9_u32*)addr), keine
 * 4-Byte-Serialisierung wie das Boot-Listen-Format -- auf einem 64-Bit-
 * LP64-Host (macOS/Linux) ist Q9_u32 (=unsigned long) dort ohnehin
 * 64 Bit breit, ein echter Host-Pointer passt verlustfrei rein.
 *
 * Q9_D_ARENA/Q9K_ARENA_HEAD/Q9K_ARENA_TAIL sind im Original FESTE,
 * niedrige Adressen (Kontrollblock bei 0x3FC, Kopf/Schwanz bei +0x08/
 * +0x0C) -- auf einem normalen Host-Prozess nicht gemappt, und die reale
 * 4-Byte-Distanz zwischen Kopf/Schwanz wuerde bei einem 8-Byte-breiten
 * Q9_u32 auf diesem 64-Bit-Host ohnehin ueberlappen (s. Kommentar unten).
 * Deshalb hier per #define VOR dem #include auf zwei getrennte, echte
 * Testvariablen umgebogen (Standard-Trick fuer genau diesen Fall). Die
 * eigentliche Allokations-/Freigabelogik (das, was tatsaechlich getestet
 * werden soll) ist davon unberuehrt.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_arena test_q9kernel_arena.c && \
 *       ./test_q9kernel_arena
 *
 * Exit-Code 0 = alle Tests bestanden, 1 = mindestens ein Fehlschlag.
 */

#include <stdio.h>
#include <string.h>

/* Q9_u32 (=unsigned long) ist auf diesem 64-Bit-LP64-Host tatsaechlich
 * 8 Byte breit, nicht 4 wie auf dem echten 32-Bit-Zielsystem. Die realen
 * Q9K_ARENA_HEAD/TAIL-Offsets (+0x08/+0x0C, nur 4 Byte auseinander --
 * korrekt fuers 32-Bit-Ziel, wo Felder genuin 4 Byte breit sind) wuerden
 * sich beim 8-Byte-Q9_u32 auf diesem Host ueberlappen (per Segfault +
 * Byte-Dump gefunden, nicht nur vermutet). Deshalb Kopf/Schwanz hier auf
 * zwei GETRENNTE Testfelder umgebogen, statt die reale Byte-Distanz
 * nachzustellen -- fuer den Algorithmus-Test irrelevant, die reale
 * Struktur ist ohnehin schon separat (Thema 01) verifiziert. */
static unsigned long g_fakeArenaHead;
static unsigned long g_fakeArenaTail;

#define Q9_D_ARENA 0  /* unbenutzt, da Q9K_ARENA_HEAD/TAIL direkt ueberschrieben werden */
#define Q9K_ARENA_HEAD ((unsigned long)&g_fakeArenaHead)
#define Q9K_ARENA_TAIL ((unsigned long)&g_fakeArenaTail)
#include "q9kernel_arena.c"

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

static void checkBool(const char *label, int got, int want)
{
    if (got == want) {
        printf("[OK]   %-55s -> %d\n", label, got);
    } else {
        printf("[FAIL] %-55s -> %d (erwartet %d)\n", label, got, want);
        failures++;
    }
}

int main(void)
{
    static unsigned char pool[1024];
    Q9_u32 poolBase = (Q9_u32)(unsigned long)pool;
    Q9_u32 a, b, c;

    g_fakeArenaHead = 0;
    g_fakeArenaTail = 0;
    memset(pool, 0xCC, sizeof(pool)); /* Kanarienvogel-Muster, um versehentliches Lesen/Schreiben ausserhalb zu erkennen */

    /* Fall 1: leere Arena -- Allokation muss fehlschlagen (0) */
    checkU32("Allokation vor ArenaInit (muss fehlschlagen)", Q9K_AllocMem(16), 0);

    Q9K_ArenaInit(poolBase, sizeof(pool));

    /* Fall 2: einfache Allokation, muss am Pool-Anfang landen */
    a = Q9K_AllocMem(16);
    checkU32("Erste Allokation (16 Byte) == Pool-Basis", a, poolBase);

    /* Fall 3: zweite Allokation, muss NACH der ersten liegen (Split hat funktioniert) */
    b = Q9K_AllocMem(32);
    checkBool("Zweite Allokation liegt nach der ersten", b > a, 1);
    checkBool("Zweite Allokation ueberlappt die erste nicht (>= 16 Byte Abstand)", (b - a) >= 16, 1);

    /* Fall 4: Groesse wird auf 16 Byte aufgerundet -- eine 1-Byte-Anfrage
     * darf keinen 0-grossen Bereich liefern (indirekt getestet ueber
     * genuegend Abstand zur naechsten Allokation) */
    c = Q9K_AllocMem(1);
    checkBool("Dritte Allokation (1 Byte, wird aufgerundet) liegt nach der zweiten", c > b, 1);

    /* Fall 5: Freigeben und erneut allozieren -- muss den freigegebenen
     * Block wiederverwenden (einfache LIFO-Wiederverwendung, kein
     * Koaleszieren noetig fuer diesen Fall) */
    {
        Q9_u32 freed = Q9K_AllocMem(64);
        checkBool("Vierte Allokation erfolgreich", freed != 0, 1);
        Q9K_FreeMem(freed, 64);
        {
            Q9_u32 reused = Q9K_AllocMem(64);
            checkU32("Nach Freigabe: gleiche Adresse wiederverwendet", reused, freed);
        }
    }

    /* Fall 6: Erschoepfung -- eine zu grosse Anfrage muss sauber
     * fehlschlagen (0), nicht abstuerzen oder falsche Adresse liefern */
    checkU32("Zu grosse Anfrage (2x Poolgroesse) schlaegt sauber fehl", Q9K_AllocMem(sizeof(pool) * 2), 0);

    /* Fall 7: nach der fehlgeschlagenen Erschoepfungs-Anfrage muss eine
     * normale, kleine Allokation weiterhin funktionieren -- der
     * Fehlschlag darf die Freiliste nicht beschaedigt haben. */
    checkBool("Normale Allokation nach Fehlschlag funktioniert noch", Q9K_AllocMem(16) != 0, 1);

    /* Fall 8 (NACHTRAG 2026-08-30, Abschnitt "F$SRqMem/F$SRtMem"):
     * Q9K_AllocLargest -- frische Arena mit zwei unterschiedlich grossen
     * Freibloecken (per zwei Allokationen + gezieltem Freigeben
     * praepariert), muss den GROESSEREN finden und komplett
     * herausgeben. */
    {
        Q9_u32 outSize = 0xDEADBEEF;
        Q9_u32 x, y, z, largest;

        g_fakeArenaHead = 0;
        g_fakeArenaTail = 0;
        memset(pool, 0xCC, sizeof(pool));
        Q9K_ArenaInit(poolBase, sizeof(pool));

        /* Arena (1024 Byte) EXAKT in drei Teile zerlegen (32+800+192=1024,
         * kein Rest -- sonst bliebe ein drittes, moeglicherweise groesseres
         * Fragment frei und wuerde den Test verfaelschen). Beide ersten
         * Bloecke wieder freigeben -- x als kleinen, y als grossen
         * separaten Freiblock (kein Koaleszieren, s. Kopfkommentar
         * Q9K_FreeMem -- bleiben deshalb GETRENNTE Freiblock-Eintraege,
         * genau das, was dieser Test braucht). z bleibt belegt. */
        x = Q9K_AllocMem(32);
        y = Q9K_AllocMem(800);
        z = Q9K_AllocMem(192);
        checkBool("F8-Vorbereitung: alle drei Allokationen erfolgreich",
                  (x != 0 && y != 0 && z != 0), 1);
        Q9K_FreeMem(x, 32);
        Q9K_FreeMem(y, 800);
        /* z bleibt absichtlich belegt -- verhindert Koaleszieren-
         * unabhaengige Verwechslung, ist aber ohnehin nicht Teil der
         * Freiliste. */

        largest = Q9K_AllocLargest(&outSize);
        checkU32("Q9K_AllocLargest findet den GROESSEREN Freiblock (y, 800 Byte)", largest, y);
        checkU32("Q9K_AllocLargest liefert die echte (ungerundete) Blockgroesse", outSize, 800);

        /* Nach der Entnahme muss NUR noch der kleine Block (x, 32 Byte)
         * uebrig sein -- eine weitere Q9K_AllocLargest-Anfrage darf NICHT
         * mehr den grossen liefern (der ist ja bereits vergeben). */
        {
            Q9_u32 outSize2 = 0xDEADBEEF;
            Q9_u32 second = Q9K_AllocLargest(&outSize2);
            checkU32("Nach Entnahme: Q9K_AllocLargest findet nur noch den kleinen Block (x)", second, x);
            checkU32("... mit dessen echter Groesse (32 Byte)", outSize2, 32);
        }
    }

    /* Fall 9: Q9K_AllocLargest bei komplett leerer Arena -- liefert 0,
     * *outSize wird auf 0 gesetzt (kein Fake-Erfolg). */
    {
        Q9_u32 outSize = 0xDEADBEEF;
        Q9_u32 result;

        g_fakeArenaHead = 0;
        g_fakeArenaTail = 0;
        /* Q9K_ArenaInit NICHT aufgerufen -- Arena bleibt im leeren
         * Anfangszustand (Kopf=0). */

        result = Q9K_AllocLargest(&outSize);
        checkU32("Q9K_AllocLargest bei leerer Arena -> 0", result, 0);
        checkU32("... *outSize wird auf 0 gesetzt", outSize, 0);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
