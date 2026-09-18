/*
 * q9kernel_sema.c -- Q9-OS eigener Kernel: F$Sema (Callcode $62,
 *                    2026-09-18).
 *
 * DIE ABI STEHT NICHT IM HANDBUCH -- sie ist aus Microwares eigener
 * Bibliothek disassembliert. Das Handbuch beschreibt Semaphore
 * ausfuehrlich (Kapitel 4) und nennt Struktur und Operationscodes, sagt
 * aber nirgends, in welchen Registern der Aufruf seine Argumente
 * erwartet. Belegt wurde es an MWOS/OS9/68020/LIB/os_lib.l:
 *
 *   _os_sema_p bei $9310:            _os_sema_v bei $9386:
 *     exg.l   d0,a0                    movem.l d1/a0,-(a7)
 *     addq.l  #1,4(a0)      s_lock++   movea.l d0,a0
 *     tas.b   0(a0)         s_value    btst.b  #0,$1f(a0)   s_flags Bit 0
 *     exg.l   d0,a0                    beq.b   ...          (kein cas)
 *     bne.b   ...           belegt     ...
 *     moveq   #0,d0         frei       subq.l  #1,4(a0)     s_lock--
 *     rts                   OHNE Trap  bne.b   ...          Warter da?
 *     ...                              moveq   #0,d0        sonst fertig
 *     movea.l d0,a0                    rts                  OHNE Trap
 *     moveq   #1,d1         P          ...
 *     trap    #0            F$Sema     move.l  a0,d0
 *     tas.b   0(a0)         erneut     moveq   #2,d1        V
 *     bne.b   ...           wieder     trap    #0           F$Sema
 *
 * Daraus folgt die vollstaendige Aufrufkonvention:
 *
 *   EIN  d0.l = Zeiger auf die Semaphorstruktur (a0 traegt ihn ebenfalls)
 *        d1.w = Operation: 1 = P (reservieren), 2 = V (freigeben)
 *   AUS  keine; bei Fehler Carry und d1.w = Fehlercode
 *
 * Die Struktur-Offsets aus dem Disassemblat decken sich exakt mit
 * MWOS/OS9/SRC/DEFS/semaphore.h -- s_value +0, s_lock +4, s_flags +28
 * (dessen unterstes Byte bei +$1f geprueft wird), s_sync +32. Das ist
 * eine unabhaengige Bestaetigung, dass der Header zum Code passt.
 *
 * WIE DIE ARBEITSTEILUNG AUSSIEHT, und warum dieser Aufruf so klein ist:
 * der unstrittige Fall laeuft GANZ OHNE Kernel. Der Nutzercode belegt den
 * Semaphor selbst per "tas" (Test-And-Set, unteilbar) und kehrt zurueck;
 * erst wenn das fehlschlaegt -- der Semaphor ist belegt -- ruft er
 * F$Sema(P), und nur dann muss der Kernel den Aufrufer schlafen legen.
 * Ebenso gibt der Nutzercode den Semaphor selbst frei und ruft F$Sema(V)
 * nur, wenn s_lock verraet, dass noch jemand wartet. Der Kernel sieht
 * also ausschliesslich den umkaempften Fall.
 *
 * WICHTIG fuer P: nach dem Aufwachen versucht der NUTZERCODE das "tas"
 * erneut (s. $9330 oben) und ruft gegebenenfalls wieder F$Sema(P). Dieser
 * Kernel muss also nicht garantieren, dass der geweckte Prozess den
 * Semaphor auch bekommt -- er muss ihn nur wecken. Das vereinfacht die
 * Sache erheblich und ist genau das, was das Handbuch beschreibt: "the
 * first process in the semaphore's queue is activated and RETRIES the
 * reserve operation".
 *
 * DIE WARTESCHLANGE liegt im Semaphor selbst (s_qnext = Kopf, s_qprev =
 * Schwanz, s_length = Laenge). Verkettet werden die wartenden Prozesse
 * ueber dasselbe Deskriptorfeld, das sonst die Ready-Queue benutzt
 * (Q9K_READYQ_NEXT_OFF): ein Prozess steht immer nur in EINER
 * Warteschlange, und solange er auf einen Semaphor wartet, ist er nicht
 * lauffaehig. Dieselbe Doppelnutzung macht das Original.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;

extern int    Q9K_ProcAProc(Q9_u32 desc, Q9_u16 *outError);      /* q9kernel_procapi.c */
extern Q9_u32 Q9K_SchedFirstPick(void);                          /* q9kernel_sched.c   */

/* Zustandscode eines auf einen Semaphor wartenden Prozesses. NICHT
 * geraten: MWOS/OS9/SRC/DEFS/process.a fuehrt ihn als
 * "Q_Sema: equ 'p' semaphore queue" -- eigener Zustand neben 'w'
 * (Warteschlange) und 's' (schlafend). */
#ifndef Q9K_PROCDESC_STATE_OFF
#define Q9K_PROCDESC_STATE_OFF 0x1DUL
#endif
#define Q9K_PROCDESC_STATE_SEMA 'p'

#ifndef Q9K_SetU8
static void Q9K_SetU8(Q9_u32 addr, unsigned char value) { *(volatile unsigned char *)addr = value; }
#endif

#ifndef Q9K_E_BPADDR
#define Q9K_E_BPADDR 0xD2U
#endif
#ifndef Q9K_E_UNKSVC
#define Q9K_E_UNKSVC 0xD0U
#endif

#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif

/* Verkettungsfeld im Prozessdeskriptor -- dasselbe wie in der
 * Ready-Queue, s. Kopfkommentar. */
#ifndef Q9K_READYQ_NEXT_OFF
#define Q9K_READYQ_NEXT_OFF 0x30UL
#endif

/* Feldabstaende der Semaphorstruktur, aus semaphore.h und im Disassemblat
 * bestaetigt. Per #ifndef ueberschreibbar, gleicher Grund wie ueberall:
 * auf dem 64-Bit-Testhost ist Q9_u32 acht Byte breit. */
#ifndef Q9K_SEMA_OFF_VALUE
#define Q9K_SEMA_OFF_VALUE   0UL   /* s_value  -- frei/belegt, vom Nutzercode per tas */
#define Q9K_SEMA_OFF_LOCK    4UL   /* s_lock   -- Benutzungszaehler, vom Nutzercode */
#define Q9K_SEMA_OFF_QNEXT   8UL   /* s_qnext  -- Kopf der Warteschlange */
#define Q9K_SEMA_OFF_QPREV  12UL   /* s_qprev  -- Schwanz der Warteschlange */
#define Q9K_SEMA_OFF_LENGTH 16UL   /* s_length -- Laenge der Warteschlange */
#define Q9K_SEMA_OFF_OWNER  20UL   /* s_owner  -- derzeitiger Eigentuemer */
#endif

#define Q9K_SEMA_P 1UL
#define Q9K_SEMA_V 2UL

/* Scratch-Bruecke, hinter F$GBlkMp ($1B58-$1B7F). */
#ifndef Q9K_SEMA_SCRATCH_PTR
#define Q9K_SEMA_SCRATCH_PTR   0x1B80UL /* Q9_u32, d0.l EIN            */
#define Q9K_SEMA_SCRATCH_OP    0x1B84UL /* Q9_u32, d1.w EIN            */
#define Q9K_SEMA_SCRATCH_ERROR 0x1B88UL /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_SEMA_SCRATCH_OK    0x1B8CUL /* Q9_u32, 0/1                 */
#define Q9K_SEMA_SCRATCH_SLEEP 0x1B90UL /* Q9_u32, 1 = Aufrufer muss warten    */
#define Q9K_SEMA_SCRATCH_NEXT  0x1B94UL /* Q9_u32, naechster Prozess (P)       */
#endif

#ifndef Q9K_CELL_ACCESSORS_PROVIDED
static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
#endif

/* Q9K_SemaEnqueue -- den Aufrufer hinten an die Warteschlange haengen.
 *
 * Hinten, nicht vorn: das Handbuch sagt "the process is suspended and
 * placed at the END of the semaphore's wait queue", und die Reihenfolge
 * entscheidet darueber, ob ein wartender Prozess je drankommt. */
void Q9K_SemaEnqueue(Q9_u32 sema, Q9_u32 desc)
{
    Q9_u32 tail = Q9K_GetU32(sema + Q9K_SEMA_OFF_QPREV);

    Q9K_SetU32(desc + Q9K_READYQ_NEXT_OFF, 0UL);   /* letzter in der Kette */

    if (tail == 0UL) {
        Q9K_SetU32(sema + Q9K_SEMA_OFF_QNEXT, desc);
    } else {
        Q9K_SetU32(tail + Q9K_READYQ_NEXT_OFF, desc);
    }
    Q9K_SetU32(sema + Q9K_SEMA_OFF_QPREV, desc);
    Q9K_SetU32(sema + Q9K_SEMA_OFF_LENGTH,
               Q9K_GetU32(sema + Q9K_SEMA_OFF_LENGTH) + 1UL);
}

/* Q9K_SemaDequeue -- den ersten Warter entnehmen; 0, wenn keiner da ist. */
Q9_u32 Q9K_SemaDequeue(Q9_u32 sema)
{
    Q9_u32 head = Q9K_GetU32(sema + Q9K_SEMA_OFF_QNEXT);
    Q9_u32 next;

    if (head == 0UL)
        return 0UL;

    next = Q9K_GetU32(head + Q9K_READYQ_NEXT_OFF);
    Q9K_SetU32(sema + Q9K_SEMA_OFF_QNEXT, next);
    if (next == 0UL)
        Q9K_SetU32(sema + Q9K_SEMA_OFF_QPREV, 0UL);   /* Schlange leer */
    Q9K_SetU32(head + Q9K_READYQ_NEXT_OFF, 0UL);

    {
        Q9_u32 length = Q9K_GetU32(sema + Q9K_SEMA_OFF_LENGTH);
        Q9K_SetU32(sema + Q9K_SEMA_OFF_LENGTH, length ? length - 1UL : 0UL);
    }
    return head;
}

/* Q9K_SysSemaImpl -- Bruecke fuer F$Sema.
 *
 * P wechselt den Prozess NICHT selbst: das muss auf der ASM-Seite
 * geschehen, weil dafuer ein Exception-Rahmen noetig ist (gleiche
 * Arbeitsteilung wie bei F$Sleep). Diese Funktion reiht ein, waehlt den
 * naechsten Prozess und legt ihn samt Warteanforderung in den
 * Scratch-Zellen ab. */
void Q9K_SysSemaImpl(void)
{
    Q9_u32 sema = Q9K_GetU32(Q9K_SEMA_SCRATCH_PTR);
    Q9_u32 op   = Q9K_GetU32(Q9K_SEMA_SCRATCH_OP);
    Q9_u32 self = Q9K_GetU32(Q9_D_PROC);

    Q9K_SetU32(Q9K_SEMA_SCRATCH_OK, 0UL);
    Q9K_SetU32(Q9K_SEMA_SCRATCH_SLEEP, 0UL);

    if (sema == 0UL) {
        Q9K_SetU32(Q9K_SEMA_SCRATCH_ERROR, Q9K_E_BPADDR);
        return;
    }

    switch (op) {
    case Q9K_SEMA_P:
        /* Der Aufrufer ist hier nur, weil sein eigenes "tas"
         * fehlgeschlagen ist -- der Semaphor ist belegt. Einreihen und
         * schlafen legen. */
        if (self == 0UL) {
            /* Ohne laufenden Prozess gibt es niemanden zum Schlafenlegen.
             * Kann im normalen Betrieb nicht vorkommen; lieber ein
             * ehrlicher Fehler als ein stiller Selbstblockierer. */
            Q9K_SetU32(Q9K_SEMA_SCRATCH_ERROR, Q9K_E_BPADDR);
            return;
        }
        /* Hier wird NUR entschieden, noch nicht eingereiht: das
         * Einreihen muss geschehen, NACHDEM die ASM-Seite den
         * Registersatz des Aufrufers gesichert und SavedSP aktualisiert
         * hat -- sonst traegt der wartende Prozess einen veralteten
         * Stackzeiger, und das Aufwachen landet im Nichts. Dafuer gibt
         * es Q9K_SysSemaWaitImpl unten. */
        Q9K_SetU32(Q9K_SEMA_SCRATCH_SLEEP, 1UL);
        Q9K_SetU32(Q9K_SEMA_SCRATCH_OK, 1UL);
        break;

    case Q9K_SEMA_V:
        /* Den ersten Warter wecken. Ob er den Semaphor dann WIRKLICH
         * bekommt, entscheidet sein eigenes "tas" nach dem Aufwachen --
         * der Kernel verspricht hier nichts (s. Kopfkommentar). Dass
         * niemand wartet, ist kein Fehler: zwischen dem s_lock-Test des
         * Aufrufers und diesem Aufruf kann sich die Lage geaendert haben. */
        {
            Q9_u32 waiter = Q9K_SemaDequeue(sema);
            if (waiter != 0UL) {
                Q9_u16 err = 0U;
                if (!Q9K_ProcAProc(waiter, &err)) {
                    Q9K_SetU32(Q9K_SEMA_SCRATCH_ERROR, (Q9_u32)err);
                    return;
                }
            }
            Q9K_SetU32(Q9K_SEMA_SCRATCH_OK, 1UL);
        }
        break;

    default:
        /* Die Bibliothek kennt nur 1 und 2; _os_sema_init und
         * _os_sema_term kommen ohne Kernel aus (sie setzen die Struktur
         * nur zurueck). Alles andere ist kein Semaphordienst. */
        Q9K_SetU32(Q9K_SEMA_SCRATCH_ERROR, Q9K_E_UNKSVC);
        break;
    }
}

/* Q9K_SysSemaWaitImpl -- zweiter Teil von P, aufgerufen von der ASM-Seite
 * erst NACH dem Sichern des Registersatzes.
 *
 * Der Wartende gehoert an die Semaphor-Warteschlange und NICHT in die
 * Schlafliste. Das ist kein Feinschliff, sondern noetig: beide verketten
 * ueber dasselbe Deskriptorfeld, ein Prozess in beiden zugleich zerstoert
 * beide Listen. Und inhaltlich ist er auch nicht schlafend -- er wartet,
 * bis jemand freigibt. Das Original fuehrt dafuer einen eigenen Zustand
 * ('p', s. process.a).
 *
 * Legt den naechsten lauffaehigen Prozess in Q9K_SEMA_SCRATCH_NEXT ab
 * (0 = keiner mehr bereit, dann bleibt der ASM-Seite nur K$Idle). */
void Q9K_SysSemaWaitImpl(void)
{
    Q9_u32 sema = Q9K_GetU32(Q9K_SEMA_SCRATCH_PTR);
    Q9_u32 self = Q9K_GetU32(Q9_D_PROC);

    Q9K_SemaEnqueue(sema, self);
    Q9K_SetU8(self + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_SEMA);
    Q9K_SetU32(Q9K_SEMA_SCRATCH_NEXT, Q9K_SchedFirstPick());
}
