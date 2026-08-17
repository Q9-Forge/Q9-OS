/*
 * q9moduleheader.c -- Implementierung der Formaterkennung aus
 *                     q9moduleheader.h.
 *
 * Host-seitiges Tooling (reines ANSI-C, kein OS-9-Cross-Build) -- Grundlage
 * fuer ein "ident"-artiges Werkzeug (s. q9ident.c), das Module aller vier
 * bekannten Generationen (6809/68K/OS-9000/Q9-eigen) anhand des Sync-Worts
 * erkennt, unabhaengig von Byte-Reihenfolge und Pointerbreite des Ziels.
 *
 * Ehrlich offene Einschraenkung: Big-Endian-Sync $4AFC wird IMMER als 68K
 * erkannt, nie als Big-Endian-OS-9000 (PowerPC/ARM) -- fuer dieses Projekt
 * aktuell kein echtes Problem, weil hier nur 68K (BE) und x86-OS-9000 (LE)
 * als reale Dateien vorliegen (siehe modules/os9000-x86/vendor-live/), kein
 * PowerPC/ARM-OS-9000-Sample. Eine echte Unterscheidung braeuchte die
 * Pruefsummen-Verifikation gegen beide Kandidaten-Headerlaengen (46 vs. 88
 * Byte) -- absichtlich noch nicht implementiert, nicht stillschweigend
 * angenommen.
 */

#include "q9moduleheader.h"
#include <string.h>

/* Liest ein 16-Bit-Wort aus rohen Bytes, wahlweise Big- oder Little-Endian --
 * ohne Annahme ueber die Host-Endianness (kein Pointer-Cast auf uint16_t*). */
static uint16_t readU16(const uint8_t *p, int littleEndian)
{
    if (littleEndian)
        return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

Q9_ModHeadFormat Q9_DetectModuleHeaderFormat(const uint8_t *rawBytes, uint32_t availableLen)
{
    uint16_t syncBE, syncLE;

    if (rawBytes == NULL || availableLen < 2)
        return Q9_MHFMT_UNKNOWN;

    syncBE = readU16(rawBytes, 0);
    syncLE = readU16(rawBytes, 1);

    /* Q9-eigenes Format zuerst pruefen -- eigener Sync-Wert, kollisionsfrei
     * zu den beiden folgenden Generationen (s. q9moduleheader.h) */
    if (syncBE == Q9_MH_SYNC_Q9OWN || syncLE == Q9_MH_SYNC_Q9OWN)
        return Q9_MHFMT_Q9OWN;

    /* OS-9/6809 -- nur Big-Endian bekannt, keine LE-Historie */
    if (syncBE == Q9_MH_SYNC_6809)
        return Q9_MHFMT_6809;

    /* 68K/OS-9000-Sync-Familie -- Disambiguierung s. Dateikopf-Kommentar */
    if (syncBE == Q9_MH_SYNC_OS9)
        return Q9_MHFMT_68K;
    if (syncLE == Q9_MH_SYNC_OS9)
        return Q9_MHFMT_OS9000_LE;

    return Q9_MHFMT_UNKNOWN;
}

/* Liest den NUL-terminierten Namensstring eines 68K- oder OS-9000-Moduls
 * (beide Formate: 4-Byte-Offset ab Headerbasis, NUL-terminiert -- s.
 * [[feedback_q9_check_official_manuals]], die fruehere High-Bit-Annahme
 * fuer 68K war falsch). destSize inklusive Platz fuer die NUL. Gibt die
 * Anzahl kopierter Zeichen zurueck (ohne NUL), oder 0 bei Fehler. */
uint32_t Q9_ReadModuleName(const uint8_t *rawBytes, uint32_t availableLen,
                            uint32_t nameOffset, int littleEndian,
                            char *dest, uint32_t destSize)
{
    uint32_t i;

    (void)littleEndian; /* der Namensstring selbst ist reines ASCII, keine Endian-Frage */

    if (rawBytes == NULL || dest == NULL || destSize == 0)
        return 0;
    if (nameOffset >= availableLen)
        return 0;

    for (i = 0; i < destSize - 1 && (nameOffset + i) < availableLen; i++) {
        uint8_t c = rawBytes[nameOffset + i];
        if (c == 0)
            break;
        dest[i] = (char)c;
    }
    dest[i] = '\0';
    return i;
}
