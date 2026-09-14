/*
 * q9kernel_modsearch.c -- Q9-OS eigener Kernel: generische Modulsuche
 *                        per Namen in einer Speicherregion-Liste.
 *
 * Frage, 2026-08-18: "suchen wir nur init, oder auch noch andere?"
 * Antwort (s. intern dokumentiert):
 * der echte Kernel sucht an DIESER Stelle im Bootstrap nur "init" --
 * der Kernel selbst laeuft schon (braucht sich nicht selbst zu finden),
 * andere Systemmodule kommen laut Thema 10 ueber die separate, simple
 * Boot-ROM-Kette (Init->Kernel->weitere Module, contiguous per
 * Groessenfeld) rein, nicht ueber einen Namens-Scan. Deshalb wird
 * Q9K_CInit (noch zu ergaenzen) diese Funktion nur einmal mit "init"
 * aufrufen -- ABER die Funktion selbst ist bewusst generisch (Name als
 * Parameter), nicht "init" fest eingebrannt, weil derselbe Mechanismus
 * absehbar fuer ein spaeteres F$Link wiederverwendet werden kann.
 *
 * Ablauf pro Kandidat exakt wie beim echten Kernel verifiziert (Thema
 * 01): gueltiger Header per Q9K_CheckSyncWord/Q9K_ValidModuleHeader
 * (q9kernel_modcheck.c) -> bei Gueltigkeit Name gross-/klein-
 * schreibungsunabhaengig vergleichen -> bei Namenstreffer NICHT sofort
 * abbrechen, sondern weiterscannen und bei mehreren Treffern den mit
 * der hoechsten Revisionsnummer (M$Rev, Offset 0x15) behalten -- die in
 * einer frueheren Runde korrigierte Praezisierung (kein Namensvergleich-
 * Ueberspringen, sondern "nimm den ersten Treffer sofort"-Verhalten nur
 * bei einem separaten Boot-Flag, hier absichtlich NICHT nachgebaut,
 * s. u.).
 *
 * Bewusst NICHT nachgebaut: das D3-Bit-3-Sonderverhalten des echten
 * Kernels ("nimm den ersten Treffer, brich sofort ab"). Fuer unseren
 * eigenen Kernel ist der Revisions-Tiebreak allein einfacher und
 * genauso korrekt -- das Sonderverhalten war beim Original vermutlich
 * eine Boot-Zeit-Optimierung, kein Korrektheitserfordernis.
 *
 * Braucht q9kernel_config.h. Ruft Q9K_CheckSyncWord/Q9K_ValidModuleHeader
 * (q9kernel_modcheck.c) auf -- externe Deklarationen unten statt eines
 * gemeinsamen Headers (gleiche bewusst schlanke Konvention wie die
 * anderen Kernel-Dateien).
 *
 * Teststatus (2026-08-18): echte xcc-Pipeline (alle Stufen exit status
 * = 0). Q9K_NamesMatch zusaetzlich isoliert gegen Host-gcc verifiziert
 * (test_q9kernel_modsearch.c, 8 Faelle). Q9K_FindModuleByName selbst
 * NICHT end-to-end host-getestet -- interpretiert Adressen als echte
 * 4-Byte-Werte (richtig fuers 32-Bit-Zielsystem), ein Test dafuer
 * braeuchte einen Testpuffer an einer 32-Bit-tauglichen Adresse; mmap
 * mit MAP_FIXED unterhalb ca. 4 GiB wird auf diesem Host (macOS/Apple
 * Silicon) fuer jede Adresse mit ENOMEM verweigert (empirisch elf
 * Kandidatenadressen probiert). Echte Verifikation der vollen
 * Scan-Schleife braucht entweder die echte Zielumgebung (Q9-Flux/QEMU)
 * oder einen Host mit MAP_32BIT (z.B. Linux) -- s.
 * test_q9kernel_modsearch.c fuer Details.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

/* Aus q9kernel_modcheck.c */
extern int Q9K_CheckSyncWord(const Q9_u8 *addr, Q9_u32 availableLen);
extern int Q9K_ValidModuleHeader(const Q9_u8 *addr, Q9_u32 availableLen);

/* Modulheader-Offsets (s. src/q9moduleheader.h Q9_MH68K_*) -- lokal
 * dupliziert, gleiche Konvention wie die anderen Kernel-Dateien. */
#define Q9K_MH_SIZE     0x04
#define Q9K_MH_NAME     0x0C
#define Q9K_MH_REVS     0x15

#define Q9K_SCAN_STEP   2   /* 68K-Wortgrenze -- Mindestschrittweite bei ungueltigem Kandidaten */

/* Defensive Obergrenze fuer die Anzahl Regionen -- Q9K_BootList
 * (q9kernel_entry.a) ist nullterminiert, diese Grenze greift nur als
 * zusaetzliche Absicherung, falls der Terminator fehlt/beschaedigt ist
 * (kein Original-Kernel-Verhalten, eigene Vorsicht, gleiches Muster wie
 * Q9K_BootListMax dort -- hier bewusst als eigene, lokale Konstante
 * dupliziert statt ueber die Assembler-Datei referenziert, da equ-
 * Konstanten aus .a-Dateien in dieser Toolchain nicht als C-Symbole
 * sichtbar sind). */
#define Q9K_MAX_REGIONS 64

static Q9_u32 Q9K_ReadU32BE(const Q9_u8 *p)
{
    return ((Q9_u32)p[0] << 24) | ((Q9_u32)p[1] << 16) |
           ((Q9_u32)p[2] << 8) | (Q9_u32)p[3];
}

/* Gross-/kleinschreibungsunabhaengiger Namensvergleich, NUL-terminiert
 * auf beiden Seiten. moduleName darf innerhalb von availableLen liegen
 * -- bricht sicher ab, statt ueber das Regionende zu lesen. */
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
        if (a == 0) /* beide 0 (sonst waere aLower != bLower oben schon fehlgeschlagen) -> Ende, Treffer */
            return 1;
    }
    return 0; /* targetName laenger als nameMaxLen erlaubt -- kein sicherer Treffer */
}

/* Sucht ein Modul mit gegebenem Namen in einer Liste von Speicher-
 * regionen (Paare aus Basisadresse/Laenge, je 4 Byte, nullterminiert --
 * Format wie Q9K_BootList in q9kernel_entry.a). Gibt die Adresse des
 * besten Treffers zurueck (hoechste Revisionsnummer bei mehreren
 * Treffern), oder 0 wenn nichts gefunden wurde. targetName muss NUL-
 * terminiert sein und darf hoechstens 63 Zeichen lang sein (Sicherheits-
 * grenze fuer den Namensvergleich).
 *
 * outAvailableLen (optional, darf 0/NULL sein): liefert bei einem
 * Treffer, wie viele Byte ab der zurueckgegebenen Adresse sicher
 * innerhalb der urspruenglichen Region liegen (Regionende minus
 * Kandidaten-Offset) -- WICHTIG fuer den Aufrufer, der ueber den
 * standardmaessigen 0x30-Byte-Header hinaus lesen will (z.B. Init-
 * Modul-spezifische Felder ab Offset 0x62/0x68/0x7A, s.
 * q9kernel_cinit.c). Nachtrag 2026-08-18, gefunden beim Verdrahten
 * von Schritt 6a -- ohne dieses Feld haette der Aufrufer keine
 * verlaessliche Grenze fuer solche Zusatzfelder gehabt. */
const Q9_u8 *Q9K_FindModuleByName(const Q9_u8 *regionList, const char *targetName, Q9_u32 *outAvailableLen)
{
    const Q9_u8 *bestMatch = 0;
    Q9_u32 bestRevision = 0;
    Q9_u32 bestAvailableLen = 0;
    Q9_u32 regionIndex;

    if (regionList == 0 || targetName == 0)
        return 0;

    for (regionIndex = 0; regionIndex < Q9K_MAX_REGIONS; regionIndex++) {
        const Q9_u8 *entry = regionList + (regionIndex * 8);
        Q9_u32 regionBase = Q9K_ReadU32BE(entry);
        Q9_u32 regionLen = Q9K_ReadU32BE(entry + 4);
        Q9_u32 offset = 0;

        if (regionBase == 0)
            break; /* Listenende erreicht */

        while (offset + 0x30 <= regionLen) {
            const Q9_u8 *candidate = (const Q9_u8 *)(regionBase + offset);
            Q9_u32 remaining = regionLen - offset;

            if (!Q9K_CheckSyncWord(candidate, remaining)) {
                offset += Q9K_SCAN_STEP;
                continue;
            }
            if (!Q9K_ValidModuleHeader(candidate, remaining)) {
                offset += Q9K_SCAN_STEP;
                continue;
            }

            {
                Q9_u32 moduleSize = Q9K_ReadU32BE(candidate + Q9K_MH_SIZE);
                Q9_u32 nameOffset = Q9K_ReadU32BE(candidate + Q9K_MH_NAME);

                if (moduleSize == 0 || nameOffset >= remaining) {
                    offset += Q9K_SCAN_STEP; /* unplausibel, kein sinnvoller Sprung moeglich */
                    continue;
                }

                if (Q9K_NamesMatch(candidate + nameOffset, remaining - nameOffset, targetName)) {
                    Q9_u32 revision = candidate[Q9K_MH_REVS];
                    if (bestMatch == 0 || revision > bestRevision) {
                        bestMatch = candidate;
                        bestRevision = revision;
                        bestAvailableLen = remaining;
                    }
                }

                offset += moduleSize; /* zum naechsten Kandidaten in dieser Region springen */
            }
        }
    }

    if (outAvailableLen != 0)
        *outAvailableLen = bestAvailableLen;
    return bestMatch;
}
