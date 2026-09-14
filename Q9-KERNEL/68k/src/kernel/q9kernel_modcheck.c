/*
 * q9kernel_modcheck.c -- Q9-OS eigener Kernel: Sync-/Pruefsummen-
 *                        Validierung fuer Modul-Kandidaten (68K).
 *
 * Bewusst in eine eigene, kleine Datei ausgelagert statt in
 * q9kernel_cinit.c mit aufgenommen -- Andreas' Hinweis (2026-08-18): der
 * QCC-Compiler hat aktuell ein Problem ab einer bestimmten Modulgroesse.
 * Vermutlich weit jenseits von Kernel-Groessenordnungen, aber
 * vorsichtshalber lieber kleine, fokussierte Uebersetzungseinheiten als
 * einen wachsenden Monolithen -- gute Gelegenheit, das gleich so
 * anzufangen.
 *
 * Algorithmus empirisch verifiziert (2026-08-18, Python-Vorabtest gegen
 * die echten Binaries, nicht nur aus der Doku uebernommen): 24 Big-
 * Endian-Words XOR-verknuepft ueber Offset 0x00-0x2F (Standard-68K-
 * Headerlaenge), muss 0xFFFF ergeben -- die intern dokumentierte
 * "24-Word-XOR-Pruefsumme" (Thema 00/10). Gegen alle acht echten
 * 68K-Kernel-Varianten in vendor/68020/ (dker/aker x 020/030 x s/b)
 * getestet: alle exakt 0xFFFF; eine einzelne verfaelschte Byte ergibt
 * nachweislich etwas anderes (Korruptionserkennung funktioniert).
 *
 * Erfolgreich gegen die echte xcc-Pipeline kompiliert (2026-08-18, alle
 * Stufen exit status = 0, urspruenglich 316-Byte-Objekt). Danach auf
 * Andreas' Nachfrage in Q9K_CheckSyncWord (billige Vorpruefung fuer die
 * Scan-Schleife) und Q9K_ValidModuleHeader (volle Pruefung, ruft die
 * erste intern auf) aufgeteilt -- erneut getestet, weiterhin exit status
 * = 0 durch alle Stufen (424-Byte-Objekt). Noch nicht mit
 * q9kernel_cinit.c zusammen gelinkt (l68).
 *
 * Braucht seit q9kernel_config.h zwingend -DQ9K_KERNEL_ATOMIC oder
 * -DQ9K_KERNEL_DEVELOPMENT PLUS -DQ9K_ALLOC_STANDARD oder
 * -DQ9K_ALLOC_BUDDY beim Bauen (sonst #error) -- inhaltlich noch ohne
 * Wirkung hier, reine Vorbereitung fuer kuenftigen variantenabhaengigen
 * Code, s. q9kernel_config.h. Alle vier Kombinationen real getestet.
 *
 * Bewusst klassische C-Typen, kein stdint.h/stddef.h -- gleiche
 * Begruendung wie in q9kernel_cinit.c (Zieltoolchain-Unsicherheit).
 */

#include "q9kernel_config.h"

typedef unsigned short Q9_u16;
typedef unsigned long  Q9_u32;
typedef unsigned char  Q9_u8;

#define Q9_SYNC_68K     0x4AFCUL
#define Q9_HDRLEN_68K   0x30

/* Liest ein Big-Endian-16-Bit-Wort ab addr (kein Pointer-Cast auf
 * Q9_u16*, um keine Annahmen ueber Host-Alignment/Endianness zu
 * brauchen -- gleiche Vorsicht wie q9moduleheader.c's readU16). */
static Q9_u16 Q9K_ReadU16BE(const Q9_u8 *addr)
{
    return (Q9_u16)(((Q9_u16)addr[0] << 8) | addr[1]);
}

/* Billige Vorpruefung fuer eine Scan-Schleife, die viele Kandidaten-
 * adressen abklappert (Andreas, 2026-08-18: "der vereinfachte Header-
 * Check, der zuerst beim Scannen laeuft") -- nur der Sync-Wort-Vergleich,
 * KEINE Pruefsumme. Genau das Muster, das laut Thema 10 auch der reale
 * Boot-ROM faehrt: erst CMPI.W #$4AFC an der Kandidatenadresse, teure
 * Pruefsumme nur bei Treffer. availableLen muss mindestens 2 Byte
 * umfassen. Eigenstaendig aufrufbar, damit die Scan-Schleife (Schritt 6a,
 * noch zu schreiben) sie pro Kandidatenadresse einzeln nutzen kann, statt
 * jedesmal die volle Pruefsumme mitzuberechnen. */
int Q9K_CheckSyncWord(const Q9_u8 *addr, Q9_u32 availableLen)
{
    if (addr == 0 || availableLen < 2)
        return 0;
    return (Q9K_ReadU16BE(addr) == Q9_SYNC_68K) ? 1 : 0;
}

/* Vollstaendige Pruefung: Sync-Wort (per Q9K_CheckSyncWord) UND 24-Word-
 * XOR-Pruefsumme ueber die ersten 0x30 Byte = 0xFFFF. Gibt 1 (gueltig)
 * oder 0 (ungueltig) zurueck. availableLen muss mindestens 0x30 Byte
 * umfassen (Bounds-Check VOR jedem Lesezugriff, keine ungeprueften
 * Lesevorgaenge ausserhalb der bekannten Region -- wichtig, weil der
 * Aufrufer (Q9K_CInit, spaeter) hiermit rohe, nicht vertrauenswuerdige
 * Boot-Zeit-Speicherbereiche scannt). Fuer die Scan-Schleife selbst: nur
 * bei einem Q9K_CheckSyncWord-Treffer aufrufen, nicht pro Adresse. */
int Q9K_ValidModuleHeader(const Q9_u8 *addr, Q9_u32 availableLen)
{
    Q9_u16 checksum;
    Q9_u32 i;

    if (availableLen < Q9_HDRLEN_68K)
        return 0;

    if (!Q9K_CheckSyncWord(addr, availableLen))
        return 0;

    checksum = 0;
    for (i = 0; i < Q9_HDRLEN_68K; i += 2)
        checksum ^= Q9K_ReadU16BE(addr + i);

    return (checksum == 0xFFFF) ? 1 : 0;
}
