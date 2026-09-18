/*
 * test_q9kernel_alarm.c -- Regressionstest fuer q9kernel_alarm.c (F$Alarm).
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests: Tabellen- und
 * Scratch-Adressen werden per #define VOR dem #include auf Testpuffer
 * umgebogen. Q9K_ProcSend/Q9K_ProcIdForDesc sind hier steuerbare Stubs --
 * beide sind in ihren eigenen Tests bereits abgedeckt; hier geht es um
 * die Alarmbuchhaltung und darum, DASS zum richtigen Tick zugestellt
 * wird.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_alarm test_q9kernel_alarm.c && ./test_q9kernel_alarm
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x200];
static unsigned char g_alarmTable[0x200];

#define Q9_D_PROC                 ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9K_ALARM_BASE            ((unsigned long)(g_alarmTable + 0x000))
/* Real 16 Byte je Eintrag mit 4-Byte-Feldern -- auf diesem Host ist
 * Q9_u32 8 Byte breit, deshalb grosszuegig auf 32-Byte-Eintraege
 * gelegt. Betrifft NUR diesen Test. */
#define Q9K_ALARM_STRIDE          48UL
#define Q9K_ALARM_OFF_PID          8UL
#define Q9K_ALARM_OFF_SIGNAL      16UL
#define Q9K_ALARM_OFF_TICKS       24UL
#define Q9K_ALARM_OFF_DAY         32UL
#define Q9K_ALARM_OFF_SEC         40UL
#define Q9K_ALARM_NEXTID          ((unsigned long)(g_alarmTable + 0x100))
#define Q9K_ALARM_SCRATCH_FUNC    ((unsigned long)(g_fakeGlobals + 0x020))
#define Q9K_ALARM_SCRATCH_IDIN    ((unsigned long)(g_fakeGlobals + 0x040))
#define Q9K_ALARM_SCRATCH_SIGNAL  ((unsigned long)(g_fakeGlobals + 0x060))
#define Q9K_ALARM_SCRATCH_TICKS   ((unsigned long)(g_fakeGlobals + 0x080))
#define Q9K_ALARM_SCRATCH_ERROR   ((unsigned long)(g_fakeGlobals + 0x0A0))
#define Q9K_ALARM_SCRATCH_SUCCESS ((unsigned long)(g_fakeGlobals + 0x0C0))
#define Q9K_ALARM_SCRATCH_DATE    ((unsigned long)(g_fakeGlobals + 0x0E0))

/* Steuerbare Stubs. Beide Funktionen sind in ihren eigenen Testsuiten
 * abgedeckt (test_q9kernel_procsleep.c bzw. test_q9kernel_procapi.c). */
static int g_sendCalls;
static unsigned short g_sendLastPid;
static unsigned short g_sendLastSignal;
int Q9K_ProcSend(unsigned short pid, unsigned short signal, unsigned short *outError)
{
    g_sendCalls++;
    g_sendLastPid = pid;
    g_sendLastSignal = signal;
    *outError = 0;
    return 1;
}

static unsigned short g_idForDesc = 2;
unsigned short Q9K_ProcIdForDesc(unsigned long desc)
{
    return desc ? g_idForDesc : 0;
}

/* Q9K_JulianFromDate lebt in q9kernel_date.c (dort gegen JULBASE und die
 * Wochentagsformel geprueft). Hier eine einfache, monoton steigende
 * Ersatzrechnung -- dieser Test prueft die Alarmlogik, nicht die
 * Kalenderarithmetik. */
unsigned long Q9K_JulianFromDate(unsigned long y, unsigned long m, unsigned long d)
{
    if (m < 1 || m > 12 || d < 1 || d > 31) return 0;
    return y * 400UL + m * 31UL + d;
}

/* Steuerbare Uhr: der Test stellt die "Gegenwart", statt sich auf die
 * echte RTC zu verlassen (die es auf dem Host nicht gibt). */
static unsigned long g_nowDay = 1000, g_nowSec = 0;
#define Q9K_TEST_RTC_OVERRIDE 1

#include "q9kernel_alarm.c"

static int failures;

static void check(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want)
        printf("[OK]   %-64s = %lu\n", label, (unsigned long)got);
    else {
        printf("[FAIL] %-64s = %lu (erwartet %lu)\n", label,
               (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

static void reset(void)
{
    memset(g_fakeGlobals, 0, sizeof(g_fakeGlobals));
    memset(g_alarmTable, 0, sizeof(g_alarmTable));
    Q9K_SetU32(Q9_D_PROC, 0x4000);   /* irgendein gueltiger Deskriptorzeiger */
    g_sendCalls = 0;
    g_idForDesc = 2;
}

int main(void)
{
    Q9_u32 id = 0, id2 = 0;
    Q9_u16 err;

    /* A$Set: nach genau n Ticks ein Signal, keinen Tick frueher. */
    reset();
    err = 0xFFFF;
    check("A$Set nimmt einen Alarm an", (Q9_u32)Q9K_AlarmSet(42, 3, 0, &id, &err), 1);
    check("A$Set liefert eine Alarm-ID ungleich 0", (Q9_u32)(id != 0), 1);
    check("A$Set meldet keinen Fehler", (Q9_u32)err, 0);

    check("nach Tick 1 noch kein Signal", Q9K_AlarmTick(), 0);
    check("nach Tick 2 noch kein Signal", Q9K_AlarmTick(), 0);
    check("nach Tick 3 loest der Alarm aus", Q9K_AlarmTick(), 1);
    check("das Signal ging an den richtigen Prozess", (Q9_u32)g_sendLastPid, 2);
    check("mit dem richtigen Signalcode", (Q9_u32)g_sendLastSignal, 42);
    check("ein einmaliger Alarm loest nur einmal aus", Q9K_AlarmTick(), 0);
    check("und gibt seinen Tabellenplatz frei",
          Q9K_GetU32(Q9K_ALARM_ID(0)), 0);

    /* A$Cycle: wiederholt sich im selben Abstand. */
    reset();
    Q9K_AlarmSet(7, 2, 2, &id, &err);
    check("A$Cycle: Tick 1 still", Q9K_AlarmTick(), 0);
    check("A$Cycle: Tick 2 loest aus", Q9K_AlarmTick(), 1);
    check("A$Cycle: Tick 3 still", Q9K_AlarmTick(), 0);
    check("A$Cycle: Tick 4 loest wieder aus", Q9K_AlarmTick(), 1);
    check("A$Cycle belegt seinen Platz weiter",
          (Q9_u32)(Q9K_GetU32(Q9K_ALARM_ID(0)) != 0), 1);
    check("insgesamt zweimal zugestellt", (Q9_u32)g_sendCalls, 2);

    /* A$Delete mit ID: genau dieser Alarm verschwindet. */
    reset();
    Q9K_AlarmSet(1, 5, 0, &id, &err);
    Q9K_AlarmSet(2, 5, 0, &id2, &err);
    check("zwei Alarme haben verschiedene IDs", (Q9_u32)(id != id2), 1);
    check("A$Delete nimmt eine ID an", (Q9_u32)Q9K_AlarmDelete(id, &err), 1);
    check("der geloeschte Platz ist frei", Q9K_GetU32(Q9K_ALARM_ID(0)), 0);
    check("der andere Alarm bleibt", Q9K_GetU32(Q9K_ALARM_ID(1)), id2);

    /* A$Delete mit 0: alle eigenen Alarme. */
    reset();
    Q9K_AlarmSet(1, 5, 0, &id, &err);
    Q9K_AlarmSet(2, 5, 0, &id2, &err);
    Q9K_AlarmDelete(0, &err);
    check("A$Delete(0) raeumt alle eigenen Alarme", Q9K_GetU32(Q9K_ALARM_ID(0)), 0);
    check("auch den zweiten", Q9K_GetU32(Q9K_ALARM_ID(1)), 0);

    /* Fremde Alarme bleiben unangetastet -- ein Prozess darf nur seine
     * eigenen loeschen. */
    reset();
    g_idForDesc = 2;
    Q9K_AlarmSet(1, 5, 0, &id, &err);
    g_idForDesc = 3;                         /* jetzt ist ein anderer Prozess dran */
    Q9K_AlarmSet(2, 5, 0, &id2, &err);
    Q9K_AlarmDelete(0, &err);                /* loescht nur die von Prozess 3 */
    check("der fremde Alarm bleibt erhalten", Q9K_GetU32(Q9K_ALARM_ID(0)), id);
    check("der eigene ist weg", Q9K_GetU32(Q9K_ALARM_ID(1)), 0);

    /* Fehlerfaelle. */
    reset();
    err = 0;
    check("Intervall 0 wird abgewiesen", (Q9_u32)Q9K_AlarmSet(1, 0, 0, &id, &err), 0);
    check("und meldet E_BPADDR", (Q9_u32)err, Q9K_E_BPADDR);

    Q9K_SetU32(Q9_D_PROC, 0);
    err = 0;
    check("ohne aktuellen Prozess kein Alarm",
          (Q9_u32)Q9K_AlarmSet(1, 5, 0, &id, &err), 0);
    check("und meldet E_PRCID", (Q9_u32)err, Q9K_E_PRCID);

    /* Tabelle voll. */
    reset();
    {
        Q9_u32 k;
        int okAll = 1;
        for (k = 0; k < Q9K_ALARM_SLOTS; ++k)
            if (!Q9K_AlarmSet(1, 10, 0, &id, &err)) okAll = 0;
        check("alle Tabellenplaetze lassen sich belegen", (Q9_u32)okAll, 1);
        err = 0;
        check("der naechste Alarm wird sauber abgewiesen",
              (Q9_u32)Q9K_AlarmSet(1, 10, 0, &id, &err), 0);
        check("und meldet E_BPADDR", (Q9_u32)err, Q9K_E_BPADDR);
    }

    /* Bruecke: Funktionscodes und volle Zellbreite. */
    reset();
    Q9K_SetU32(Q9K_ALARM_SCRATCH_FUNC, Q9K_A_SET);
    Q9K_SetU32(Q9K_ALARM_SCRATCH_SIGNAL, 9);
    Q9K_SetU32(Q9K_ALARM_SCRATCH_TICKS, 4);
    Q9K_SysAlarmImpl();
    check("Bridge A$Set meldet Erfolg in voller Zellbreite",
          Q9K_GetU32(Q9K_ALARM_SCRATCH_SUCCESS), 1);
    check("Bridge A$Set legt die ID in der Zelle ab",
          (Q9_u32)(Q9K_GetU32(Q9K_ALARM_SCRATCH_IDIN) != 0), 1);

    Q9K_SetU32(Q9K_ALARM_SCRATCH_FUNC, Q9K_A_DELETE);
    Q9K_SetU32(Q9K_ALARM_SCRATCH_IDIN, 0);
    Q9K_SysAlarmImpl();
    check("Bridge A$Delete meldet Erfolg",
          Q9K_GetU32(Q9K_ALARM_SCRATCH_SUCCESS), 1);

    /* Absolute Alarme (A$AtJul / A$AtDate): faellig, sobald die Uhr das
     * Ziel erreicht -- nicht frueher, aber auch dann noch, wenn der
     * Zeitpunkt verschlafen wurde. */
    reset();
    g_nowDay = 1000; g_nowSec = 100;
    err = 0xFFFF;
    check("A$AtJul nimmt einen absoluten Alarm an",
          (Q9_u32)Q9K_AlarmSetAbsolute(55, 1000, 200, &id, &err), 1);
    check("vor dem Zeitpunkt passiert nichts", Q9K_AlarmTick(), 0);
    g_nowSec = 199;
    check("eine Sekunde davor immer noch nichts", Q9K_AlarmTick(), 0);
    g_nowSec = 200;
    check("genau zum Zeitpunkt loest er aus", Q9K_AlarmTick(), 1);
    check("mit dem richtigen Signal", (Q9_u32)g_sendLastSignal, 55);
    check("und ist danach abgeraeumt", Q9K_GetU32(Q9K_ALARM_ID(0)), 0);

    /* Ein verschlafener Zeitpunkt verfaellt nicht -- die Beschreibung
     * sagt "greater than or equal". */
    reset();
    g_nowDay = 1000; g_nowSec = 0;
    Q9K_AlarmSetAbsolute(56, 1000, 500, &id, &err);
    g_nowSec = 4000;                      /* weit darueber hinaus */
    check("ein verpasster Zeitpunkt loest trotzdem aus", Q9K_AlarmTick(), 1);

    /* Ein spaeterer Tag zaehlt, nicht nur die Uhrzeit. */
    reset();
    g_nowDay = 1000; g_nowSec = 50000;
    Q9K_AlarmSetAbsolute(57, 1001, 10, &id, &err);
    check("am Vortag bleibt er still, auch spaet am Tag", Q9K_AlarmTick(), 0);
    g_nowDay = 1001; g_nowSec = 10;
    check("am Zieltag loest er aus", Q9K_AlarmTick(), 1);

    reset();
    err = 0;
    check("Tageszahl 0 wird abgewiesen",
          (Q9_u32)Q9K_AlarmSetAbsolute(1, 0, 0, &id, &err), 0);
    err = 0;
    check("eine Sekundenzahl ab dem Tagesende wird abgewiesen",
          (Q9_u32)Q9K_AlarmSetAbsolute(1, 1000, 86400, &id, &err), 0);

    /* Bridge: A$AtDate rechnet das Kalenderdatum selbst um. */
    reset();
    g_nowDay = 0; g_nowSec = 0;
    Q9K_SetU32(Q9K_ALARM_SCRATCH_FUNC, 3);                 /* A$AtDate */
    Q9K_SetU32(Q9K_ALARM_SCRATCH_SIGNAL, 12);
    Q9K_SetU32(Q9K_ALARM_SCRATCH_TICKS, (1UL << 16) | (2UL << 8) | 3UL);  /* 01:02:03 */
    Q9K_SetU32(Q9K_ALARM_SCRATCH_DATE, (2026UL << 16) | (9UL << 8) | 18UL);
    Q9K_SysAlarmImpl();
    check("Bridge A$AtDate nimmt ein Kalenderdatum an",
          Q9K_GetU32(Q9K_ALARM_SCRATCH_SUCCESS), 1);
    check("und legt die umgerechnete Tageszahl ab",
          Q9K_GetU32(Q9K_ALARM_DAY(0)), Q9K_JulianFromDate(2026, 9, 18));
    check("und die aus hh/mm/ss gerechnete Sekundenzahl",
          Q9K_GetU32(Q9K_ALARM_SEC(0)), 1UL * 3600UL + 2UL * 60UL + 3UL);

    Q9K_SetU32(Q9K_ALARM_SCRATCH_TICKS, (25UL << 16));     /* Stunde 25 */
    Q9K_SysAlarmImpl();
    check("Bridge A$AtDate weist eine unmoegliche Uhrzeit ab",
          Q9K_GetU32(Q9K_ALARM_SCRATCH_SUCCESS), 0);
    Q9K_SetU32(Q9K_ALARM_SCRATCH_TICKS, (1UL << 16) | (2UL << 8) | 3UL);

    Q9K_SetU32(Q9K_ALARM_SCRATCH_DATE, (2026UL << 16) | (13UL << 8) | 18UL);  /* Monat 13 */
    Q9K_SysAlarmImpl();
    check("Bridge A$AtDate weist ein unmoegliches Datum ab",
          Q9K_GetU32(Q9K_ALARM_SCRATCH_SUCCESS), 0);

    /* A$Reset bleibt unbekannt -- die Beschreibung sagt nicht, was es
     * zuruecksetzen soll. */
    Q9K_SetU32(Q9K_ALARM_SCRATCH_FUNC, 5);
    Q9K_SysAlarmImpl();
    check("Bridge weist A$Reset ab", Q9K_GetU32(Q9K_ALARM_SCRATCH_SUCCESS), 0);
    check("und meldet E_UNKSVC", Q9K_GetU32(Q9K_ALARM_SCRATCH_ERROR), Q9K_E_UNKSVC);

    Q9K_SetU32(Q9K_ALARM_SCRATCH_FUNC, 99);  /* unbekannt */
    Q9K_SysAlarmImpl();
    check("Bridge weist einen unbekannten Funktionscode ab",
          Q9K_GetU32(Q9K_ALARM_SCRATCH_SUCCESS), 0);

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
