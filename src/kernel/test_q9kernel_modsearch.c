/*
 * test_q9kernel_modsearch.c -- Regressionstest fuer q9kernel_modsearch.c.
 *
 * Reines portables C (Host-gcc), kein OS-9-Cross-Build noetig fuer den
 * Teil, der hier tatsaechlich getestet wird.
 *
 * EHRLICHE EINSCHRAENKUNG (2026-08-18, empirisch auf diesem Host
 * herausgefunden, nicht nur vermutet): Q9K_FindModuleByName interpretiert
 * Speicherregion-Adressen als echte 4-Byte-Werte (richtig fuers 32-Bit-
 * Zielsystem, wo eine Adresse per Definition in 4 Byte passt). Auf einem
 * 64-Bit-Host-Rechner passt ein echter Pointer da nicht rein. Versucht:
 * mmap an eine feste, niedrige 32-Bit-taugliche Adresse (0x01000000 bis
 * 0xF0000000, mehrere Kandidaten) -- MAP_FIXED wird auf diesem Host
 * (macOS/Apple Silicon) fuer JEDE Adresse unterhalb ca. 0x100000000
 * (4 GiB) mit ENOMEM verweigert, vermutlich Hardened-Runtime/PAC-
 * bedingt. Deshalb NICHT end-to-end testbar auf diesem Host -- das ist
 * eine Plattformeinschraenkung, kein Kernel-Logik-Fehler. Echte
 * Verifikation der vollen Scan-Schleife muesste in der echten
 * Zielumgebung (Q9-Flux/QEMU) oder auf einem Host mit MAP_32BIT
 * (z.B. Linux) passieren.
 *
 * Stattdessen hier getestet: Q9K_NamesMatch (die gross-/klein-
 * schreibungsunabhaengige Namensvergleichslogik) direkt, mit normalen
 * Host-Pointern -- die braucht keine Adress-Rekonstruktion und ist
 * dadurch echt testbar. Das ist der Teil, der beim urspruenglichen Fund
 * (Thema 01, XOR+andi.b #$DF-Maskierung) am ehesten subtil falsch sein
 * koennte (Off-by-one, NUL-Behandlung, Laengenbegrenzung).
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -o test_q9kernel_modsearch test_q9kernel_modsearch.c && \
 *       ./test_q9kernel_modsearch
 *
 * Exit-Code 0 = alle Tests bestanden, 1 = mindestens ein Fehlschlag.
 */

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

/* Q9K_NamesMatch 1:1 aus q9kernel_modsearch.c uebernommen (nicht per
 * #include, weil die restliche Datei die nicht-testbaren, absolute-
 * Adress-Funktionen enthaelt und externe, hier nicht vorhandene
 * Symbole (Q9K_CheckSyncWord etc.) braucht). Wenn sich das Original
 * aendert, muss diese Kopie mitgezogen werden -- klar dokumentiertes
 * Risiko, s. Kopfkommentar. */
static int Q9K_NamesMatch(const Q9_u8 *moduleName, Q9_u32 nameMaxLen, const char *targetName)
{
    Q9_u32 i;

    for (i = 0; i < nameMaxLen; i++) {
        Q9_u8 a = moduleName[i];
        Q9_u8 b = (Q9_u8)targetName[i];
        Q9_u8 aLower = (a >= 'A' && a <= 'Z') ? (Q9_u8)(a + ('a' - 'A')) : a;
        Q9_u8 bLower = (b >= 'A' && b <= 'Z') ? (Q9_u8)(b + ('a' - 'A')) : b;

        if (aLower != bLower)
            return 0;
        if (a == 0)
            return 1;
    }
    return 0;
}

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check(const char *label, int got, int want)
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
    check("\"kernel\" == \"kernel\"", Q9K_NamesMatch((const Q9_u8 *)"kernel\0xx", 9, "kernel"), 1);
    check("\"KERNEL\" == \"kernel\" (Grossschreibung)", Q9K_NamesMatch((const Q9_u8 *)"KERNEL\0xx", 9, "kernel"), 1);
    check("\"KeRnEl\" == \"kernel\" (gemischt)", Q9K_NamesMatch((const Q9_u8 *)"KeRnEl\0xx", 9, "kernel"), 1);
    check("\"init\" == \"kernel\" (falscher Name)", Q9K_NamesMatch((const Q9_u8 *)"init\0xxxx", 9, "kernel"), 0);
    check("\"kernelx\" == \"kernel\" (Praefix, kein exakter Treffer)", Q9K_NamesMatch((const Q9_u8 *)"kernelx\0x", 9, "kernel"), 0);
    check("\"kern\" == \"kernel\" (zu kurz)", Q9K_NamesMatch((const Q9_u8 *)"kern\0xxxx", 9, "kernel"), 0);
    check("leerer Name, leeres Ziel", Q9K_NamesMatch((const Q9_u8 *)"\0xxxxxxxx", 9, ""), 1);
    check("nameMaxLen=0 (nichts lesbar)", Q9K_NamesMatch((const Q9_u8 *)"kernel\0xx", 0, "kernel"), 0);

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
