/*
 * q9kernel_alarm.c -- Q9-OS eigener Kernel: F$Alarm (Callcode $56,
 *                     2026-09-18).
 *
 * Verifizierte ABI (68k_tech.pdf, Appendix D). F$Alarm ist ein Aufruf mit
 * Funktionscode in d1.w; die Codes sind aus der realen Tabelle in
 * MWOS/OS9/SRC/DEFS/funcs.a gezaehlt, nicht geraten:
 *
 *   A$Delete = 0  d0.l = Alarm-ID (0 = alle), keine Ausgabe
 *   A$Set    = 1  d0.l = 0, d2.w = Signalcode, d3.l = Intervall
 *                 -> d0.l = Alarm-ID
 *   A$Cycle  = 2  wie A$Set, aber der Alarm wiederholt sich
 *   A$AtDate = 3  absolute Zeit (gregorianisch)   -- s. u.
 *   A$AtJul  = 4  absolute Zeit (julianisch)      -- s. u.
 *   A$Reset  = 5  d0.l = Alarm-ID, d2.w = neues Signal, d3.l = Intervall
 *
 * UMGESETZT SIND A$Delete, A$Set, A$Cycle, A$AtDate UND A$AtJul. Nur
 * A$Reset setzt einen bestehenden eigenen Alarm auf ein neues relatives
 * Intervall zurueck. Diese Semantik ist durch die Microware-DPIO-API
 * (_os_alarm_reset(alarm_id, signal, interval)) belegt.
 *
 * VORGESCHICHTE (2026-09-18, am selben Tag): die absoluten Varianten
 * fehlten zunaechst, erst mit der Begruendung "keine Systemuhr" (falsch --
 * Q9K_SysFTime liest einen echten RTC72421 bei $FFFFD000), dann mit der
 * richtigen: F$Time und F$Julian widersprachen sich im Datumsformat, und
 * ein absoluter Alarm haette deshalb still zum falschen Zeitpunkt
 * gefeuert. Der Widerspruch ist inzwischen am Originalkernel entschieden
 * (s. Q9K_SysFTime in q9kernel_entry.a: "yyyymmdd" meint FELDER) und
 * F$Time entsprechend korrigiert. Damit stehen beide Seiten auf
 * demselben Format, und die absoluten Varianten sind das, was sie sein
 * sollten: ein Vergleich zweier Zahlen.
 *
 * ZEITEINHEIT: Die Beschreibung sagt, das Intervall koenne "in system
 * clock ticks, or 256ths of a second" angegeben werden, nennt aber an
 * dieser Stelle nicht, woran der Kernel beides unterscheidet. Diese
 * Fassung rechnet deshalb ausschliesslich in Ticks -- das ist die
 * Einheit, die der Tickzaehler dieses Kernels ohnehin fuehrt (10 ms, s.
 * Q9K_TimerIRQHandler). Ein Aufrufer, der Sekundenbruchteile meint,
 * bekommt also eine andere Wartezeit als erwartet; das ist dokumentiert
 * und nicht stillschweigend geraten.
 *
 * WARUM EIGENE TABELLE: Der Q9-Header kennt die beiden Warteschlangen des
 * Originals (Q9_D_ALMQ1/ALMQ2), und q9kernel_cinit.c legt sie beim Boot
 * als leere Ringlisten an. Deren INNERES Knotenformat ist aber nur aus
 * der Disassemblierung des Originalkernels bekannt und fuer uns ohne
 * Nutzen -- kein fremdes Modul liest unsere Alarmknoten. Diese Fassung
 * fuehrt deshalb eine eigene, feste Tabelle, genauso wie der Kernel es
 * bei Prozess-, Pfad- und Speicherverwaltung schon haelt. Die beiden
 * Ringlisten bleiben unberuehrt.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

extern int Q9K_ProcSend(Q9_u16 pid, Q9_u16 signal, Q9_u16 *outError); /* q9kernel_procsleep.c */
extern Q9_u16 Q9K_ProcIdForDesc(Q9_u32 desc);                         /* q9kernel_procapi.c  */
extern Q9_u32 Q9K_JulianFromDate(Q9_u32 year, Q9_u32 month, Q9_u32 day); /* q9kernel_date.c */
extern void   Q9K_RtcRead(Q9_u32 *outDay, Q9_u32 *outSeconds);        /* unten in dieser Datei */
extern void   Q9K_ClockRead(Q9_u32 *outDay, Q9_u32 *outSeconds);      /* q9kernel_clock.c */

#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif

/* Alarmtabelle (2026-09-18): fester Pool, gleiche bewusst schlanke
 * Bauart wie die F$SRqMem-Eigentuemertabelle in q9kernel_sysmem.c.
 * Acht gleichzeitige Alarme sind fuer diesen Kernelstand reichlich; eine
 * verkettete Liste kann das spaeter ersetzen, ohne dass ein Aufrufer es
 * merkt. Liegt hinter dem F$DatMod-Scratch ($19B8-$19D8) und der
 * Scheduler-Mindestprioritaet ($19E0). */
#ifndef Q9K_ALARM_BASE
#define Q9K_ALARM_BASE   0x1A00UL
#endif
#define Q9K_ALARM_SLOTS  8UL
/* Eintragsgroesse und Feldabstaende: 28 Byte mit 4-Byte-Feldern. Das eigene
 * Zyklusfeld ist bewusst ein volles Langwort; die erste Fassung packte es
 * in das obere Signalwort und kuerzte dadurch gueltige 32-Bit-Intervalle.
 * Per #ifndef ueberschreibbar, gleicher Grund wie bei jeder anderen
 * Tabelle dieses Kernels -- auf einem 64-Bit-Testhost ist Q9_u32 8 Byte
 * breit, ein Schreibzugriff auf ID reichte dort bis in PID hinein. */
#ifndef Q9K_ALARM_STRIDE
#define Q9K_ALARM_STRIDE     28UL
#define Q9K_ALARM_OFF_PID     4UL
#define Q9K_ALARM_OFF_SIGNAL  8UL
#define Q9K_ALARM_OFF_TICKS  12UL
#define Q9K_ALARM_OFF_DAY    16UL   /* absoluter Alarm: julianische Tageszahl, 0 = relativ */
#define Q9K_ALARM_OFF_SEC    20UL   /* absoluter Alarm: Sekunden nach Mitternacht          */
#define Q9K_ALARM_OFF_CYCLE  24UL   /* zyklischer Alarm: volles 32-Bit-Intervall           */
#endif
#define Q9K_ALARM_ID(i)       (Q9K_ALARM_BASE + (i) * Q9K_ALARM_STRIDE)       /* Q9_u32, 0 = frei */
#define Q9K_ALARM_PID(i)      (Q9K_ALARM_ID(i) + Q9K_ALARM_OFF_PID)            /* Q9_u32, Empfaenger */
#define Q9K_ALARM_SIGNAL(i)   (Q9K_ALARM_ID(i) + Q9K_ALARM_OFF_SIGNAL)         /* Q9_u32, Signalcode */
#define Q9K_ALARM_TICKS(i)    (Q9K_ALARM_ID(i) + Q9K_ALARM_OFF_TICKS)          /* Q9_u32, Restticks  */
#define Q9K_ALARM_DAY(i)      (Q9K_ALARM_ID(i) + Q9K_ALARM_OFF_DAY)            /* Q9_u32, Zieltag    */
#define Q9K_ALARM_SEC(i)      (Q9K_ALARM_ID(i) + Q9K_ALARM_OFF_SEC)            /* Q9_u32, Zielsekunde */
#define Q9K_ALARM_CYCLE(i)    (Q9K_ALARM_ID(i) + Q9K_ALARM_OFF_CYCLE)          /* Q9_u32, Zyklus     */

/* Fortlaufender Zaehler fuer Alarm-IDs, direkt hinter der Tabelle. */
#ifndef Q9K_ALARM_NEXTID
#define Q9K_ALARM_NEXTID (Q9K_ALARM_BASE + Q9K_ALARM_SLOTS * Q9K_ALARM_STRIDE)
#endif

#define Q9K_A_DELETE 0U
#define Q9K_A_SET    1U
#define Q9K_A_CYCLE  2U
#define Q9K_A_ATDATE 3U
#define Q9K_A_ATJUL  4U
#define Q9K_A_RESET  5U

#define Q9K_E_UNKSVC 0x00D0U /* errno.h: Unknown Service Request */
#define Q9K_E_BPADDR 0x00D2U /* errno.h: Bad Parameter/Address    */
#define Q9K_E_PRCID  0x00E0U /* errno.h: Invalid Process ID       */

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }

/* Q9K_AlarmSet -- A$Set und A$Cycle. cycleTicks = 0 bedeutet einmalig,
 * sonst das Wiederholungsintervall. Rueckgabe 1 = Erfolg, *outId traegt
 * dann die Alarm-ID. */
int Q9K_AlarmSet(Q9_u16 signal, Q9_u32 ticks, Q9_u32 cycleTicks,
                 Q9_u32 *outId, Q9_u16 *outError)
{
    Q9_u32 desc = Q9K_GetU32(Q9_D_PROC);
    Q9_u16 pid;
    Q9_u32 i;
    Q9_u32 id;

    *outId = 0;
    *outError = 0;

    if (ticks == 0UL) {
        /* Ein Alarm ohne Wartezeit haette keine Bedeutung -- er muesste
         * sofort und damit noch im laufenden Aufruf ausloesen. */
        *outError = Q9K_E_BPADDR;
        return 0;
    }

    if (desc == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }
    pid = Q9K_ProcIdForDesc(desc);
    if (pid == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }

    for (i = 0; i < Q9K_ALARM_SLOTS; ++i) {
        if (Q9K_GetU32(Q9K_ALARM_ID(i)) == 0UL)
            break;
    }
    if (i >= Q9K_ALARM_SLOTS) {
        *outError = Q9K_E_BPADDR;   /* Tabelle voll */
        return 0;
    }

    id = Q9K_GetU32(Q9K_ALARM_NEXTID) + 1UL;
    if (id == 0UL)
        id = 1UL;                    /* 0 ist als "alle" reserviert */
    Q9K_SetU32(Q9K_ALARM_NEXTID, id);

    Q9K_SetU32(Q9K_ALARM_ID(i), id);
    Q9K_SetU32(Q9K_ALARM_PID(i), (Q9_u32)pid);
    Q9K_SetU32(Q9K_ALARM_SIGNAL(i), (Q9_u32)signal);
    Q9K_SetU32(Q9K_ALARM_TICKS(i), ticks);
    Q9K_SetU32(Q9K_ALARM_CYCLE(i), cycleTicks);

    *outId = id;
    return 1;
}

/* Q9K_AlarmReset -- A$Reset. Der Alarm bleibt derselbe (und damit bleibt
 * seine ID stabil), aber Signal und relative Restzeit werden ersetzt. Ein
 * zuvor absoluter Alarm wird dadurch bewusst relativ; ein vorhandenes
 * Zyklusintervall bleibt erhalten, damit Reset auch fuer _os_alarm_cycle()
 * den naheliegenden "neu starten"-Effekt hat. */
int Q9K_AlarmReset(Q9_u32 id, Q9_u16 signal, Q9_u32 ticks,
                   Q9_u16 *outError)
{
    Q9_u32 desc = Q9K_GetU32(Q9_D_PROC);
    Q9_u16 pid;
    Q9_u32 i;

    *outError = 0;
    if (ticks == 0UL) {
        *outError = Q9K_E_BPADDR;
        return 0;
    }
    if (desc == 0UL) {
        *outError = Q9K_E_PRCID;
        return 0;
    }
    pid = Q9K_ProcIdForDesc(desc);
    if (pid == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }
    for (i = 0; i < Q9K_ALARM_SLOTS; ++i) {
        Q9_u32 slotId = Q9K_GetU32(Q9K_ALARM_ID(i));
        if (slotId != id || Q9K_GetU32(Q9K_ALARM_PID(i)) != (Q9_u32)pid)
            continue;
        Q9K_SetU32(Q9K_ALARM_SIGNAL(i), (Q9_u32)signal);
        Q9K_SetU32(Q9K_ALARM_TICKS(i), ticks);
        Q9K_SetU32(Q9K_ALARM_DAY(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_SEC(i), 0UL);
        return 1;
    }
    *outError = Q9K_E_BPADDR;
    return 0;
}


/* RTC72421 des Q9-Boards, BCD, bei $FFFFD000 -- dieselbe Quelle, aus der
 * auch Q9K_SysFTime liest (q9kernel_entry.a). Registerfolge dort
 * abgelesen: +0 Sekunden, +2 Minuten, +4 Stunden, +6 Tag, +8 Monat,
 * +10 Jahr (zweistellig, +2000). Jedes Register haelt eine BCD-Ziffer je
 * Halbbyte.
 *
 * Liefert die Gegenwart als dasselbe Zahlenpaar, in dem ein absoluter
 * Alarm sein Ziel speichert: julianische Tageszahl und Sekunden nach
 * Mitternacht. Damit ist der Faelligkeitstest unten ein reiner
 * Zahlenvergleich. */
#ifndef Q9K_RTC_BASE
#define Q9K_RTC_BASE 0xFFFFD000UL
#endif

#ifndef Q9K_TEST_RTC_OVERRIDE
static Q9_u32 Q9K_RtcBcd(Q9_u32 offset)
{
    Q9_u32 raw = (Q9_u32)(*(volatile Q9_u8 *)(Q9K_RTC_BASE + offset));
    return ((raw >> 4) & 0x0FUL) * 10UL + (raw & 0x0FUL);
}
#endif

/* Q9K_RtcReadFields -- der Hardwarestand in seinen Einzelfeldern.
 *
 * F$STime braucht die Felder einzeln (s. q9kernel_clock.c: bei Monat 0
 * stammt das Jahrhundert vom Aufrufer, der Rest aus der Uhr), waehrend
 * jeder andere Aufrufer mit Tageszahl und Sekunden besser bedient ist.
 * Deshalb liest diese Routine die Hardware, und Q9K_RtcRead rechnet
 * darueber nur noch um -- statt beides getrennt aus denselben Registern
 * zu lesen und auseinanderlaufen zu lassen. */
void Q9K_RtcReadFields(Q9_u32 *outYear, Q9_u32 *outMonth, Q9_u32 *outDay,
                       Q9_u32 *outHour, Q9_u32 *outMin, Q9_u32 *outSec)
{
#ifdef Q9K_TEST_RTC_OVERRIDE
    /* Im Hosttest gibt es keine RTC -- dort stellt der Test die Uhr. */
    extern unsigned long g_rtcYear, g_rtcMonth, g_rtcDay;
    extern unsigned long g_rtcHour, g_rtcMin, g_rtcSec;
    *outYear = g_rtcYear; *outMonth = g_rtcMonth; *outDay = g_rtcDay;
    *outHour = g_rtcHour; *outMin = g_rtcMin; *outSec = g_rtcSec;
#else
    *outSec   = Q9K_RtcBcd(0);
    *outMin   = Q9K_RtcBcd(2);
    *outHour  = Q9K_RtcBcd(4);
    *outDay   = Q9K_RtcBcd(6);
    *outMonth = Q9K_RtcBcd(8);
    *outYear  = Q9K_RtcBcd(10) + 2000UL;
#endif
}

void Q9K_RtcRead(Q9_u32 *outDay, Q9_u32 *outSeconds)
{
#ifdef Q9K_TEST_RTC_OVERRIDE
    /* Im Hosttest gibt es keine RTC -- dort stellt der Test die Uhr. */
    extern unsigned long g_nowDay, g_nowSec;
    *outDay = g_nowDay;
    *outSeconds = g_nowSec;
    return;
#else
    Q9_u32 year, month, day, hour, min, sec;
    Q9K_RtcReadFields(&year, &month, &day, &hour, &min, &sec);
    *outSeconds = hour * 3600UL + min * 60UL + sec;
    *outDay = Q9K_JulianFromDate(year, month, day);
#endif
}

/* Q9K_AlarmSetAbsolute -- A$AtDate und A$AtJul. Beide unterscheiden sich
 * nur in der Form des uebergebenen Datums: A$AtJul bekommt die
 * julianische Tageszahl direkt, A$AtDate ein Kalenderdatum in derselben
 * Feldkodierung wie ueberall (Jahr im oberen Wort, dann Monat und Tag).
 * Intern ist danach beides dasselbe.
 *
 * Rueckgabe 1 = Erfolg. */
int Q9K_AlarmSetAbsolute(Q9_u16 signal, Q9_u32 julianDay, Q9_u32 seconds,
                         Q9_u32 *outId, Q9_u16 *outError)
{
    Q9_u32 desc = Q9K_GetU32(Q9_D_PROC);
    Q9_u16 pid;
    Q9_u32 i;
    Q9_u32 id;

    *outId = 0;
    *outError = 0;

    if (julianDay == 0UL || seconds >= 24UL * 3600UL) {
        *outError = Q9K_E_BPADDR;
        return 0;
    }
    if (desc == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }
    pid = Q9K_ProcIdForDesc(desc);
    if (pid == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }

    for (i = 0; i < Q9K_ALARM_SLOTS; ++i) {
        if (Q9K_GetU32(Q9K_ALARM_ID(i)) == 0UL)
            break;
    }
    if (i >= Q9K_ALARM_SLOTS) {
        *outError = Q9K_E_BPADDR;
        return 0;
    }

    id = Q9K_GetU32(Q9K_ALARM_NEXTID) + 1UL;
    if (id == 0UL)
        id = 1UL;
    Q9K_SetU32(Q9K_ALARM_NEXTID, id);

    Q9K_SetU32(Q9K_ALARM_ID(i), id);
    Q9K_SetU32(Q9K_ALARM_PID(i), (Q9_u32)pid);
    Q9K_SetU32(Q9K_ALARM_SIGNAL(i), (Q9_u32)signal);
    Q9K_SetU32(Q9K_ALARM_TICKS(i), 0UL);      /* 0 = kein Tickzaehler, absolut */
    Q9K_SetU32(Q9K_ALARM_DAY(i), julianDay);
    Q9K_SetU32(Q9K_ALARM_SEC(i), seconds);
    Q9K_SetU32(Q9K_ALARM_CYCLE(i), 0UL);

    *outId = id;
    return 1;
}

/* Q9K_AlarmDelete -- A$Delete. id == 0 loescht alle Alarme DES
 * AUFRUFENDEN Prozesses; die Beschreibung sagt "all pending alarm
 * requests", und fremde Prozesse gehen einen Aufrufer nichts an.
 * Rueckgabe 1 = Erfolg (auch wenn nichts zu loeschen war -- ein
 * abgelaufener Alarm ist kein Fehler). */
int Q9K_AlarmDelete(Q9_u32 id, Q9_u16 *outError)
{
    Q9_u32 desc = Q9K_GetU32(Q9_D_PROC);
    Q9_u16 pid;
    Q9_u32 i;

    *outError = 0;

    if (desc == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }
    pid = Q9K_ProcIdForDesc(desc);

    for (i = 0; i < Q9K_ALARM_SLOTS; ++i) {
        Q9_u32 slotId = Q9K_GetU32(Q9K_ALARM_ID(i));

        if (slotId == 0UL)
            continue;
        if (Q9K_GetU32(Q9K_ALARM_PID(i)) != (Q9_u32)pid)
            continue;               /* fremder Alarm, nicht anfassen */
        if (id != 0UL && slotId != id)
            continue;

        Q9K_SetU32(Q9K_ALARM_ID(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_PID(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_SIGNAL(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_TICKS(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_DAY(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_SEC(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_CYCLE(i), 0UL);
    }
    return 1;
}

/* Q9K_AlarmCleanupProcess -- lifecycle hook for F$UAcct semantics.
 *
 * F$UAcct is an optional extension callback in OS-9, invoked by the kernel
 * when a process is forked, chained, or exits.  The Q9 kernel does not load
 * an OS9P2/SysExt accounting module, but it still owns this resource: an
 * alarm must never survive the process that created it.  This descriptor-
 * based variant is deliberately independent of D_Proc so zombie reaping can
 * clean the dead child rather than whichever process is currently running.
 */
void Q9K_AlarmCleanupProcess(Q9_u32 desc)
{
    Q9_u16 pid;
    Q9_u32 i;

    if (desc == 0UL)
        return;
    pid = Q9K_ProcIdForDesc(desc);
    if (pid == 0U)
        return;

    for (i = 0; i < Q9K_ALARM_SLOTS; ++i) {
        if (Q9K_GetU32(Q9K_ALARM_ID(i)) == 0UL ||
            Q9K_GetU32(Q9K_ALARM_PID(i)) != (Q9_u32)pid)
            continue;
        Q9K_SetU32(Q9K_ALARM_ID(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_PID(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_SIGNAL(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_TICKS(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_DAY(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_SEC(i), 0UL);
        Q9K_SetU32(Q9K_ALARM_CYCLE(i), 0UL);
    }
}

/* Q9K_AlarmTick -- EINMAL PRO TICK aus dem Scheduler aufgerufen (dort,
 * wo auch die Schlafliste heruntergezaehlt wird, s.
 * Q9K_SchedReschedule). Zaehlt jeden belegten Alarm herunter und stellt
 * bei 0 sein Signal zu.
 *
 * Ein zyklischer Alarm wird sofort neu geladen, ein einmaliger
 * freigegeben. Die Zustellung laeuft ueber denselben F$Send-Pfad wie
 * jedes andere Signal -- damit gilt fuer Alarmsignale automatisch auch
 * die Signalmaske (F$SigMask): ist der Empfaenger maskiert, bleibt das
 * Signal anstehen, statt verloren zu gehen.
 *
 * Rueckgabe: Anzahl zugestellter Alarme (fuer den Host-Test). */
Q9_u32 Q9K_AlarmTick(void)
{
    Q9_u32 fired = 0;
    Q9_u32 i;
    Q9_u32 nowDay = 0, nowSec = 0;
    int haveNow = 0;

    for (i = 0; i < Q9K_ALARM_SLOTS; ++i) {
        Q9_u32 ticks;

        if (Q9K_GetU32(Q9K_ALARM_ID(i)) == 0UL)
            continue;

        ticks = Q9K_GetU32(Q9K_ALARM_TICKS(i));

        if (ticks == 0UL) {
            /* Absoluter Alarm: faellig, sobald die Systemzeit sein Ziel
             * erreicht oder ueberschritten hat -- so formuliert es auch
             * die Beschreibung ("anytime the system date/time becomes
             * greater than or equal to the alarm time"), damit ein
             * verschlafener Zeitpunkt nicht einfach verfaellt.
             *
             * Gelesen wird die SYSTEMUHR (q9kernel_clock.c), nicht die
             * Hardware: was F$STime stellt, muss auch fuer Alarme
             * gelten, sonst feuert ein Alarm nach gestellter Uhr zum
             * falschen Zeitpunkt. Die Uhr wird hoechstens EINMAL pro Tick
             * gelesen, und nur wenn ueberhaupt ein absoluter Alarm
             * eingetragen ist. */
            Q9_u32 day = Q9K_GetU32(Q9K_ALARM_DAY(i));

            if (!haveNow) {
                Q9K_ClockRead(&nowDay, &nowSec);
                haveNow = 1;
            }
            if (nowDay < day)
                continue;
            if (nowDay == day && nowSec < Q9K_GetU32(Q9K_ALARM_SEC(i)))
                continue;
            /* faellig -- faellt unten in die Zustellung */
        } else if (ticks > 1UL) {
            Q9K_SetU32(Q9K_ALARM_TICKS(i), ticks - 1UL);
            continue;
        }

        {
            Q9_u32 packed = Q9K_GetU32(Q9K_ALARM_SIGNAL(i));
            Q9_u16 signal = (Q9_u16)(packed & 0xFFFFUL);
            Q9_u32 cycle  = Q9K_GetU32(Q9K_ALARM_CYCLE(i));
            Q9_u16 err = 0;

            Q9K_ProcSend((Q9_u16)Q9K_GetU32(Q9K_ALARM_PID(i)), signal, &err);
            fired++;

            if (cycle != 0UL) {
                Q9K_SetU32(Q9K_ALARM_TICKS(i), cycle);
            } else {
                Q9K_SetU32(Q9K_ALARM_ID(i), 0UL);
                Q9K_SetU32(Q9K_ALARM_PID(i), 0UL);
                Q9K_SetU32(Q9K_ALARM_SIGNAL(i), 0UL);
                Q9K_SetU32(Q9K_ALARM_TICKS(i), 0UL);
                Q9K_SetU32(Q9K_ALARM_DAY(i), 0UL);
                Q9K_SetU32(Q9K_ALARM_SEC(i), 0UL);
                Q9K_SetU32(Q9K_ALARM_CYCLE(i), 0UL);
            }
        }
    }
    return fired;
}

/* Scratch-Bruecke fuer F$Alarm (2026-09-18).
 *
 * ADRESSFALLE, hier einmal teuer bezahlt: der Block lag urspruenglich ab
 * $1A90, direkt hinter einer Tabelle aus 8 Eintraegen zu 16 Byte
 * ($1A00-$1A7F). Als die absoluten Alarme die Eintragsgroesse auf 24 Byte
 * anhoben, wuchs die Tabelle bis $1ABF -- Slot 6 lag danach exakt auf
 * _FUNC und Slot 7 auf _DATE. Gemerkt haette es erst der siebte
 * gleichzeitige Alarm, und dann als stille Verfaelschung der Argumente
 * mitten im Aufruf. Kein Test konnte das sehen: der Hosttest legt Tabelle
 * und Scratch in zwei getrennte Felder und bildet die echte Adresslage
 * gar nicht ab.
 *
 * Deshalb liegt der Block jetzt bei $1B00 mit Luft dazwischen -- und die
 * Pruefung darunter macht daraus einen Baufehler statt eines Laufzeit-
 * raetsels. */
#ifndef Q9K_ALARM_SCRATCH_FUNC
#define Q9K_ALARM_SCRATCH_FUNC   0x1B00UL /* Q9_u32, d1.w EIN = Funktionscode   */
#define Q9K_ALARM_SCRATCH_IDIN   0x1B04UL /* Q9_u32, d0.l EIN / d0.l AUS = ID   */
#define Q9K_ALARM_SCRATCH_SIGNAL 0x1B08UL /* Q9_u32, d2.w EIN                   */
#define Q9K_ALARM_SCRATCH_TICKS  0x1B0CUL /* Q9_u32, d3.l EIN = Intervall/Sekunden */
#define Q9K_ALARM_SCRATCH_ERROR  0x1B10UL /* Q9_u32, d1.w AUS bei Fehler        */
#define Q9K_ALARM_SCRATCH_SUCCESS 0x1B14UL /* Q9_u32, 0/1                       */
#define Q9K_ALARM_SCRATCH_DATE   0x1B18UL /* Q9_u32, d4.l EIN = Datum/Tageszahl    */
#define Q9K_ALARM_ADDRESSES_ARE_REAL 1
#endif

/* Baut nicht, wenn die Alarmtabelle je wieder in den Scratch-Block
 * hineinwaechst. Nur im echten Kernelbau aktiv -- im Hosttest sind beide
 * Adressen Zeigerausdruecke und keine Konstanten. */
#ifdef Q9K_ALARM_ADDRESSES_ARE_REAL
typedef char Q9K_AlarmTableMustNotReachScratch[
    (Q9K_ALARM_NEXTID + 4UL <= Q9K_ALARM_SCRATCH_FUNC) ? 1 : -1];
#endif

void Q9K_SysAlarmImpl(void)
{
    Q9_u32 func = Q9K_GetU32(Q9K_ALARM_SCRATCH_FUNC) & 0xFFFFUL;
    Q9_u32 id = 0;
    Q9_u16 err = 0;
    int ok;

    switch (func) {
    case Q9K_A_DELETE:
        ok = Q9K_AlarmDelete(Q9K_GetU32(Q9K_ALARM_SCRATCH_IDIN), &err);
        break;
    case Q9K_A_SET:
        ok = Q9K_AlarmSet((Q9_u16)Q9K_GetU32(Q9K_ALARM_SCRATCH_SIGNAL),
                          Q9K_GetU32(Q9K_ALARM_SCRATCH_TICKS), 0UL, &id, &err);
        break;
    case Q9K_A_CYCLE:
        {
            Q9_u32 iv = Q9K_GetU32(Q9K_ALARM_SCRATCH_TICKS);
            ok = Q9K_AlarmSet((Q9_u16)Q9K_GetU32(Q9K_ALARM_SCRATCH_SIGNAL),
                              iv, iv, &id, &err);
        }
        break;
    case Q9K_A_ATJUL:
        ok = Q9K_AlarmSetAbsolute((Q9_u16)Q9K_GetU32(Q9K_ALARM_SCRATCH_SIGNAL),
                                  Q9K_GetU32(Q9K_ALARM_SCRATCH_DATE),
                                  Q9K_GetU32(Q9K_ALARM_SCRATCH_TICKS),
                                  &id, &err);
        break;
    case Q9K_A_ATDATE:
        {
            /* Kalenderdatum in derselben Feldkodierung wie ueberall --
             * Jahr im oberen Wort, dann je ein Byte Monat und Tag (am
             * Originalkernel belegt, s. Q9K_SysFTime). */
            Q9_u32 packed = Q9K_GetU32(Q9K_ALARM_SCRATCH_DATE);
            Q9_u32 clock  = Q9K_GetU32(Q9K_ALARM_SCRATCH_TICKS);
            Q9_u32 jd = Q9K_JulianFromDate((packed >> 16) & 0xFFFFUL,
                                           (packed >> 8) & 0xFFUL,
                                           packed & 0xFFUL);
            /* Die Uhrzeit kommt in derselben Feldkodierung wie das
             * Datum -- ein Byte je Stunde, Minute, Sekunde. Genau diese
             * beiden Formen rechnet F$Julian ineinander um; A$AtJul
             * nimmt die andere (Sekunden seit Mitternacht) direkt. */
            Q9_u32 hh = (clock >> 16) & 0xFFUL;
            Q9_u32 mm = (clock >> 8) & 0xFFUL;
            Q9_u32 ss = clock & 0xFFUL;
            if (jd == 0UL || hh > 23UL || mm > 59UL || ss > 59UL) {
                ok = 0;
                err = Q9K_E_BPADDR;
            } else {
                ok = Q9K_AlarmSetAbsolute((Q9_u16)Q9K_GetU32(Q9K_ALARM_SCRATCH_SIGNAL),
                                          jd, hh * 3600UL + mm * 60UL + ss,
                                          &id, &err);
            }
        }
        break;
    case Q9K_A_RESET:
        id = Q9K_GetU32(Q9K_ALARM_SCRATCH_IDIN);
        ok = Q9K_AlarmReset(id,
                            (Q9_u16)Q9K_GetU32(Q9K_ALARM_SCRATCH_SIGNAL),
                            Q9K_GetU32(Q9K_ALARM_SCRATCH_TICKS), &err);
        break;
    default:
        ok = 0;
        err = Q9K_E_UNKSVC;
        break;
    }

    if (ok) {
        Q9K_SetU32(Q9K_ALARM_SCRATCH_IDIN, id);
        Q9K_SetU32(Q9K_ALARM_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_ALARM_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_ALARM_SCRATCH_SUCCESS, 0UL);
    }
}
