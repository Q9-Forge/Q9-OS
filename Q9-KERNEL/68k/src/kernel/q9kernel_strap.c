/*
 * q9kernel_strap.c -- Q9-OS eigener Kernel: F$STrap (Callcode $0E,
 *                     2026-09-19) -- prozesseigene Behandler fuer
 *                     Programmfehler-Ausnahmen.
 *
 * Verifizierte ABI (68k_tech.pdf S.513):
 *
 *   EIN  (a0) = Stack, der im Ausnahmefall benutzt werden soll
 *              (oder 0 = der aktuelle Stack)
 *        (a1) = Zeiger auf eine Initialisierungstabelle
 *   AUS  keine; bei Fehler Carry und d1.w
 *
 * Die Tabelle besteht aus Wortpaaren und endet mit -1. Das Handbuch zeigt
 * sie so:
 *
 *     ExcpTbl  dc.w  T_TRAPV,OvfError-*-4
 *              dc.w  T_CHK,CHKError-*-4
 *              dc.w  -1               End of Table
 *
 * Das erste Wort ist der Ausnahmeeintrag als BYTE-OFFSET in der
 * CPU-Vektortabelle (T_BusErr = 8, T_IllIns = 16, ... -- s.
 * MWOS/OS9/SRC/DEFS/sysglob.a, wo sie mit "org 0 / do.l 1" je Vektor
 * definiert sind, die Nummer also mal vier). Das zweite ist ein
 * PC-RELATIVER Abstand zur Behandlerroutine: der Assemblerausdruck
 * "Routine-*-4" bedeutet, dass die Routine bei (Adresse des Paares) + 4 +
 * Abstand liegt. Dadurch ist die Tabelle verschieblich -- was sie sein
 * muss, denn sie steht im Programmmodul, das an beliebiger Adresse
 * geladen wird.
 *
 * WOHIN DIE BEHANDLER KOMMEN: in den Prozessdeskriptor, Feld P$Except --
 * "Program error exception vectors", zehn Langworte (process.a). Die
 * Zuordnung ist Index = Vektornummer - 2, also Bus Error (2) auf Index 0
 * bis Line-1111-Emulator (11) auf Index 9. Genau diese zehn Ausnahmen
 * zaehlt das Handbuch als abfangbar auf ("Bus error, Address error,
 * Illegal instruction, Zero Divide, CHK, TRAPV, Privilege violation,
 * Line 1010, Line 1111") -- mit Trace (9) dazwischen sind es die zehn
 * Eintraege, die P$Except vorsieht.
 *
 * Die Feldadresse ist abgeleitet, nicht geraten: process.a listet die
 * Felder luecklos, und zwei davon sind in diesem Kernel unabhaengig
 * belegt (P$SigVec $28, P$PModul $38). Zaehlt man von dort weiter, faellt
 * P$Except auf $3C und P$ExStk auf $64 -- beide Ankerwerte stimmen, die
 * Ableitung dazwischen also auch.
 *
 * WIE DER SPRUNG PASSIERT, und warum das so wenig Code ist: tritt die
 * Ausnahme ein, liegt auf dem Stack bereits der 68030-Ausnahmerahmen mit
 * SR, PC und Format-/Vektorwort. Der Kernel muss den PC in diesem Rahmen
 * nur durch die Behandleradresse ersetzen -- das anschliessende "rte"
 * springt dann dorthin, mit unveraendertem Registersatz. Der Behandler
 * sieht also genau den Zustand, in dem der Fehler auftrat, was fuer eine
 * Fehlerbehandlung der einzig brauchbare Zustand ist.
 *
 * NICHT UMGESETZT: der eigene Ausnahmestack aus (a0). P$ExStk ist dafuer
 * da, und der Wert wird auch eingetragen, aber der Sprung benutzt
 * weiterhin den laufenden Stack. Ein Stackwechsel mitten im
 * Ausnahmerahmen will sorgfaeltiger geprueft sein, als ohne einen
 * Aufrufer moeglich ist, der ihn wirklich braucht; (a0) = 0 ist der
 * dokumentierte Normalfall und funktioniert vollstaendig.
 *
 * KEINE RUECKKEHR: Das Handbuch nennt keinen Weg aus einem
 * Ausnahmebehandler zurueck in den Code, der den Fehler ausgeloest hat --
 * anders als bei Signalen, wo F$RTE genau das leistet. Ein Behandler
 * beendet den Prozess oder springt per longjmp() an eine sichere Stelle.
 * Dieser Kernel macht es ebenso: er springt hinein und ueberlaesst dem
 * Behandler, wie es weitergeht.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

#ifndef Q9K_E_BPADDR
#define Q9K_E_BPADDR 0xD2U
#endif

#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif

/* P$Except / P$ExStk -- s. Kopfkommentar zur Herleitung. */
#ifndef Q9K_PROCDESC_EXCEPT_OFF
#define Q9K_PROCDESC_EXCEPT_OFF 0x3CUL   /* 10 Langworte: Behandleradressen */
#define Q9K_PROCDESC_EXSTK_OFF  0x64UL   /* 10 Langworte: Stackzeiger dazu  */
#endif
#define Q9K_EXCEPT_SLOTS 10UL
/* Abstand zweier Tabellenplaetze. Real vier Byte; per #ifndef
 * ueberschreibbar, aus dem ueblichen Grund: auf einem 64-Bit-Testhost
 * schreibt Q9K_SetU32 acht Byte und griffe sonst in den naechsten Platz
 * hinein -- im Hosttest real beobachtet, bevor dieser Schalter da war. */
#ifndef Q9K_EXCEPT_STRIDE
#define Q9K_EXCEPT_STRIDE 4UL
#endif

/* Erster und letzter abfangbarer CPU-Vektor (Bus Error bis Line 1111). */
#define Q9K_EXCEPT_FIRST_VECTOR 2UL
#define Q9K_EXCEPT_LAST_VECTOR  11UL

/* Scratch-Bruecke, hinter F$RTE/F$SigReset ($1BD0-$1BDB). */
#ifndef Q9K_STRAP_SCRATCH_STACK
#define Q9K_STRAP_SCRATCH_STACK 0x1BE4UL /* Q9_u32, (a0) EIN                 */
#define Q9K_STRAP_SCRATCH_TABLE 0x1BE8UL /* Q9_u32, (a1) EIN                 */
#define Q9K_STRAP_SCRATCH_ERROR 0x1BECUL /* Q9_u32, d1.w AUS bei Fehler      */
#define Q9K_STRAP_SCRATCH_OK    0x1BF0UL /* Q9_u32, 0/1                      */
#define Q9K_STRAP_SCRATCH_COUNT 0x1BF4UL /* Q9_u32, eingetragene Behandler   */
#define Q9K_STRAP_SCRATCH_VEC   0x1BF8UL /* Q9_u32, Vektornummer EIN (Dispatch) */
#define Q9K_STRAP_SCRATCH_JUMP  0x1BFCUL /* Q9_u32, Sprungziel AUS (0 = keins)  */
#endif

#ifndef Q9K_CELL_ACCESSORS_PROVIDED
static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
#endif
#ifndef Q9K_STRAP_WORD_ACCESSORS
static Q9_u16 Q9K_GetU16(Q9_u32 addr) { return *(volatile Q9_u16 *)addr; }
#endif

/* Q9K_StrapInstall -- die Initialisierungstabelle auswerten und die
 * Behandler in P$Except eintragen.
 *
 * Rueckgabe: Zahl der eingetragenen Behandler, oder -1 bei einem
 * fehlerhaften Eintrag. "If an entry for a particular routine already
 * exists, it is replaced" -- deshalb wird ohne Rueckfrage ueberschrieben.
 *
 * Die Tabelle endet bei -1. Eine fehlende Endmarkierung waere ein Lesen
 * ins Blaue, deshalb ist die Zahl der Eintraege zusaetzlich begrenzt:
 * mehr als die zehn moeglichen Ausnahmen (jede einmal) kann eine sinnvolle
 * Tabelle nicht haben, und ein Vielfaches davon deutet auf eine kaputte
 * Tabelle hin. */
long Q9K_StrapInstall(Q9_u32 desc, Q9_u32 tableAddr, Q9_u32 stackAddr)
{
    Q9_u32 entry = tableAddr;
    long installed = 0;
    Q9_u32 guard;

    if (desc == 0UL || tableAddr == 0UL)
        return -1;

    for (guard = 0UL; guard < 4UL * Q9K_EXCEPT_SLOTS; guard++) {
        Q9_u16 code = Q9K_GetU16(entry);
        Q9_u16 rel;
        Q9_u32 vector, slot, handler;

        if (code == 0xFFFFU)
            break;                        /* -1: Ende der Tabelle */

        rel = Q9K_GetU16(entry + 2UL);

        /* Der Eintrag ist ein Byte-Offset in die Vektortabelle; vier Byte
         * je Vektor. Ein ungerader oder unpassender Wert ist keine
         * Ausnahme, die dieser Kernel kennt. */
        if ((code & 3U) != 0U)
            return -1;
        vector = (Q9_u32)code / 4UL;
        if (vector < Q9K_EXCEPT_FIRST_VECTOR || vector > Q9K_EXCEPT_LAST_VECTOR)
            return -1;
        slot = vector - Q9K_EXCEPT_FIRST_VECTOR;

        /* PC-relativ, vorzeichenbehaftet: "Routine-*-4" heisst, dass die
         * Routine vier Byte hinter dem Anfang des Paares plus diesem
         * Abstand liegt. */
        handler = entry + 4UL + (Q9_u32)(long)(short)rel;

        Q9K_SetU32(desc + Q9K_PROCDESC_EXCEPT_OFF + slot * Q9K_EXCEPT_STRIDE, handler);
        Q9K_SetU32(desc + Q9K_PROCDESC_EXSTK_OFF + slot * Q9K_EXCEPT_STRIDE, stackAddr);
        installed++;
        entry += 4UL;
    }

    return installed;
}

/* Q9K_StrapHandlerFor -- den eingetragenen Behandler einer Ausnahme
 * nachschlagen; 0, wenn der Prozess keinen hat (dann bleibt es beim
 * bisherigen Verhalten: der Kernel haelt mit seiner Diagnose an). */
Q9_u32 Q9K_StrapHandlerFor(Q9_u32 desc, Q9_u32 vector)
{
    if (desc == 0UL)
        return 0UL;
    if (vector < Q9K_EXCEPT_FIRST_VECTOR || vector > Q9K_EXCEPT_LAST_VECTOR)
        return 0UL;
    return Q9K_GetU32(desc + Q9K_PROCDESC_EXCEPT_OFF
                      + (vector - Q9K_EXCEPT_FIRST_VECTOR) * Q9K_EXCEPT_STRIDE);
}

/* --- Bruecken ---------------------------------------------------- */

void Q9K_SysSTrapImpl(void)
{
    long n = Q9K_StrapInstall(Q9K_GetU32(Q9_D_PROC),
                              Q9K_GetU32(Q9K_STRAP_SCRATCH_TABLE),
                              Q9K_GetU32(Q9K_STRAP_SCRATCH_STACK));

    if (n < 0) {
        Q9K_SetU32(Q9K_STRAP_SCRATCH_ERROR, Q9K_E_BPADDR);
        Q9K_SetU32(Q9K_STRAP_SCRATCH_OK, 0UL);
        return;
    }
    Q9K_SetU32(Q9K_STRAP_SCRATCH_COUNT, (Q9_u32)n);
    Q9K_SetU32(Q9K_STRAP_SCRATCH_OK, 1UL);
}

/* Q9K_SysExcDispatchImpl -- dieselbe Auswahl, die Q9K_ExcTrap trifft,
 * als C-Funktion.
 *
 * DER KERNEL RUFT SIE NICHT: Q9K_ExcTrap schlaegt den Behandler in
 * Assembler nach (wenige Zeilen, s. dort). Ein erster Entwurf rief von
 * dort diese Funktion -- und der Aufruf kehrte nicht zurueck. Der Grund
 * steht im Kopfkommentar von Q9K_ExcTrap: der Compiler stellt jeder
 * C-Funktion einen Stack-Check-Prolog voran, und aus dem Ausnahmekontext
 * heraus traegt der nicht. Genau deshalb ist jener Handler ueberhaupt
 * reiner Assembler.
 *
 * Sie bleibt trotzdem stehen, weil der Hosttest die Auswahllogik damit
 * pruefen kann, ohne Assembler auszufuehren -- und weil beide Seiten
 * dieselbe Zuordnung benutzen muessen, ist sie hier die lesbare
 * Fassung der Regel. */
void Q9K_SysExcDispatchImpl(void)
{
    Q9K_SetU32(Q9K_STRAP_SCRATCH_JUMP,
               Q9K_StrapHandlerFor(Q9K_GetU32(Q9_D_PROC),
                                   Q9K_GetU32(Q9K_STRAP_SCRATCH_VEC)));
}
