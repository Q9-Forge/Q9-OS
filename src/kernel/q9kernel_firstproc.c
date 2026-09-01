/*
 * q9kernel_firstproc.c -- Q9-OS eigener Kernel: Prozess-Erzeugung
 *                         (urspruenglich Abschnitt 2, Punkt 7 --
 *                         "minimales Geruest fuer den ersten
 *                         Ausfuehrungskontext" --, seit Abschnitt
 *                         "Scheduler" 2026-08-21 auf ECHTE
 *                         Mehrfach-Prozess-Erzeugung erweitert).
 *
 * URSPRUENGLICHER Umfang (2026-08-18, mit Andreas abgestimmt): nur den
 * MECHANISMUS zeigen (Deskriptor aus dem Punkt-5/6-Pool holen, Kontext-
 * wechsel durchfuehren) -- OHNE echten Scheduler, nur EIN Kontext,
 * Sprung per rohem "jmp" (Q9K_JumpToFirstProc) auf einen reinen
 * Platzhalter. NACHTRAG 2026-08-21: dieser Sonderfall ist entfallen.
 * Q9K_ProcCreate(entryPC, priority) ersetzt das alte Q9K_StartFirstProcess
 * (parameterlos, fest auf Q9K_FirstProcPlaceholder) -- baut fuer JEDEN
 * neu erzeugten Prozess von Anfang an den vollstaendigen "Fake"-Rahmen
 * auf dessen eigenem Stack auf (60 Byte genullter Registersatz + 8 Byte
 * Hardware-Exception-Frame), mit dem sowohl der allererste Prozessstart
 * (Q9K_SchedRun) als auch jeder spaetere, echte Timer-Interrupt-
 * getriebene Kontextwechsel (Q9K_TimerIRQHandler) denselben "movem.l
 * (sp)+,d0-d7/a0-a6 / rte"-Wiederaufnahme-Pfad benutzen kann -- kein
 * Sonderfall fuer den ersten Prozess mehr. Die Einreihung in die
 * Ready-Queue erfolgt jetzt ueber Q9K_SchedInsert (q9kernel_sched.c,
 * setzt zusaetzlich Age=Prioritaet, reale OS-9-Konvention) statt der
 * fruehen, lokalen Q9K_ReadyQueueAppend.
 *
 * Prozess-Deskriptor-Layout (eigene Festlegung, KEIN Kompat-Erfordernis,
 * gleiche Begruendung wie schon bei den Punkt-5/6-Pools in
 * q9kernel_tables.c -- Deskriptor-Innenleben ist reine Kernel-
 * Implementierung, kein Modul sieht das je):
 *   +0x00  State    (1 Byte, ASCII -- 'a' = aktiv, s. Thema 01/
 *          Q9_scheduler_183a-Fund fuer die reale Konvention; NACHTRAG
 *          2026-08-22, Abschnitt "F$Exit/F$Wait": zusaetzlich 'z' =
 *          Zombie (beendet, noch nicht von F$Wait abgeholt) und 'w' =
 *          wartend (in F$Wait blockiert, s. q9kernel_procend.c))
 *   +0x01  Priority (1 Byte, eigene Ergaenzung, s. q9kernel_sched.c)
 *   +0x02  Age      (2 Byte, eigene Ergaenzung, s. q9kernel_sched.c)
 *   +0x04  ParentDesc  (4 Byte, NACHTRAG 2026-08-22) -- Deskriptoradresse
 *          des erzeugenden Prozesses, 0 = kein Elternprozess (per
 *          Q9K_ProcCreate direkt erzeugt, z.B. TestProcA/B). Gebraucht
 *          von F$Exit (Elternprozess reaktivieren, falls der auf genau
 *          dieses Kind wartet) und F$Wait (Pool-Scan: "hat DIESER
 *          Aufrufer irgendein Kind?").
 *   +0x08  ModuleHdr   (4 Byte, NACHTRAG 2026-08-22) -- Modulkopf-Adresse
 *          (hdrAddr aus Q9K_ProcFork), 0 falls kein echtes Modul (per
 *          Q9K_ProcCreate erzeugt). F$Exit braucht das, um den Link-
 *          Zaehler per Q9K_ModDirUnlinkByHeader zurueckzunehmen.
 *   +0x0C  ExitStatus  (2 Byte, NACHTRAG 2026-08-22) -- vom Kind per
 *          F$Exit gesetzter Statuscode, von F$Wait an den Elternprozess
 *          zurueckgegeben. Nur gueltig, wenn State=='z'.
 *   +0x0E  SleepTicks  (4 Byte, NACHTRAG 2026-08-30, Abschnitt
 *          "F$Sleep") -- Countdown in Ticks bis zum Aufwachen (echte
 *          F$Sleep-Konvention: "inserted into active queue after (n-1)
 *          ticks", s. q9kernel_procsleep.c). Sentinel 0xFFFFFFFF =
 *          Sleep(0) = unendlich (kann in diesem Kernel nur per
 *          F$Send/Signal geweckt werden -- existiert noch nicht,
 *          bewusste Grenze). Nur gueltig, wenn State=='s'.
 *   +0x30  Next     (4 Byte) -- Ready-Queue-Link
 *   +0x34  Prev     (4 Byte) -- Ready-Queue-Link
 *   +0x38  SavedSP  (4 Byte, eigene Ergaenzung) -- zeigt auf den
 *          Fake-Rahmen/echten geretteten Registersatz auf dem
 *          EIGENEN Stack des Prozesses, s. Kopfkommentar oben
 *   +0x3C  EntryPC  (4 Byte, eigene Ergaenzung, nur Buchfuehrung/
 *          Diagnose -- der Fake-Rahmen selbst traegt die fuer den
 *          Start tatsaechlich massgebliche Kopie)
 * Next/Prev bewusst auf +0x30/+0x34 gelegt -- KEIN Zufall, sondern
 * dieselben Offsets, mit denen Q9_D_ACTIVQ (die Ready-Queue selbst) in
 * q9kernel_cinit.c schon als leerer, selbstreferenzierender Sentinel-
 * Knoten initialisiert wird (Q9K_InitEmptyQueue(Q9_D_ACTIVQ,0x30,0x34))
 * -- am echten Kernel verifiziert, dass der Sentinel selbst "deskriptor-
 * foermig" behandelt wird (Thema: Q9_scheduler_183a). Rest des 128-Byte-
 * Pool-Slots (Q9K_PROCDESC_SIZE, q9kernel_tables.c) bleibt reserviert/
 * TODO fuer alles, was ein echter Scheduler noch braucht (Prozess-ID,
 * Pfad-Tabelle, ...).
 *
 * Slot-Offset 0 kollidiert absichtlich mit der Punkt-5/6-Freiliste
 * (die dort den "naechster freier Slot"-Zeiger an Offset 0 ablegt) --
 * unproblematisch, weil "frei" und "als Deskriptor belegt" sich
 * gegenseitig ausschliessen (klassisches Freispeicher-durch-ungenutzten-
 * Speicher-Muster, gleiches Prinzip wie im Arena-Allokator).
 *
 * Fake-Rahmen-Layout ab SavedSP (68 Byte gesamt, s. Q9K_ProcCreate
 * unten) -- muss EXAKT zu dem passen, was "movem.l (sp)+,d0-d7/a0-a6 /
 * rte" erwartet (68030-Kurzformat-Exception-Frame, Format $0, 8 Byte):
 *   +0x00..0x3B  D0-D7/A0-A6 (15 x 4 Byte) -- Speicherreihenfolge bei
 *                "movem.l ...,-(sp)"/"movem.l (sp)+,..." mit DEMSELBEN
 *                Registersatz ist symmetrisch: D0 an der niedrigsten,
 *                A6 an der hoechsten Adresse dieses Bereichs (+0x38).
 *                ALLE bis auf A6 werden genullt -- A6 (+0x38) ist in
 *                diesem Kernel KEIN echtes Prozessregister, sondern
 *                permanent auf Q9K_CRuntimeData fixiert (ECHTER BUG
 *                GEFUNDEN+GEFIXT, s. ausfuehrlichen Kommentar bei
 *                Q9K_ProcCreate unten) und bekommt deshalb den echten,
 *                zur Laufzeit ausgelesenen Wert (Q9K_GetA6).
 *   +0x3C        SR   (2 Byte) -- $2000 (Supervisor, IPL=0: alle
 *                Interrupt-Ebenen frei, der Board-Timer kann sofort
 *                unterbrechen)
 *   +0x3E        PC   (4 Byte) -- entryPC
 *   +0x42        Format/Vektor-Wort (2 Byte) -- $0000 (Format 0, keine
 *                zusaetzlichen Frame-Daten)
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

extern Q9_u32 Q9K_AllocMem(Q9_u32 requestedSize);
extern void   Q9K_SchedInsert(Q9_u32 desc);  /* q9kernel_sched.c -- setzt Age=Prioritaet, haengt in Q9_D_ACTIVQ ein */
extern Q9_u32 Q9K_GetA6(void);  /* q9kernel_entry.a -- liefert den aktuellen (permanent auf
                                  * Q9K_CRuntimeData fixierten) a6-Wert, s. dortigen Kommentar */
extern Q9_u32 Q9K_ModDirLinkByName(Q9_u16 desiredTyLang, const char *name);   /* q9kernel_moddir.c */
extern Q9_u32 Q9K_ModDirUnlinkByHeader(Q9_u32 hdrAddr);                       /* q9kernel_moddir.c */

#ifndef Q9K_PROCPOOL_FREE_ADDR
#define Q9K_PROCPOOL_FREE_ADDR 0x120CUL   /* s. q9kernel_tables.c */
#endif

/* Real gewaehlte Feldabstaende (+0x30/+0x34/+0x38/+0x3C, s. Kopfkommentar)
 * -- per #ifndef ueberschreibbar, gleicher Grund wie schon bei
 * Q9_D_MODDIR_END in q9kernel_tables.c: nur 4 Byte auseinander, auf dem
 * 32-Bit-Ziel korrekt (Q9_u32 dort 4 Byte breit), auf einem 64-Bit-
 * Testhost wuerde ein Q9K_SetU32-Schreibzugriff Nachbarfelder
 * ueberschreiben. Fuer den echten Kernel-Build unveraendert. */
#ifndef Q9K_READYQ_NEXT_OFF
#define Q9K_READYQ_NEXT_OFF      0x30UL
#endif
#ifndef Q9K_READYQ_PREV_OFF
#define Q9K_READYQ_PREV_OFF      0x34UL
#endif
#define Q9K_PROCDESC_STATE_OFF   0x00UL
#ifndef Q9K_PROCDESC_PRIORITY_OFF
#define Q9K_PROCDESC_PRIORITY_OFF 0x01UL   /* s. q9kernel_sched.c */
#endif
#ifndef Q9K_PROCDESC_PARENT_OFF
#define Q9K_PROCDESC_PARENT_OFF  0x04UL   /* NACHTRAG 2026-08-22, s. Kopfkommentar */
#endif
#ifndef Q9K_PROCDESC_MODHDR_OFF
#define Q9K_PROCDESC_MODHDR_OFF  0x08UL   /* NACHTRAG 2026-08-22, s. Kopfkommentar */
#endif
#ifndef Q9K_PROCDESC_EXITSTATUS_OFF
#define Q9K_PROCDESC_EXITSTATUS_OFF 0x0CUL   /* NACHTRAG 2026-08-22, s. Kopfkommentar */
#endif
#ifndef Q9K_PROCDESC_SLEEPTICKS_OFF
#define Q9K_PROCDESC_SLEEPTICKS_OFF 0x0EUL   /* NACHTRAG 2026-08-30, s. Kopfkommentar */
#endif
#ifndef Q9K_PROCDESC_SAVEDSP_OFF
#define Q9K_PROCDESC_SAVEDSP_OFF 0x38UL
#endif
#ifndef Q9K_PROCDESC_ENTRYPC_OFF
#define Q9K_PROCDESC_ENTRYPC_OFF 0x3CUL
#endif
#define Q9K_PROCDESC_STATE_ACTIVE 'a'   /* s. Kopfkommentar */
#define Q9K_PROCDESC_STATE_ZOMBIE 'z'   /* NACHTRAG 2026-08-22, s. Kopfkommentar */
#define Q9K_PROCDESC_STATE_WAITING 'w'  /* NACHTRAG 2026-08-22, s. Kopfkommentar */
#define Q9K_PROCDESC_STATE_SLEEPING 's' /* NACHTRAG 2026-08-30, s. Kopfkommentar */

/* ECHTER BUG GEFUNDEN + GEFIXT (2026-08-21/22, Abschnitt "F$Fork"): bei
 * 2048 Byte hing das System nach einem erfolgreichen F$Fork zuverlaessig
 * -- kein Absturz, aber ab dann kein einziger weiterer Timer-Tick mehr
 * sichtbar (per Bisektions-Diagnose auf den Moment EXAKT am Ende von
 * Q9K_ProcFork eingegrenzt, dort wo dessen eigener, seit Funktionseintritt
 * "offener" Stack-Rahmen mit allen 13 lokalen Variablen am tiefsten ist).
 * Ursache: TRAP #0 wechselt NICHT auf einen separaten Supervisor-Stack
 * (der aufrufende Prozess ist bereits im Supervisor-Zustand, s.
 * Q9K_INITIAL_SR) -- die komplette Verschachtelung Q9K_TrapDispatch ->
 * Q9K_SysFFork (asm) -> Q9K_SysForkImpl -> Q9K_ProcFork ->
 * Q9K_ModDirLinkByName/Q9K_AllocMem/Q9K_SchedInsert/Q9K_ListAppend laeuft
 * komplett auf dem AUFRUFENDEN Prozesses EIGENEM, kleinen Stack. Kommt
 * dazu noch ein Timer-Tick (movem.l d0-d7/a0-a6 = 60 weitere Byte) GENAU
 * an diesem tiefsten Punkt, reichten 2048 Byte nicht mehr. Fix: auf 4096
 * Byte verdoppelt -- real verifiziert per Boot-Test: 499 saubere,
 * durchgehend korrekte Prozesswechsel in exakter Rotation
 * TestProcA->forkchild->TestProcB, kein Haenger mehr.
 *
 * ECHTER BUG NR. 2 DERSELBEN KLASSE, GEFUNDEN + GEFIXT (2026-08-22,
 * Abschnitt "F$Exit/F$Wait"): nach dem Hinzufuegen von ParentDesc (neue
 * lokale Variable `parentDesc` + zwei zusaetzliche Schreibzugriffe ans
 * Ende von Q9K_ProcFork, s. dort) reichten die 4096 Byte NICHT MEHR --
 * derselbe stille Haenger wie oben, per identischer Bisektions-Diagnose
 * (temporaere 'Z'/'z'-Marker direkt vor/nach dem F$Fork-TRAP in
 * Q9K_TestProcA, q9kernel_entry.a) auf denselben Ort eingegrenzt: 'Z'
 * erschien, 'z' NIE -- der Haenger liegt wieder irgendwo INNERHALB der
 * Q9K_TrapDispatch->...->Q9K_ProcFork-Verschachtelung. Bestaetigt die im
 * ersten Fund dokumentierte Einschaetzung, dass die Marge bei 4096 knapp
 * war -- schon eine einzige zusaetzliche lokale Variable in Q9K_ProcFork
 * reichte, sie wieder zu ueberschreiten. Fix: auf 8192 Byte verdoppelt --
 * real verifiziert per Boot-Test (15s, Q9-Flux-Emulator, echtes
 * OS9SYS.hda+eigener bootfile per os9-Toolshed `gen -b=`): kompletter
 * F$Fork->F$Exit->F$Wait-Ablauf lief exakt einmal sauber durch ("L U Z z
 * K F F F W" je genau einmal im Diagnose-Strom, kein 'k'/'w'/'H'/'S'),
 * danach 41101x 'A' und 41884x 'B' durchgehend fehlerfreies Round-Robin
 * ueber die volle Laufzeit -- kein Haenger, keine Korruption. */
/* NACHTRAG 2026-09-01 (IOMan-Integration): von 8192 auf 32768 erhoeht,
 * als Sicherheitsmarge fuer den seit Kurzem viel tieferen Aufrufpfad in
 * Q9K_TestProcA (jsr in IOMans echten Einsprungpunkt, s. dort -- IOMan
 * selbst ist eine reale, komplexe Fremdkomponente mit unbekanntem
 * eigenen Stack-Bedarf). Getestet: behebt NICHT den aktuell offenen
 * "kein A"-Befund (der hat eine andere, geklaerte Ursache -- s.
 * Memory-Notiz q9-os-eigener-kernel-c, IOMan leitet I$Open an einen
 * fehlenden Treiber weiter), bleibt aber als generelle Absicherung
 * bestehen (Host-Test bestaetigt funktionale Korrektheit). */
#define Q9K_PROC_STACK_SIZE 32768UL

/* Fake-Rahmen-Geometrie, s. Kopfkommentar -- muss exakt zu "movem.l
 * (sp)+,d0-d7/a0-a6 / rte" passen. */
#define Q9K_PROCDESC_REGSAVE_SIZE 60UL   /* movem.l d0-d7/a0-a6 = 15 Register x 4 Byte */
/* Lage von a6 INNERHALB des Registersatzes: movem.l d0-d7/a0-a6 legt die
 * Register (bei symmetrischem -(sp)-Push/(sp)+-Pop) in Listreihenfolge
 * ab -- D0 an der niedrigsten Adresse, a6 (letztes Element der Liste) an
 * der hoechsten, also bei +0x38 (8 D-Register x 4 + 6 vorangehende
 * A-Register x 4 = 32+24 = 56 = 0x38). */
#define Q9K_REGSAVE_A6_OFF         0x38UL
#define Q9K_EXCFRAME_SR_OFF        0x00UL
#define Q9K_EXCFRAME_PC_OFF        0x02UL
#define Q9K_EXCFRAME_FMTVEC_OFF    0x06UL
#define Q9K_EXCFRAME_SIZE          0x08UL
#define Q9K_FAKEFRAME_SIZE  (Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_SIZE)  /* 68 */
#define Q9K_INITIAL_SR 0x2000U   /* Supervisor, IPL=0 -- Board-Timer kann sofort unterbrechen */

/* Abschnitt "F$Fork" (2026-08-21) -- echte Modulkopf-Offsets (s.
 * src/q9moduleheader.h Q9_MH68K_*), lokal dupliziert, gleiche schlanke
 * Konvention wie q9kernel_moddir.c/q9kernel_modsearch.c. M$Mem/M$Stack
 * frisch [VERIFIZIERT] per echtem r68/l68-Build (s. forkchild.a,
 * q9moduleheader.h-Kommentar). */
#ifndef Q9K_MH_EXEC
#define Q9K_MH_EXEC  0x30UL
#endif
#ifndef Q9K_MH_MEM
#define Q9K_MH_MEM   0x38UL
#endif
#ifndef Q9K_MH_STACK
#define Q9K_MH_STACK 0x3CUL
#endif

/* Q9_D_PROC -- s. q9kernel_sched.c (dortselbe Definition, lokal
 * dupliziert). Nur fuer die Prioritaets-Vererbung gebraucht (s.
 * Q9K_ForkInheritPriority unten). */
#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif

/* Prozessdeskriptor-Poolgroesse -- s. q9kernel_tables.c
 * (Q9K_PROCDESC_SIZE) und Q9K_PROCPOOL_BASE_ADDR (Q9K_SetU32-Ziel dort,
 * "Q9K_SetU32(Q9K_PROCPOOL_BASE_ADDR, cursor)") -- hier lokal dupliziert,
 * gebraucht fuer die Prozess-ID-Berechnung (s. Q9K_ProcFork). */
#ifndef Q9K_PROCDESC_SIZE
#define Q9K_PROCDESC_SIZE 0x200UL  /* deckt P$Path bis 0x1A8, s. q9kernel_tables.c */
#endif
#ifndef Q9K_PROCPOOL_BASE_ADDR
#define Q9K_PROCPOOL_BASE_ADDR 0x1204UL
#endif

/* Echte Fehlercodes, MWOS/SRC/DEFS/errno.h (gleiche Primaerquelle wie
 * schon fuer E_MNF/E_MODBSY/E_BNAM bei F$Link/F$UnLink verwendet). */
#define Q9K_E_MNF     0x00DDU   /* Module Not Found */
#define Q9K_E_MEMFUL  0x00CFU   /* Process Memory Full */
#define Q9K_E_PRCFUL  0x00E5U   /* Process Table Full */

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
static Q9_u16 Q9K_GetU16(Q9_u32 addr) { return *(volatile Q9_u16 *)addr; }
static void   Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }
static Q9_u8  Q9K_GetU8(Q9_u32 addr) { return *(volatile Q9_u8 *)addr; }
static void   Q9K_SetU8(Q9_u32 addr, Q9_u8 value) { *(volatile Q9_u8 *)addr = value; }

/* Grossgeschriebenes Big-Endian-Lesen aus FREMDEN Moduldaten (Modulkopf-
 * Felder) -- byteweise statt Pointer-Cast, gleiche Begruendung/Konvention
 * wie q9kernel_moddir.c's Q9K_ReadU32BE (Host-Endianness/Alignment-
 * Unabhaengigkeit). */
static Q9_u32 Q9K_ReadHdrU32BE(Q9_u32 addr)
{
    const Q9_u8 *p = (const Q9_u8 *)addr;
    return ((Q9_u32)p[0] << 24) | ((Q9_u32)p[1] << 16) | ((Q9_u32)p[2] << 8) | (Q9_u32)p[3];
}

/* Schreibt value in Register regIndex des 60-Byte-Registersatz-Bereichs
 * ab frameBase (0-7 = D0-D7, 8-14 = A0-A6) -- s. Kopfkommentar oben zur
 * Speicherreihenfolge. Nur fuer Q9K_ProcFork gebraucht (Q9K_ProcCreate
 * braucht nur den A6-Sonderfall, s. dort).
 *
 * ECHTER BUG GEFUNDEN + GEFIXT (2026-08-21, per Host-Test): benutzte
 * urspruenglich Q9K_SetU32 (Q9_u32 = unsigned long) fuer den Schreib-
 * zugriff -- auf dem echten 32-Bit-Ziel exakt 4 Byte breit (jedes
 * Register belegt seinen 4-Byte-Slot exakt, keine Ueberlappung), auf
 * DIESEM 64-Bit-Testhost aber 8 Byte breit. Bei 15 Registern im
 * 4-Byte-Raster ueberschrieb dadurch JEDER Schreibzugriff auch die
 * oberen 4 Byte des NAECHSTEN Registers -- durch die anschliessenden
 * Schreibzugriffe der folgenden Register wieder verdeckt, ausser bei
 * den LETZTEN paar Registern eines Laufs (a3/hdrAddr wurde real durch
 * die spaeteren Schreibzugriffe auf a4/a5/a6 teilweise ueberschrieben,
 * per echtem Host-Test-Fehlschlag gefunden: a3 las nicht mehr fakeHdr).
 *
 * NACHTRAG: ein erster Fix nutzte "unsigned int" fuer einen expliziten
 * 4-Byte-Zugriff -- auf DIESEM Testhost korrekt, auf dem ECHTEN
 * 32-Bit-Ziel aber eine UNVERIFIZIERTE Annahme (die Microware-Toolchain
 * ist klassisches K&R-Alter, "int" koennte dort ebenso gut 16 Bit sein
 * wie auf mancher aelteren 68K-Compiler-Generation -- nirgends in
 * diesem Projekt bisher belegt). Ein still falsch-breiter Zugriff HIER
 * (Register-Rahmen fuer den echten "movem.l d0-d7/a0-a6"-Wiederaufnahme-
 * pfad) waere besonders gefaehrlich: kein Absturz, sondern nur
 * halb-muellige Registerwerte -- schwer zu findender Folgefehler.
 * Deshalb stattdessen byteweise, host- UND zielunabhaengig eindeutige
 * Zusammensetzung -- gleiche Vorsicht/Technik wie schon bei fremden
 * Modulkopf-Feldern (Q9K_ReadHdrU32BE, q9kernel_moddir.c). */
static void Q9K_SetFrameReg(Q9_u32 frameBase, Q9_u32 regIndex, Q9_u32 value)
{
    Q9_u32 addr = frameBase + regIndex * 4UL;

    Q9K_SetU8(addr + 0, (Q9_u8)(value >> 24));
    Q9K_SetU8(addr + 1, (Q9_u8)(value >> 16));
    Q9K_SetU8(addr + 2, (Q9_u8)(value >> 8));
    Q9K_SetU8(addr + 3, (Q9_u8)value);
}

/* Holt EINEN Deskriptor aus der Punkt-5/6-Freiliste -- die dortige
 * Freiliste wurde nur AUFGEBAUT (q9kernel_tables.c), Pop war nicht Teil
 * von Punkt 5/6. Rueckgabe 0 = Pool erschoepft. */
static Q9_u32 Q9K_ProcPoolAlloc(void)
{
    Q9_u32 head = Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR);

    if (head == 0)
        return 0;

    Q9K_SetU32(Q9K_PROCPOOL_FREE_ADDR, Q9K_GetU32(head));
    return head;
}

/* Erzeugt EINEN neuen Prozess: Deskriptor aus dem Pool holen, eigenen
 * Stack allozieren (Arena), darauf den Fake-Rahmen aufbauen (s.
 * Kopfkommentar), Status aktiv/Prioritaet setzen, per Q9K_SchedInsert in
 * die Ready-Queue einhaengen (setzt dabei auch Age=Prioritaet). KEIN
 * Sprung/Umschalten hier drin -- der Aufrufer (q9kernel_cinit.c)
 * entscheidet per Q9K_SchedFirstPick+Q9K_SchedRun, wann der allererste
 * Prozess tatsaechlich zu laufen beginnt; ab dann treibt ausschliesslich
 * der Timer-Interrupt (Q9K_TimerIRQHandler) weitere Wechsel.
 * Rueckgabe: Deskriptoradresse (Erfolg) oder 0 (Deskriptor-Pool oder
 * Arena erschoepft) -- kein Fake-Erfolg. */
Q9_u32 Q9K_ProcCreate(Q9_u32 entryPC, Q9_u8 priority)
{
    Q9_u32 desc = Q9K_ProcPoolAlloc();
    Q9_u32 stackBase;
    Q9_u32 frameBase;
    Q9_u32 i;

    if (desc == 0)
        return 0;

    stackBase = Q9K_AllocMem(Q9K_PROC_STACK_SIZE);
    if (stackBase == 0)
        return 0;

    /* Stack waechst abwaerts -- der Fake-Rahmen liegt deshalb GANZ OBEN,
     * direkt unterhalb des allozierten Blockendes. */
    frameBase = stackBase + Q9K_PROC_STACK_SIZE - Q9K_FAKEFRAME_SIZE;

    for (i = 0; i < Q9K_PROCDESC_REGSAVE_SIZE; i += 4)
        Q9K_SetU32(frameBase + i, 0);   /* D0-D7/A0-A6 genullt -- s. Kopfkommentar */

    /* ECHTER BUG GEFUNDEN + GEFIXT (2026-08-21, per Boot-Test: Q9K_StkHandler
     * ('S'-Diagnose) loeste aus, sobald der erste per Scheduler gestartete
     * Prozess TRAP #0 aufrief): a6 ist in DIESEM Kernel KEIN echtes
     * Prozessregister, sondern seit der C-Laufzeit-Umstellung permanent
     * auf Q9K_CRuntimeData fixiert (s. Kopfkommentar Q9K_TrapDispatch,
     * q9kernel_entry.a) -- JEDER von uns compilierte C-Code (auch die
     * Syscall-Handler, die Q9K_TrapDispatch aufruft) erwartet a6 GENAU
     * DORT fuer seinen eigenen Stack-Ueberlauf-Check. Mit a6=0 (wie es
     * die obige Nullungsschleife zunaechst setzt) laesst dieser Check
     * Datenmuell lesen/schreiben -- genau das loeste den Stack-Ueberlauf-
     * Handler aus. Fix: a6-Slot NACHTRAEGLICH mit dem echten, zur
     * Laufzeit ausgelesenen a6-Wert ueberschreiben (Q9K_GetA6, s. dort --
     * die absolute Adresse ist wegen des REENT-Ladens an variabler
     * Laufzeitadresse nicht compile-zeit-bekannt). */
    Q9K_SetU32(frameBase + Q9K_REGSAVE_A6_OFF, Q9K_GetA6());

    Q9K_SetU16(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_SR_OFF, Q9K_INITIAL_SR);
    Q9K_SetU32(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_PC_OFF, entryPC);
    Q9K_SetU16(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_FMTVEC_OFF, 0);

    Q9K_SetU8(desc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);
    Q9K_SetU8(desc + Q9K_PROCDESC_PRIORITY_OFF, priority);
    /* NACHTRAG 2026-08-22: kein Elternprozess/echtes Modul -- Q9K_ProcCreate
     * wird nur fuer die beiden fest verdrahteten Testprozesse (TestProcA/B,
     * s. q9kernel_cinit.c) direkt aus Q9K_CInit heraus aufgerufen, nicht
     * ueber F$Fork. */
    Q9K_SetU32(desc + Q9K_PROCDESC_PARENT_OFF, 0);
    Q9K_SetU32(desc + Q9K_PROCDESC_MODHDR_OFF, 0);
    Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, frameBase);
    Q9K_SetU32(desc + Q9K_PROCDESC_ENTRYPC_OFF, entryPC);

    Q9K_SchedInsert(desc);

    return desc;
}

/*******************************************************************************
 * Abschnitt "F$Fork" (2026-08-21, im Anschluss an den Scheduler) -- echte
 * F$Fork-Mechanik (68k_tech.pdf S. 430-432, real per Read gelesen, nicht
 * geraten). F$Fork erzeugt einen neuen Prozess als Kind des Aufrufers:
 * echte Modulsuche (per Manual-Text WOERTLICH dieselbe In-Memory-Suche
 * wie F$Link -- "the system module directory is searched to see if the
 * program is already in memory. If so, the module is linked and
 * executed" -- deshalb hier direkte Wiederverwendung von
 * Q9K_ModDirLinkByName statt einer zweiten Implementierung), echte
 * Speicherallokation aus den Modulkopf-Feldern M$Mem/M$Stack, echter
 * Registersatz-Aufbau nach Figure D-3/Table D-9.
 *
 * EIGENE ENTSCHEIDUNG, Manual-Text ungenau (dokumentiert, nicht
 * geraten): die Beschreibung sagt nur "OS-9 attempts to allocate RAM
 * equal to the required data storage size [M$Mem] plus any additional
 * size specified in d1, plus the size of any parameter passed" -- OHNE
 * M$Stack zu erwaehnen. Figure D-3 zeigt aber einen DRITTEN,
 * eigenstaendigen "Stack Area"-Bereich im selben zusammenhaengenden
 * Block -- ohne M$Stack einzurechnen haette ein frisch geforkter Prozess
 * ueberhaupt keinen Platz zum Laufen. Deshalb hier: totalSize = M$Mem +
 * M$Stack + d1(addMem) + d2(paramSize), mit d1 (die einzige generische
 * "additional memory"-Groesse) dem Daten-Budget zugerechnet (passt zum
 * Wortlaut "additional [data storage] size").
 *
 * EIGENE ENTSCHEIDUNG: a6 bleibt -- wie bei Q9K_ProcCreate -- NICHT auf
 * die reale Table-D-9-Bedeutung ("Datenbereichsbasis, PRO PROZESS
 * unterschiedlich") gesetzt, sondern auf die im eigenen Kernel seit der
 * C-Laufzeit-Umstellung etablierte Konvention (fester, globaler Wert,
 * s. Q9K_ProcCreate-Kommentar) -- NUR relevant, falls ein geforktes
 * Modul selbst compilierter C-Code mit _stklimit(a6)-Pruefung waere;
 * unser einziges Testmodul (forkchild.a) ist reiner Assembler-Code und
 * benutzt a6 nicht. TODO fuer den Tag, an dem echte, compilierte
 * C-Programme geforkt werden sollen.
 *
 * EIGENE ENTSCHEIDUNG: SR=$2000 (Supervisor) statt der real
 * dokumentierten "sr = n000, n=0 fuer Non-MSP-Systeme" (=User-Zustand,
 * S-Bit geloescht) -- dieser Kernel hat noch KEINE echte User-/
 * Supervisor-Trennung (s. bereits mehrfach dokumentiertes TODO bei
 * Q9K_TrapDispatch/SYSDIS) -- ein User-Prozess koennte hier noch nicht
 * einmal TRAP #0 korrekt benutzen. Bleibt Supervisor bis diese Trennung
 * existiert.
 *
 * d3.w (Anzahl geerbter I/O-Pfade) ist immer 0 -- kein Pfadsystem
 * implementiert (bewusste Grenze, s. "erst alles fertig ohne I/Os").
 *
 * Rueckgabe: Prozess-ID (Pool-Slot-Index, s. Kopfkommentar zur eigenen
 * PID-Konvention weiter unten) oder 0 bei Fehlschlag, *outError dann
 * gesetzt. */
Q9_u32 Q9K_ProcFork(Q9_u16 typeLang, Q9_u32 addMem, Q9_u32 paramSize,
                     Q9_u32 namePtr, Q9_u32 paramPtr, Q9_u16 priorityIn,
                     Q9_u16 *outError)
{
    Q9_u32 hdrAddr;
    Q9_u32 dataSize, stackSize, execOff;
    Q9_u32 totalSize;
    Q9_u32 block;
    Q9_u32 desc;
    Q9_u32 frameBase;
    Q9_u32 blockTop, spBoundary, entryPC;
    Q9_u16 priority;
    Q9_u32 i;
    Q9_u32 pid;
    Q9_u32 parentDesc;

    /* NACHTRAG 2026-08-22 (Abschnitt "F$Exit/F$Wait"): der Aufrufer wird
     * zum Elternprozess des neuen Kindes -- MUSS vor jeder eigenen
     * Deskriptor-Aenderung gelesen werden (Q9_D_PROC zeigt bis zum
     * naechsten echten Kontextwechsel weiter auf den Aufrufer selbst, s.
     * gleiche Lesestelle weiter unten fuer die Prioritaets-Vererbung). 0 =
     * kein laufender Prozess (Aufruf direkt aus Q9K_CInit, real nie der
     * Fall fuer F$Fork ueber TRAP #0, aber defensiv wie bei der
     * Prioritaets-Vererbung unten behandelt). */
    parentDesc = Q9K_GetU32(Q9_D_PROC);

    hdrAddr = Q9K_ModDirLinkByName(typeLang, (const char *)(unsigned long)namePtr);
    if (hdrAddr == 0) {
        *outError = (Q9_u16)Q9K_E_MNF;
        return 0;
    }

    dataSize  = Q9K_ReadHdrU32BE(hdrAddr + Q9K_MH_MEM);
    stackSize = Q9K_ReadHdrU32BE(hdrAddr + Q9K_MH_STACK);
    execOff   = Q9K_ReadHdrU32BE(hdrAddr + Q9K_MH_EXEC);

    totalSize = dataSize + stackSize + addMem + paramSize; /* s. Kopfkommentar */

    block = Q9K_AllocMem(totalSize);
    if (block == 0) {
        Q9K_ModDirUnlinkByHeader(hdrAddr); /* Link-Zaehler wieder zuruecknehmen -- Fork bricht ab */
        *outError = (Q9_u16)Q9K_E_MEMFUL;
        return 0;
    }

    desc = Q9K_ProcPoolAlloc();
    if (desc == 0) {
        Q9K_ModDirUnlinkByHeader(hdrAddr);
        *outError = (Q9_u16)Q9K_E_PRCFUL;
        return 0;
    }

    /* Speicherlayout, Figure D-3 (68k_tech.pdf S. 431), HOCH->NIEDRIG:
     *   [blockTop]     Parameter-Bereichsende -- (a1) zeigt hierher
     *   Parameter-Bereich (paramSize Byte)
     *   [spBoundary]   (a5), Anfangs-SP (a7) -- s. u.
     *   Stack-Bereich (stackSize Byte)
     *   Daten-Bereich (dataSize Byte)
     *   [block]        (a6), niedrigste Adresse
     * Der Fake-Rahmen selbst liegt am UNTEREN Ende des Stack-Bereichs
     * (frameBase = spBoundary - Q9K_FAKEFRAME_SIZE) -- nach dem
     * "movem.l (sp)+,d0-d7/a0-a6 / rte"-Wiederaufnahmepfad (a7 wird
     * dabei NIE explizit gesetzt, nur implizit -- s. Kopfkommentar oben
     * bei Q9K_ProcCreate) landet der ECHTE a7 danach automatisch GENAU
     * bei spBoundary -- exakt Table D-9s "(a5), (a7)"-Grenzmarkierung,
     * ohne dass a7 hier separat berechnet werden muss. */
    blockTop = block + totalSize;
    spBoundary = blockTop - paramSize;

    /* Parameter kopieren (a1-Eingabe -> Parameter-Bereich) -- byteweise,
     * kein Pointer-Cast (Q9_u32 ist auf dem Testhost breiter als auf dem
     * echten Ziel). paramPtr==0/paramSize==0 ist ein gueltiger Fall
     * (kein Parameter, Schleife laeuft dann null Mal). */
    for (i = 0; i < paramSize; i++)
        Q9K_SetU8(spBoundary + i, Q9K_GetU8(paramPtr + i));

    frameBase = spBoundary - Q9K_FAKEFRAME_SIZE;

    priority = (priorityIn != 0) ? priorityIn
             : (Q9K_GetU32(Q9_D_PROC) != 0)
                 ? (Q9_u16)Q9K_GetU8(Q9K_GetU32(Q9_D_PROC) + Q9K_PROCDESC_PRIORITY_OFF)
                 : 1U; /* kein laufender Prozess (Aufruf direkt aus Q9K_CInit) -- sinnvoller Default */

    entryPC = hdrAddr + execOff;
    /* ECHTER BUG GEFUNDEN + GEFIXT (2026-08-21, per Host-Test): eine
     * 0-basierte PID (Pool-Slot-Index direkt) macht "Slot 0 erfolgreich
     * belegt" ununterscheidbar von "Q9K_SysForkImpl deutet pid==0 als
     * Fehlschlag" -- ausgerechnet der ERSTE, meistgenutzte Poolplatz
     * waere betroffen gewesen. Fix: 1-basiert (Index+1), passt zudem
     * eher zur realen OS-9-Konvention (PID 1 = erster Prozess). */
    pid = (desc - Q9K_GetU32(Q9K_PROCPOOL_BASE_ADDR)) / Q9K_PROCDESC_SIZE + 1;

    /* Table D-9 (68k_tech.pdf S. 432) -- Register, die der neue Prozess
     * beim allerersten Start in seinem eigenen Kontext vorfindet. */
    Q9K_SetFrameReg(frameBase, 0, pid);        /* d0.w = Process ID */
    Q9K_SetFrameReg(frameBase, 1, 0);          /* d1.l = Group/user number -- kein User-System */
    Q9K_SetFrameReg(frameBase, 2, priority);   /* d2.w = Priority */
    Q9K_SetFrameReg(frameBase, 3, 0);          /* d3.w = Anzahl geerbter Pfade -- kein Pfadsystem */
    Q9K_SetFrameReg(frameBase, 4, 0);          /* d4.l = Undefined */
    Q9K_SetFrameReg(frameBase, 5, paramSize);  /* d5.l = Parameter size */
    Q9K_SetFrameReg(frameBase, 6, totalSize);  /* d6.l = Total initial memory allocation */
    Q9K_SetFrameReg(frameBase, 7, 0);          /* d7.l = Undefined */
    Q9K_SetFrameReg(frameBase, 8, 0);          /* a0 = Undefined */
    Q9K_SetFrameReg(frameBase, 9, blockTop);   /* a1 = Top of memory pointer */
    Q9K_SetFrameReg(frameBase, 10, 0);         /* a2 = Undefined */
    Q9K_SetFrameReg(frameBase, 11, hdrAddr);   /* a3 = Primary (forked) module pointer */
    Q9K_SetFrameReg(frameBase, 12, 0);         /* a4 = Undefined */
    Q9K_SetFrameReg(frameBase, 13, spBoundary);/* a5 = (a5),(a7)-Grenze */
    Q9K_SetFrameReg(frameBase, 14, block);     /* a6 = Datenbereichsbasis (eigene Kernel-Konvention, s. Kopfkommentar) */

    Q9K_SetU16(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_SR_OFF, Q9K_INITIAL_SR);
    Q9K_SetU32(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_PC_OFF, entryPC);
    Q9K_SetU16(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_FMTVEC_OFF, 0);

    Q9K_SetU8(desc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);
    Q9K_SetU8(desc + Q9K_PROCDESC_PRIORITY_OFF, (Q9_u8)priority);
    Q9K_SetU32(desc + Q9K_PROCDESC_PARENT_OFF, parentDesc);   /* NACHTRAG 2026-08-22 */
    Q9K_SetU32(desc + Q9K_PROCDESC_MODHDR_OFF, hdrAddr);      /* NACHTRAG 2026-08-22 */
    Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, frameBase);
    Q9K_SetU32(desc + Q9K_PROCDESC_ENTRYPC_OFF, entryPC);

    Q9K_SchedInsert(desc);

    return pid;
}

/* Eigene Kernel-Global-Erweiterungen fuer die ASM<->C-Uebergabe von
 * Q9K_SysFFork (q9kernel_entry.a) -- gleiches, bereits etabliertes
 * Muster wie Q9K_TrapHandlerScratch/Q9K_MODDIR_FREE_ADDR: feste,
 * duplizierte Adressen statt eines unverifizierten Mehrparameter-C-ABI
 * ueber die Assembler-Grenze (s. ausfuehrliche Begruendung im
 * Kopfkommentar von Q9K_SysForkImpl unten). Direkt hinter
 * Q9K_READYQ_SENTINEL_ADDR ($1240, s. q9kernel_sched.c) + dessen 0x38
 * Byte eigenem Next/Prev-Bereich -- naechste freie Adresse $1278. */
#ifndef Q9K_FORK_SCRATCH_TYPELANG
#define Q9K_FORK_SCRATCH_TYPELANG   0x1278UL   /* Q9_u16, d0.w EIN */
#endif
#ifndef Q9K_FORK_SCRATCH_ADDMEM
#define Q9K_FORK_SCRATCH_ADDMEM     0x127CUL   /* Q9_u32, d1.l EIN */
#endif
#ifndef Q9K_FORK_SCRATCH_PARAMSIZE
#define Q9K_FORK_SCRATCH_PARAMSIZE  0x1280UL   /* Q9_u32, d2.l EIN */
#endif
#ifndef Q9K_FORK_SCRATCH_NUMPATHS
#define Q9K_FORK_SCRATCH_NUMPATHS   0x1284UL   /* Q9_u16, d3.w EIN (bisher ungenutzt, s. o.) */
#endif
#ifndef Q9K_FORK_SCRATCH_PRIORITY
#define Q9K_FORK_SCRATCH_PRIORITY   0x1288UL   /* Q9_u16, d4.w EIN */
#endif
#ifndef Q9K_FORK_SCRATCH_NAMEPTR
#define Q9K_FORK_SCRATCH_NAMEPTR    0x128CUL   /* Q9_u32, (a0) EIN/AUS (nach dem Namen aktualisiert) */
#endif
#ifndef Q9K_FORK_SCRATCH_PARAMPTR
#define Q9K_FORK_SCRATCH_PARAMPTR   0x1290UL   /* Q9_u32, (a1) EIN */
#endif
#ifndef Q9K_FORK_SCRATCH_CHILDPID
#define Q9K_FORK_SCRATCH_CHILDPID   0x1294UL   /* Q9_u16, d0.w AUS (Erfolg) */
#endif
#ifndef Q9K_FORK_SCRATCH_ERRORCODE
#define Q9K_FORK_SCRATCH_ERRORCODE  0x1298UL   /* Q9_u16, d1.w AUS (Fehlschlag) */
#endif
#ifndef Q9K_FORK_SCRATCH_SUCCESS
#define Q9K_FORK_SCRATCH_SUCCESS    0x129CUL   /* Q9_u16, 0=Fehlschlag/1=Erfolg, fuer den Carry-Entscheid im Trampolin */
#endif

/* Q9K_SysForkImpl -- duenne, PARAMETERLOSE Bruecke zwischen dem
 * Assembler-Trampolin Q9K_SysFFork (q9kernel_entry.a, echte F$Fork-
 * Registerkonvention) und Q9K_ProcFork (echte Mehrparameter-C-Funktion,
 * s. oben). Q9K_SysFFork schreibt alle sieben Eingaberegister vorher in
 * die obigen Kernel-Global-Adressen, ruft diese Funktion parameterlos
 * per "bsr" auf (exakt dieselbe Ein-Wert-/Register-freie Konvention wie
 * Q9K_CInit/Q9K_SchedRun), diese liest sie zurueck, ruft die echte
 * C-Funktion mit einer normalen, echten Mehrparameter-C-Aufrufkonvention
 * auf (KEIN Risiko -- reiner C-zu-C-Aufruf, derselbe Compiler auf
 * beiden Seiten, anders als die Assembler<->C-Grenze, fuer die dieses
 * Umweg-Schema extra existiert), und schreibt Ergebnis/Fehlercode/Erfolg
 * wieder in die Scratch-Adressen zurueck, die das Trampolin danach in
 * die echten Ausgaberegister uebertraegt. */
void Q9K_SysForkImpl(void)
{
    Q9_u16 typeLang  = Q9K_GetU16(Q9K_FORK_SCRATCH_TYPELANG);   /* Wortbreite -- Trampolin schreibt "move.w" */
    Q9_u32 addMem    = Q9K_GetU32(Q9K_FORK_SCRATCH_ADDMEM);
    Q9_u32 paramSize = Q9K_GetU32(Q9K_FORK_SCRATCH_PARAMSIZE);
    Q9_u16 priority  = Q9K_GetU16(Q9K_FORK_SCRATCH_PRIORITY);   /* Wortbreite -- Trampolin schreibt "move.w" */
    Q9_u32 namePtr   = Q9K_GetU32(Q9K_FORK_SCRATCH_NAMEPTR);
    Q9_u32 paramPtr  = Q9K_GetU32(Q9K_FORK_SCRATCH_PARAMPTR);
    Q9_u16 error = 0;
    Q9_u32 pid;

    pid = Q9K_ProcFork(typeLang, addMem, paramSize, namePtr, paramPtr, priority, &error);

    if (pid == 0) {
        Q9K_SetU16(Q9K_FORK_SCRATCH_ERRORCODE, error);
        Q9K_SetU16(Q9K_FORK_SCRATCH_SUCCESS, 0);
        return;
    }

    Q9K_SetU16(Q9K_FORK_SCRATCH_CHILDPID, (Q9_u16)pid);
    Q9K_SetU16(Q9K_FORK_SCRATCH_SUCCESS, 1);
}
