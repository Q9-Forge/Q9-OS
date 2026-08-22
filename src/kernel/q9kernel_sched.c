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
 * KEIN SMP hier (2026-08-21, mit Andreas besprochen): echte
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

#define Q9K_PROCDESC_STATE_OFF    0x00UL
#define Q9K_PROCDESC_PRIORITY_OFF 0x01UL   /* eigene Erweiterung, 1 Byte (0-255) */
#define Q9K_PROCDESC_AGE_OFF      0x02UL   /* eigene Erweiterung, 2 Byte -- "Ages never
                                             * increment beyond $ffff" (Manual), passt exakt */
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

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
static Q9_u16 Q9K_GetU16(Q9_u32 addr) { return *(volatile Q9_u16 *)addr; }
static void   Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }
static Q9_u8  Q9K_GetU8(Q9_u32 addr) { return *(volatile Q9_u8 *)addr; }

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

    while (node != Q9K_READYQ_SENTINEL_ADDR) {
        Q9_u16 age = Q9K_GetU16(node + Q9K_PROCDESC_AGE_OFF);

        if (best == 0 || age > bestAge) {
            best = node;
            bestAge = age;
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
