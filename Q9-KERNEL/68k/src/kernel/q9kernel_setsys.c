/*
 * q9kernel_setsys.c -- Q9-OS eigener Kernel: F$SetSys (Callcode 0x27,
 *                      s. modules/SYSCALL_MODULE_MAP.md und
 *                      docs/REVERSE_ENGINEERING.md "F$SetSys").
 *
 * NACHTRAG 2026-09-13 (Fortsetzung 58, direkter Anschluss an den
 * csl-Freilisten-Patch, s. q9kernel_traplink.c
 * Q9K_PatchCslFreelistBug): live gefunden, NICHT im Handbuch
 * nachgeschlagen -- csl.mod's malloc()-Wachstumslogik (Dateiversatz
 * $44a06, "einmalige Initialisierung beim ersten Aufruf") fragt per
 * einem internen Wrapper (Dateiversatz $49d18) eine Systemvariable ab,
 * um die minimale Speicherblock-Zuwachsgroesse zu bestimmen. Dieser
 * Wrapper macht "trap #0" mit Callcode $27 -- bei Q9-OS bisher
 * VOELLIG UNIMPLEMENTIERT (kein Eintrag in q9kernel_cinit.c). Der
 * Wrapper erkennt den daraus resultierenden Fehlschlag zwar korrekt
 * (Carry gesetzt) und liefert einen Fehlercode, ABER der AUFRUFER
 * (die $44a06-Routine) prueft das Ergebnis NICHT und liest die lokale
 * Ausgabevariable trotzdem -- die bleibt dadurch uninitialisiert
 * (praktisch: 0). Das fuehrt wenig spaeter zu einer Division durch
 * genau diesen (fuer eine Zuwachsgroesse voellig unsinnigen) Wert 0
 * -- Vektor 5 (Zero Divide), live reproduziert.
 *
 * Reale Konvention EMPIRISCH aus dem aufrufenden Code hergeleitet
 * (kein Zugriff auf den exakten 68k_tech.pdf-Abschnitt in dieser
 * Sitzung -- anders als bei den anderen Syscalls in diesem Kernel
 * also NICHT woertlich aus dem Handbuch zitiert, s. docs/
 * OWN_KERNEL_STATUS.md Fortsetzung 58 fuer die vollstaendige
 * Herleitung per Live-Disassemblierung):
 *   IN  d0.l = Systemvariablen-Nummer
 *       d1.l = Bit 31 gesetzt = "lesen" (sonst "schreiben"), untere
 *              Bits = erwartete/gemeldete Groesse in Byte (1/2/4)
 *       d2.l = beim Schreiben: der zu setzende Wert
 *   OUT d2.l = beim Lesen: der gelesene Wert
 *       Carry im geretteten SR: 0 = Erfolg (bei uns IMMER, s. u.)
 *
 * EIGENE ENTSCHEIDUNG, dokumentiert (gleiches Muster wie F$CCtl,
 * q9kernel_cinit.c): KEIN echtes, persistentes System-Global-Register
 * fuer beliebige Variablennummern implementiert -- dafuer fehlt die
 * vollstaendige Liste aller realen Variablennummern samt Bedeutung.
 * "Schreiben" wird bestaetigt (Carry geloescht), aber NICHT
 * gespeichert. "Lesen" liefert fuer die EINE live als Ausloeser
 * gefundene Variable (124/$7C, die csl-Speicherzuwachsgroesse) einen
 * sinnvollen Standardwert (4096 Byte, uebliche Seiten-/Blockgroesse);
 * fuer jede andere, (noch) unbekannte Variable 0 -- BEWUSST nicht
 * einfach "Fehler melden", weil unklar ist, ob und wie andere
 * Aufrufer (anders als der hier gefundene) das Fehlschlagen pruefen.
 */

typedef unsigned long Q9_u32;

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }

/* Wie Q9K_SetSysScratch_* per #define VOR dem #include auf einen
 * echten Testpuffer umlenkbar -- gleiches, im ganzen Kernel etabliertes
 * Muster (s. z. B. q9kernel_ssvc.c). */
#ifndef Q9K_SetSysScratch_VarCode
#define Q9K_SetSysScratch_VarCode 0x1644UL   /* Q9_u32, d0.l EIN */
#endif
#ifndef Q9K_SetSysScratch_Flags
#define Q9K_SetSysScratch_Flags   0x1648UL   /* Q9_u32, d1.l EIN */
#endif
#ifndef Q9K_SetSysScratch_Value
#define Q9K_SetSysScratch_Value   0x164CUL   /* Q9_u32, d2.l EIN (Schreiben)/AUS (Lesen) */
#endif

#define Q9K_SETSYS_GETFLAG 0x80000000UL

/* Bislang die EINZIGE bekannte, live gebrauchte Variable -- s.
 * Kopfkommentar. Name/Nummer NICHT aus dem Handbuch, sondern aus dem
 * Aufrufer (csl.mod $44a1a: "moveq #$7c,d0") uebernommen. */
#define Q9K_SETSYS_VAR_CSL_MALLOC_INCR 0x7CUL
#define Q9K_SETSYS_DEFAULT_MALLOC_INCR 4096UL

/* Q9K_ProcSetSys -- echte F$SetSys-Kernlogik (s. Kopfkommentar).
 * Rueckgabe immer 1 (Erfolg) -- s. dortige Begruendung. */
int Q9K_ProcSetSys(Q9_u32 varCode, Q9_u32 flags, Q9_u32 valueIn, Q9_u32 *outValue)
{
    if ((flags & Q9K_SETSYS_GETFLAG) != 0) {
        *outValue = (varCode == Q9K_SETSYS_VAR_CSL_MALLOC_INCR)
                        ? Q9K_SETSYS_DEFAULT_MALLOC_INCR
                        : 0UL;
    } else {
        *outValue = valueIn;   /* bestaetigt, aber nicht gespeichert (s. Kopfkommentar) */
    }
    return 1;
}

/* Q9K_SysSetSysImpl -- duenne, PARAMETERLOSE Bruecke zwischen dem
 * Assembler-Trampolin und Q9K_ProcSetSys -- gleiches Muster wie
 * ueberall (Q9K_SysSSvcImpl usw.). */
void Q9K_SysSetSysImpl(void)
{
    Q9_u32 varCode = Q9K_GetU32(Q9K_SetSysScratch_VarCode);
    Q9_u32 flags   = Q9K_GetU32(Q9K_SetSysScratch_Flags);
    Q9_u32 valueIn = Q9K_GetU32(Q9K_SetSysScratch_Value);
    Q9_u32 result  = 0UL;

    Q9K_ProcSetSys(varCode, flags, valueIn, &result);
    Q9K_SetU32(Q9K_SetSysScratch_Value, result);
}
