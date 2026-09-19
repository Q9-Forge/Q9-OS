/*
 * q9kernel_nproc.c -- Q9-OS eigener Kernel: F$NProc (Callcode $2D,
 *                     2026-09-19).
 *
 * Verifizierte ABI (68k_tech.pdf S.468):
 *
 *   EIN  keine
 *   AUS  "Control does not return to caller."
 *   FEHLER cc = Carry, d1.w = Fehlercode
 *   Zustand: System
 *
 * Das Handbuch ist an einer Stelle ungewoehnlich deutlich, und die
 * bestimmt den ganzen Entwurf:
 *
 *   "The process calling NProc should already be in one of the system's
 *    process queues. If it is not, the calling process becomes unknown
 *    to the system even though the process descriptor still exists and
 *    is printed out by a procs command."
 *
 * Dieser Aufruf reiht den Aufrufer also BEWUSST NICHT ein. Er nimmt nur
 * den naechsten aus der Bereitliste und springt hinein. Wer sich vorher
 * nicht selbst auf eine Warteschlange gelegt hat, verschwindet -- das
 * ist die Semantik, nicht ein vergessener Handgriff. Genau deshalb ist
 * F$NProc ein System-State-Dienst: er ist fuer Treiber und Manager
 * gedacht, die ihre Warteschlangen selbst fuehren.
 *
 * Die ASM-Seite (Q9K_SysFNProc, q9kernel_entry.a) sichert vorher den
 * Registersatz und traegt SavedSP ein -- der uebliche Fall ist ja
 * gerade der, dass der Aufrufer sehr wohl auf einer Schlange steht und
 * spaeter fortgesetzt wird. Ohne das traegt er beim Aufwachen einen
 * veralteten Stackzeiger, dieselbe Falle wie bei F$Sema und Ev$Wait.
 *
 * KEIN PROZESS BEREIT: das Handbuch sagt "OS-9 waits for an interrupt,
 * and then checks the active process queue again". Dieser Kernel geht
 * stattdessen ueber F$Panic(K$Idle) -- denselben Weg wie F$Sema und
 * Ev$Wait, und den, den das Handbuch bei F$Panic selbst beschreibt:
 * "F$Panic is called only when the kernel believes there are no
 * processes remaining to be executed". Ein per F$SSvc installierter
 * Panic-Dienst ist damit genau die Stelle, an der ein System hier
 * eingreifen koennte -- im echten OS-9 waere das ein OS9P2-Modul.
 *
 * WARUM EINE EIGENE DATEI und nicht ein Zusatz in q9kernel_sched.c:
 * weil eine neue Funktion in einem frueh gelinkten Modul alle spaeter
 * gelinkten verschiebt, und dann reisst irgendwo ein bestehendes "bsr"
 * die 16-Bit-Reichweite. Genau das ist beim ersten Versuch passiert
 * (l68: "operand size error", ohne Angabe der Stelle). Neue Module
 * gehoeren deshalb ans ENDE der Link-Liste in build.sh.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;

extern Q9_u32 Q9K_SchedFirstPick(void);   /* q9kernel_sched.c */

#ifndef Q9K_NPROC_SCRATCH_NEXT
#define Q9K_NPROC_SCRATCH_NEXT 0x1E50UL /* Q9_u32, naechster Prozess AUS */
#endif

static void Q9K_SetU32(Q9_u32 addr, Q9_u32 value)
{
    *(volatile Q9_u32 *)addr = value;
}

/* Q9K_SysNProcImpl -- C-Teil von F$NProc. Von der ASM-Seite erst
 * gerufen, NACHDEM sie den Registersatz gesichert und SavedSP gesetzt
 * hat. Q9K_SchedFirstPick traegt den Gewaehlten zugleich in Q9_D_PROC
 * ein und laedt die Zeitscheibe auf; 0 heisst "keiner bereit". */
void Q9K_SysNProcImpl(void)
{
    Q9K_SetU32(Q9K_NPROC_SCRATCH_NEXT, Q9K_SchedFirstPick());
}
