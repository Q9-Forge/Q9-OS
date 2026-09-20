/*
 * q9kernel_firstproc.c -- Q9-OS eigener Kernel: Prozess-Erzeugung
 *                         (urspruenglich Abschnitt 2, Punkt 7 --
 *                         "minimales Geruest fuer den ersten
 *                         Ausfuehrungskontext" --, seit Abschnitt
 *                         "Scheduler" 2026-08-21 auf ECHTE
 *                         Mehrfach-Prozess-Erzeugung erweitert).
 *
 * URSPRUENGLICHER Umfang (2026-08-18, so abgestimmt): nur den
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
 *   +0x1B0 AllocBase   (4 Byte, NACHTRAG 2026-09-01) -- Basis des von
 *          diesem Prozess belegten Daten-/Stack-Blocks. Liegt bewusst
 *          HINTER der kompatiblen P$Path-Tabelle (bis +0x1A8), so dass
 *          sie von keiner OS-9-Pfadlogik beruehrt wird.
 *   +0x1B4 AllocSize   (4 Byte) -- exakt die zu AllocBase gehoerige
 *          Allokationsgroesse. Beide Felder machen die Freigabe beim
 *          F$Exit moeglich; 0/0 bedeutet "kein eigener Block".
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
extern void   Q9K_FreeMem(Q9_u32 addr, Q9_u32 size);
extern void   Q9K_SchedInsert(Q9_u32 desc);  /* q9kernel_sched.c -- setzt Age=Prioritaet, haengt in Q9_D_ACTIVQ ein */
extern Q9_u32 Q9K_GetA6(void);  /* q9kernel_entry.a -- liefert den aktuellen (permanent auf
                                  * Q9K_CRuntimeData fixierten) a6-Wert, s. dortigen Kommentar */
extern Q9_u32 Q9K_ModDirLinkByName(Q9_u16 desiredTyLang, const char *name);   /* q9kernel_moddir.c */
extern Q9_u32 Q9K_ModDirUnlinkByHeader(Q9_u32 hdrAddr);                       /* q9kernel_moddir.c */
extern Q9_u32 Q9K_ProcLookup(Q9_u16 pid);                                     /* q9kernel_procapi.c */
extern Q9_u16 Q9K_ProcIdForDesc(Q9_u32 desc);                                 /* q9kernel_procapi.c */
extern void   Q9K_SchedRemove(Q9_u32 desc);                                   /* q9kernel_sched.c */

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
#define Q9K_PROCDESC_STATE_OFF   0x1DUL
#ifndef Q9K_PROCDESC_PRIORITY_OFF
#define Q9K_PROCDESC_PRIORITY_OFF 0x19UL   /* s. q9kernel_sched.c */
#endif
#ifndef Q9K_PROCDESC_PARENT_OFF
#define Q9K_PROCDESC_PARENT_OFF  0x1B8UL   /* NACHTRAG 2026-08-22, s. Kopfkommentar */
#endif
#ifndef Q9K_PROCDESC_MODHDR_OFF
#define Q9K_PROCDESC_MODHDR_OFF  0x38UL   /* NACHTRAG 2026-08-22, s. Kopfkommentar */
#endif
#ifndef Q9K_PROCDESC_EXITSTATUS_OFF
#define Q9K_PROCDESC_EXITSTATUS_OFF 0x1C0UL   /* NACHTRAG 2026-08-22, s. Kopfkommentar */
#endif
#ifndef Q9K_PROCDESC_SLEEPTICKS_OFF
#define Q9K_PROCDESC_SLEEPTICKS_OFF 0x1C4UL   /* NACHTRAG 2026-08-30, s. Kopfkommentar */
#endif
#ifndef Q9K_PROCDESC_SAVEDSP_OFF
#define Q9K_PROCDESC_SAVEDSP_OFF 0x08UL
#endif
#ifndef Q9K_PROCDESC_PATH_OFF
/* P$Path -- 32 Pfadnummern a 2 Byte, s. q9kernel_tables.c Kopfkommentar
 * ("P$Path direkt danach@0x168, NumPaths(32)*2=64 Byte bis 0x1A8"). IOMans
 * I$Open sucht hier das erste freie Wort; der Index IST die Pfadnummer. */
#endif
#ifndef Q9K_PROCDESC_ID_OFF
extern Q9_u16 Q9K_ProcIdForDesc(Q9_u32 desc);  /* q9kernel_procapi.c -- Deskriptor -> PID */

/* P$ID -- Prozess-ID als WORT bei Offset $00, exakt wie im echten OS-9
 * (internem Referenzmaterial). Fremde Module lesen sie dort: die
 * sc68681-ISR holt sich von hier die ID des wartenden Lesers, um ihn per
 * F$Send zu wecken. Vor der Angleichung standen an dieser Stelle unsere
 * State- und Prioritaets-Bytes, weshalb der Treiber die ID $6105 sah
 * ('a' = STATE_ACTIVE, 5 = Prioritaet) und niemanden weckte. */
#define Q9K_PROCDESC_ID_OFF      0x00UL
/* P$User ($14, zwei Worte: Gruppe im oberen, Benutzer im unteren) --
 * Offset Feld fuer Feld aus dem realen Prozesslayout aufsummiert (org 0
 * ab P$ID: ID/PID/SID/CID je ein Wort, dann P$sp/P$usp/P$MemSiz je ein
 * Langwort). Passt damit lueckenlos zu den bereits uebernommenen
 * Nachbarfeldern P$Prior ($18) und P$State ($1c). Geschrieben wird es
 * von F$SUser (s. q9kernel_procapi.c), gelesen von F$ID. */
#ifndef Q9K_PROCDESC_USER_OFF
#define Q9K_PROCDESC_USER_OFF    0x14UL
#endif
#define Q9K_PROCDESC_PATH_OFF    0x168UL
#define Q9K_PROCDESC_PATH_COUNT  32UL
#ifndef Q9K_FORK_SCRATCH_NUMPATHS
#define Q9K_FORK_SCRATCH_NUMPATHS 0x1284UL
#endif
#endif
#ifndef Q9K_PROCDESC_ENTRYPC_OFF
#define Q9K_PROCDESC_ENTRYPC_OFF 0x1C8UL
#endif
#ifndef Q9K_PROCDESC_ALLOCBASE_OFF
#define Q9K_PROCDESC_ALLOCBASE_OFF 0x1B0UL
#endif
#ifndef Q9K_PROCDESC_ALLOCSIZE_OFF
#define Q9K_PROCDESC_ALLOCSIZE_OFF 0x1B4UL
#endif
#define Q9K_PROCDESC_STATE_ACTIVE 'a'   /* s. Kopfkommentar */
#define Q9K_PROCDESC_STATE_ZOMBIE 'z'   /* NACHTRAG 2026-08-22, s. Kopfkommentar */
#define Q9K_PROCDESC_STATE_WAITING 'w'  /* NACHTRAG 2026-08-22, s. Kopfkommentar */
#define Q9K_PROCDESC_STATE_SLEEPING 's' /* NACHTRAG 2026-08-30, s. Kopfkommentar */

/* F$DFork keeps the child allocated but outside the ready queue.  The
 * register image uses the same 68-byte layout as the child's initial fake
 * frame: D0-D7/A0-A6, SR, PC and format/vector word. */
#ifndef Q9K_DEBUGFORK_E_BPADDR
#define Q9K_DEBUGFORK_E_BPADDR 0x00D2U
#endif
#ifndef Q9K_DEBUGFORK_REGIMAGE_SIZE
#define Q9K_DEBUGFORK_REGIMAGE_SIZE 68UL
#endif

static Q9_u32 Q9K_GetU32(Q9_u32 addr);
static Q9_u8  Q9K_GetU8(Q9_u32 addr);
static void   Q9K_SetU8(Q9_u32 addr, Q9_u8 value);

Q9_u32 Q9K_ProcFork(Q9_u16 typeLang, Q9_u32 addMem, Q9_u32 paramSize,
                    Q9_u32 namePtr, Q9_u32 paramPtr, Q9_u16 priority,
                    Q9_u16 *outError);

Q9_u32 Q9K_ProcDebugFork(Q9_u16 typeLang, Q9_u32 addMem, Q9_u32 paramSize,
                         Q9_u32 namePtr, Q9_u32 paramPtr, Q9_u16 priority,
                         Q9_u32 registerBuffer, Q9_u16 *outError)
{
    Q9_u32 pid;
    Q9_u32 desc;
    Q9_u32 frame;
    Q9_u32 i;

    *outError = 0;
    if (registerBuffer == 0) {
        *outError = Q9K_DEBUGFORK_E_BPADDR;
        return 0;
    }

    pid = Q9K_ProcFork(typeLang, addMem, paramSize, namePtr, paramPtr,
                       priority, outError);
    if (pid == 0)
        return 0;

    desc = Q9K_ProcLookup((Q9_u16)pid);
    if (desc == 0) {
        *outError = Q9K_DEBUGFORK_E_BPADDR;
        return 0;
    }
    frame = Q9K_GetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF);
    if (frame == 0) {
        *outError = Q9K_DEBUGFORK_E_BPADDR;
        return 0;
    }

    /* Publish the child's initial register image before making it
     * inaccessible to the scheduler.  Byte copies keep this bridge correct
     * on the host test build as well as on the 68000 target. */
    for (i = 0; i < Q9K_DEBUGFORK_REGIMAGE_SIZE; i++)
        Q9K_SetU8(registerBuffer + i, Q9K_GetU8(frame + i));

    Q9K_SchedRemove(desc);
    Q9K_SetU8(desc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_WAITING);
    return pid;
}

/* F$AllPrc-/F$DelPrc-Scratch (2026-09-18), hinter dem F$GPrDBT-Block
 * ($194C-$1950, q9kernel_procapi.c). */
#ifndef Q9K_ALLPRC_SCRATCH_DESC
#define Q9K_ALLPRC_SCRATCH_DESC    0x1954UL /* Q9_u32, (a2) AUS            */
#define Q9K_ALLPRC_SCRATCH_ERROR   0x1958UL /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_ALLPRC_SCRATCH_SUCCESS 0x195CUL /* Q9_u32, 0/1                 */
#endif
#ifndef Q9K_DELPRC_SCRATCH_PID
#define Q9K_DELPRC_SCRATCH_PID     0x1960UL /* Q9_u32, d0.w EIN            */
#define Q9K_DELPRC_SCRATCH_ERROR   0x1964UL /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_DELPRC_SCRATCH_SUCCESS 0x1968UL /* Q9_u32, 0/1                 */
#endif

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
/* NACHTRAG (2026-09-11, Fortsetzung 49) -- M$IData/M$IRefs, 68k_tech.pdf
 * Table 1-8. Beide 0 = Modul hat keine initialisierten globalen/
 * statischen Daten (z.B. hellosvc/forkchild) -- der bisherige Code
 * bleibt fuer diese Module unveraendert (Datenbereich bleibt wie immer
 * ungenullt/Speichermuell, s. Q9K_AllocMem-Kopfkommentar). ECHTE C-
 * Programme wie "echo" (uebersetzt gegen csl) haben beide gesetzt --
 * s. ausfuehrliche Byte-Ebenen-Verifikation gegen echo.mod in
 * docs/OWN_KERNEL_STATUS.md, Fortsetzung 49. */
#ifndef Q9K_MH_IDATA
#define Q9K_MH_IDATA 0x40UL
#endif
#ifndef Q9K_MH_IREFS
#define Q9K_MH_IREFS 0x44UL
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
#define Q9K_PROCDESC_SIZE 0x400UL  /* = P$PrcBody (echte Groesse), s. q9kernel_tables.c */
#endif
#ifndef Q9K_PROCPOOL_BASE_ADDR
#define Q9K_PROCPOOL_BASE_ADDR 0x1204UL
#endif

/* Echte Fehlercodes, internem Referenzmaterial (gleiche Primaerquelle wie
 * schon fuer E_MNF/E_MODBSY/E_BNAM bei F$Link/F$UnLink verwendet). */
#define Q9K_E_MNF     0x00DDU   /* Module Not Found */
#define Q9K_E_MEMFUL  0x00CFU   /* Process Memory Full */
#define Q9K_E_PRCFUL  0x00E5U   /* Process Table Full */
#define Q9K_E_PRCID   0x00E0U   /* Invalid Process ID (E$IPrcID), s. q9kernel_procapi.c */

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
/* Nicht mehr static: q9kernel_chain.c baut denselben Prozessrahmen auf
 * und braucht dieselben drei Bausteine (s. dortigen Kopfkommentar). */
Q9_u32 Q9K_ReadHdrU32BE(Q9_u32 addr)
{
    const Q9_u8 *p = (const Q9_u8 *)addr;
    return ((Q9_u32)p[0] << 24) | ((Q9_u32)p[1] << 16) | ((Q9_u32)p[2] << 8) | (Q9_u32)p[3];
}

/* Wie Q9K_ReadHdrU32BE, nur 16 Bit -- gebraucht fuer M$IRefs (Table 1-8:
 * MS-Wort/Anzahl-Wort/Adressversatz-Woerter, s. Q9K_ApplyInitializedData
 * unten). */
static Q9_u16 Q9K_ReadHdrU16BE(Q9_u32 addr)
{
    const Q9_u8 *p = (const Q9_u8 *)addr;
    return (Q9_u16)(((Q9_u32)p[0] << 8) | (Q9_u32)p[1]);
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
void Q9K_SetFrameReg(Q9_u32 frameBase, Q9_u32 regIndex, Q9_u32 value)   /* s. Hinweis bei Q9K_ReadHdrU32BE */
{
    Q9_u32 addr = frameBase + regIndex * 4UL;

    Q9K_SetU8(addr + 0, (Q9_u8)(value >> 24));
    Q9K_SetU8(addr + 1, (Q9_u8)(value >> 16));
    Q9K_SetU8(addr + 2, (Q9_u8)(value >> 8));
    Q9K_SetU8(addr + 3, (Q9_u8)value);
}

/* Gegenstueck zu Q9K_SetFrameReg -- byteweise aus demselben Grund: der
 * Rahmen liegt im Prozessspeicher mit 4-Byte-Registern, waehrend Q9_u32
 * auf einem Testhost breiter ist. Gebraucht seit q9kernel_icpt.c den
 * Registersatz eines unterbrochenen Prozesses in den Rahmen der
 * Intercept-Routine uebernimmt. */
Q9_u32 Q9K_GetFrameReg(Q9_u32 frameBase, Q9_u32 regIndex)
{
    Q9_u32 addr = frameBase + regIndex * 4UL;

    return ((Q9_u32)Q9K_GetU8(addr + 0) << 24)
         | ((Q9_u32)Q9K_GetU8(addr + 1) << 16)
         | ((Q9_u32)Q9K_GetU8(addr + 2) << 8)
         | (Q9_u32)Q9K_GetU8(addr + 3);
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

/* Gegenstueck zum Pop oben fuer einen abgebrochenen Erzeugungsvorgang.
 * Der Slot war noch nie sichtbar/aktiv und braucht daher keine weitere
 * Bereinigung; das Zurueckhaengen muss aber erfolgen, damit ein
 * fehlgeschlagenes Q9K_ProcCreate keinen Pool-Slot verliert. */
static void Q9K_ProcPoolAbortAlloc(Q9_u32 desc)
{
    Q9K_SetU32(desc, Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR));
    Q9K_SetU32(Q9K_PROCPOOL_FREE_ADDR, desc);
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
    if (stackBase == 0) {
        Q9K_ProcPoolAbortAlloc(desc);
        return 0;
    }

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
    /* P$ID mitfuehren -- fremde Module lesen die Prozess-ID an Offset $00
     * (s. Kommentar bei Q9K_PROCDESC_ID_OFF). Die Nummer selbst ergibt sich
     * wie bisher aus der Lage des Deskriptors in der Tabelle. */
    Q9K_SetU16(desc + Q9K_PROCDESC_ID_OFF, Q9K_ProcIdForDesc(desc));
    Q9K_SetU8(desc + Q9K_PROCDESC_PRIORITY_OFF, priority);
    /* NACHTRAG 2026-08-22: kein Elternprozess/echtes Modul -- Q9K_ProcCreate
     * wird nur fuer die beiden fest verdrahteten Testprozesse (TestProcA/B,
     * s. q9kernel_cinit.c) direkt aus Q9K_CInit heraus aufgerufen, nicht
     * ueber F$Fork. */
    Q9K_SetU32(desc + Q9K_PROCDESC_PARENT_OFF, 0);
    /* Gruppe/Benutzer 0.0 = Superuser. Pool-Slots werden beim Anlegen
     * NICHT genullt (Q9K_ProcPoolAlloc reicht den Slot unveraendert
     * durch), ein wiederverwendeter Slot traegt sonst die ID seines
     * Vorgaengers. */
    Q9K_SetU32(desc + Q9K_PROCDESC_USER_OFF, 0);
    Q9K_SetU32(desc + Q9K_PROCDESC_MODHDR_OFF, 0);
    Q9K_SetU32(desc + Q9K_PROCDESC_ALLOCBASE_OFF, stackBase);
    Q9K_SetU32(desc + Q9K_PROCDESC_ALLOCSIZE_OFF, Q9K_PROC_STACK_SIZE);
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

/* NACHTRAG (2026-09-11, Fortsetzung 49) -- M$IData/M$IRefs (68k_tech.pdf
 * Table 1-8). Bisher wurde der neue Datenbereich eines geforkten
 * Prozesses NIE initialisiert (Q9K_AllocMem liefert unveraendertes
 * Speichermuell aus einer frueheren Belegung, s. Kopfkommentar dort) --
 * fuer reinen Assembler-Code (forkchild.a, kein M$IData) unschaedlich,
 * fuer ECHTE compilierte C-Programme (z.B. "echo", uebersetzt gegen
 * csl) fatal: die C-Laufzeit erwartet dort ihre initialisierten
 * globalen/statischen Variablen -- inklusive einer kleinen, vom Compiler
 * erzeugten Sprungtabelle mit Zeigern auf Code UND auf weitere Daten.
 *
 * FORMAT, per Byte-Dump von echo.mod GEGEN das Manual verifiziert (s.
 * docs/OWN_KERNEL_STATUS.md, Fortsetzung 49 -- NICHT geraten):
 *
 *   M$IData (Modulkopf-Offset $40, 0 = keine Tabelle vorhanden):
 *     Folge von Eintraegen bis zum Erreichen von M$IRefs (KEIN eigener
 *     Endemarker -- die beiden Tabellen liegen im Modul unmittelbar
 *     hintereinander, ihr gemeinsamer Uebergang IST die Terminierung):
 *       +0  Datenbereich-Versatz (4 Byte)  -- Ziel: block + Versatz
 *       +4  Anzahl Bytes N (4 Byte)
 *       +8  N Bytes, woertlich an obiges Ziel zu kopieren
 *     (echo.mod hat genau EINEN solchen Eintrag: Versatz $734, N=$38 --
 *     dessen Ende [N-Byte-Nutzlast] trifft exakt auf M$IRefs' Start.)
 *
 *   M$IRefs (Modulkopf-Offset $44, 0 = keine Tabelle vorhanden):
 *     GENAU ZWEI Gruppen hintereinander (Manual: Code- und Datenzeiger
 *     werden unterschiedlich behandelt -- zwei Gruppen sind die
 *     natuerliche Kodierung dafuer), jede Gruppe:
 *       +0  MS-Wort (2 Byte, bei allen bisher gesehenen Werten 0 --
 *           Bedeutung sonst unbekannt, wird gelesen und verworfen)
 *       +2  Anzahl Eintraege M (2 Byte)
 *       +4  M Woerter (je 2 Byte): Datenbereich-Versatz eines bereits
 *           per M$IData kopierten 32-Bit-Zeigerfeldes
 *       danach ein Terminierungspaar MS=0/Anzahl=0 (2+2 Byte)
 *     Erste Gruppe = KODEZEIGER (an jedem genannten Versatz steht ein
 *     modulrelativer Kodeversatz -- Zielwert = alter Wert + hdrAddr).
 *     Zweite Gruppe = DATENZEIGER (an jedem genannten Versatz steht ein
 *     datenbereichsrelativer Versatz -- Zielwert = alter Wert + block).
 *     (echo.mod: Gruppe 1 hat 8 Eintraege, alle im Bereich $1f0-$30a --
 *     zu klein/passend fuer Kodeversaetze in einem <3,2-KB-Modul, zu
 *     klein fuer M$Mem=$76c waeren sie zwar auch, aber Gruppe 2 belegt
 *     genau die ERSTEN beiden 32-Bit-Felder der kopierten Nutzlast
 *     [$734,$738] mit Werten $44/$6c4 -- beides plausible Datenversaetze,
 *     WEIT unter M$Mem=$76c, waehrend $2a8-$30a als Datenversaetze zwar
 *     auch passen wuerden, aber als Kodeversaetze eindeutiger sind, da
 *     sie in einem <3,2-KB-Modul liegen UND aus einem <2-KB-Kopfbereich
 *     [M$Exec=$4e] heraus sinnvolle Sprungziele waeren.)
 *
 * Trailing Bytes nach der zweiten Terminierung (bei echo.mod 4 Byte)
 * werden NICHT gelesen -- vermutlich die reale OS-9-Modul-CRC (letzte 3
 * Byte des Moduls) plus ein Fuellbyte; da die Anzahl der Gruppen fest
 * (zwei) ist, muss danach ohnehin nichts mehr gelesen werden. */
void Q9K_ApplyInitializedData(Q9_u32 hdrAddr, Q9_u32 block)             /* s. Hinweis bei Q9K_ReadHdrU32BE */
{
    Q9_u32 idataOff = Q9K_ReadHdrU32BE(hdrAddr + Q9K_MH_IDATA);
    Q9_u32 irefsOff = Q9K_ReadHdrU32BE(hdrAddr + Q9K_MH_IREFS);
    Q9_u32 p, end;
    Q9_u32 group;
    Q9_u32 relocBase;

    if (idataOff == 0 && irefsOff == 0)
        return;   /* Normalfall fuer reinen Assembler-Code -- unveraendert */

    /* M$IData kopieren -- laeuft bis zum Beginn von M$IRefs (s.
     * Kopfkommentar: kein eigener Endemarker noetig/vorhanden). */
    if (idataOff != 0) {
        p = hdrAddr + idataOff;
        end = hdrAddr + irefsOff;
        while (p < end) {
            Q9_u32 dstOff = Q9K_ReadHdrU32BE(p);
            Q9_u32 count  = Q9K_ReadHdrU32BE(p + 4);
            Q9_u32 i;
            for (i = 0; i < count; i++)
                Q9K_SetU8(block + dstOff + i, Q9K_GetU8(p + 8 + i));
            p += 8 + count;
        }
    }

    /* M$IRefs anwenden -- genau zwei Gruppen (Kodezeiger, dann
     * Datenzeiger), s. Kopfkommentar. */
    if (irefsOff != 0) {
        p = hdrAddr + irefsOff;
        for (group = 0; group < 2; group++) {
            Q9_u32 count;
            Q9_u32 j;

            (void)Q9K_ReadHdrU16BE(p);        /* MS-Wort -- bisher immer 0, verworfen */
            count = Q9K_ReadHdrU16BE(p + 2);
            p += 4;
            relocBase = (group == 0) ? hdrAddr : block;
            for (j = 0; j < count; j++) {
                Q9_u32 fieldOff = Q9K_ReadHdrU16BE(p);
                Q9_u32 fieldAddr = block + fieldOff;
                Q9_u32 newVal;
                p += 2;
                /* ABSICHTLICH byteweise wie Q9K_SetFrameReg/Q9K_ReadHdrU32BE
                 * -- NICHT Q9K_GetU32/Q9K_SetU32 (native Zeiger-Breite, auf
                 * DIESEM 64-Bit-Testhost 8 statt 4 Byte, s. dortigen
                 * Kopfkommentar). Bei eng benachbarten M$IRefs-Versaetzen
                 * (hier real nur 4 Byte auseinander) wuerde das sonst
                 * Nachbarfelder ueberschreiben -- exakt der schon zweimal
                 * dokumentierte Fund (Q9K_SetFrameReg, test_q9kernel_*.c). */
                newVal = Q9K_ReadHdrU32BE(fieldAddr) + relocBase;
                Q9K_SetU8(fieldAddr + 0, (Q9_u8)(newVal >> 24));
                Q9K_SetU8(fieldAddr + 1, (Q9_u8)(newVal >> 16));
                Q9K_SetU8(fieldAddr + 2, (Q9_u8)(newVal >> 8));
                Q9K_SetU8(fieldAddr + 3, (Q9_u8)newVal);
            }
            p += 4;   /* Terminierungspaar MS=0/Anzahl=0 der Gruppe ueberspringen */
        }
    }
}

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

    /* NACHTRAG (2026-09-11, Fortsetzung 51): der Fortsetzung-48-Adress-
     * Override (Q9K_FORK_BLOCK_OVERRIDE) ist entfallen -- er loeste ein
     * Problem, das durch den fehlenden $8000-Bias auf a6 (s. u.) nur
     * VORGETAEUSCHT wurde. Normale, unveraenderte Q9K_AllocMem-Allokation
     * genuegt jetzt wieder fuer JEDEN Aufrufer. */
    block = Q9K_AllocMem(totalSize);
    if (block == 0) {
        Q9K_ModDirUnlinkByHeader(hdrAddr); /* Link-Zaehler wieder zuruecknehmen -- Fork bricht ab */
        *outError = (Q9_u16)Q9K_E_MEMFUL;
        return 0;
    }

    /* NACHTRAG 2026-09-11 (Fortsetzung 49): M$IData/M$IRefs -- s.
     * ausfuehrlichen Kopfkommentar bei Q9K_ApplyInitializedData oben.
     * Muss VOR jeder weiteren Verwendung von "block" als Datenbereich
     * laufen (hier: unmittelbar nachdem block feststeht, vor Deskriptor-
     * Aufbau/Parameterkopie -- Reihenfolge zu diesen beiden ist egal, da
     * unabhaengige Speicherbereiche/Felder betroffen sind). Fuer Module
     * ohne M$IData/M$IRefs (hellosvc/forkchild, beide Felder 0) exakt
     * kein Verhaltensunterschied (frueher Ruecksprung in der Funktion). */
    Q9K_ApplyInitializedData(hdrAddr, block);

    desc = Q9K_ProcPoolAlloc();
    if (desc == 0) {
        Q9K_FreeMem(block, totalSize);
        Q9K_ModDirUnlinkByHeader(hdrAddr);
        *outError = (Q9_u16)Q9K_E_PRCFUL;
        return 0;
    }

    /* Speicherlayout, Figure D-3 (68k_tech.pdf S. 431), HOCH->NIEDRIG:
     *   [blockTop]     Parameter-Bereichsende -- (a1) und (a5) zeigen hierher
     *   Parameter-Bereich (paramSize Byte)
     *   [spBoundary]   Anfangs-SP (a7) -- s. u.
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
    /* Preserve the F$Fork path-count contract.  The path table itself is
     * copied below; external programs also receive the requested count in
     * d3.w and may use it when initializing their standard paths. */
    Q9K_SetFrameReg(frameBase, 3,
                    Q9K_GetU16(Q9K_FORK_SCRATCH_NUMPATHS));
    Q9K_SetFrameReg(frameBase, 4, 0);          /* d4.l = Undefined */
    Q9K_SetFrameReg(frameBase, 5, paramSize);  /* d5.l = Parameter size */
    Q9K_SetFrameReg(frameBase, 6, totalSize);  /* d6.l = Total initial memory allocation */
    Q9K_SetFrameReg(frameBase, 7, 0);          /* d7.l = Undefined */
    Q9K_SetFrameReg(frameBase, 8, 0);          /* a0 = Undefined */
    Q9K_SetFrameReg(frameBase, 9, blockTop);   /* a1 = Top of memory pointer */
    Q9K_SetFrameReg(frameBase, 10, 0);         /* a2 = Undefined */
    Q9K_SetFrameReg(frameBase, 11, hdrAddr);   /* a3 = Primary (forked) module pointer */
    Q9K_SetFrameReg(frameBase, 12, 0);         /* a4 = Undefined */
    /* OS-9 passes the parameter area through A5 as a pointer to its upper
     * boundary.  The shell walks back from that boundary using D5, while
     * the initial A7 still resumes at spBoundary below the parameters. */
    Q9K_SetFrameReg(frameBase, 13, blockTop);   /* a5 = parameter boundary */
    /* ECHTER BUG GEFUNDEN + GEFIXT (2026-09-11, Fortsetzung 51) -- Manual
     * WOERTLICH (68k_tech.pdf, Table 2-6 UND Table D-7, beide Stellen
     * gegengeprueft): "(a6) is always biased by $8000 ... the OS-9 linker
     * automatically adjusts for it" / "(a6) is actually biased by $8000
     * ... the linker biases all data references by -$8000". Das heisst:
     * ECHTE, vom Microware-Linker gebaute Programme (wie "echo") haben
     * JEDEN negativen a6-relativen Zugriff bereits so einkompiliert, dass
     * er einen a6-Wert um +$8000 UEBER der rohen Datenbereichsbasis
     * erwartet -- NICHT die rohe Basis selbst. Bisher wurde hier direkt
     * "block" (unverschoben) uebergeben, was fuer reinen, selbst
     * geschriebenen Assembler-Code (forkchild.a/hellosvc.a, kein
     * Linker-Bias einkompiliert) folgenlos blieb, aber JEDEN negativen
     * a6-Zugriff eines ECHTEN, linker-gebauten C-Programms um exakt
     * $8000 daneben treffen liess.
     *
     * Das war die WAHRE Ursache der gesamten "echo springt/liest/schreibt
     * an einer falschen Adresse"-Serie (Fortsetzung 44 [Sprungziel um
     * ~68 Byte daneben], 48 [Zeiger liest Speichermuell], 49/50 [Schreib-
     * zugriff zerstoert csl] -- alle drei sind NUR unterschiedliche
     * Symptome DESSELBEN fehlenden $8000-Bias, s. docs/OWN_KERNEL_STATUS.md
     * Fortsetzung 51 fuer die vollstaendige Herleitung ueber die
     * M$IData/M$IRefs-Bytewerte). Die kunstvolle Kombi-Allokation aus
     * Fortsetzung 48 (Q9K_ExperimentalCombinedAlloc) loeste ein Problem,
     * das es bei korrektem Bias nie gegeben haette -- deshalb in
     * derselben Fortsetzung vollstaendig zurueckgebaut (s.
     * q9kernel_traplink.c/q9kernel_entry.a).
     *
     * NACHTRAG: M$IData/M$IRefs (Q9K_ApplyInitializedData oben) bleiben
     * UNVERAENDERT -- deren Tabellenoffsets sind, wie im selben Zug
     * verifiziert, IMMER relativ zur ROHEN (unverschobenen) Datenbereichs-
     * basis "block" zu verstehen (M$Mem passt nur zur rohen Basis, s.
     * Fortsetzung 51), nicht zum gebiasten a6. */
    Q9K_SetFrameReg(frameBase, 14, block + 0x8000UL); /* a6 = Datenbereichsbasis + $8000-Bias (Table 2-6/D-7) */

    Q9K_SetU16(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_SR_OFF, Q9K_INITIAL_SR);
    Q9K_SetU32(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_PC_OFF, entryPC);
    Q9K_SetU16(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_FMTVEC_OFF, 0);

    Q9K_SetU8(desc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);
    /* P$ID mitfuehren -- fremde Module lesen die Prozess-ID an Offset $00
     * (s. Kommentar bei Q9K_PROCDESC_ID_OFF). Die Nummer selbst ergibt sich
     * wie bisher aus der Lage des Deskriptors in der Tabelle. */
    Q9K_SetU16(desc + Q9K_PROCDESC_ID_OFF, Q9K_ProcIdForDesc(desc));
    Q9K_SetU8(desc + Q9K_PROCDESC_PRIORITY_OFF, (Q9_u8)priority);
    Q9K_SetU32(desc + Q9K_PROCDESC_PARENT_OFF, parentDesc);   /* NACHTRAG 2026-08-22 */
    /* Gruppe/Benutzer vom Elternprozess erben -- reale F$Fork-Semantik.
     * Ohne Elternprozess (sollte bei F$Fork nicht vorkommen) bleibt es
     * bei 0.0 = Superuser. */
    Q9K_SetU32(desc + Q9K_PROCDESC_USER_OFF,
               parentDesc ? Q9K_GetU32(parentDesc + Q9K_PROCDESC_USER_OFF) : 0UL);

    /* NACHTRAG 2026-09-04: Pfade vom Elternprozess erben.
     *
     * Ohne das kann ein geforktes Kind keine Ein-/Ausgabe machen: seine
     * P$Path-Tabelle waere leer, und ein I$Write/I$ReadLn auf Pfad 0..2
     * liefe ins Leere. In OS-9 erbt ein Kind die offenen Pfade des Erzeugers
     * -- das ist der Grund, warum ein normales Programm einfach auf
     * Standard-Ein/Ausgabe schreiben kann, ohne selbst etwas zu oeffnen.
     *
     * BEWUSSTE VEREINFACHUNG, klar benannt: die Eintraege werden KOPIERT,
     * nicht per I$Dup dupliziert. Real erhoeht jeder geerbte Pfad den
     * Referenzzaehler seines Pfaddeskriptors, damit ein I$Close des einen
     * Prozesses dem anderen den Pfad nicht unter den Fuessen wegzieht.
     * Solange Pfade in diesem Kernel nie geschlossen werden, ist die Kopie
     * gleichwertig -- sobald es I$Close gibt, MUSS hier I$Dup stehen. */
    if (parentDesc != 0) {          /* beim allerersten Prozess gibt es keinen Erzeuger */
        for (i = 0; i < Q9K_PROCDESC_PATH_COUNT; i++) {
            Q9K_SetU16(desc + Q9K_PROCDESC_PATH_OFF + i * 2UL,
                       Q9K_GetU16(parentDesc + Q9K_PROCDESC_PATH_OFF + i * 2UL));
        }
    } else {
        /* ECHTER BUG, gefunden 2026-09-06: Ohne Erzeuger blieb die
         * P$Path-Tabelle voellig UNINITIALISIERT -- sie enthielt den
         * Speichermuell, der zufaellig an der Stelle stand (gemessen:
         * $6600000C $23CD0000, also 68k-Code aus einer frueheren Belegung).
         *
         * IOMans I$Open sucht dort das erste freie Wort
         * ("lea $168(a4),a0 / moveq #$1f,d0 / tst.w (a0)+ / dbeq d0,...").
         * Unter 32 Muellworten steht nie eine Null, also meldete IOMan
         * E$PthFul und gab "can't open console device" aus -- aber NUR,
         * sobald A4 korrekt auf den Prozessdeskriptor zeigt. Solange der
         * Dispatcher A4 mit der Handleradresse ueberschrieb, suchte IOMan
         * in unserem Kernelcode und fand dort zufaellig eine Null. Genau
         * dieser Zufall hat den Fehler jahrelang verdeckt und zugleich die
         * Speicherkorruption erzeugt (RBFs "move.l d1,$14e(a4)"). */
        for (i = 0; i < Q9K_PROCDESC_PATH_COUNT; i++) {
            Q9K_SetU16(desc + Q9K_PROCDESC_PATH_OFF + i * 2UL, 0);
        }
    }
    Q9K_SetU32(desc + Q9K_PROCDESC_MODHDR_OFF, hdrAddr);      /* NACHTRAG 2026-08-22 */
    Q9K_SetU32(desc + Q9K_PROCDESC_ALLOCBASE_OFF, block);
    Q9K_SetU32(desc + Q9K_PROCDESC_ALLOCSIZE_OFF, totalSize);
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

/* F$DFork bridge.  The child is fully constructed by Q9K_ProcDebugFork,
 * but remains in the WAITING state and outside the ready queue until the
 * future F$DExec call explicitly resumes it. */
void Q9K_SysDForkImpl(void)
{
    Q9_u16 error = 0;
    Q9_u32 pid = Q9K_ProcDebugFork(
        Q9K_GetU16(0x1EBCUL), Q9K_GetU32(0x1EC0UL),
        Q9K_GetU32(0x1EC4UL), Q9K_GetU32(0x1ED0UL),
        Q9K_GetU32(0x1ED4UL), Q9K_GetU16(0x1ECCUL),
        Q9K_GetU32(0x1ED8UL), &error);

    if (pid == 0) {
        Q9K_SetU16(0x1EE0UL, error);
        Q9K_SetU16(0x1EE4UL, 0);
        return;
    }
    Q9K_SetU16(0x1EDCUL, (Q9_u16)pid);
    Q9K_SetU16(0x1EE4UL, 1);
}

/* Q9K_ProcAllPrc -- echte F$AllPrc-Kernlogik (Callcode $4B, "Allocate
 * Process Descriptor"). Verifizierte ABI (68k_tech.pdf S. 373): keine
 * Eingabe, AUS (a2) = Deskriptorzeiger, E$PrcFul wenn der Pool leer ist.
 * Systemzustand.
 *
 * Die Beschreibung nennt drei Schritte -- Deskriptor loeschen, Zustand
 * auf Systemzustand setzen, MMU-Abbild als unbelegt markieren -- und
 * schliesst mit dem hier entscheidenden Satz: "On systems without memory
 * management/protection, this is a direct call to F$AllPD." Genau das ist
 * dieser Kernel, der dritte Schritt entfaellt also ersatzlos.
 *
 * Eine Feinheit, die nicht aus der Beschreibung kommt, sondern aus diesem
 * Kernel: ein Pool-Slot gilt hier nur dann als belegt, wenn sein
 * Zustandsbyte einen der vier bekannten Werte traegt (s.
 * Q9K_ProcIsAllocated, q9kernel_procapi.c). Ein bloss genullter
 * Deskriptor waere aus der Freiliste heraus, fuer den uebrigen Kernel
 * aber unsichtbar -- F$GPrDBT wuerde ihn nicht auffuehren und F$DelPrc
 * ihn nicht wiederfinden. Der frische Deskriptor bekommt deshalb
 * WAITING: existiert, aber laeuft nicht. Erst F$AProc macht ihn
 * lauffaehig. (Das "system-state" der Beschreibung meint den
 * Ausfuehrungsmodus des Prozesses, nicht dieses Belegungsfeld.)
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
int Q9K_ProcAllPrc(Q9_u32 *outDesc, Q9_u16 *outError)
{
    Q9_u32 desc;
    Q9_u32 i;

    *outError = 0;
    *outDesc = 0;

    desc = Q9K_ProcPoolAlloc();
    if (desc == 0) {
        *outError = (Q9_u16)Q9K_E_PRCFUL;
        return 0;
    }

    for (i = 0; i < Q9K_PROCDESC_SIZE; ++i)
        Q9K_SetU8(desc + i, 0);

    Q9K_SetU8(desc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_WAITING);
    Q9K_SetU16(desc + Q9K_PROCDESC_ID_OFF, Q9K_ProcIdForDesc(desc));

    *outDesc = desc;
    return 1;
}

/* Q9K_ProcDelPrc -- echte F$DelPrc-Kernlogik (Callcode $4C,
 * "De-allocate Process Descriptor"). Verifizierte ABI (68k_tech.pdf
 * S. 403): d0.w = freizugebende Prozess-ID, keine Ausgabe.
 * Systemzustand.
 *
 * Die Beschreibung ist in einem Punkt ausdruecklich: "You must ensure any
 * system resources used by the process are returned before calling
 * F$DelPrc." Dieser Call gibt also NUR den Deskriptor zurueck und fasst
 * weder Speicher noch Pfade an -- anders als der Aufraeumpfad bei
 * F$Exit (q9kernel_procend.c), der beides mit erledigt. Die Felder, auf
 * denen die Pool-Invariante beruht (ParentDesc == 0 bei einem freien
 * Slot), werden vor dem Zurueckhaengen geloescht, damit der Pool-Scan in
 * q9kernel_procend.c weiter stimmt.
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
int Q9K_ProcDelPrc(Q9_u16 pid, Q9_u16 *outError)
{
    Q9_u32 desc = Q9K_ProcLookup(pid);

    *outError = 0;

    if (desc == 0) {
        *outError = (Q9_u16)Q9K_E_PRCID;
        return 0;
    }

    Q9K_SetU32(desc + Q9K_PROCDESC_PARENT_OFF, 0);
    Q9K_SetU32(desc + Q9K_PROCDESC_MODHDR_OFF, 0);
    Q9K_SetU8(desc + Q9K_PROCDESC_STATE_OFF, 0);
    Q9K_ProcPoolAbortAlloc(desc);   /* Slot zurueck in die Freiliste */
    return 1;
}

void Q9K_SysAllPrcImpl(void)
{
    Q9_u32 desc = 0;
    Q9_u16 err = 0;

    if (Q9K_ProcAllPrc(&desc, &err)) {
        Q9K_SetU32(Q9K_ALLPRC_SCRATCH_DESC, desc);
        Q9K_SetU32(Q9K_ALLPRC_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_ALLPRC_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_ALLPRC_SCRATCH_SUCCESS, 0UL);
    }
}

void Q9K_SysDelPrcImpl(void)
{
    Q9_u16 err = 0;

    if (Q9K_ProcDelPrc((Q9_u16)Q9K_GetU32(Q9K_DELPRC_SCRATCH_PID), &err)) {
        Q9K_SetU32(Q9K_DELPRC_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_DELPRC_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_DELPRC_SCRATCH_SUCCESS, 0UL);
    }
}
