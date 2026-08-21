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
 *          Q9_scheduler_183a-Fund fuer die reale Konvention)
 *   +0x01  Priority (1 Byte, eigene Ergaenzung, s. q9kernel_sched.c)
 *   +0x02  Age      (2 Byte, eigene Ergaenzung, s. q9kernel_sched.c)
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
#ifndef Q9K_PROCDESC_SAVEDSP_OFF
#define Q9K_PROCDESC_SAVEDSP_OFF 0x38UL
#endif
#ifndef Q9K_PROCDESC_ENTRYPC_OFF
#define Q9K_PROCDESC_ENTRYPC_OFF 0x3CUL
#endif
#define Q9K_PROCDESC_STATE_ACTIVE 'a'   /* s. Kopfkommentar */

#define Q9K_PROC_STACK_SIZE 2048UL

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

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
static void   Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }
static void   Q9K_SetU8(Q9_u32 addr, Q9_u8 value) { *(volatile Q9_u8 *)addr = value; }

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
    Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, frameBase);
    Q9K_SetU32(desc + Q9K_PROCDESC_ENTRYPC_OFF, entryPC);

    Q9K_SchedInsert(desc);

    return desc;
}
