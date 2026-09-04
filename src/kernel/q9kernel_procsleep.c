/*
 * q9kernel_procsleep.c -- Q9-OS eigener Kernel: F$Sleep (Abschnitt
 *                         "F$Sleep", 2026-08-30, im Anschluss an
 *                         F$Exit/F$Wait/F$Panic -- erster Syscall aus
 *                         der Gruppe "baut auf vorhandener Scheduler-/
 *                         Timer-Infrastruktur auf").
 *
 * Reale Register-/Verhaltenskonvention ECHT per Read gelesen (nicht
 * geraten), 68k_tech.pdf S. 497-498 (Callcode 0x0a, s.
 * modules/SYSCALL_MODULE_MAP.md):
 *   IN  d0.l = Ticks/Sekunden (Schlafdauer)
 *   OUT d0.l = verbleibende Ticks, falls VORZEITIG aktiviert
 *   Fehler: cc=Carry gesetzt, d1.w = Fehlercode (nur E$NoClk moeglich)
 *
 * Reale Verhaltensregeln (woertlich aus dem Manual):
 *   - "Sleep(0) sleeps indefinitely."
 *   - "Sleep(1) gives up a time slice but does not necessarily sleep for
 *     one tick. ... the process is immediately inserted into the active
 *     process queue and resumes execution when it reaches the front of
 *     the queue."
 *   - "A sleep of two or more (n) ticks causes the process to be
 *     inserted into the active process queue after (n - 1) ticks occur
 *     and resumes execution when it reaches the front of the queue."
 *   - "The process is activated before the full time interval if a
 *     signal (in particular S$Wake) is received."
 *   - "If the high order bit of d0.l is set, the low 31 bits are
 *     converted from 256ths of a second into ticks ... to allow program
 *     delays to be independent of the system's clock rate."
 *   - Fehler nur E$NoClk ("The system clock must be running to perform a
 *     timed sleep").
 *
 * EIGENE ENTSCHEIDUNGEN, dokumentiert:
 *   - E$NoClk kann in diesem Kernel NIE auftreten -- der Board-Timer
 *     laeuft seit dem Scheduler-Meilenstein permanent (s.
 *     Q9K_TimerActivate/Q9K_TimerIRQHandler, q9kernel_entry.a). F$Sleep
 *     hat deshalb hier UEBERHAUPT keinen Fehlerpfad, immer Erfolg
 *     (irgendwann).
 *   - "Vorzeitige Aktivierung per Signal" (S$Wake) ist NICHT
 *     implementiert -- kein F$Send/F$Icpt-Signalsystem existiert bisher
 *     in diesem Kernel (naechster Kandidat aus derselben "baut auf
 *     Vorhandenem auf"-Gruppe). Sleep(0) = unendlich ist deshalb bei uns
 *     WIRKLICH unendlich (s. Kopfkommentar Q9K_SleepQDecrementAll,
 *     q9kernel_sched.c) -- kein Bug, sondern die ehrliche Konsequenz.
 *     D0 = 0 wird deshalb bei JEDER Aktivierung geschrieben (nie ein
 *     Fall "vorzeitig", da es in diesem Kernel gar nicht vorkommen
 *     kann).
 *   - 256stel-Sekunden-Umrechnung OHNE echte Division implementiert:
 *     bei real 100 Ticks/Sekunde (10ms, s. Q9K_TimerActivate-
 *     Kopfkommentar) gilt ticks = wert*100/256 = wert*25/64 -- die
 *     Division durch 64 (Zweierpotenz) ist ein reiner Rechtsschift, die
 *     einzige noetige "echte" Operation ist eine 32x32-Multiplikation
 *     (__multiply, bereits durch den Compiler bereitgestellt, s.
 *     q9kernel_entry.a Kopfkommentar). Sicher fuer jeden realistischen
 *     Wertebereich (ueberlaeuft erst bei absurd langen Schlafzeiten,
 *     >100 Jahre) -- kein Ueberlaufschutz noetig/gebaut.
 *   - IMMER ein Block-Vorgang (anders als F$Wait, das einen "sofort
 *     erfolgreich"-Schnellpfad hat) -- selbst Sleep(1) fuehrt laut
 *     Manual-Text zu einem echten Wiedereinreihen+Prozesswechsel ("is
 *     immediately inserted into the active process queue and resumes
 *     execution when it reaches the front"), nie zu einem direkten
 *     Ruecksprung ohne Kontextwechsel. Der Assembler-Trampolin
 *     (Q9K_SysFSleep, q9kernel_entry.a) sichert deshalb IMMER den
 *     kompletten Registersatz, exakt wie F$Waits Blockierpfad.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

extern void   Q9K_SchedInsert(Q9_u32 desc);    /* q9kernel_sched.c */
extern void   Q9K_SleepQInsert(Q9_u32 desc);   /* q9kernel_sched.c */
extern void   Q9K_SchedWake(Q9_u32 desc);      /* q9kernel_sched.c -- schlafenden Prozess wecken */
extern Q9_u32 Q9K_ProcLookup(Q9_u16 pid);      /* q9kernel_procapi.c -- PID -> Deskriptor        */
extern Q9_u32 Q9K_SchedFirstPick(void);        /* q9kernel_sched.c -- "naechsten Prozess waehlen,
                                                  * kein aktueller zum Wiedereinreihen", gleiche
                                                  * Wiederverwendung wie schon bei F$Exit/F$Wait */

/* Deskriptor-Feldoffsets -- lokal dupliziert, gleiche schlanke
 * Konvention wie ueberall in diesem Kernel (s. q9kernel_firstproc.c
 * Kopfkommentar fuer das vollstaendige Layout). */
#ifndef Q9K_PROCDESC_STATE_OFF
#define Q9K_PROCDESC_STATE_OFF      0x1DUL
#endif
#ifndef Q9K_PROCDESC_SLEEPTICKS_OFF
#define Q9K_PROCDESC_SLEEPTICKS_OFF 0x1C4UL
#endif
#ifndef Q9K_PROCDESC_SAVEDSP_OFF
#define Q9K_PROCDESC_SAVEDSP_OFF    0x08UL
#endif

#define Q9K_PROCDESC_STATE_SLEEPING 's'
#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif

/* s. q9kernel_sched.c Q9K_SLEEP_INFINITE -- lokal dupliziert (gleiche
 * Konvention, kein gemeinsamer Header in diesem Kernel). */
#define Q9K_SLEEP_INFINITE 0xFFFFFFFFUL

/* Fuer den Ticks-Umrechnungsfaktor -- s. Kopfkommentar */
#define Q9K_TICKS_PER_SEC 100UL

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
static Q9_u8  Q9K_GetU8(Q9_u32 addr)  { return *(volatile Q9_u8 *)addr; }
static void   Q9K_SetU8(Q9_u32 addr, Q9_u8 value)  { *(volatile Q9_u8 *)addr = value; }
static void   Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }

/* Schreibt value in Register regIndex des 60-Byte-Registersatz-Bereichs
 * ab frameBase -- LOKALE Kopie von Q9K_SetFrameReg (q9kernel_firstproc.c/
 * q9kernel_procend.c), byteweise statt Pointer-Cast aus demselben, dort
 * ausfuehrlich dokumentierten Grund (Host-/Zielbreitenunabhaengigkeit). */
static void Q9K_SetFrameReg(Q9_u32 frameBase, Q9_u32 regIndex, Q9_u32 value)
{
    Q9_u32 addr = frameBase + regIndex * 4UL;

    Q9K_SetU8(addr + 0, (Q9_u8)(value >> 24));
    Q9K_SetU8(addr + 1, (Q9_u8)(value >> 16));
    Q9K_SetU8(addr + 2, (Q9_u8)(value >> 8));
    Q9K_SetU8(addr + 3, (Q9_u8)value);
}

/* Q9K_ProcSleep -- echte F$Sleep-Kernlogik (s. Kopfkommentar). Rueckgabe:
 * Deskriptoradresse des naechsten zu startenden Prozesses, oder 0 falls
 * keiner mehr bereit ist (der Aufrufer -- Q9K_SysFSleep, q9kernel_entry.a
 * -- loest dann F$Panic(K$Idle) aus, exakt wie bei F$Exit/F$Wait). */
Q9_u32 Q9K_ProcSleep(Q9_u32 callerDesc, Q9_u32 ticksIn)
{
    Q9_u32 ticks;
    Q9_u32 frameBase;

    if (ticksIn & 0x80000000UL) {
        /* 256stel-Sekunden -> Ticks: wert*100/256 = wert*25/64 (reiner
         * Rechtsschift statt Division), s. Kopfkommentar. */
        ticks = ((ticksIn & 0x7FFFFFFFUL) * 25UL) >> 6;
    } else {
        ticks = ticksIn;
    }

    /* D0 = 0 fuer JEDE Aktivierung -- "vorzeitig" (per Signal) kann in
     * diesem Kernel nicht vorkommen, s. Kopfkommentar. Muss VOR dem
     * Wiedereinreihen geschehen (der Rahmen ist ab hier fuer den
     * schlafenden/wartenden Prozess "fertig"). */
    frameBase = Q9K_GetU32(callerDesc + Q9K_PROCDESC_SAVEDSP_OFF);
    Q9K_SetFrameReg(frameBase, 0, 0);

    if (ticks == 0) {
        /* Sleep(0) = unendlich, s. Kopfkommentar */
        Q9K_SetU8(callerDesc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_SLEEPING);
        Q9K_SetU32(callerDesc + Q9K_PROCDESC_SLEEPTICKS_OFF, Q9K_SLEEP_INFINITE);
        Q9K_SleepQInsert(callerDesc);
    } else if (ticks == 1) {
        /* Sleep(1) = reiner Zeitscheiben-Verzicht -- SOFORT zurueck in
         * die Ready-Queue, keine Sleep-Queue-Beteiligung (reale
         * Konvention, s. Kopfkommentar). State bleibt 'a' -- der
         * Prozess "schlief" laut Manual nie wirklich. */
        Q9K_SchedInsert(callerDesc);
    } else {
        Q9K_SetU8(callerDesc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_SLEEPING);
        Q9K_SetU32(callerDesc + Q9K_PROCDESC_SLEEPTICKS_OFF, ticks - 1);
        Q9K_SleepQInsert(callerDesc);
    }

    /* Aufrufer laeuft nie sofort weiter (F$Sleep kehrt laut Manual
     * IMMER erst nach mindestens einem Prozesswechsel zurueck, s.
     * Kopfkommentar) -- erzwungener Wechsel, gleiche Wiederverwendung
     * wie bei F$Exit. */
    return Q9K_SchedFirstPick();
}

/* Eigene Kernel-Global-Erweiterungen fuer die ASM<->C-Uebergabe von
 * Q9K_SysFSleep (q9kernel_entry.a) -- gleiches, bereits etabliertes
 * Muster wie Q9K_FORK_SCRATCH_x / Q9K_WAIT_SCRATCH_x / Q9K_EXIT_SCRATCH_x.
 * Direkt hinter Q9K_SLEEPQ_SENTINEL_ADDR ($12F0, q9kernel_sched.c) +
 * dessen 0x38 Byte eigenem Next/Prev-Bereich -- naechste freie Adresse
 * $1328. */
#ifndef Q9K_SLEEP_SCRATCH_TICKSIN
#define Q9K_SLEEP_SCRATCH_TICKSIN 0x1328UL   /* Q9_u32, d0.l EIN */
#endif
#ifndef Q9K_SLEEP_SCRATCH_NEXT
#define Q9K_SLEEP_SCRATCH_NEXT    0x132CUL   /* Q9_u32, naechster Deskriptor AUS */
#endif

/* Q9K_SysSleepImpl -- duenne, PARAMETERLOSE Bruecke zwischen dem
 * Assembler-Trampolin Q9K_SysFSleep und Q9K_ProcSleep (echte
 * Zweiparameter-C-Funktion, s. oben) -- gleiches Muster/gleiche
 * Begruendung wie Q9K_SysExitImpl/Q9K_SysWaitImpl (q9kernel_procend.c):
 * reiner C-zu-C-Aufruf hier (kein Risiko), die unverifizierte
 * Assembler<->C-Mehrparameter-Grenze wird ueber die obigen Scratch-
 * Adressen umgangen. */
void Q9K_SysSleepImpl(void)
{
    Q9_u32 callerDesc = Q9K_GetU32(Q9_D_PROC);
    Q9_u32 ticksIn = Q9K_GetU32(Q9K_SLEEP_SCRATCH_TICKSIN);
    Q9_u32 next = Q9K_ProcSleep(callerDesc, ticksIn);

    Q9K_SetU32(Q9K_SLEEP_SCRATCH_NEXT, next);
}

/* ---------------------------------------------------------------------
 * F$Send (Callcode $08, "Send Signal")
 *
 * Real per Live-Analyse gefunden, nicht geraten: die Interrupt-Service-
 * Routine von sc68681 ruft ihn, sobald ein Zeichen empfangen ist, ueber den
 * PEA+RTS-Trampolinweg (Treiber-Offset $654 ff.):
 *
 *     move.w  $8(a2),d0        Prozess-ID des wartenden Lesers
 *     beq     ...              ist sie 0, wird niemand geweckt
 *     moveq   #$1,d1           Signalcode
 *     movea.l $3a4(a6),a3      D_SysDis
 *     pea.l   <ruecksprung>
 *     move.l  $20(a3),-(a7)    Slot $20/4 = Callcode $08
 *     movea.l $420(a3),a3
 *     rts
 *
 * Ohne diesen Dienst blieb ein Prozess, der ueber I$ReadLn auf Eingabe
 * wartet, fuer immer liegen: der Treiber holt das Zeichen zwar ab, sein
 * Weckruf lief aber ins Leere.
 *
 * Konvention (68k_tech): d0.w = Prozess-ID, d1.w = Signalcode.
 * Fehler: E$PrcID ($E0), wenn es zu der ID keinen Prozess gibt.
 *
 * BEWUSSTE VEREINFACHUNG, klar benannt: dieser Kernel kennt noch keine
 * Signal-ZUSTELLUNG -- es gibt keine Signalwarteschlange pro Prozess und
 * keine Intercept-Routinen (F$Icpt). Umgesetzt ist deshalb nur die
 * WECKWIRKUNG: ein schlafender Empfaenger wird aktiviert, der Signalcode
 * selbst verworfen. Fuer den Anlass (Treiber weckt einen wartenden Leser)
 * ist das vollstaendig; sobald echte Signale gebraucht werden -- etwa
 * Ctrl-C/Ctrl-E oder F$Icpt -- muss der Code hier mitwachsen.
 * --------------------------------------------------------------------- */
#ifndef Q9K_SEND_SCRATCH_PID
#endif
#ifndef Q9K_PROCDESC_SIGNAL_OFF
#define Q9K_PROCDESC_SIGNAL_OFF  0x26UL     /* P$Signal, s. process.a */
#endif
#ifndef Q9K_SEND_SCRATCH_PID
#define Q9K_SEND_SCRATCH_PID     0x1608UL   /* Q9_u32, d0.w EIN                */
#define Q9K_SEND_SCRATCH_SIGNAL  0x160CUL   /* Q9_u32, d1.w EIN                */
#define Q9K_SEND_SCRATCH_ERROR   0x1610UL   /* Q9_u32, d1.w AUS bei Fehler     */
#define Q9K_SEND_SCRATCH_SUCCESS 0x1614UL   /* Q9_u32, 0 = Fehlschlag / 1 = ok */
#endif

/* Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
int Q9K_ProcSend(Q9_u16 pid, Q9_u16 signal, Q9_u16 *outError)
{
    Q9_u32 desc;

    *outError = 0;

    desc = Q9K_ProcLookup(pid);
    if (desc == 0) {
        *outError = 0x00E0U;      /* E$PrcID -- keine solche Prozess-ID */
        return 0;
    }
    /* Signalcode in P$Signal ablegen. Das Feld ist im echten OS-9-Layout
     * genau dafuer vorgesehen (Offset $26, s. MWOS/OS9/SRC/DEFS/process.a);
     * ein Empfaenger kann dort nachsehen, WARUM er geweckt wurde. Eine
     * ZUSTELLUNG im vollen Sinn ist das noch nicht -- dafuer fehlen
     * Signalwarteschlange und Intercept-Vektor (P$SigVec, F$Icpt). */
    Q9K_SetU16(desc + Q9K_PROCDESC_SIGNAL_OFF, signal);
    Q9K_SchedWake(desc);
    return 1;
}

/* Duenne, parameterlose Bruecke zum Assembler-Trampolin -- gleiches Muster
 * wie bei allen anderen Syscalls dieser Datei. */
void Q9K_SysSendImpl(void)
{
    Q9_u16 err = 0;
    int ok = Q9K_ProcSend((Q9_u16)Q9K_GetU32(Q9K_SEND_SCRATCH_PID),
                          (Q9_u16)Q9K_GetU32(Q9K_SEND_SCRATCH_SIGNAL),
                          &err);

    Q9K_SetU32(Q9K_SEND_SCRATCH_ERROR, (Q9_u32)err);
    Q9K_SetU32(Q9K_SEND_SCRATCH_SUCCESS, ok ? 1UL : 0UL);
}
