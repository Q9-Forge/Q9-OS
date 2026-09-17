/*
 * q9kernel_date.c -- Q9-OS eigener Kernel: Datumsumrechnung
 *                    (F$Julian $20 und F$Gregor $54, 2026-09-17).
 *
 * Beide Calls sind reine Rechnung: kein Treiber, keine Systemuhr, kein
 * Prozesszustand. Sie stehen deshalb bewusst in einer eigenen Datei und
 * sind vollstaendig auf dem Host testbar.
 *
 * Verifizierte ABI (68k_tech.pdf S. 444/458):
 *
 *   F$Julian: d0.l = Zeit (00hhmmss), d1.l = Datum (yyyymmdd)
 *             -> d0.l = Sekunden seit Mitternacht, d1.l = Julianisches Datum
 *   F$Gregor: d0.l = Sekunden seit Mitternacht, d1.l = Julianisches Datum
 *             -> d0.l = Zeit (00hhmmss), d1.l = Datum (yyyymmdd)
 *
 * KODIERUNG: "yyyymmdd" und "00hhmmss" sind FELDER, keine Dezimalzahlen
 * -- das Handbuch spricht bei F$STime ausdruecklich vom "month field in
 * the date parameter". Datum = Jahr im oberen Wort, dann je ein Byte
 * Monat und Tag; Zeit = oberstes Byte 0, dann Stunde, Minute, Sekunde.
 *
 * NULLPUNKT (der eigentliche Knackpunkt, nicht geraten): OS-9 zaehlt
 * julianische Tage ab MITTERNACHT, die astronomische Zaehlung ab MITTAG
 * -- OS-9s Wert liegt deshalb um genau 1 unter der astronomischen
 * Tageszahl. Belegt aus MWOS/SRC/DEFS/time.h: `JULBASE 2440587` ist dort
 * als "julian date for Jan 1, 1970" definiert, waehrend die
 * astronomische Zahl fuer diesen Tag 2440588 ist. Gegengeprueft mit der
 * im Handbuch angegebenen Wochentagsformel MOD(Julian+2, 7) (0=Sonntag):
 * sie trifft mit diesem Nullpunkt fuer 1970-01-01 (Donnerstag),
 * 2000-01-01 (Samstag) und 1582-10-15 (Freitag) zu, mit der
 * astronomischen Zahl dagegen fuer keines davon.
 *
 * KALENDERUMSTELLUNG: ebenfalls im Handbuch festgelegt -- der Wechsel
 * vom julianischen auf den gregorianischen Kalender erfolgt am
 * 15. Oktober 1582. Davor gilt die julianische Schaltjahrregel. Das ist
 * bewusst mit implementiert und nicht als Randfall weggelassen: ohne
 * diesen Zweig lieferten alte Daten still falsche Werte.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;

/* F$Julian-/F$Gregor-Scratch (2026-09-17): hinter dem F$GPrDsc-Block
 * ($1918-$1928, q9kernel_procapi.c). Beide Calls teilen sich die
 * Zellen, da sie nie gleichzeitig laufen koennen (ein Trap nach dem
 * anderen), und beide Felder sind EIN- UND AUSGABE. */
#ifndef Q9K_DATE_SCRATCH_TIME
#define Q9K_DATE_SCRATCH_TIME    0x1930UL /* Q9_u32, d0.l EIN/AUS */
#endif
#ifndef Q9K_DATE_SCRATCH_DATE
#define Q9K_DATE_SCRATCH_DATE    0x1934UL /* Q9_u32, d1.l EIN/AUS */
#endif
#ifndef Q9K_DATE_SCRATCH_ERROR
#define Q9K_DATE_SCRATCH_ERROR   0x1938UL /* Q9_u32, d1.w AUS bei Fehler */
#endif
#ifndef Q9K_DATE_SCRATCH_SUCCESS
#define Q9K_DATE_SCRATCH_SUCCESS 0x193CUL /* Q9_u32, 0/1 */
#endif

#define Q9K_E_BPADDR 0x00D2U /* errno.h: Bad Parameter/Address */

/* Erster Tag, an dem der gregorianische Kalender gilt (s. Kopfkommentar),
 * als OS-9-Tageszahl -- daran entscheidet Q9K_DateFromJulian, welcher
 * Rueckrechnungszweig gilt. */
#define Q9K_JULIAN_GREGORIAN_START 2299160UL

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }

/* Gilt fuer (year, month, day) bereits der gregorianische Kalender? */
static int Q9K_DateIsGregorian(Q9_u32 year, Q9_u32 month, Q9_u32 day)
{
    if (year != 1582)
        return year > 1582;
    if (month != 10)
        return month > 10;
    return day >= 15;
}

/* Q9K_JulianFromDate -- Kalenderdatum in die OS-9-Tageszahl.
 *
 * Beide Zweige sind die uebliche, ganzzahlige Tageszahlformel; der
 * abschliessende "- 1" ist der im Kopfkommentar begruendete Versatz
 * zwischen Mitternachts- und Mittagszaehlung. Rueckgabe 0 = ungueltiges
 * Datum. */
Q9_u32 Q9K_JulianFromDate(Q9_u32 year, Q9_u32 month, Q9_u32 day)
{
    Q9_u32 a, y, m;

    if (month < 1 || month > 12 || day < 1 || day > 31)
        return 0;

    a = (14UL - month) / 12UL;
    y = year + 4800UL - a;
    m = month + 12UL * a - 3UL;

    if (Q9K_DateIsGregorian(year, month, day)) {
        return day + (153UL * m + 2UL) / 5UL + 365UL * y
               + y / 4UL - y / 100UL + y / 400UL - 32045UL - 1UL;
    }
    return day + (153UL * m + 2UL) / 5UL + 365UL * y
           + y / 4UL - 32083UL - 1UL;
}

/* Q9K_DateFromJulian -- Umkehrung von Q9K_JulianFromDate. */
void Q9K_DateFromJulian(Q9_u32 julian, Q9_u32 *outYear, Q9_u32 *outMonth,
                        Q9_u32 *outDay)
{
    Q9_u32 l, n, i, j, k;

    julian += 1UL;   /* zurueck auf die astronomische Tageszahl */

    if (julian >= Q9K_JULIAN_GREGORIAN_START + 1UL) {
        l = julian + 68569UL;
        n = (4UL * l) / 146097UL;
        l = l - (146097UL * n + 3UL) / 4UL;
        i = (4000UL * (l + 1UL)) / 1461001UL;
        l = l - (1461UL * i) / 4UL + 31UL;
        j = (80UL * l) / 2447UL;
        *outDay = l - (2447UL * j) / 80UL;
        l = j / 11UL;
        *outMonth = j + 2UL - 12UL * l;
        *outYear = 100UL * (n - 49UL) + i + l;
        return;
    }

    j = julian + 1402UL;
    k = (j - 1UL) / 1461UL;
    l = j - 1461UL * k;
    n = (l - 1UL) / 365UL - l / 1461UL;
    i = l - 365UL * n + 30UL;
    j = (80UL * i) / 2447UL;
    *outDay = i - (2447UL * j) / 80UL;
    i = j / 11UL;
    *outMonth = j + 2UL - 12UL * i;
    *outYear = 4UL * k + n + i - 4716UL;
}

/* Q9K_ProcJulian -- echte F$Julian-Kernlogik. Rueckgabe 1 = Erfolg. */
int Q9K_ProcJulian(Q9_u32 packedTime, Q9_u32 packedDate,
                   Q9_u32 *outSeconds, Q9_u32 *outJulian, Q9_u16 *outError)
{
    Q9_u32 year  = (packedDate >> 16) & 0xFFFFUL;
    Q9_u32 month = (packedDate >> 8) & 0xFFUL;
    Q9_u32 day   = packedDate & 0xFFUL;
    Q9_u32 hour  = (packedTime >> 16) & 0xFFUL;
    Q9_u32 min   = (packedTime >> 8) & 0xFFUL;
    Q9_u32 sec   = packedTime & 0xFFUL;
    Q9_u32 julian;

    *outError = 0;
    *outSeconds = 0;
    *outJulian = 0;

    if (hour > 23 || min > 59 || sec > 59) {
        *outError = Q9K_E_BPADDR;
        return 0;
    }

    julian = Q9K_JulianFromDate(year, month, day);
    if (julian == 0) {
        *outError = Q9K_E_BPADDR;
        return 0;
    }

    *outSeconds = hour * 3600UL + min * 60UL + sec;
    *outJulian = julian;
    return 1;
}

/* Q9K_ProcGregor -- echte F$Gregor-Kernlogik, Umkehrung von oben. */
int Q9K_ProcGregor(Q9_u32 seconds, Q9_u32 julian,
                   Q9_u32 *outTime, Q9_u32 *outDate, Q9_u16 *outError)
{
    Q9_u32 year, month, day;

    *outError = 0;
    *outTime = 0;
    *outDate = 0;

    if (julian == 0 || seconds >= 24UL * 3600UL) {
        *outError = Q9K_E_BPADDR;
        return 0;
    }

    Q9K_DateFromJulian(julian, &year, &month, &day);

    *outTime = ((seconds / 3600UL) << 16) |
               (((seconds / 60UL) % 60UL) << 8) |
               (seconds % 60UL);
    *outDate = (year << 16) | (month << 8) | day;
    return 1;
}

void Q9K_SysJulianImpl(void)
{
    Q9_u32 seconds = 0, julian = 0;
    Q9_u16 err = 0;

    if (Q9K_ProcJulian(Q9K_GetU32(Q9K_DATE_SCRATCH_TIME),
                       Q9K_GetU32(Q9K_DATE_SCRATCH_DATE),
                       &seconds, &julian, &err)) {
        Q9K_SetU32(Q9K_DATE_SCRATCH_TIME, seconds);
        Q9K_SetU32(Q9K_DATE_SCRATCH_DATE, julian);
        Q9K_SetU32(Q9K_DATE_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_DATE_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_DATE_SCRATCH_SUCCESS, 0UL);
    }
}

void Q9K_SysGregorImpl(void)
{
    Q9_u32 packedTime = 0, packedDate = 0;
    Q9_u16 err = 0;

    if (Q9K_ProcGregor(Q9K_GetU32(Q9K_DATE_SCRATCH_TIME),
                       Q9K_GetU32(Q9K_DATE_SCRATCH_DATE),
                       &packedTime, &packedDate, &err)) {
        Q9K_SetU32(Q9K_DATE_SCRATCH_TIME, packedTime);
        Q9K_SetU32(Q9K_DATE_SCRATCH_DATE, packedDate);
        Q9K_SetU32(Q9K_DATE_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_DATE_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_DATE_SCRATCH_SUCCESS, 0UL);
    }
}
