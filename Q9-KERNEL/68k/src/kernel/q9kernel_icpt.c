/*
 * q9kernel_icpt.c -- Q9-OS eigener Kernel: die AUSFUEHRUNG von
 *                    Signal-Intercept-Routinen, dazu F$RTE (Callcode $1E)
 *                    und F$SigReset (Callcode $63). 2026-09-18.
 *
 * Verifizierte ABI (68k_tech.pdf S.388, 484, 502):
 *
 *   F$Icpt      EIN (a0) = Intercept-Routine, (a6) = Datenbereich
 *               AUS d0 = Zahl der anstehenden Signale
 *               -- die Registrierung steht schon in q9kernel_procsleep.c
 *   F$RTE       keine Ein- und Ausgaben. Beendet die Intercept-Routine
 *               und setzt das Hauptprogramm fort; stehen weitere Signale
 *               an, laeuft die Routine erneut, "until the queue is
 *               exhausted".
 *   F$SigReset  keine Ein- und Ausgaben. Verwirft den gesicherten
 *               Kontext, statt ihn per F$RTE zurueckzuholen.
 *
 * Die Intercept-Routine bekommt:  d1.w = Signalcode, (a6) = Datenbereich.
 *
 * WAS HIER NEU IST: F$Icpt konnte bisher nur registrieren. Der Kernel
 * legte ein Signal in P$Signal ab und weckte den Prozess -- die
 * eingetragene Routine lief nie. Damit war jedes Programm, das sich auf
 * einen Tastatur-Abbruch einstellen wollte, auf Nachsehen angewiesen.
 * Diese Datei stellt Signale wirklich zu.
 *
 * WIE DIE ZUSTELLUNG FUNKTIONIERT, und warum sie ohne neuen Speicher
 * auskommt: der unterbrochene Prozess hat seinen vollstaendigen Zustand
 * bereits auf dem eigenen Stack liegen -- Registersatz plus
 * Exception-Rahmen, zusammen 68 Byte, und P$SavedSP zeigt darauf. Die
 * Zustellung legt einfach einen ZWEITEN solchen Rahmen darunter, der die
 * Intercept-Routine anspringt, und laesst P$SavedSP auf diesen zeigen.
 *
 *      hoehere Adressen
 *      ...................
 *      [ Rahmen des unterbrochenen Hauptprogramms ]   <- altes P$SavedSP
 *      [ Rahmen der Intercept-Routine            ]   <- neues P$SavedSP
 *      ...................
 *      niedrigere Adressen (Stack waechst nach unten)
 *
 * F$RTE ist damit fast nichts: P$SavedSP um einen Rahmen zurueck, und das
 * Hauptprogramm laeuft weiter, als sei nichts geschehen. Genau dieses
 * Stapeln beschreibt das Handbuch mit "each time the intercept routine is
 * called, 70 bytes are used on the user's stack" -- diese Fassung braucht
 * 68, weil ihr Rahmen genau so gross ist; die zwei Byte Unterschied sind
 * Ausrichtung im Original und hier nicht noetig.
 *
 * WARUM DER ZAEHLER: F$RTE muss wissen, ob ueberhaupt ein Kontext zum
 * Zurueckkehren da ist -- ein F$RTE ohne vorangegangenen Intercept wuerde
 * sonst einen Rahmen abraeumen, der dem Hauptprogramm gehoert, und den
 * Prozess ins Nirgendwo schicken. P$SigLvl kann das nicht leisten (das
 * ist die Maske), deshalb fuehrt der Deskriptor einen eigenen Zaehler
 * verschachtelter Intercepts.
 *
 * BEWUSSTE GRENZE -- Zustellung nur an einen NICHT laufenden Prozess:
 * Wer gerade laeuft, hat seinen Zustand in den CPU-Registern und nicht
 * auf dem Stack; P$SavedSP ist dann veraltet. Ein Signal an den laufenden
 * Prozess wird deshalb wie bisher nur abgelegt und beim naechsten
 * Prozesswechsel zugestellt. Der haeufige Fall (Prozess A signalisiert
 * Prozess B, oder eine ISR weckt einen Wartenden) ist damit abgedeckt;
 * der Fall "ISR signalisiert den gerade unterbrochenen Prozess" braucht
 * den Zustand aus dem ISR-Rahmen und bleibt offen. Lieber diese Grenze
 * benannt als ein Rahmen, der aus veralteten Registern gebaut wird.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

extern void Q9K_SetFrameReg(Q9_u32 frameBase, Q9_u32 index, Q9_u32 value);
extern Q9_u32 Q9K_GetFrameReg(Q9_u32 frameBase, Q9_u32 index);

#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif

#ifndef Q9K_PROCDESC_SAVEDSP_OFF
#define Q9K_PROCDESC_SAVEDSP_OFF 0x08UL
#endif
#ifndef Q9K_PROCDESC_SIGNAL_OFF
#define Q9K_PROCDESC_SIGNAL_OFF  0x26UL   /* P$Signal */
#define Q9K_PROCDESC_SIGVEC_OFF  0x28UL   /* P$SigVec */
#define Q9K_PROCDESC_SIGDAT_OFF  0x2CUL   /* P$SigDat */
#endif

/* Zaehler verschachtelter Intercepts. Ein eigenes Feld im Deskriptor,
 * hinter den bekannten OS-9-Feldern -- das Original fuehrt den Zustand
 * anders (System- und Userstack), was hier keinen Vorteil braechte.
 * Der Wert zaehlt, wie viele Rahmen die Zustellung gestapelt hat. */
#ifndef Q9K_PROCDESC_ICPTDEPTH_OFF
#define Q9K_PROCDESC_ICPTDEPTH_OFF 0x1CCUL
#endif

/* Rahmenaufbau, identisch zu q9kernel_firstproc.c -- Werte von dort
 * UEBERNOMMEN, nicht nachgeschlagen (bei F$Chain hat genau dieser
 * Unterschied einen Absturz gekostet). */
#ifndef Q9K_PROCDESC_REGSAVE_SIZE
#define Q9K_PROCDESC_REGSAVE_SIZE 60UL
#endif
#ifndef Q9K_EXCFRAME_SR_OFF
#define Q9K_EXCFRAME_SR_OFF     0x00UL
#define Q9K_EXCFRAME_PC_OFF     0x02UL
#define Q9K_EXCFRAME_FMTVEC_OFF 0x06UL
#endif
#ifndef Q9K_EXCFRAME_SIZE
#define Q9K_EXCFRAME_SIZE 8UL
#endif
#ifndef Q9K_FAKEFRAME_SIZE
#define Q9K_FAKEFRAME_SIZE (Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_SIZE)
#endif
#ifndef Q9K_INITIAL_SR
#define Q9K_INITIAL_SR 0x2000U
#endif

/* Registerindizes im gesicherten Satz (movem.l d0-d7/a0-a6). */
#define Q9K_FRAMEREG_D1 1UL
#define Q9K_FRAMEREG_A6 14UL

/* Scratch-Bruecke, hinter F$Chain ($1BA0-$1BCF). */
#ifndef Q9K_ICPT_SCRATCH_OK
#define Q9K_ICPT_SCRATCH_OK    0x1BD0UL /* Q9_u32, 0/1                       */
#define Q9K_ICPT_SCRATCH_ERROR 0x1BD4UL /* Q9_u32, d1.w AUS bei Fehler       */
#define Q9K_ICPT_SCRATCH_DEPTH 0x1BD8UL /* Q9_u32, Tiefe nach der Operation  */
#endif

#ifndef Q9K_E_BPADDR
#define Q9K_E_BPADDR 0xD2U
#endif

#ifndef Q9K_CELL_ACCESSORS_PROVIDED
static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
#endif
#ifndef Q9K_ICPT_SHORT_ACCESSORS
static Q9_u16 Q9K_GetU16(Q9_u32 addr) { return *(volatile Q9_u16 *)addr; }
static void   Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }
#endif

/* Q9K_IcptDeliver -- einen Intercept-Rahmen fuer desc aufsetzen.
 *
 * Rueckgabe 1, wenn zugestellt wurde; 0, wenn der Prozess keine Routine
 * eingetragen hat (dann bleibt es beim blossen Ablegen in P$Signal, wie
 * bisher) oder wenn kein gueltiger Rahmen vorliegt.
 *
 * Der Aufrufer muss sicherstellen, dass desc NICHT der laufende Prozess
 * ist -- s. Kopfkommentar. */
int Q9K_IcptDeliver(Q9_u32 desc, Q9_u16 signal)
{
    Q9_u32 vector, oldFrame, newFrame;

    if (desc == 0UL)
        return 0;

    vector = Q9K_GetU32(desc + Q9K_PROCDESC_SIGVEC_OFF);
    if (vector == 0UL)
        return 0;                    /* kein Intercept eingetragen */

    oldFrame = Q9K_GetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF);
    if (oldFrame == 0UL)
        return 0;                    /* kein gesicherter Zustand vorhanden */

    /* Der neue Rahmen liegt UNTER dem alten -- der Stack waechst nach
     * unten, und der alte Rahmen muss unversehrt liegen bleiben, bis
     * F$RTE ihn wieder aufnimmt. */
    newFrame = oldFrame - Q9K_FAKEFRAME_SIZE;

    /* Der Registersatz der Intercept-Routine: das Handbuch nennt genau
     * zwei belegte Register, alles andere ist ihr gegenueber undefiniert.
     * Hier wird der Satz des Hauptprogramms uebernommen und nur diese
     * beiden gesetzt -- so findet eine Routine, die (entgegen der
     * Empfehlung) doch mehr benutzt, wenigstens keinen Muell vor. */
    {
        Q9_u32 i;
        for (i = 0UL; i < 15UL; i++)
            Q9K_SetFrameReg(newFrame, i, Q9K_GetFrameReg(oldFrame, i));
    }
    Q9K_SetFrameReg(newFrame, Q9K_FRAMEREG_D1, (Q9_u32)signal);
    Q9K_SetFrameReg(newFrame, Q9K_FRAMEREG_A6,
                    Q9K_GetU32(desc + Q9K_PROCDESC_SIGDAT_OFF));

    Q9K_SetU16(newFrame + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_SR_OFF,
               Q9K_INITIAL_SR);
    Q9K_SetU32(newFrame + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_PC_OFF,
               vector);
    Q9K_SetU16(newFrame + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_FMTVEC_OFF, 0U);

    Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, newFrame);
    Q9K_SetU32(desc + Q9K_PROCDESC_ICPTDEPTH_OFF,
               Q9K_GetU32(desc + Q9K_PROCDESC_ICPTDEPTH_OFF) + 1UL);

    /* Das Signal gilt als entgegengenommen -- es steckt jetzt im
     * Registersatz der Routine. Bliebe es zusaetzlich in P$Signal stehen,
     * liefe die Routine beim naechsten F$RTE ein zweites Mal fuer
     * dasselbe Signal. */
    Q9K_SetU16(desc + Q9K_PROCDESC_SIGNAL_OFF, 0U);
    return 1;
}

/* Q9K_IcptReturn -- F$RTE.
 *
 * Nimmt den obersten Intercept-Rahmen zurueck. Steht noch ein Signal an,
 * wird sofort ein neuer Rahmen aufgesetzt, statt ins Hauptprogramm
 * zurueckzukehren -- "if there are unprocessed signals pending, the
 * interrupt routine executes again (until the queue is exhausted)".
 *
 * Rueckgabe 1 = in Ordnung. 0 = es gab keinen Intercept, aus dem man
 * haette zurueckkehren koennen. */
int Q9K_IcptReturn(Q9_u32 desc, Q9_u16 *outError)
{
    Q9_u32 depth, frame;
    Q9_u16 pending;

    if (desc == 0UL) {
        *outError = (Q9_u16)Q9K_E_BPADDR;
        return 0;
    }

    depth = Q9K_GetU32(desc + Q9K_PROCDESC_ICPTDEPTH_OFF);
    if (depth == 0UL) {
        /* F$RTE ohne Intercept. Wuerde man hier trotzdem einen Rahmen
         * abraeumen, verloere das Hauptprogramm seinen eigenen Zustand
         * und liefe an einer beliebigen Stelle weiter. */
        *outError = (Q9_u16)Q9K_E_BPADDR;
        return 0;
    }

    frame = Q9K_GetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF) + Q9K_FAKEFRAME_SIZE;
    Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, frame);
    Q9K_SetU32(desc + Q9K_PROCDESC_ICPTDEPTH_OFF, depth - 1UL);

    /* Noch etwas in der Warteschlange? Dann gleich wieder hinein. */
    pending = Q9K_GetU16(desc + Q9K_PROCDESC_SIGNAL_OFF);
    if (pending != 0U)
        (void)Q9K_IcptDeliver(desc, pending);

    return 1;
}

/* Q9K_IcptReset -- F$SigReset.
 *
 * Verwirft den gesicherten Kontext, statt per F$RTE zurueckzukehren. Das
 * braucht ein Programm, das die Intercept-Routine per longjmp() verlaesst
 * und nie zurueckkehrt: ohne diesen Aufruf bliebe der Rahmen auf dem
 * Stack liegen, und bei jedem weiteren Signal kaeme einer dazu.
 *
 * Das Handbuch nennt keinen Fehlerfall ("Error Output: None"), also ist
 * auch ein Aufruf ohne offenen Intercept in Ordnung -- er tut dann
 * schlicht nichts. */
void Q9K_IcptReset(Q9_u32 desc)
{
    if (desc == 0UL)
        return;
    Q9K_SetU32(desc + Q9K_PROCDESC_ICPTDEPTH_OFF, 0UL);
}

/* --- Bruecken ---------------------------------------------------- */

void Q9K_SysRTEImpl(void)
{
    Q9_u16 err = 0U;
    Q9_u32 desc = Q9K_GetU32(Q9_D_PROC);

    Q9K_SetU32(Q9K_ICPT_SCRATCH_OK, 0UL);
    if (!Q9K_IcptReturn(desc, &err)) {
        Q9K_SetU32(Q9K_ICPT_SCRATCH_ERROR, (Q9_u32)err);
        return;
    }
    Q9K_SetU32(Q9K_ICPT_SCRATCH_DEPTH,
               Q9K_GetU32(desc + Q9K_PROCDESC_ICPTDEPTH_OFF));
    Q9K_SetU32(Q9K_ICPT_SCRATCH_OK, 1UL);
}

void Q9K_SysSigResetImpl(void)
{
    Q9K_IcptReset(Q9K_GetU32(Q9_D_PROC));
    Q9K_SetU32(Q9K_ICPT_SCRATCH_DEPTH, 0UL);
    Q9K_SetU32(Q9K_ICPT_SCRATCH_OK, 1UL);
}
