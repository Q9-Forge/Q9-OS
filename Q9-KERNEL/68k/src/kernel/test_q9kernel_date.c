/*
 * test_q9kernel_date.c -- Regressionstest fuer q9kernel_date.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests; die Scratch-
 * Adressen werden per #define VOR dem #include auf einen Testpuffer
 * umgebogen.
 *
 * Die Pruefwerte sind NICHT aus der eigenen Implementierung gewonnen,
 * sondern von aussen belegt:
 *   * 1970-01-01 -> 2440587 ist woertlich `JULBASE` aus
 *     MWOS/SRC/DEFS/time.h ("julian date for Jan 1, 1970").
 *   * Alle Wochentage kommen aus der im Handbuch angegebenen Formel
 *     MOD(Julian+2, 7) und sind gegen den tatsaechlichen Wochentag
 *     geprueft.
 *   * 1582-10-15 ist der im Handbuch genannte Umstellungstag.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_date test_q9kernel_date.c && ./test_q9kernel_date
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_globals[0x100];

#define Q9K_DATE_SCRATCH_TIME    ((unsigned long)(g_globals + 0x00))
#define Q9K_DATE_SCRATCH_DATE    ((unsigned long)(g_globals + 0x20))
#define Q9K_DATE_SCRATCH_ERROR   ((unsigned long)(g_globals + 0x40))
#define Q9K_DATE_SCRATCH_SUCCESS ((unsigned long)(g_globals + 0x60))

#include "q9kernel_date.c"

static int failures;

static void check(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want)
        printf("[OK]   %-62s = %lu\n", label, (unsigned long)got);
    else {
        printf("[FAIL] %-62s = %lu (erwartet %lu)\n", label,
               (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

#define PACK_DATE(y, m, d) (((Q9_u32)(y) << 16) | ((Q9_u32)(m) << 8) | (Q9_u32)(d))
#define PACK_TIME(h, mi, s) (((Q9_u32)(h) << 16) | ((Q9_u32)(mi) << 8) | (Q9_u32)(s))

int main(void)
{
    Q9_u32 seconds, julian, packedTime, packedDate;
    Q9_u16 err;

    memset(g_globals, 0, sizeof(g_globals));

    /* Der Ankerpunkt aus time.h. Stimmt dieser Wert, stimmt der gesamte
     * Nullpunkt der Zaehlung. */
    err = 0xFFFF;
    check("F$Julian nimmt den 1970-01-01 an",
          (Q9_u32)Q9K_ProcJulian(PACK_TIME(0, 0, 0), PACK_DATE(1970, 1, 1),
                                 &seconds, &julian, &err), 1);
    check("F$Julian trifft JULBASE aus time.h", julian, 2440587UL);
    check("F$Julian meldet dabei keinen Fehler", (Q9_u32)err, 0);

    /* Wochentagsformel des Handbuchs: MOD(Julian+2, 7), 0 = Sonntag.
     * 1970-01-01 war ein Donnerstag (4). */
    check("Wochentagsformel ergibt Donnerstag fuer 1970-01-01",
          (julian + 2UL) % 7UL, 4UL);

    Q9K_ProcJulian(PACK_TIME(0, 0, 0), PACK_DATE(2000, 1, 1), &seconds, &julian, &err);
    check("Wochentagsformel ergibt Samstag fuer 2000-01-01",
          (julian + 2UL) % 7UL, 6UL);

    /* Umstellungstag: der 15.10.1582 war ein Freitag. */
    Q9K_ProcJulian(PACK_TIME(0, 0, 0), PACK_DATE(1582, 10, 15), &seconds, &julian, &err);
    check("Umstellungstag 1582-10-15 ergibt Freitag", (julian + 2UL) % 7UL, 5UL);

    /* Der julianische Kalender laeuft unmittelbar davor weiter: der
     * 4.10.1582 ist der Tag VOR dem 15.10.1582, die Tageszahlen liegen
     * deshalb genau eins auseinander -- obwohl das Kalenderdatum zehn
     * Tage springt. Genau das leistet der zweite Rechenzweig. */
    {
        Q9_u32 before, after;

        Q9K_ProcJulian(PACK_TIME(0, 0, 0), PACK_DATE(1582, 10, 4), &seconds, &before, &err);
        Q9K_ProcJulian(PACK_TIME(0, 0, 0), PACK_DATE(1582, 10, 15), &seconds, &after, &err);
        check("1582-10-04 und 1582-10-15 sind aufeinanderfolgende Tage",
              after - before, 1UL);
    }

    /* Zeitfelder: 00hhmmss -> Sekunden seit Mitternacht. */
    Q9K_ProcJulian(PACK_TIME(13, 45, 30), PACK_DATE(1970, 1, 1), &seconds, &julian, &err);
    check("F$Julian rechnet 13:45:30 in Sekunden um", seconds,
          13UL * 3600UL + 45UL * 60UL + 30UL);

    Q9K_ProcJulian(PACK_TIME(23, 59, 59), PACK_DATE(1970, 1, 1), &seconds, &julian, &err);
    check("F$Julian rechnet 23:59:59 in Sekunden um", seconds, 86399UL);

    /* Ungueltige Eingaben. */
    err = 0;
    check("F$Julian weist Stunde 24 ab",
          (Q9_u32)Q9K_ProcJulian(PACK_TIME(24, 0, 0), PACK_DATE(1970, 1, 1),
                                 &seconds, &julian, &err), 0);
    check("F$Julian meldet dabei E_BPADDR", (Q9_u32)err, Q9K_E_BPADDR);

    err = 0;
    check("F$Julian weist Monat 0 ab",
          (Q9_u32)Q9K_ProcJulian(PACK_TIME(0, 0, 0), PACK_DATE(1970, 0, 1),
                                 &seconds, &julian, &err), 0);
    err = 0;
    check("F$Julian weist Monat 13 ab",
          (Q9_u32)Q9K_ProcJulian(PACK_TIME(0, 0, 0), PACK_DATE(1970, 13, 1),
                                 &seconds, &julian, &err), 0);
    err = 0;
    check("F$Julian weist Tag 0 ab",
          (Q9_u32)Q9K_ProcJulian(PACK_TIME(0, 0, 0), PACK_DATE(1970, 1, 0),
                                 &seconds, &julian, &err), 0);

    /* F$Gregor ist laut Handbuch die Umkehrfunktion von F$Julian. Das
     * wird hier ueber einen langen Datumsbereich wirklich nachgerechnet,
     * nicht nur an Einzelwerten behauptet: jeder Tag von 1582-10-15 bis
     * weit hinter 2100 muss verlustfrei hin- und zurueckgerechnet
     * werden. */
    {
        Q9_u32 j;
        Q9_u32 mismatches = 0;
        Q9_u32 firstJ, lastJ;

        Q9K_ProcJulian(PACK_TIME(0, 0, 0), PACK_DATE(1582, 10, 15), &seconds, &firstJ, &err);
        Q9K_ProcJulian(PACK_TIME(0, 0, 0), PACK_DATE(2200, 12, 31), &seconds, &lastJ, &err);

        for (j = firstJ; j <= lastJ; ++j) {
            Q9_u32 backJ;

            if (!Q9K_ProcGregor(0, j, &packedTime, &packedDate, &err)) {
                mismatches++;
                continue;
            }
            if (!Q9K_ProcJulian(PACK_TIME(0, 0, 0), packedDate, &seconds, &backJ, &err) ||
                backJ != j) {
                mismatches++;
            }
        }
        check("F$Gregor ist ueber 1582-2200 exakt die Umkehrung von F$Julian",
              mismatches, 0);
        /* Absicherung, dass die Schleife oben wirklich gelaufen ist und
         * nicht etwa leer war: 1582-10-15 bis 2200-12-31 sind gut 618
         * Jahre, also rund 618 * 365.2425 Tage. */
        check("dabei wurden wirklich alle Tage geprueft", lastJ - firstJ + 1UL, 225798UL);
    }

    /* Einzelwerte in der Rueckrichtung, gegen dieselben Anker. */
    err = 0xFFFF;
    check("F$Gregor nimmt JULBASE an",
          (Q9_u32)Q9K_ProcGregor(0, 2440587UL, &packedTime, &packedDate, &err), 1);
    check("F$Gregor liefert dafuer den 1970-01-01", packedDate, PACK_DATE(1970, 1, 1));
    check("F$Gregor liefert dafuer 00:00:00", packedTime, PACK_TIME(0, 0, 0));

    Q9K_ProcGregor(13UL * 3600UL + 45UL * 60UL + 30UL, 2440587UL,
                   &packedTime, &packedDate, &err);
    check("F$Gregor rechnet Sekunden zurueck in 13:45:30", packedTime,
          PACK_TIME(13, 45, 30));

    Q9K_ProcGregor(86399UL, 2440587UL, &packedTime, &packedDate, &err);
    check("F$Gregor rechnet 86399 Sekunden in 23:59:59 zurueck", packedTime,
          PACK_TIME(23, 59, 59));

    err = 0;
    check("F$Gregor weist eine Sekundenzahl ab dem Tagesende ab",
          (Q9_u32)Q9K_ProcGregor(86400UL, 2440587UL, &packedTime, &packedDate, &err), 0);
    check("F$Gregor meldet dabei E_BPADDR", (Q9_u32)err, Q9K_E_BPADDR);

    err = 0;
    check("F$Gregor weist die Tageszahl 0 ab",
          (Q9_u32)Q9K_ProcGregor(0, 0, &packedTime, &packedDate, &err), 0);

    /* Die Scratch-Bruecken: beide Zellen sind EIN- UND AUSGABE, und der
     * Assembler liest Erfolg als unteres Wort einer 32-Bit-Zelle. */
    Q9K_SetU32(Q9K_DATE_SCRATCH_TIME, PACK_TIME(12, 0, 0));
    Q9K_SetU32(Q9K_DATE_SCRATCH_DATE, PACK_DATE(1970, 1, 1));
    Q9K_SysJulianImpl();
    check("F$Julian-Bridge meldet Erfolg in voller Zellbreite",
          Q9K_GetU32(Q9K_DATE_SCRATCH_SUCCESS), 1);
    check("F$Julian-Bridge ersetzt die Zeitzelle durch Sekunden",
          Q9K_GetU32(Q9K_DATE_SCRATCH_TIME), 12UL * 3600UL);
    check("F$Julian-Bridge ersetzt die Datumszelle durch die Tageszahl",
          Q9K_GetU32(Q9K_DATE_SCRATCH_DATE), 2440587UL);

    /* Direkt weiter mit F$Gregor: die Bridge muss aus ihrem eigenen
     * Ergebnis wieder die Ausgangswerte herstellen. */
    Q9K_SysGregorImpl();
    check("F$Gregor-Bridge meldet Erfolg in voller Zellbreite",
          Q9K_GetU32(Q9K_DATE_SCRATCH_SUCCESS), 1);
    check("F$Gregor-Bridge stellt die Ausgangszeit wieder her",
          Q9K_GetU32(Q9K_DATE_SCRATCH_TIME), PACK_TIME(12, 0, 0));
    check("F$Gregor-Bridge stellt das Ausgangsdatum wieder her",
          Q9K_GetU32(Q9K_DATE_SCRATCH_DATE), PACK_DATE(1970, 1, 1));

    Q9K_SetU32(Q9K_DATE_SCRATCH_TIME, PACK_TIME(24, 0, 0));
    Q9K_SetU32(Q9K_DATE_SCRATCH_DATE, PACK_DATE(1970, 1, 1));
    Q9K_SysJulianImpl();
    check("F$Julian-Bridge meldet eine ungueltige Zeit als Fehlschlag",
          Q9K_GetU32(Q9K_DATE_SCRATCH_SUCCESS), 0);
    check("F$Julian-Bridge legt E_BPADDR in voller Zellbreite ab",
          Q9K_GetU32(Q9K_DATE_SCRATCH_ERROR), Q9K_E_BPADDR);

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
