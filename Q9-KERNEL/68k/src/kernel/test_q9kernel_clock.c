/*
 * test_q9kernel_clock.c -- Hosttest fuer die Systemuhr und F$STime
 *                          (q9kernel_clock.c, 2026-09-18).
 *
 * Bauen und laufen lassen wie jede andere Reihe hier:
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_clock test_q9kernel_clock.c && ./test_clock
 *
 * Die Kernelzellen liegen im echten Betrieb ab $1B20. Hier zeigen sie in
 * ein eigenes Feld -- gleiche Konvention wie ueberall in diesem
 * Verzeichnis, aus zwei Gruenden: auf dem Testhost ist Q9_u32 acht Byte
 * breit (die echten Abstaende wuerden ineinanderlaufen), und ein Zugriff
 * auf $1B20 waere schlicht ein Absturz.
 */

#include <stdio.h>
#include <string.h>

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;

static unsigned char g_cells[0x400];

#define CELL(off) ((unsigned long)(g_cells + (off)))

#define Q9K_CLOCK_DAY   CELL(0x000)
#define Q9K_CLOCK_SEC   CELL(0x020)
#define Q9K_CLOCK_TICK  CELL(0x040)
#define Q9K_CLOCK_VALID CELL(0x060)

#define Q9K_STIME_SCRATCH_TIME    CELL(0x080)
#define Q9K_STIME_SCRATCH_DATE    CELL(0x0A0)
#define Q9K_STIME_SCRATCH_ERROR   CELL(0x0C0)
#define Q9K_STIME_SCRATCH_SUCCESS CELL(0x0E0)

/* Die Ersatz-RTC. Q9K_RtcRead/-Fields liegen im echten Kernel in
 * q9kernel_alarm.c; hier stehen sie als Stub, damit diese Reihe die
 * Alarmverwaltung nicht mitziehen muss. */
static unsigned long g_rtcYear = 2026, g_rtcMonth = 9, g_rtcDay = 18;
static unsigned long g_rtcHour = 12, g_rtcMin = 34, g_rtcSec = 56;
static int g_rtcReads;

void Q9K_RtcReadFields(Q9_u32 *y, Q9_u32 *mo, Q9_u32 *d,
                       Q9_u32 *h, Q9_u32 *mi, Q9_u32 *s)
{
    g_rtcReads++;
    *y = g_rtcYear; *mo = g_rtcMonth; *d = g_rtcDay;
    *h = g_rtcHour; *mi = g_rtcMin;   *s = g_rtcSec;
}

extern Q9_u32 Q9K_JulianFromDate(Q9_u32 year, Q9_u32 month, Q9_u32 day);

void Q9K_RtcRead(Q9_u32 *outDay, Q9_u32 *outSeconds)
{
    Q9_u32 y, mo, d, h, mi, s;
    Q9K_RtcReadFields(&y, &mo, &d, &h, &mi, &s);
    *outDay = Q9K_JulianFromDate(y, mo, d);
    *outSeconds = h * 3600UL + mi * 60UL + s;
}

/* Reihenfolge mit Absicht: q9kernel_date.c bringt die Zellzugriffe mit,
 * q9kernel_clock.c uebernimmt sie dann statt sie erneut zu definieren.
 * Geprueft wird hier gegen die ECHTE Datumsumrechnung, nicht gegen einen
 * Nachbau -- sonst pruefte die Reihe ihre eigene Annahme. */
#include "q9kernel_date.c"
#define Q9K_CELL_ACCESSORS_PROVIDED 1
#include "q9kernel_clock.c"

static int failures;

static void check(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-62s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-62s = %lu (erwartet %lu)\n", label,
               (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

static void reset(void)
{
    memset(g_cells, 0, sizeof g_cells);
    g_rtcReads = 0;
    g_rtcYear = 2026; g_rtcMonth = 9; g_rtcDay = 18;
    g_rtcHour = 12; g_rtcMin = 34; g_rtcSec = 56;
}

/* Feldkodierte Werte, wie sie ueber die Register kommen. */
#define DATE(y, m, d) (((Q9_u32)(y) << 16) | ((Q9_u32)(m) << 8) | (Q9_u32)(d))
#define TIME(h, m, s) (((Q9_u32)(h) << 16) | ((Q9_u32)(m) << 8) | (Q9_u32)(s))

static void tickTimes(unsigned long n)
{
    unsigned long i;
    for (i = 0; i < n; i++)
        (void)Q9K_ClockTick();
}

int main(void)
{
    Q9_u32 day = 0, sec = 0;
    Q9_u32 jdStart;

    printf("== Systemuhr und F$STime ==\n");

    /* --- Kaltstart: die Uhr holt sich den Hardwarestand beim ersten
     *     Lesen und laeuft ab da selbst weiter. --- */
    reset();
    check("vor dem ersten Lesen gilt die Uhr als ungestellt",
          Q9K_GetU32(Q9K_CLOCK_VALID), 0);
    Q9K_ClockRead(&day, &sec);
    check("das erste Lesen uebernimmt den RTC-Tag",
          day, Q9K_JulianFromDate(2026, 9, 18));
    check("und die RTC-Uhrzeit", sec, 12UL * 3600UL + 34UL * 60UL + 56UL);
    check("danach gilt sie als gestellt", Q9K_GetU32(Q9K_CLOCK_VALID), 1);
    check("und die Hardware wurde genau einmal gelesen",
          (Q9_u32)g_rtcReads, 1);

    /* Genau darum geht es: ab jetzt laeuft die Zeit in Software. Eine
     * davonlaufende Hardware-Uhr darf den Softwarestand nicht mehr
     * ueberschreiben. */
    g_rtcYear = 1999; g_rtcMonth = 1; g_rtcDay = 1;
    Q9K_ClockRead(&day, &sec);
    check("weitere Lesezugriffe fassen die Hardware nicht mehr an",
          (Q9_u32)g_rtcReads, 1);
    check("und liefern weiter den Softwarestand",
          day, Q9K_JulianFromDate(2026, 9, 18));

    /* --- Der Tickzaehler --- */
    reset();
    Q9K_ClockSet(5000, 100);
    check("Stellen setzt den Tag", Q9K_GetU32(Q9K_CLOCK_DAY), 5000);
    check("und die Sekunde", Q9K_GetU32(Q9K_CLOCK_SEC), 100);
    check("und beginnt den Sekundenbruchteil neu",
          Q9K_GetU32(Q9K_CLOCK_TICK), 0);

    tickTimes(99);
    check("99 Ticks sind noch keine Sekunde", Q9K_GetU32(Q9K_CLOCK_SEC), 100);
    check("aber sie stehen im Bruchteil", Q9K_GetU32(Q9K_CLOCK_TICK), 99);
    check("der 100. Tick macht die Sekunde voll", Q9K_ClockTick(), 1);
    check("die Sekunde ist weitergezaehlt", Q9K_GetU32(Q9K_CLOCK_SEC), 101);
    check("und der Bruchteil beginnt von vorn", Q9K_GetU32(Q9K_CLOCK_TICK), 0);

    /* Ein voller Tag, der Uebergang, auf den es ankommt. */
    reset();
    Q9K_ClockSet(5000, 86399);
    tickTimes(100);
    check("die letzte Sekunde des Tages rollt auf den naechsten Tag",
          Q9K_GetU32(Q9K_CLOCK_DAY), 5001);
    check("und die Uhrzeit beginnt bei Mitternacht",
          Q9K_GetU32(Q9K_CLOCK_SEC), 0);

    /* Eine ungestellte Uhr darf nicht ab 0 hochzaehlen -- das waere ein
     * erfundener Zeitpunkt, nicht ein unbekannter. */
    reset();
    tickTimes(500);
    check("eine ungestellte Uhr zaehlt nicht", Q9K_GetU32(Q9K_CLOCK_SEC), 0);
    check("und bleibt ungestellt", Q9K_GetU32(Q9K_CLOCK_VALID), 0);
    check("und hat die Hardware dabei nicht gelesen", (Q9_u32)g_rtcReads, 0);

    /* --- F$STime ueber die Bruecke --- */
    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(2001, 2, 3));
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, TIME(4, 5, 6));
    Q9K_SysSTimeImpl();
    check("F$STime nimmt Datum und Uhrzeit an",
          Q9K_GetU32(Q9K_STIME_SCRATCH_SUCCESS), 1);
    check("und rechnet das Datum in die Tageszahl um",
          Q9K_GetU32(Q9K_CLOCK_DAY), Q9K_JulianFromDate(2001, 2, 3));
    check("und die Uhrzeit in Sekunden",
          Q9K_GetU32(Q9K_CLOCK_SEC), 4UL * 3600UL + 5UL * 60UL + 6UL);
    check("die Hardware bleibt dabei unberuehrt", (Q9_u32)g_rtcReads, 0);

    /* Was gesetzt wurde, muss auch zurueckkommen -- das war der Grund,
     * die Uhr ueberhaupt in Software zu fuehren. */
    Q9K_ClockRead(&day, &sec);
    check("und ein anschliessendes Lesen liefert genau das",
          day, Q9K_JulianFromDate(2001, 2, 3));
    check("auch die Uhrzeit", sec, 4UL * 3600UL + 5UL * 60UL + 6UL);

    /* Die gestellte Uhr laeuft weiter, sie steht nicht. */
    jdStart = Q9K_GetU32(Q9K_CLOCK_DAY);
    tickTimes(100UL * 60UL);
    Q9K_ClockRead(&day, &sec);
    check("nach einer Minute Ticks ist sie eine Minute weiter",
          sec, 4UL * 3600UL + 6UL * 60UL + 6UL);
    check("am selben Tag", day, jdStart);

    /* --- Monatsfeld 0: Datum und Uhrzeit aus der Hardware --- */
    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(0, 0, 0));
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, TIME(0, 0, 0));
    Q9K_SysSTimeImpl();
    check("Monatsfeld 0 nimmt den Stand aus der Hardware",
          Q9K_GetU32(Q9K_STIME_SCRATCH_SUCCESS), 1);
    check("mit dem Datum der Uhr",
          Q9K_GetU32(Q9K_CLOCK_DAY), Q9K_JulianFromDate(2026, 9, 18));
    check("und ihrer Uhrzeit -- die mitgegebene wird verworfen",
          Q9K_GetU32(Q9K_CLOCK_SEC), 12UL * 3600UL + 34UL * 60UL + 56UL);

    /* Das Jahrhundert steuert der Aufrufer bei: eine Uhr, die nur zwei
     * Stellen speichert, liefert nach dem Jahrhundertwechsel sonst ein
     * Datum aus der falschen Epoche. */
    reset();
    g_rtcYear = 2003;                       /* Hardware kennt nur "03" */
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(2100, 0, 0));
    Q9K_SysSTimeImpl();
    check("ein mitgegebenes Jahr liefert das Jahrhundert",
          Q9K_GetU32(Q9K_CLOCK_DAY), Q9K_JulianFromDate(2103, 9, 18));

    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(0, 0, 0));
    Q9K_SysSTimeImpl();
    check("ohne mitgegebenes Jahr bleibt das der Uhr stehen",
          Q9K_GetU32(Q9K_CLOCK_DAY), Q9K_JulianFromDate(2026, 9, 18));

    /* --- Abweisungen --- */
    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(2026, 13, 1));
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, TIME(0, 0, 0));
    Q9K_SysSTimeImpl();
    check("Monat 13 wird abgewiesen",
          Q9K_GetU32(Q9K_STIME_SCRATCH_SUCCESS), 0);
    check("mit E_BPADDR", Q9K_GetU32(Q9K_STIME_SCRATCH_ERROR), 0xD2);
    check("und die Uhr bleibt ungestellt", Q9K_GetU32(Q9K_CLOCK_VALID), 0);

    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(2026, 9, 18));
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, TIME(25, 0, 0));
    Q9K_SysSTimeImpl();
    check("Stunde 25 wird abgewiesen",
          Q9K_GetU32(Q9K_STIME_SCRATCH_SUCCESS), 0);

    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(2026, 9, 18));
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, TIME(0, 60, 0));
    Q9K_SysSTimeImpl();
    check("Minute 60 wird abgewiesen",
          Q9K_GetU32(Q9K_STIME_SCRATCH_SUCCESS), 0);

    /* GRENZE, bewusst so und hier festgehalten: ein Tag, den es in dem
     * Monat nicht gibt, wird NICHT abgewiesen, sondern von der
     * Julian-Formel weitergerechnet -- der 31. September ergibt den
     * 1. Oktober. Q9K_JulianFromDate prueft nur Monat 1-12 und Tag 1-31,
     * und das Handbuch sagt zu F$STime ausdruecklich "The date and time
     * are not checked for validity". Eine strengere Pruefung waere hier
     * eine Erfindung ueber das Original hinaus; wer sie braucht, sieht an
     * dieser Stelle, dass sie fehlt. */
    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(2026, 9, 31));
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, TIME(0, 0, 0));
    Q9K_SysSTimeImpl();
    check("der 31. September wird angenommen",
          Q9K_GetU32(Q9K_STIME_SCRATCH_SUCCESS), 1);
    check("und auf den 1. Oktober weitergerechnet",
          Q9K_GetU32(Q9K_CLOCK_DAY), Q9K_JulianFromDate(2026, 10, 1));

    /* Mitternacht und die letzte Sekunde des Tages sind gueltig -- eine
     * zu strenge Pruefung waere hier genauso falsch wie eine fehlende. */
    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(2026, 9, 18));
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, TIME(0, 0, 0));
    Q9K_SysSTimeImpl();
    check("Mitternacht ist gueltig", Q9K_GetU32(Q9K_STIME_SCRATCH_SUCCESS), 1);
    check("und ergibt Sekunde 0", Q9K_GetU32(Q9K_CLOCK_SEC), 0);

    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(2026, 9, 18));
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, TIME(23, 59, 59));
    Q9K_SysSTimeImpl();
    check("23:59:59 ist gueltig", Q9K_GetU32(Q9K_STIME_SCRATCH_SUCCESS), 1);
    check("und ergibt die letzte Sekunde des Tages",
          Q9K_GetU32(Q9K_CLOCK_SEC), 86399);

    /* --- F$Time ueber die Bruecke: der Rueckweg --- */
    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(2026, 9, 18));
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, TIME(12, 34, 56));
    Q9K_SysSTimeImpl();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, 0);
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, 0);
    Q9K_SysTimeImpl();
    check("F$Time gibt das gestellte Datum feldkodiert zurueck",
          Q9K_GetU32(Q9K_STIME_SCRATCH_DATE), DATE(2026, 9, 18));
    check("und die gestellte Uhrzeit ebenso",
          Q9K_GetU32(Q9K_STIME_SCRATCH_TIME), TIME(12, 34, 56));
    check("und liest dafuer die Hardware nicht an", (Q9_u32)g_rtcReads, 0);

    /* Der eigentliche Punkt der ganzen Uebung: was F$STime setzt, muss
     * F$Time zurueckgeben. Vorher las F$Time die Hardware und haette den
     * gesetzten Wert verworfen. */
    reset();
    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE, DATE(1999, 12, 31));
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME, TIME(23, 59, 59));
    Q9K_SysSTimeImpl();
    tickTimes(100);                       /* eine Sekunde weiter */
    Q9K_SysTimeImpl();
    check("eine Sekunde nach Silvester ist Neujahr",
          Q9K_GetU32(Q9K_STIME_SCRATCH_DATE), DATE(2000, 1, 1));
    check("um Mitternacht",
          Q9K_GetU32(Q9K_STIME_SCRATCH_TIME), TIME(0, 0, 0));

    printf("\n%s\n", failures ? "TESTS FEHLGESCHLAGEN" : "ALLE TESTS BESTANDEN");
    return failures ? 1 : 0;
}
