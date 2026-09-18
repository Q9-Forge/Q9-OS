/*
 * q9kernel_clock.c -- Q9-OS eigener Kernel: die Systemuhr und F$STime
 *                     (Callcode $16, 2026-09-18).
 *
 * Verifizierte ABI (68k_tech.pdf S.511):
 *
 *   F$STime  d0.l = Uhrzeit (00hhmmss), d1.l = Datum (yyyymmdd)
 *            AUS: nichts; bei Fehler cc=Carry und d1.w = Fehlercode
 *
 * Beide Angaben sind FELDKODIERT -- je ein Byte pro Feld, Jahr im oberen
 * Wort. Dass "yyyymmdd" Felder und keine Dezimalzahl meint, ist am
 * Originalkernel bewiesen (s. Q9K_SysFTime in q9kernel_entry.a).
 *
 * WARUM ES DIESE DATEI GIBT -- eine Uhr statt eines Hardwarelesers:
 *
 * F$Time las bisher bei jedem Aufruf direkt den RTC72421 bei $FFFFD000.
 * Das liefert zwar eine richtige Zeit, macht F$STime aber unmoeglich: was
 * der Aufrufer setzt, waere beim naechsten F$Time wieder weg. Das
 * Handbuch beschreibt an dieser Stelle auch ausdruecklich das Gegenteil:
 *
 *   "The OS-9 kernel keeps track of the current date and time in software
 *    to make clock modules small and simple."
 *
 * Die Uhr gehoert also in den Kernel, und die Hardware liefert nur den
 * Startwert. Genau das macht diese Datei:
 *
 *   * Q9K_ClockTick()  zaehlt einmal je Timer-Interrupt hoch und rollt
 *                      Ticks -> Sekunden -> Tage weiter.
 *   * Q9K_ClockRead()  liefert den Softwarestand. Ist die Uhr noch nie
 *                      gestellt worden, holt sie sich beim ersten Lesen
 *                      den RTC-Stand und laeuft ab da selbst weiter --
 *                      das ist der Kaltstart mit batteriegepufferter Uhr,
 *                      den das Handbuch beschreibt.
 *   * Q9K_ClockSet()   stellt sie, und zwar so, dass der Sekundenbruchteil
 *                      bei 0 neu beginnt.
 *
 * Datum und Uhrzeit liegen intern als julianische Tageszahl plus Sekunden
 * seit Mitternacht vor -- dieselbe Form, in der die absoluten Alarme
 * rechnen, und die einzige, in der ein Tageswechsel nicht zum Sonderfall
 * wird. Die Feldkodierung ist reine Aussenform und wird an der Bruecke
 * umgerechnet.
 *
 * DAS JAHRESFELD BEIM KALTSTART: Das Handbuch sagt zu F$STime
 *
 *   "On systems with a battery-backed clock, it is usually only necessary
 *    to supply the year to the F$STime call. The actual date and time are
 *    read from the real-time clock. To read the time, the month field in
 *    the date parameter must be 0."
 *
 * Ein Monatsfeld von 0 heisst also: Datum und Uhrzeit kommen aus der
 * Hardware, und nur das Jahr stammt vom Aufrufer. Das ergibt Sinn fuer
 * Uhren, die das Jahrhundert nicht speichern -- der RTC72421 gehoert
 * dazu, er liefert zwei Stellen. Diese Fassung setzt es genau so um: bei
 * Monat 0 wird die Hardware gelesen und, falls der Aufrufer ein Jahr
 * mitgibt, dessen Jahrhundert eingesetzt.
 *
 * NICHT UMGESETZT ist der zweite Teil der Beschreibung -- "starts the
 * system real-time clock to produce time-slice interrupts ... and then
 * linking the clock module". Dieser Kernel hat kein ladbares clock-Modul;
 * sein Timer-Interrupt laeuft seit dem Boot (Q9K_TimerIRQHandler). Es
 * gibt hier also nichts zu starten, und F$STime tut das, was auf dieser
 * Maschine davon uebrig bleibt: es stellt die Uhr.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;

extern Q9_u32 Q9K_JulianFromDate(Q9_u32 year, Q9_u32 month, Q9_u32 day); /* q9kernel_date.c  */
extern void   Q9K_DateFromJulian(Q9_u32 julian, Q9_u32 *outYear,
                                 Q9_u32 *outMonth, Q9_u32 *outDay);      /* q9kernel_date.c  */
extern void   Q9K_RtcRead(Q9_u32 *outDay, Q9_u32 *outSeconds);           /* q9kernel_alarm.c */
extern void   Q9K_RtcReadFields(Q9_u32 *outYear, Q9_u32 *outMonth, Q9_u32 *outDay,
                                Q9_u32 *outHour, Q9_u32 *outMin, Q9_u32 *outSec);

#ifndef Q9K_E_BPADDR
#define Q9K_E_BPADDR 0xD2U
#endif

/* 100 Ticks je Sekunde -- dieselbe Konstante, mit der q9kernel_procsleep.c
 * rechnet. Beide beziehen sich auf denselben Timer-Interrupt. */
#ifndef Q9K_TICKS_PER_SEC
#define Q9K_TICKS_PER_SEC 100UL
#endif

#define Q9K_SECS_PER_DAY 86400UL

/* Uhrzellen, direkt hinter dem F$Alarm-Scratch ($1B00-$1B1B). Zur
 * Adressfalle an dieser Stelle s. den Kopf des Scratch-Blocks in
 * q9kernel_alarm.c -- hier ist bewusst Abstand gelassen. */
#ifndef Q9K_CLOCK_DAY
#define Q9K_CLOCK_DAY   0x1B20UL /* Q9_u32, julianische Tageszahl          */
#define Q9K_CLOCK_SEC   0x1B24UL /* Q9_u32, Sekunden seit Mitternacht      */
#define Q9K_CLOCK_TICK  0x1B28UL /* Q9_u32, Ticks innerhalb der Sekunde    */
#define Q9K_CLOCK_VALID 0x1B2CUL /* Q9_u32, 0 = nie gestellt               */
#endif

/* Scratch-Bruecke fuer F$STime. */
#ifndef Q9K_STIME_SCRATCH_TIME
#define Q9K_STIME_SCRATCH_TIME    0x1B30UL /* Q9_u32, d0.l EIN            */
#define Q9K_STIME_SCRATCH_DATE    0x1B34UL /* Q9_u32, d1.l EIN            */
#define Q9K_STIME_SCRATCH_ERROR   0x1B38UL /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_STIME_SCRATCH_SUCCESS 0x1B3CUL /* Q9_u32, 0/1                 */
#endif

/* Zellzugriffe wie in jeder anderen Datei dieses Verzeichnisses. Der
 * Schalter ist fuer Hosttests da, die MEHRERE Kerneldateien in dieselbe
 * Uebersetzungseinheit einbinden: dort bringt die erste sie schon mit,
 * und eine zweite Definition waere ein Uebersetzungsfehler. */
#ifndef Q9K_CELL_ACCESSORS_PROVIDED
static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
#endif

/* Q9K_ClockSet -- Uhr stellen. Der Sekundenbruchteil beginnt neu, sonst
 * kaeme die gesetzte Sekunde je nach Tickstand bis zu 10 ms zu frueh. */
void Q9K_ClockSet(Q9_u32 julianDay, Q9_u32 seconds)
{
    Q9K_SetU32(Q9K_CLOCK_DAY, julianDay);
    Q9K_SetU32(Q9K_CLOCK_SEC, seconds);
    Q9K_SetU32(Q9K_CLOCK_TICK, 0UL);
    Q9K_SetU32(Q9K_CLOCK_VALID, 1UL);
}

/* Q9K_ClockRead -- aktueller Stand. Beim allerersten Lesen uebernimmt die
 * Uhr den Hardwarestand und laeuft ab da in Software weiter. */
void Q9K_ClockRead(Q9_u32 *outDay, Q9_u32 *outSeconds)
{
    if (Q9K_GetU32(Q9K_CLOCK_VALID) == 0UL) {
        Q9_u32 day = 0UL, sec = 0UL;
        Q9K_RtcRead(&day, &sec);
        Q9K_ClockSet(day, sec);
    }
    *outDay = Q9K_GetU32(Q9K_CLOCK_DAY);
    *outSeconds = Q9K_GetU32(Q9K_CLOCK_SEC);
}

/* Q9K_ClockTick -- einmal je Timer-Interrupt, neben Q9K_AlarmTick.
 *
 * Laeuft nur, wenn die Uhr schon steht: vor dem ersten Lesen gibt es
 * keinen Stand, den man weiterzaehlen koennte, und ein Hochzaehlen ab 0
 * wuerde beim Kaltstart einen falschen Zeitpunkt vortaeuschen.
 *
 * Rueckgabe: 1, wenn dabei eine Sekunde voll wurde. */
Q9_u32 Q9K_ClockTick(void)
{
    Q9_u32 ticks;

    if (Q9K_GetU32(Q9K_CLOCK_VALID) == 0UL) {
        return 0UL;
    }

    ticks = Q9K_GetU32(Q9K_CLOCK_TICK) + 1UL;
    if (ticks < Q9K_TICKS_PER_SEC) {
        Q9K_SetU32(Q9K_CLOCK_TICK, ticks);
        return 0UL;
    }

    Q9K_SetU32(Q9K_CLOCK_TICK, 0UL);
    {
        Q9_u32 sec = Q9K_GetU32(Q9K_CLOCK_SEC) + 1UL;
        if (sec >= Q9K_SECS_PER_DAY) {
            sec = 0UL;
            Q9K_SetU32(Q9K_CLOCK_DAY, Q9K_GetU32(Q9K_CLOCK_DAY) + 1UL);
        }
        Q9K_SetU32(Q9K_CLOCK_SEC, sec);
    }
    return 1UL;
}

/* Q9K_SysTimeImpl -- Bruecke fuer F$Time (Callcode $15).
 *
 * Liefert den Stand der SYSTEMUHR, nicht den der Hardware: erst dadurch
 * hat F$STime ueberhaupt eine Wirkung. Ausgegeben wird in derselben
 * Feldkodierung, in der F$STime seine Eingaben erwartet -- beide Seiten
 * desselben Formats, was sie vorher nicht waren (s. Kopfkommentar). */
void Q9K_SysTimeImpl(void)
{
    Q9_u32 day = 0UL, sec = 0UL;
    Q9_u32 year = 0UL, month = 0UL, dayOfMonth = 0UL;

    Q9K_ClockRead(&day, &sec);
    Q9K_DateFromJulian(day, &year, &month, &dayOfMonth);

    Q9K_SetU32(Q9K_STIME_SCRATCH_DATE,
               (year << 16) | (month << 8) | dayOfMonth);
    Q9K_SetU32(Q9K_STIME_SCRATCH_TIME,
               ((sec / 3600UL) << 16) | (((sec / 60UL) % 60UL) << 8)
               | (sec % 60UL));
}

/* Q9K_SysSTimeImpl -- Bruecke fuer F$STime.
 *
 * Datum und Uhrzeit kommen feldkodiert an. Ein Monatsfeld von 0 bedeutet
 * laut Handbuch "nimm Datum und Uhrzeit aus der Hardware"; ein in dem
 * Fall mitgegebenes Jahr liefert das Jahrhundert, das der RTC72421 nicht
 * speichert. */
void Q9K_SysSTimeImpl(void)
{
    Q9_u32 date = Q9K_GetU32(Q9K_STIME_SCRATCH_DATE);
    Q9_u32 time = Q9K_GetU32(Q9K_STIME_SCRATCH_TIME);
    Q9_u32 year  = (date >> 16) & 0xFFFFUL;
    Q9_u32 month = (date >> 8) & 0xFFUL;
    Q9_u32 day   = date & 0xFFUL;
    Q9_u32 hour  = (time >> 16) & 0xFFUL;
    Q9_u32 min   = (time >> 8) & 0xFFUL;
    Q9_u32 sec   = time & 0xFFUL;
    Q9_u32 jd;

    Q9K_SetU32(Q9K_STIME_SCRATCH_SUCCESS, 0UL);

    if (month == 0UL) {
        /* Batteriegepufferter Kaltstart: alles aus der Hardware, nur das
         * Jahrhundert darf der Aufrufer beisteuern. */
        Q9_u32 rYear = 0UL, rMonth = 0UL, rDay = 0UL;
        Q9_u32 rHour = 0UL, rMin = 0UL, rSec = 0UL;
        Q9K_RtcReadFields(&rYear, &rMonth, &rDay, &rHour, &rMin, &rSec);
        if (year != 0UL) {
            rYear = (year / 100UL) * 100UL + (rYear % 100UL);
        }
        year = rYear; month = rMonth; day = rDay;
        hour = rHour; min = rMin; sec = rSec;
    }

    /* Das Handbuch sagt ausdruecklich "The date and time are not checked
     * for validity". Dieser Kernel prueft trotzdem -- aber nur so weit,
     * wie ein Wert sonst gar keinen Zeitpunkt ergibt: Monat 13 oder
     * Stunde 25 lassen sich in keine Tageszahl umrechnen, und eine so
     * gestellte Uhr waere fuer jeden absoluten Alarm unbrauchbar.
     *
     * NICHT geprueft wird die Monatslaenge: der 31. September wird von
     * der Julian-Formel zum 1. Oktober weitergerechnet (s. dort). Das ist
     * das Verhalten der Formel, nicht ein Versehen -- eine strengere
     * Pruefung waere eine Erfindung ueber das Original hinaus.
     * Festgehalten in test_q9kernel_clock.c. */
    if (hour > 23UL || min > 59UL || sec > 59UL) {
        Q9K_SetU32(Q9K_STIME_SCRATCH_ERROR, Q9K_E_BPADDR);
        return;
    }
    jd = Q9K_JulianFromDate(year, month, day);
    if (jd == 0UL) {
        Q9K_SetU32(Q9K_STIME_SCRATCH_ERROR, Q9K_E_BPADDR);
        return;
    }

    Q9K_ClockSet(jd, hour * 3600UL + min * 60UL + sec);
    Q9K_SetU32(Q9K_STIME_SCRATCH_SUCCESS, 1UL);
}
