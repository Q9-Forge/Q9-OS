/*
 * q9kernel_iopath.c -- Q9-OS eigener Kernel: I$Open (Abschnitt "IOMan-
 *                      Einbindung", 2026-09-01, im Anschluss an den
 *                      A6-Schutz/Trampolin-Fehler-Stub-Fund -- IOMan
 *                      kommt seitdem stabil bis I$Open, haengt dort
 *                      aber in einer eigenen Retry-Schleife fest, weil
 *                      der Callcode 0x84 bisher nicht implementiert
 *                      ist).
 *
 * Reale Konvention ECHT per Read gelesen (68k_tech.pdf S. 568-570,
 * "I$Open -- Open a Path to a File or Device"):
 *
 *   IN  d0.b = Access mode (Bit 0=Read, 1=Write, 2=Execute, 4=Append,
 *       6=Non-sharable, 7=Directory)
 *       (a0)  = Pathname pointer
 *   OUT (Erfolg): d0.w = Path number, (a0) = hinter den Namen
 *       aktualisiert, Carry im geretteten SR geloescht.
 *   OUT (Fehlschlag): Carry gesetzt, d1.w = Fehlercode.
 *   "I$Open always uses the lowest path number available for the
 *   process." -- also eigentlich PRO PROZESS eine eigene, dichte
 *   Nummerierung. Diese erste Implementierung hat noch KEINE
 *   pro-Prozess-Pfadtabelle (der Prozessdeskriptor hat dafuer noch
 *   kein reserviertes Feld, s. q9kernel_tables.c Kopfkommentar
 *   "Q9K_PROCDESC_SIZE PLATZHALTER") -- Pfadnummern kommen deshalb
 *   vorerst aus einem einzigen GLOBALEN Zaehler/Pool, was bei mehreren
 *   gleichzeitig laufenden Prozessen mit eigenen offenen Pfaden falsch
 *   waere, fuer den aktuellen Ein-Prozess-Testkontext (IOMan oeffnet
 *   genau einen Pfad) aber funktional aequivalent ist. Echtes TODO,
 *   sobald mehrere Prozesse gleichzeitig I/O machen.
 *
 * ECHTE Pfadaufloesung (Pathname -> Geraet) existiert in diesem Kernel
 * noch NICHT -- kein Dateisystem, keine Geraetetabelle mit benannten
 * Eintraegen. Diese erste Implementierung akzeptiert JEDEN Pathname
 * (ueberspringt ihn nur bis zum NUL-Byte, wie vom Manual verlangt: "(a0)
 * = Updated past pathname") und verbindet JEDEN geoeffneten Pfad
 * pauschal mit der einzigen echten Ausgabe, die dieser Kernel kennt --
 * dem DUART (Q9K_DiagWriteD7-Mechanismus). Fuer den konkreten Anlass
 * (IOMan versucht laut Disassemblierung + Live-Diagnose von D_Init
 * "/term" zu oeffnen, s. Session-Notizen 2026-08-31) ist das inhaltlich
 * korrekt -- "/term" IST die Konsole.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

#ifndef Q9K_PATHPOOL_BASE_ADDR
#define Q9K_PATHPOOL_BASE_ADDR 0x1214UL
#endif
#ifndef Q9K_PATHPOOL_FREE_ADDR
#define Q9K_PATHPOOL_FREE_ADDR 0x121CUL
#endif
#ifndef Q9K_PATHDESC_SIZE
#define Q9K_PATHDESC_SIZE 32UL
#endif

/* Pfad-Deskriptor-Layout (32 Byte, s. Kopfkommentar q9kernel_tables.c):
 *   +0x00 (4)  bei FREI: Next-Zeiger (Freilisten-Verkettung, gleiches
 *              Prinzip wie Q9K_ProcPoolAlloc); bei ALLOZIERT: unteres
 *              Byte = Pfad-TYP (1 = Konsole/DUART, einziger bisher
 *              unterstuetzter Typ), Rest 0.
 *   +0x04..+0x1F: reserviert fuer spaetere Erweiterung (Dateiposition,
 *              Geraetereferenz, Zugriffsmodus) -- bleibt vorerst 0.
 */
#define Q9K_PATHDESC_TYPE_OFF 0x00UL
#define Q9K_PATHDESC_TYPE_CONSOLE 1UL

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }

/* Holt EINEN Deskriptor aus der Pfad-Freiliste (q9kernel_tables.c hat
 * sie nur aufgebaut, Pop war nicht Teil davon) -- gleiches Muster wie
 * Q9K_ProcPoolAlloc (q9kernel_firstproc.c). Rueckgabe 0 = Pool
 * erschoepft. */
static Q9_u32 Q9K_PathPoolAlloc(void)
{
    Q9_u32 head = Q9K_GetU32(Q9K_PATHPOOL_FREE_ADDR);

    if (head == 0)
        return 0;

    Q9K_SetU32(Q9K_PATHPOOL_FREE_ADDR, Q9K_GetU32(head));
    return head;
}

/* Q9K_ProcIOpen -- echte I$Open-Kernlogik (s. Kopfkommentar).
 * IN: mode (nur fuer eine spaetere, echte Zugriffspruefung reserviert,
 *     bisher ungenutzt), pathnamePtr (Zeiger auf den NUL-terminierten
 *     Pfadnamen).
 * OUT: Q9_u32 -- 0 = Fehlschlag (Pool erschoepft, einziger bisher
 *     moeglicher Fehlerfall), sonst die Pfadnummer (immer >= 3, s. u.).
 *     *outPastName wird IMMER (auch im Fehlerfall, das Manual verlangt
 *     "(a0) = Updated past pathlist" nur fuer den Erfolgsfall, aber ein
 *     korrekt aktualisierter Zeiger schadet im Fehlerfall nicht und
 *     spart eine Sonderfall-Unterscheidung im Assembler-Trampolin).
 */
Q9_u32 Q9K_ProcIOpen(Q9_u32 mode, Q9_u32 pathnamePtr, Q9_u32 *outPastName)
{
    Q9_u32 p = pathnamePtr;
    Q9_u32 slot;

    (void)mode; /* s. Kopfkommentar -- noch keine echte Zugriffspruefung */

    while (*(volatile Q9_u8 *)p != 0)
        p++;
    p++; /* hinter das NUL-Byte selbst, "past pathname" */
    *outPastName = p;

    slot = Q9K_PathPoolAlloc();
    if (slot == 0)
        return 0;

    *(volatile Q9_u32 *)(slot + Q9K_PATHDESC_TYPE_OFF) = Q9K_PATHDESC_TYPE_CONSOLE;

    /* Pfadnummer aus der Slot-Position ableiten (kein pro-Prozess-
     * Zaehler vorhanden, s. Kopfkommentar) -- Offset 3, damit die bei
     * echtem OS-9 fuer stdin/stdout/stderr reservierten Nummern 0-2
     * nicht kollidieren, auch wenn wir diese noch nicht wirklich
     * vorbelegen. */
    return (slot - Q9K_GetU32(Q9K_PATHPOOL_BASE_ADDR)) / Q9K_PATHDESC_SIZE + 3UL;
}

/* Eigene Kernel-Global-Erweiterungen fuer die ASM<->C-Uebergabe von
 * Q9K_SysIOpenImpl (q9kernel_entry.a) -- gleiches, etabliertes Muster
 * wie ueberall in diesem Kernel. Direkt hinter den SSvc-Scratch-Feldern
 * ($134C-$1354, q9kernel_entry.a/ssvc.c) -- naechste freie Adresse
 * $1360 (die davor genutzten temporaeren Debug-Adressen der
 * "!"-Raetsel-Sitzung sind inzwischen wieder frei). */
#ifndef Q9K_IOpenScratch_Mode
#define Q9K_IOpenScratch_Mode     0x1360UL   /* Q9_u32, d0.b EIN (nur unteres Byte real genutzt) */
#endif
#ifndef Q9K_IOpenScratch_NamePtr
#define Q9K_IOpenScratch_NamePtr  0x1364UL   /* Q9_u32, (a0) EIN */
#endif
#ifndef Q9K_IOpenScratch_PastName
#define Q9K_IOpenScratch_PastName 0x1368UL   /* Q9_u32, (a0) AUS */
#endif
#ifndef Q9K_IOpenScratch_PathNum
#define Q9K_IOpenScratch_PathNum  0x136CUL   /* Q9_u32, d0.w AUS (0 = Fehlschlag) */
#endif

/* Q9K_SysIOpenImpl -- duenne, PARAMETERLOSE Bruecke zwischen dem
 * Assembler-Trampolin und Q9K_ProcIOpen -- gleiches Muster wie ueberall
 * (Q9K_SysForkImpl usw.). */
void Q9K_SysIOpenImpl(void)
{
    Q9_u32 mode     = Q9K_GetU32(Q9K_IOpenScratch_Mode);
    Q9_u32 namePtr  = Q9K_GetU32(Q9K_IOpenScratch_NamePtr);
    Q9_u32 pastName = 0;
    Q9_u32 pathNum;

    pathNum = Q9K_ProcIOpen(mode, namePtr, &pastName);

    Q9K_SetU32(Q9K_IOpenScratch_PastName, pastName);
    Q9K_SetU32(Q9K_IOpenScratch_PathNum, pathNum);
}
