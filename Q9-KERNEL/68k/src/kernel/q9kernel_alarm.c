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
 *   A$Reset  = 5  -- s. u.
 *
 * UMGESETZT SIND A$Delete, A$Set UND A$Cycle -- die drei, die in Ticks
 * rechnen. A$AtDate, A$AtJul und A$Reset melden E$UnkSvc.
 *
 * KORREKTUR ZUR URSPRUENGLICHEN BEGRUENDUNG (2026-09-18, noch am selben
 * Tag): hier stand zuerst, die absoluten Varianten fehlten, weil es
 * keine Systemuhr gaebe. Das ist falsch -- es gibt eine: Q9K_SysFTime
 * (q9kernel_entry.a) liest einen echten RTC72421 bei $FFFFD000, den der
 * Emulator bereitstellt und der die Hostuhr spiegelt.
 *
 * Der wirkliche Grund ist ein anderer und wiegt schwerer: die beiden
 * vorhandenen Zeitquellen widersprechen sich im DATUMSFORMAT. Q9K_SysFTime
 * baut sein Datum als DEZIMALZAHL zusammen (Jahr*10000 + Monat*100 + Tag,
 * also 20260918), waehrend Q9K_ProcJulian (q9kernel_date.c) FELDER
 * erwartet (Jahr im oberen Wort, dann je ein Byte Monat und Tag). Wer
 * F$Time aufruft und das Ergebnis an F$Julian weiterreicht, bekommt
 * Unsinn. Welche der beiden Lesarten von "yyyymmdd" die reale ist, laesst
 * sich aus dem Handbuchtext allein nicht entscheiden -- beide passen auf
 * die Schreibweise. Solange das nicht geklaert ist, waere ein absoluter
 * Alarm auf Sand gebaut, und zwar auf eine Art, die beim Testen nicht
 * auffaellt: er ginge einfach zum falschen Zeitpunkt los.
 *
 * Ist die Frage entschieden und beide Seiten auf dasselbe Format
 * gebracht, sind A$AtDate und A$AtJul klein: F$Time liefert die
 * Gegenwart, F$Julian rechnet das Zieldatum in eine Tageszahl, und der
 * Tick-Durchlauf unten vergleicht zwei Zahlen.
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
/* Eintragsgroesse und Feldabstaende: real 16 Byte mit 4-Byte-Feldern.
 * Per #ifndef ueberschreibbar, gleicher Grund wie bei jeder anderen
 * Tabelle dieses Kernels -- auf einem 64-Bit-Testhost ist Q9_u32 8 Byte
 * breit, ein Schreibzugriff auf ID reichte dort bis in PID hinein. */
#ifndef Q9K_ALARM_STRIDE
#define Q9K_ALARM_STRIDE     16UL
#define Q9K_ALARM_OFF_PID     4UL
#define Q9K_ALARM_OFF_SIGNAL  8UL
#define Q9K_ALARM_OFF_TICKS  12UL
#endif
#define Q9K_ALARM_ID(i)       (Q9K_ALARM_BASE + (i) * Q9K_ALARM_STRIDE)       /* Q9_u32, 0 = frei */
#define Q9K_ALARM_PID(i)      (Q9K_ALARM_ID(i) + Q9K_ALARM_OFF_PID)            /* Q9_u32, Empfaenger */
#define Q9K_ALARM_SIGNAL(i)   (Q9K_ALARM_ID(i) + Q9K_ALARM_OFF_SIGNAL)         /* Q9_u32, Signalcode */
#define Q9K_ALARM_TICKS(i)    (Q9K_ALARM_ID(i) + Q9K_ALARM_OFF_TICKS)          /* Q9_u32, Restticks  */
/* Das Intervall eines zyklischen Alarms teilt sich die Zelle mit der
 * Signalnummer nicht -- es steht im oberen Wort von SIGNAL, damit der
 * Eintrag bei 16 Byte bleibt. 0 = einmaliger Alarm. */
#define Q9K_ALARM_CYCLE_SHIFT 16

/* Fortlaufender Zaehler fuer Alarm-IDs, direkt hinter der Tabelle. */
#ifndef Q9K_ALARM_NEXTID
#define Q9K_ALARM_NEXTID (Q9K_ALARM_BASE + Q9K_ALARM_SLOTS * Q9K_ALARM_STRIDE)
#endif

#define Q9K_A_DELETE 0U
#define Q9K_A_SET    1U
#define Q9K_A_CYCLE  2U

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
    Q9K_SetU32(Q9K_ALARM_SIGNAL(i),
               ((cycleTicks & 0xFFFFUL) << Q9K_ALARM_CYCLE_SHIFT) | (Q9_u32)signal);
    Q9K_SetU32(Q9K_ALARM_TICKS(i), ticks);

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
    }
    return 1;
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

    for (i = 0; i < Q9K_ALARM_SLOTS; ++i) {
        Q9_u32 ticks;

        if (Q9K_GetU32(Q9K_ALARM_ID(i)) == 0UL)
            continue;

        ticks = Q9K_GetU32(Q9K_ALARM_TICKS(i));
        if (ticks > 1UL) {
            Q9K_SetU32(Q9K_ALARM_TICKS(i), ticks - 1UL);
            continue;
        }

        {
            Q9_u32 packed = Q9K_GetU32(Q9K_ALARM_SIGNAL(i));
            Q9_u16 signal = (Q9_u16)(packed & 0xFFFFUL);
            Q9_u32 cycle  = (packed >> Q9K_ALARM_CYCLE_SHIFT) & 0xFFFFUL;
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
            }
        }
    }
    return fired;
}

/* Scratch-Bruecke fuer F$Alarm (2026-09-18). */
#ifndef Q9K_ALARM_SCRATCH_FUNC
#define Q9K_ALARM_SCRATCH_FUNC   0x1A90UL /* Q9_u32, d1.w EIN = Funktionscode   */
#define Q9K_ALARM_SCRATCH_IDIN   0x1A94UL /* Q9_u32, d0.l EIN / d0.l AUS = ID   */
#define Q9K_ALARM_SCRATCH_SIGNAL 0x1A98UL /* Q9_u32, d2.w EIN                   */
#define Q9K_ALARM_SCRATCH_TICKS  0x1A9CUL /* Q9_u32, d3.l EIN                   */
#define Q9K_ALARM_SCRATCH_ERROR  0x1AA0UL /* Q9_u32, d1.w AUS bei Fehler        */
#define Q9K_ALARM_SCRATCH_SUCCESS 0x1AA4UL /* Q9_u32, 0/1                       */
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
    default:
        /* A$AtDate/A$AtJul/A$Reset und alles Unbekannte -- s.
         * Kopfkommentar: ohne Systemuhr waere jede Antwort geraten. */
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
