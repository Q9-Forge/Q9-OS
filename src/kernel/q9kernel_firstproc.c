/*
 * q9kernel_firstproc.c -- Q9-OS eigener Kernel: minimales Geruest fuer
 *                         den ersten Ausfuehrungskontext (Abschnitt 2,
 *                         Punkt 7).
 *
 * Mit Andreas abgestimmter Umfang (2026-08-18): MECHANISMUS zeigen
 * (Deskriptor aus dem Punkt-5/6-Pool holen, in die Ready-Queue
 * einhaengen, Kontextwechsel durchfuehren) -- OHNE ein echtes geladenes
 * Programm zu starten. F$Link/Modul-Laden existiert noch nicht, es gibt
 * also noch nichts Echtes zum Ausfuehren; das Sprungziel ist bewusst ein
 * reiner Platzhalter (Q9K_FirstProcPlaceholder, q9kernel_entry.a).
 *
 * Prozess-Deskriptor-Layout (eigene Festlegung, KEIN Kompat-Erfordernis,
 * gleiche Begruendung wie schon bei den Punkt-5/6-Pools in
 * q9kernel_tables.c -- Deskriptor-Innenleben ist reine Kernel-
 * Implementierung, kein Modul sieht das je):
 *   +0x00  State (1 Byte, ASCII -- 'a' = aktiv, s. Thema 01/
 *          Q9_scheduler_183a-Fund fuer die reale Konvention)
 *   +0x30  Next  (4 Byte) -- Ready-Queue-Link
 *   +0x34  Prev  (4 Byte) -- Ready-Queue-Link
 *   +0x38  SavedSP (4 Byte, eigene Ergaenzung)
 *   +0x3C  EntryPC (4 Byte, eigene Ergaenzung)
 * Next/Prev bewusst auf +0x30/+0x34 gelegt -- KEIN Zufall, sondern
 * dieselben Offsets, mit denen Q9_D_ACTIVQ (die Ready-Queue selbst) in
 * q9kernel_cinit.c schon als leerer, selbstreferenzierender Sentinel-
 * Knoten initialisiert wird (Q9K_InitEmptyQueue(Q9_D_ACTIVQ,0x30,0x34))
 * -- am echten Kernel verifiziert, dass der Sentinel selbst "deskriptor-
 * foermig" behandelt wird (Thema: Q9_scheduler_183a). Rest des 128-Byte-
 * Pool-Slots (Q9K_PROCDESC_SIZE, q9kernel_tables.c) bleibt reserviert/
 * TODO fuer alles, was ein echter Scheduler/Kontextwechsel noch braucht
 * (Register-Sicherung, Prioritaet, ...).
 *
 * Slot-Offset 0 kollidiert absichtlich mit der Punkt-5/6-Freiliste
 * (die dort den "naechster freier Slot"-Zeiger an Offset 0 ablegt) --
 * unproblematisch, weil "frei" und "als Deskriptor belegt" sich
 * gegenseitig ausschliessen (klassisches Freispeicher-durch-ungenutzten-
 * Speicher-Muster, gleiches Prinzip wie im Arena-Allokator).
 */

#include "q9kernel_config.h"

typedef unsigned long Q9_u32;
typedef unsigned char Q9_u8;

extern Q9_u32 Q9K_AllocMem(Q9_u32 requestedSize);
extern void   Q9K_FirstProcPlaceholder(void);  /* q9kernel_entry.a, reiner Platzhalter */

#ifndef Q9_D_ACTIVQ
#define Q9_D_ACTIVQ 0x37CUL   /* s. q9sysglob.h, per #ifndef ueberschreibbar fuer Host-Tests */
#endif
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
#ifndef Q9K_PROCDESC_SAVEDSP_OFF
#define Q9K_PROCDESC_SAVEDSP_OFF 0x38UL
#endif
#ifndef Q9K_PROCDESC_ENTRYPC_OFF
#define Q9K_PROCDESC_ENTRYPC_OFF 0x3CUL
#endif
#define Q9K_PROCDESC_STATE_ACTIVE 'a'   /* s. Kopfkommentar */

#define Q9K_FIRSTPROC_STACK_SIZE 2048UL

/* eigene Kernel-Global-Erweiterungen -- Uebergabe an Q9K_JumpToFirstProc
 * (q9kernel_entry.a) ueber feste Adressen statt Funktionsparameter,
 * s. dortigen Kommentar. */
#ifndef Q9K_FIRSTPROC_SP_ADDR
#define Q9K_FIRSTPROC_SP_ADDR 0x1224UL
#endif
#ifndef Q9K_FIRSTPROC_PC_ADDR
#define Q9K_FIRSTPROC_PC_ADDR 0x122CUL
#endif

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
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

/* Haengt node hinten an eine zirkulaere doppelt verkettete Liste mit
 * Sentinel-Kopf sentinel an -- funktioniert unabhaengig davon, ob die
 * Liste vorher leer war (Sentinel zeigt dann auf sich selbst, s.
 * Q9K_InitEmptyQueue in q9kernel_cinit.c). Node-Next/Prev-Offsets sind
 * dieselben wie beim Sentinel selbst (+0x30/+0x34), s. Kopfkommentar. */
static void Q9K_ReadyQueueAppend(Q9_u32 sentinel, Q9_u32 node)
{
    Q9_u32 tail = Q9K_GetU32(sentinel + Q9K_READYQ_PREV_OFF);

    Q9K_SetU32(node + Q9K_READYQ_NEXT_OFF, sentinel);
    Q9K_SetU32(node + Q9K_READYQ_PREV_OFF, tail);
    Q9K_SetU32(tail + Q9K_READYQ_NEXT_OFF, node);
    Q9K_SetU32(sentinel + Q9K_READYQ_PREV_OFF, node);
}

/* Baut den ersten (und bisher einzigen) Ausfuehrungskontext auf:
 * Deskriptor aus dem Pool holen, eigenen Stack allozieren (Arena),
 * Status aktiv setzen, in Q9_D_ACTIVQ einhaengen, Sprungziel/SP fuer
 * Q9K_JumpToFirstProc hinterlegen. KEIN Sprung hier drin -- der
 * Aufrufer (q9kernel_cinit.c) entscheidet, ob/wann gesprungen wird.
 * Rueckgabe 0 = Erfolg, 1 = Allokation fehlgeschlagen (Deskriptor-Pool
 * oder Arena erschoepft) -- kein Fake-Erfolg. */
Q9_u32 Q9K_StartFirstProcess(void)
{
    Q9_u32 desc = Q9K_ProcPoolAlloc();
    Q9_u32 stackBase;

    if (desc == 0)
        return 1;

    stackBase = Q9K_AllocMem(Q9K_FIRSTPROC_STACK_SIZE);
    if (stackBase == 0)
        return 1;

    Q9K_SetU8(desc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);
    Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, stackBase + Q9K_FIRSTPROC_STACK_SIZE); /* Stack waechst abwaerts */
    Q9K_SetU32(desc + Q9K_PROCDESC_ENTRYPC_OFF, (Q9_u32)(unsigned long)Q9K_FirstProcPlaceholder);

    Q9K_ReadyQueueAppend(Q9_D_ACTIVQ, desc);

    Q9K_SetU32(Q9K_FIRSTPROC_SP_ADDR, stackBase + Q9K_FIRSTPROC_STACK_SIZE);
    Q9K_SetU32(Q9K_FIRSTPROC_PC_ADDR, (Q9_u32)(unsigned long)Q9K_FirstProcPlaceholder);

    return 0;
}
