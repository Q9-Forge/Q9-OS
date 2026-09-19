/*
 * q9kernel_sched.c -- Q9-OS eigener Kernel: minimaler, echter,
 *                     preemptiver Scheduler-Kern (Abschnitt "Scheduler",
 *                     2026-08-21, im Anschluss an F$Link/F$UnLink).
 *
 * Reale OS-9-Konvention, direkt aus dem Manual uebernommen (68k_tech.pdf,
 * Kapitel 2 "Process Scheduling" -- Zitate im Kommentar unten):
 *   - Jeder aktive Prozess hat eine feste PRIORITAET (bei Erzeugung
 *     vergeben) und ein veraenderliches ALTER (AGE) als Sortier-
 *     Schluessel der Ready-Queue.
 *   - "When a process is placed in the active queue, its age is set to
 *     the process's assigned priority and the ages of all OTHER
 *     processes increment." -- Q9K_SchedInsert/Q9K_SchedAgeAll setzen
 *     genau das um: der GERADE eingefuegte/wiedereingefuegte Prozess
 *     bekommt Age=Priority, ALLE ANDEREN in der Queue altern (klassischer
 *     Schutz gegen Verhungern niedrigpriorer Prozesse).
 *   - "After the currently executing process's timeslice, the kernel
 *     executes the process with the highest age." -- Q9K_SchedPickHighestAge.
 *   - Tick = 10 ms (Board-Timer, s. Q9-Flux/docs/BOARD.md), Timeslice =
 *     Q9K_SCHED_TSLICE Ticks (Manual-Default: 2).
 *
 * BEWUSST NUR Tick-getriebenes Round-Robin-mit-Aging -- KEIN echtes
 * Prioritaets-Preemption-beim-Aufwachen ("OS-9 provides ... by preempting
 * the currently executing process when a process with a higher priority
 * becomes active" -- das braucht F$Sleep/F$Wait/Signale, die es in
 * diesem Kernel noch nicht gibt). Gleiches "minimales Geruest"-Prinzip
 * wie schon bei Punkt 7 (q9kernel_firstproc.c) -- ehrlich nicht
 * vorgetaeuscht, TODO markiert.
 *
 * Der eigentliche Registersatz-Sicherungs-/Wiederherstellungs-
 * Mechanismus (was bei einem Tick WIRKLICH auf dem Stack passiert)
 * bleibt vollstaendig in Assembler (Q9K_TimerIRQHandler,
 * q9kernel_entry.a) -- diese Datei liest/schreibt NUR den aktuellen
 * Prozess-Deskriptor (Q9_D_PROC) und die Ready-Queue (Q9K_READYQ_SENTINEL_ADDR),
 * kennt den Registersatz selbst nicht.
 *
 * KEIN SMP hier (2026-08-21, so besprochen): echte
 * Mehrprozessor-Hardware existiert fuer 68K nicht und ist fuer den
 * eigenen Kernel auf absehbare Zeit auch nicht geplant -- die Ready-
 * Queue-Manipulation ist deshalb bewusst NUR ueber Interrupt-Maskierung
 * geschuetzt (wie beim echten Kernel), nicht ueber einen echten
 * Spinlock. Ein SMP-faehiger Q9-Kernel fuer eine spaetere Mehrkern-
 * Zielarchitektur braeuchte hier einen echten TAS-Spinlock statt
 * SR-Maskierung -- bewusst nicht vorweggenommen, s. Notiz in
 * [[project_q9_multiarch]].
 */

#include "q9kernel_config.h"

/* Aus q9kernel_alarm.c -- externe Deklaration statt gemeinsamem Header,
 * gleiche schlanke Konvention wie ueberall in diesem Verzeichnis. */
extern unsigned long Q9K_AlarmTick(void);
extern unsigned long Q9K_ClockTick(void);   /* q9kernel_clock.c */

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL   /* [VERIFIZIERT], s. q9sysglob.h */
#endif

/* ECHTER BUG GEFUNDEN + GEFIXT (2026-08-21, per Boot-Test: Q9K_SchedFirstPick
 * lieferte trotz zwei erfolgreich erzeugter Prozesse immer 0): die
 * Ready-Queue-Verkettung benutzte urspruenglich die REALE Adresse
 * Q9_D_ACTIVQ ($3AC, s. q9sysglob.h) direkt als Sentinel und behandelte
 * sie -- wie einen echten Prozessdeskriptor -- mit eigenen Next/Prev-
 * Feldern bei +0x30/+0x34 relativ zu ihrer EIGENEN Adresse. Das
 * kollidiert real: Q9_D_ACTIVQ+0x34 = $3E0 ist WORTGLEICH mit dem
 * echten, dicht danebenliegenden Q9_D_COMPAT2 ($3E0, s. q9sysglob.h) --
 * Q9K_CInit schreibt Q9_D_COMPAT2 VOR jedem Scheduler-Aufruf (Init-Modul-
 * Konfiguration uebernehmen), was das obere Byte von Q9_D_ACTIVQs eigenem
 * "Tail"-Selbstverweis zerstoert; der naechste "sentinel.next = node"-
 * Schreibzugriff (Adresse dann faelschlich "tail+0x30" mit korrumpiertem
 * "tail") landet dadurch auf einer voelligen Wildadresse statt auf
 * Q9_D_ACTIVQ+0x30 -- die Warteschlange bleibt fuer Q9_D_ACTIVQ.next
 * dauerhaft leer, obwohl Q9_D_ACTIVQ.prev zufaellig richtig aussieht
 * (wird vom naechsten Insert sauber ueberschrieben). Per gezielten
 * Zwischenwert-Diagnoseausgaben nach jedem einzelnen Schreibzugriff im
 * echten Emulator zweifelsfrei bestaetigt (nicht nur vermutet) -- auch
 * bei -O0 IDENTISCH reproduzierbar, also kein Optimierer-Artefakt.
 * Q9_D_SLEEPQ/Q9_D_WAITQ traegen denselben latenten Fehler (Q9_D_SLEEPQ+
 * 0x34=$3E8 faellt in den echten Q9_D_POLTBL-Bereich) -- bisher
 * folgenlos, weil sie von NIEMANDEM gelesen werden (kein F$Sleep/F$Wait
 * existiert), TODO fuer wer auch immer das als naechstes anfasst.
 *
 * Fix: die "+0x30/+0x34 relativ zur eigenen Adresse"-Konvention ist an
 * sich RICHTIG und bereits host-getestet korrekt (funktioniert
 * einwandfrei fuer echte, aus dem Arena-Pool geholte Prozessdeskriptoren,
 * s. q9kernel_firstproc.c) -- das Problem ist NUR, sie auf eine echte,
 * dicht mit anderen Feldern gepackte q9sysglob.h-Adresse anzuwenden.
 * Deshalb: Sentinel jetzt auf eine EIGENE, garantiert freie Kernel-
 * Global-Erweiterung gelegt (kein Kompat-Erfordernis fuer eine rein
 * interne Struktur, die kein Modul je inspiziert -- gleiches Prinzip wie
 * Q9K_MODDIR_HEAD_ADDR/Q9K_SCHED_SLICE_ADDR), NICHT mehr auf die reale
 * Q9_D_ACTIVQ-Adresse. Q9_D_ACTIVQ selbst bleibt unangetastet (weiterhin
 * von q9kernel_cinit.c als leere Ringliste "initialisiert", aber jetzt
 * folgenlos/ungenutzt -- TODO: bei Gelegenheit auch dort durch die neue
 * Adresse ersetzen oder ganz weglassen). */
#ifndef Q9K_READYQ_SENTINEL_ADDR
#define Q9K_READYQ_SENTINEL_ADDR 0x1240UL  /* eigene Erweiterung, s. Kopfkommentar --
                                             * braucht 0x38 Byte ab hier fuer die
                                             * eigenen Next/Prev-Felder bei +0x30/+0x34,
                                             * naechste freie Adresse nach
                                             * Q9K_SCHED_SLICE_ADDR ($123C, 2 Byte) */
#endif

/* Q9K_WAITQ_SENTINEL_ADDR -- eigene, kollisionsfreie Warteschlange fuer
 * per F$Wait blockierte Prozesse (Abschnitt "F$Exit/F$Wait", 2026-08-22).
 * GLEICHE Begruendung wie Q9K_READYQ_SENTINEL_ADDR oben: die reale
 * Q9_D_WAITQ-Adresse ($3BC, s. q9sysglob.h) liegt zu dicht an anderen
 * echten, teils [PLATZHALTER] deklarierten Feldern -- +0x30/+0x34 relativ
 * dazu landet bei $3EC/$3F0, potenziell mitten in Q9_D_POLTBL ($3E4,
 * Groesse unbekannt/[PLATZHALTER]). Q9_D_WAITQ selbst bleibt wie
 * Q9_D_ACTIVQ unangetastet (Kompat-Vollstaendigkeit, q9kernel_cinit.c),
 * wird aber von diesem Kernel nicht mehr gelesen/geschrieben. Direkt
 * hinter Q9K_FORK_SCRATCH_SUCCESS ($129C+4, q9kernel_firstproc.c) --
 * naechste freie Adresse $12A0. Braucht wie Q9K_READYQ_SENTINEL_ADDR 0x38
 * Byte fuer die eigenen Next/Prev-Selbstverweis-Felder bei +0x30/+0x34. */
#ifndef Q9K_WAITQ_SENTINEL_ADDR
#define Q9K_WAITQ_SENTINEL_ADDR 0x12A0UL
#endif

/* Q9K_SLEEPQ_SENTINEL_ADDR -- eigene, kollisionsfreie Warteschlange fuer
 * per F$Sleep blockierte Prozesse (Abschnitt "F$Sleep", 2026-08-30).
 * GLEICHE Begruendung/Kollisionsvermeidung wie bei
 * Q9K_WAITQ_SENTINEL_ADDR oben: die reale Q9_D_SLEEPQ-Adresse ($3B4)
 * liegt genauso dicht an anderen echten Feldern (+0x30/+0x34 relativ
 * dazu landet bei $3E4/$3E8, mitten in Q9_D_POLTBL). Q9_D_SLEEPQ selbst
 * bleibt wie Q9_D_ACTIVQ/Q9_D_WAITQ unangetastet (Kompat-
 * Vollstaendigkeit, q9kernel_cinit.c), wird aber nicht mehr benutzt.
 * Direkt hinter den Wait/Exit-Scratch-Feldern ($12D8-$12F0,
 * q9kernel_procend.c) -- naechste freie Adresse $12F0. */
#ifndef Q9K_SLEEPQ_SENTINEL_ADDR
#define Q9K_SLEEPQ_SENTINEL_ADDR 0x12F0UL
#endif

/* ECHTER BUG GEFUNDEN + GEFIXT (2026-09-06): Q9K_PROCDESC_AGE_OFF stand auf
 * $1B. Das Feld ist 2 Byte breit und belegte damit $1B UND $1C -- und $1C ist
 * im echten Layout P$State (Wort, internem Referenzmaterial). Jedes Altern
 * schrieb also ins obere Byte von P$State.
 *
 * Das hat den ganzen Lesepfad blockiert: sc68681 prueft nach dem Aufwachen
 *
 *     $cc72  btst.b #$1,$1c(a4)    * P$State, oberes Byte
 *     $cc78  bne    ...            * gesetzt -> Abbruch mit Carry
 *
 * Stand das Alter gerade auf 6 (oder einem anderen Wert mit Bit 1), hielt der
 * Treiber den wartenden Prozess fuer "condemned" und kehrte mit Fehler
 * zurueck, statt den laengst gefuellten Eingabepuffer auszulesen. Real
 * gemessen: P$State=$0661 bei einem Prozess, dessen Alter gerade 6 war.
 *
 * Reales Layout in diesem Bereich: P$Prior $18 (Wort), P$Age $1a (Wort),
 * P$State $1c (Wort). Das Alter gehoert also nach $1A -- dort liegt es jetzt,
 * und es ist damit sogar das ECHTE P$Age statt einer Eigenerfindung.
 */
#define Q9K_PROCDESC_STATE_OFF    0x1DUL   /* unteres Byte von P$State ($1c, Wort) --
                                             * eigener Zustandsbuchstabe, kollisionsfrei
                                             * solange das obere Byte 0 bleibt */
#define Q9K_PROCDESC_PRIORITY_OFF 0x19UL   /* unteres Byte von P$Prior ($18, Wort) */
#define Q9K_PROCDESC_AGE_OFF      0x1AUL   /* P$Age (Wort) -- "Ages never increment
                                             * beyond $ffff" (Manual), passt exakt */
#ifndef Q9K_PROCDESC_SLEEPTICKS_OFF
#define Q9K_PROCDESC_SLEEPTICKS_OFF 0x1C4UL   /* s. q9kernel_firstproc.c Kopfkommentar */
#endif
#define Q9K_PROCDESC_STATE_ACTIVE 'a'   /* s. q9kernel_firstproc.c Kopfkommentar */
#define Q9K_PROCDESC_STATE_SLEEPING 's'  /* s. q9kernel_procsleep.c -- dort gesetzt */
#define Q9K_SLEEP_INFINITE 0xFFFFFFFFUL   /* Sentinel fuer Sleep(0), s. q9kernel_procsleep.c */
#ifndef Q9K_READYQ_NEXT_OFF
#define Q9K_READYQ_NEXT_OFF 0x30UL
#endif
#ifndef Q9K_READYQ_PREV_OFF
#define Q9K_READYQ_PREV_OFF 0x34UL
#endif

/* Eigene Kernel-Global-Erweiterung -- Restticks der aktuellen Zeitscheibe.
 * Direkt hinter Q9K_MODDIR_HEAD_ADDR ($1238, q9kernel_moddir.c). */
#ifndef Q9K_SCHED_SLICE_ADDR
#define Q9K_SCHED_SLICE_ADDR 0x123CUL
#endif
#define Q9K_SCHED_TSLICE 2U   /* Ticks pro Zeitscheibe, Manual-Default (D_TSlice) */

#ifndef Q9K_SCHED_MINPTY_ADDR
/* Systemweite Mindestprioritaet: ein Prozess, dessen Prioritaet DARUNTER
 * liegt, wird nicht mehr ausgewaehlt. Das ist der vom Handbuch bei
 * F$SSpd genannte Weg, einen Prozess anzuhalten ("You can suspend a
 * process by setting its priority below the system's minimum executable
 * priority level").
 *
 * BEWUSST EIN Q9-EIGENES FELD im Kernel-Scratchbereich, NICHT das
 * Systemglobal D_MinPty. Der erste Versuch benutzte die Adresse $55E aus
 * common/src/q9sysglob.h -- dort ist sie aber ausdruecklich als
 * "[HANDBUCH]" gekennzeichnet, also aus der Dokumentation abgeleitet und
 * nie am Binaercode verifiziert. Live zeigte sich, warum das zaehlt: mit
 * $55E als Quelle blieb das System gleich beim Booten stehen, IOMan kam
 * nicht einmal bis zum Fuellen seiner Geraetetabelle -- an dieser
 * Adresse steht in diesem System etwas anderes, und der Scheduler las
 * einen Zufallswert als Untergrenze und sperrte damit jeden Prozess aus.
 *
 * Solange kein fremdes Modul eine Mindestpriorietaet setzt (und keines
 * tut das -- gesucht wurde danach), ist das hier ein rein interner
 * Begriff, und ein eigenes Feld ist die ehrlichere Loesung als eine
 * geratene fremde Adresse. Bleibt 0, solange niemand sie setzt; bei 0
 * kann keine Prioritaet darunter liegen, das Verhalten ist dann
 * unveraendert. */
#define Q9K_SCHED_MINPTY_ADDR 0x19E0UL
#endif

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
static Q9_u16 Q9K_GetU16(Q9_u32 addr) { return *(volatile Q9_u16 *)addr; }
static void   Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }
static Q9_u8  Q9K_GetU8(Q9_u32 addr) { return *(volatile Q9_u8 *)addr; }
static void   Q9K_SetU8(Q9_u32 addr, Q9_u8 value) { *(volatile Q9_u8 *)addr = value; }

/* Entfernt node aus einer zirkulaeren doppelt verketteten Liste (Next/Prev
 * wie ueberall in diesem Kernel) -- funktioniert unabhaengig von der
 * Position, matcht das Grundmuster aus q9kernel_firstproc.c. */
static void Q9K_ListUnlink(Q9_u32 node)
{
    Q9_u32 next = Q9K_GetU32(node + Q9K_READYQ_NEXT_OFF);
    Q9_u32 prev = Q9K_GetU32(node + Q9K_READYQ_PREV_OFF);

    Q9K_SetU32(prev + Q9K_READYQ_NEXT_OFF, next);
    Q9K_SetU32(next + Q9K_READYQ_PREV_OFF, prev);
}

static void Q9K_ListAppend(Q9_u32 sentinel, Q9_u32 node)
{
    Q9_u32 tail = Q9K_GetU32(sentinel + Q9K_READYQ_PREV_OFF);

    Q9K_SetU32(node + Q9K_READYQ_NEXT_OFF, sentinel);
    Q9K_SetU32(node + Q9K_READYQ_PREV_OFF, tail);
    Q9K_SetU32(tail + Q9K_READYQ_NEXT_OFF, node);
    Q9K_SetU32(sentinel + Q9K_READYQ_PREV_OFF, node);
}

/* Fuegt desc in die Ready-Queue (Q9K_READYQ_SENTINEL_ADDR) ein -- Age wird auf die
 * Prioritaet zurueckgesetzt (reale Konvention, s. Kopfkommentar). Auch
 * fuer die ERSTMALIGE Einfuegung eines frisch erzeugten Prozesses
 * verwendet (Q9K_ProcCreate, q9kernel_firstproc.c), nicht nur beim
 * Wiedereinfuegen nach einer Zeitscheibe. */
void Q9K_SchedInsert(Q9_u32 desc)
{
    Q9K_SetU16(desc + Q9K_PROCDESC_AGE_OFF, (Q9_u16)Q9K_GetU8(desc + Q9K_PROCDESC_PRIORITY_OFF));
    Q9K_ListAppend(Q9K_READYQ_SENTINEL_ADDR, desc);
}

/* Q9K_SchedSetPriority -- applies a changed priority to a descriptor and,
 * when that descriptor is already waiting in the ready queue, refreshes its
 * age immediately.  The running process is not in that queue and therefore
 * keeps its current timeslice; it will receive the new priority when the
 * scheduler requeues it.  This matches the useful part of F$SPrior without
 * introducing an unsafe mid-instruction preemption from the syscall path. */
void Q9K_SchedSetPriority(Q9_u32 desc, Q9_u16 priority)
{
    Q9_u32 node;
    Q9_u8 stored = (Q9_u8)(priority & 0xFFU);

    Q9K_SetU8(desc + Q9K_PROCDESC_PRIORITY_OFF, stored);
    node = Q9K_GetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF);
    while (node != Q9K_READYQ_SENTINEL_ADDR) {
        if (node == desc) {
            Q9K_SetU16(desc + Q9K_PROCDESC_AGE_OFF, (Q9_u16)stored);
            break;
        }
        node = Q9K_GetU32(node + Q9K_READYQ_NEXT_OFF);
    }
}

/* Q9K_SchedWake -- einen schlafenden Prozess vorzeitig aktivieren
 * (2026-09-04, fuer F$Send).
 *
 * Kapselt genau das Muster, das Q9K_SleepQDecrementAll beim Ablaufen eines
 * Timeouts anwendet: aus der Schlafliste nehmen, Zustand auf ACTIVE, zurueck
 * in die Ready-Queue. Als eigene Funktion, weil Q9K_ListUnlink hier `static`
 * ist und ausserhalb dieser Datei nicht erreichbar sein soll.
 *
 * Ein Prozess, der NICHT schlaeft, bleibt unangetastet -- ein Signal an
 * einen laufenden Prozess ist kein Fehler, es weckt nur nichts.
 */
void Q9K_SchedWake(Q9_u32 desc)
{
    if (desc == 0) {
        return;
    }
    if (Q9K_GetU8(desc + Q9K_PROCDESC_STATE_OFF) != Q9K_PROCDESC_STATE_SLEEPING) {
        return;
    }
    Q9K_ListUnlink(desc);
    Q9K_SetU8(desc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);
    Q9K_SchedInsert(desc);
}


/* NACHTRAG 2026-08-22 (Abschnitt "F$Exit/F$Wait") -- Q9K_ListAppend/
 * Q9K_ListUnlink sind generisch (funktionieren mit JEDER Next/Prev-Liste,
 * s. Kopfkommentare oben), aber static -- fuer die neue Wait-Queue
 * braucht q9kernel_procend.c von AUSSEN darauf zugreifbare Varianten.
 * Duenne, exportierte Weiterleitungen statt die bestehenden Helfer
 * static->non-static umzustellen (kein Risiko fuer die bereits real
 * verifizierte Ready-Queue-Logik oben). */
void Q9K_WaitQInsert(Q9_u32 desc)
{
    Q9K_ListAppend(Q9K_WAITQ_SENTINEL_ADDR, desc);
}

void Q9K_WaitQRemove(Q9_u32 desc)
{
    Q9K_ListUnlink(desc);
}

/* NACHTRAG 2026-08-30 (Abschnitt "F$Sleep") -- exportierte Weiterleitung
 * fuer q9kernel_procsleep.c, gleiches Muster wie Q9K_WaitQInsert oben.
 * KEIN Q9K_SleepQRemove -- wird erst gebraucht, sobald ein Mechanismus
 * existiert, der einen schlafenden Prozess VORZEITIG wecken kann
 * (F$Send/Signale, s. Kopfkommentar bei Q9K_SleepQDecrementAll unten --
 * noch nicht implementiert). */
void Q9K_SleepQInsert(Q9_u32 desc)
{
    Q9K_ListAppend(Q9K_SLEEPQ_SENTINEL_ADDR, desc);
}

/* Erniedrigt den Countdown ALLER Eintraege in der Sleep-Queue um 1 --
 * einmal PRO TICK aufgerufen (aus Q9K_SchedReschedule, s. dort), reale
 * F$Sleep-Konvention: "a sleep of two or more (n) ticks causes the
 * process to be inserted into the active process queue after (n - 1)
 * ticks occur" (68k_tech.pdf S. 497-498) -- q9kernel_procsleep.c
 * speichert deshalb bereits (n-1) als Startwert, hier wird nur noch
 * simpel bis 0 heruntergezaehlt. Erreicht ein Countdown 0, wird der
 * Deskriptor aus der Sleep-Queue entfernt, State zurueck auf aktiv
 * gesetzt (Q9K_SchedInsert selbst fasst State NICHT an, s. Kopfkommentar
 * dort) und per Q9K_SchedInsert in die Ready-Queue verschoben.
 *
 * Eintraege mit dem Sentinel Q9K_SLEEP_INFINITE (Sleep(0) = unendlich)
 * werden NIE dekrementiert/geweckt -- reale Semantik: "Sleeping
 * indefinitely is a good way to wait for a signal or interrupt" --
 * dieser Kernel hat noch KEIN F$Send/Signalsystem (bewusste, bereits
 * bei Q9K_ProcSleep dokumentierte Grenze), ein solcher Prozess bleibt
 * hier also dauerhaft schlafen. Kein Bug, sondern die ehrliche
 * Konsequenz der fehlenden Signal-Infrastruktur.
 *
 * Muss rueckwaerts-sicher gegen Entfernen-waehrend-des-Durchlaufens
 * sein -- next wird deshalb VOR einem moeglichen Q9K_ListUnlink
 * gemerkt, gleiches Muster wie in Q9K_SchedAgeAll/-PickHighestAge. */
static void Q9K_SleepQDecrementAll(void)
{
    Q9_u32 node = Q9K_GetU32(Q9K_SLEEPQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF);

    while (node != Q9K_SLEEPQ_SENTINEL_ADDR) {
        Q9_u32 next = Q9K_GetU32(node + Q9K_READYQ_NEXT_OFF);
        Q9_u32 ticks = Q9K_GetU32(node + Q9K_PROCDESC_SLEEPTICKS_OFF);

        if (ticks != Q9K_SLEEP_INFINITE) {
            ticks = (ticks == 0) ? 0 : ticks - 1;   /* 0 defensiv abgefangen, s. u. */
            Q9K_SetU32(node + Q9K_PROCDESC_SLEEPTICKS_OFF, ticks);

            if (ticks == 0) {
                Q9K_ListUnlink(node);
                Q9K_SetU8(node + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);
                Q9K_SchedInsert(node);
            }
        }

        node = next;
    }
}

/* Erhoeht das Alter ALLER Eintraege in der Ready-Queue um 1 ("the ages
 * of all other processes increment", "Ages never increment beyond
 * $ffff"). Der GERADE laufende Prozess ist bewusst NICHT Teil dieser
 * Liste (wird beim Auswaehlen entfernt, s. Q9K_SchedPickHighestAge) --
 * "all OTHER processes" passt deshalb exakt zu "alle Eintraege in
 * Q9K_READYQ_SENTINEL_ADDR", ohne den laufenden Prozess extra ausschliessen zu
 * muessen. */
static void Q9K_SchedAgeAll(void)
{
    Q9_u32 node = Q9K_GetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF);

    while (node != Q9K_READYQ_SENTINEL_ADDR) {
        Q9_u16 age = Q9K_GetU16(node + Q9K_PROCDESC_AGE_OFF);

        if (age != 0xFFFFU)
            Q9K_SetU16(node + Q9K_PROCDESC_AGE_OFF, (Q9_u16)(age + 1));

        node = Q9K_GetU32(node + Q9K_READYQ_NEXT_OFF);
    }
}

/* Waehlt den Eintrag mit dem hoechsten Alter aus der Ready-Queue,
 * entfernt ihn UND gibt ihn zurueck (0 = Queue leer). "the kernel
 * executes the process with the highest age" -- bei Gleichstand
 * gewinnt der zuerst gefundene (FIFO-Reihenfolge der Liste), das
 * Manual macht dazu keine genauere Aussage. */
static Q9_u32 Q9K_SchedPickHighestAge(void)
{
    Q9_u32 node = Q9K_GetU32(Q9K_READYQ_SENTINEL_ADDR + Q9K_READYQ_NEXT_OFF);
    Q9_u32 best = 0;
    Q9_u16 bestAge = 0;

    Q9_u16 minPty = Q9K_GetU16(Q9K_SCHED_MINPTY_ADDR);

    while (node != Q9K_READYQ_SENTINEL_ADDR) {
        Q9_u16 age = Q9K_GetU16(node + Q9K_PROCDESC_AGE_OFF);

        /* Angehaltene Prozesse ueberspringen: wessen Prioritaet unter der
         * systemweiten Mindestpriorietaet liegt, kommt nicht dran (s.
         * Q9K_SCHED_MINPTY_ADDR oben). Er bleibt in der Ready-Queue --
         * angehalten heisst nicht vergessen; sobald die Prioritaet wieder
         * reicht, waehlt ihn diese Schleife von selbst wieder aus. */
        if (minPty == 0
            || (Q9_u16)Q9K_GetU8(node + Q9K_PROCDESC_PRIORITY_OFF) >= minPty) {
            if (best == 0 || age > bestAge) {
                best = node;
                bestAge = age;
            }
        }

        node = Q9K_GetU32(node + Q9K_READYQ_NEXT_OFF);
    }

    if (best != 0)
        Q9K_ListUnlink(best);

    return best;
}

/* Herzstueck: wird per bsr aus Q9K_TimerIRQHandler (q9kernel_entry.a)
 * EINMAL PRO TICK aufgerufen (Board-Timer, 10 ms, s. Q9-Flux/docs/
 * BOARD.md), NACHDEM der volle Registersatz des unterbrochenen
 * Prozesses bereits auf dessen eigenem Stack gesichert und dessen
 * SavedSP-Feld bereits aktualisiert wurde -- diese Funktion selbst
 * fasst nur Q9_D_PROC und die Ready-Queue an, nie Register direkt.
 *
 * Rueckgabe: 0 = kein Prozesswechsel (derselbe Prozess laeuft weiter,
 * der Aufrufer restauriert einfach dessen eben gesicherten Kontext
 * wieder), sonst die Deskriptoradresse des NEU zu startenden Prozesses
 * (der Aufrufer laedt DESSEN SavedSP als neuen Stack, BEVOR er den
 * Registersatz zurueckholt). */
Q9_u32 Q9K_SchedReschedule(void)
{
    Q9_u32 current = Q9K_GetU32(Q9_D_PROC);
    Q9_u16 slice;

    Q9K_SchedAgeAll();
    Q9K_SleepQDecrementAll();   /* NACHTRAG 2026-08-30, Abschnitt "F$Sleep" -- einmal pro Tick, s. dortigen Kopfkommentar */
    Q9K_AlarmTick();            /* NACHTRAG 2026-09-18, F$Alarm -- ebenfalls einmal pro Tick, s. q9kernel_alarm.c */
    Q9K_ClockTick();            /* NACHTRAG 2026-09-18, Systemuhr -- VOR den Alarmen waere falsch: ein absoluter
                                 * Alarm soll den Zeitpunkt sehen, der beim Eintragen galt, nicht den um einen
                                 * Tick vorgerueckten (s. q9kernel_clock.c) */

    slice = Q9K_GetU16(Q9K_SCHED_SLICE_ADDR);
    if (slice > 0) {
        Q9K_SetU16(Q9K_SCHED_SLICE_ADDR, (Q9_u16)(slice - 1));
        return 0;
    }

    {
        Q9_u32 next = Q9K_SchedPickHighestAge();

        Q9K_SetU16(Q9K_SCHED_SLICE_ADDR, Q9K_SCHED_TSLICE);

        if (next == 0)
            return 0; /* kein anderer Prozess bereit -- derselbe laeuft einfach weiter,
                        * eigene Zeitscheibe oben schon neu aufgeladen */

        if (current != 0)
            Q9K_SchedInsert(current); /* zurueck in die Ready-Queue, Age=Prioritaet */

        Q9K_SetU32(Q9_D_PROC, next);
        return next;
    }
}

/* Waehlt EINMALIG beim Boot den allerersten auszufuehrenden Prozess --
 * aufgerufen aus Q9K_CInit (q9kernel_cinit.c), C-seitig, BEVOR
 * ueberhaupt ein Prozess laeuft (Q9K_SchedRun, q9kernel_entry.a,
 * springt erst DANACH tatsaechlich hinein). Anders als
 * Q9K_SchedReschedule: es gibt noch keinen "aktuellen" Prozess, der bei
 * der Auswahl zurueck in die Queue muesste -- deshalb keine blosse
 * Wiederverwendung von Q9K_SchedReschedule, sondern ein eigener,
 * einfacherer Einstiegspunkt. Rueckgabe 0 = keine Prozesse angelegt
 * (Q9K_ProcCreate nie erfolgreich aufgerufen, oder Pool/Arena von
 * Anfang an erschoepft) -- der Aufrufer faellt dann wie gehabt bis
 * Q9K_HaltLoop durch, kein Fake-Fortschritt.
 *
 * ECHTER BUG GEFUNDEN + GEFIXT (2026-08-21, per Boot-Test: der erste
 * Timer-Tick loeste SOFORT einen Prozesswechsel aus, statt die volle
 * Zeitscheibe abzuwarten): Q9K_SCHED_SLICE_ADDR wird NIRGENDS vor dem
 * allerersten Tick initialisiert -- Q9K_ProcCreate/Q9K_SchedInsert
 * fassen sie nicht an (nur Q9K_SchedReschedule tut das, und zwar immer
 * erst NACHDEM sie bereits gelesen wurde). Die Boot-Zeit-Nullung
 * (Checkpoint 2) laesst sie deshalb bei 0 stehen -- Q9K_SchedReschedule
 * liest beim ersten Tick "slice=0", haelt das faelschlich fuer eine
 * VERBRAUCHTE Zeitscheibe und wechselt sofort. Fix: hier, beim
 * einmaligen Auswaehlen des allerersten Prozesses, die Zeitscheibe
 * gleich mit auf Q9K_SCHED_TSLICE aufladen. */
Q9_u32 Q9K_SchedFirstPick(void)
{
    Q9_u32 first = Q9K_SchedPickHighestAge();

    if (first != 0) {
        Q9K_SetU32(Q9_D_PROC, first);
        Q9K_SetU16(Q9K_SCHED_SLICE_ADDR, Q9K_SCHED_TSLICE);
    }

    return first;
}
