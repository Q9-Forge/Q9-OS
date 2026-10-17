/*
 * q9kernel_setsys.c -- Q9-OS eigener Kernel: F$SetSys (Callcode 0x27,
 *                      intern dokumentiert, "F$SetSys").
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
 * Herleitung per Live-Analyse):
 *   IN  d0.l = Systemvariablen-Nummer
 *       d1.l = Bit 31 gesetzt = "lesen" (sonst "schreiben"), untere
 *              Bits = erwartete/gemeldete Groesse in Byte (1/2/4)
 *       d2.l = beim Schreiben: der zu setzende Wert
 *   OUT d2.l = beim Lesen (Erfolg): der gelesene Wert
 *       Carry im geretteten SR: 0 = Erfolg, 1 = Fehlschlag
 *       d1.w (Fehlschlag) = Fehlercode
 *
 * EIGENE ENTSCHEIDUNG, dokumentiert (gleiches Muster wie F$CCtl,
 * q9kernel_cinit.c): KEIN echtes, persistentes System-Global-Register
 * fuer beliebige Variablennummern implementiert -- dafuer fehlt die
 * vollstaendige Liste aller realen Variablennummern samt Bedeutung.
 * Die EINE live als Ausloeser gefundene Variable (124/$7C, die
 * csl-Speicherzuwachsgroesse) wird als Kernel-Global gespeichert und mit
 * einem sinnvollen Standardwert (4096 Byte, uebliche Seiten-/Blockgroesse)
 * initialisiert. Andere Variablen bleiben unbekannt.
 *
 * NACHTRAG 2026-09-13 (Fortsetzung 60, auf Wunsch robuster gemacht):
 * "Lesen" einer ANDEREN, unbekannten Variable liefert jetzt E$UnkSvc
 * ($D0, internem Referenzmaterial -- Position in der
 * Fehlercode-Tabelle rueckwaerts von den bereits bekannten Werten
 * E$ModBsy=$D1/E$BPAddr=$D2 gezaehlt, nicht geraten) statt still 0 UND
 * Erfolg vorzutaeuschen. Begruendung: ein stiller Falschwert ist
 * gefaehrlicher als ein sauberer Fehlschlag -- ein Aufrufer, der die
 * Carry-Flagge tatsaechlich prueft (anders als der hier gefundene),
 * kann auf einen klaren Fehler reagieren (z. B. einen eigenen
 * Standardwert benutzen), auf einen unbemerkt falschen Wert dagegen
 * nicht. Der EINE bekannte Aufrufer (der genau NICHT prueft) bleibt
 * durch die weiterhin erfolgreiche Variable $7C unveraendert bedient.
 */

typedef unsigned long Q9_u32;

/* C5 (2026-10-17): D_MinPty und D_MaxAge leben im Scheduler (Q9-eigene Felder, s. q9kernel_sched.c). */
extern Q9_u32 Q9K_SchedGetMinPty(void);
extern void   Q9K_SchedSetMinPty(Q9_u32 value);
extern Q9_u32 Q9K_SchedGetMaxAge(void);
extern void   Q9K_SchedSetMaxAge(Q9_u32 value);

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
/* NICHT bei $1650 fortsetzen -- das ist Q9K_VMODUL_RETBUF's Start
 * (q9kernel_moddir.c, $1650-$1663), s. Kollisionslehre Fortsetzung
 * 52/58 (immer den GESAMTEN belegten Bereich pruefen, nicht nur
 * Startadressen). Naechster wirklich freier Bereich: $1664-$168F (44
 * Byte, vor Q9K_TLinkScratch_* bei $1690). */
#ifndef Q9K_SetSysScratch_Error
#define Q9K_SetSysScratch_Error   0x1664UL   /* Q9_u32, d1.w AUS bei Fehlschlag */
#endif
#ifndef Q9K_SetSysScratch_Success
#define Q9K_SetSysScratch_Success 0x1668UL   /* Q9_u32, 0 = Fehlschlag / 1 = Erfolg */
#endif

#define Q9K_SETSYS_GETFLAG 0x80000000UL

/* System-global values with a Q9-owned implementation.  $7c is the value
 * used by csl; $28 and $76 are the kernel's tick-rate and scheduler quantum
 * and therefore have stable Q9 semantics as well. */
#define Q9K_SETSYS_VAR_MINPTY          0x55EUL   /* D_MinPty, Wort -- nach dem Handbuch veraenderbar */
#define Q9K_SETSYS_VAR_MAXAGE          0x560UL   /* D_MaxAge, Wort -- nach dem Handbuch veraenderbar */
#define Q9K_SETSYS_VAR_PROCSZ          0x3E2UL   /* D_ProcSz, Wort: Groesse eines Prozessdeskriptors */
#define Q9K_SETSYS_VAR_TSLICE_HDR      0x42EUL   /* D_TSlice laut q9sysglob.h; $76 bleibt der aeltere Code derselben Zelle */
#define Q9K_SETSYS_PROCSZ_VALUE        0x400UL
#define Q9K_SETSYS_USER_OFF            0x14UL    /* P$User im Prozessdeskriptor: Gruppe im oberen Wort */
#define Q9K_ERR_PERMIT                 0x00A4UL  /* E$Permit (000:164) */
#define Q9K_ERR_PARAM                  0x00E1UL  /* E$Param (000:225) */
#define Q9K_SETSYS_VAR_CSL_MALLOC_INCR 0x7CUL
#define Q9K_SETSYS_VAR_TCKSEC          0x28UL
#define Q9K_SETSYS_VAR_TSLICE          0x76UL
#define Q9K_SETSYS_DEFAULT_MALLOC_INCR 4096UL
#define Q9K_SETSYS_DEFAULT_TCKSEC      100UL
#define Q9K_SETSYS_DEFAULT_TSLICE      2UL

/* Verifizierte System-Globals, die F$SetSys schreibgeschuetzt lesen darf.
 * Q9K_SETSYS_ADDR ist fuer Host-Tests auf einen Fake-Globalbereich
 * umlenkbar; auf dem 68k-Ziel entspricht die Adresse dem A6-relativen
 * System-Global-Offset.  Die Tickzahl wird aus dem Q9-eigenen Zaehler im
 * Kernelmodul gelesen, weil D_Ticks im Globalbereich nicht separat gepflegt
 * wird. */
#ifndef Q9K_SETSYS_ADDR
#define Q9K_SETSYS_ADDR(offset) ((Q9_u32)(offset))
#endif
#define Q9K_SETSYS_VAR_INIT       0x20UL
#define Q9K_SETSYS_VAR_COMPAT     0x2EUL
#define Q9K_SETSYS_VAR_SYSCONF    0x38UL
#define Q9K_SETSYS_VAR_MODDIR     0x3CUL
#define Q9K_SETSYS_VAR_PROC       0x4CUL
#define Q9K_SETSYS_VAR_SYSPRC     0x50UL
#define Q9K_SETSYS_VAR_FPROC      0x58UL
#define Q9K_SETSYS_VAR_SYSROM     0x64UL
#define Q9K_SETSYS_VAR_EXCJMP     0x68UL
#define Q9K_SETSYS_VAR_TOTRAM     0x6CUL
#define Q9K_SETSYS_VAR_SYSDIS     0x3A4UL
#define Q9K_SETSYS_VAR_USRDIS     0x3A8UL
#define Q9K_SETSYS_VAR_COMPAT2    0x3E0UL
#define Q9K_SETSYS_VAR_TICKS      0x54UL
#define Q9K_SETSYS_TICKCOUNT_ADDR 0x1BE0UL

/* Persistent value for the one system variable currently used by csl. */
/* BSS-only storage: initialized data is not permitted in this OS-9 module. */
static Q9_u32 Q9K_SetSysMallocIncrement;
static Q9_u32 Q9K_SetSysTickSeconds;
static Q9_u32 Q9K_SetSysTimeSlice;

static int Q9K_ReadSetSysReadonly(Q9_u32 varCode, Q9_u32 *outValue)
{
    Q9_u32 address;

    switch (varCode) {
    case Q9K_SETSYS_VAR_INIT:    address = Q9K_SETSYS_VAR_INIT; break;
    case Q9K_SETSYS_VAR_COMPAT:  address = Q9K_SETSYS_VAR_COMPAT; break;
    case Q9K_SETSYS_VAR_SYSCONF: address = Q9K_SETSYS_VAR_SYSCONF; break;
    case Q9K_SETSYS_VAR_MODDIR:  address = Q9K_SETSYS_VAR_MODDIR; break;
    case Q9K_SETSYS_VAR_PROC:    address = Q9K_SETSYS_VAR_PROC; break;
    case Q9K_SETSYS_VAR_SYSPRC:  address = Q9K_SETSYS_VAR_SYSPRC; break;
    case Q9K_SETSYS_VAR_FPROC:   address = Q9K_SETSYS_VAR_FPROC; break;
    case Q9K_SETSYS_VAR_SYSROM:  address = Q9K_SETSYS_VAR_SYSROM; break;
    case Q9K_SETSYS_VAR_EXCJMP:  address = Q9K_SETSYS_VAR_EXCJMP; break;
    case Q9K_SETSYS_VAR_TOTRAM:  address = Q9K_SETSYS_VAR_TOTRAM; break;
    case Q9K_SETSYS_VAR_SYSDIS:  address = Q9K_SETSYS_VAR_SYSDIS; break;
    case Q9K_SETSYS_VAR_USRDIS:  address = Q9K_SETSYS_VAR_USRDIS; break;
    case Q9K_SETSYS_VAR_COMPAT2: address = Q9K_SETSYS_VAR_COMPAT2; break;
    case Q9K_SETSYS_VAR_TICKS:
        *outValue = Q9K_GetU32(Q9K_SETSYS_ADDR(Q9K_SETSYS_TICKCOUNT_ADDR));
        return 1;
    default:
        return 0;
    }

    *outValue = Q9K_GetU32(Q9K_SETSYS_ADDR(address));
    return 1;
}

static Q9_u32 Q9K_GetSetSysMallocIncrement(void)
{
    if (Q9K_SetSysMallocIncrement == 0UL)
        Q9K_SetSysMallocIncrement = Q9K_SETSYS_DEFAULT_MALLOC_INCR;
    return Q9K_SetSysMallocIncrement;
}

static Q9_u32 Q9K_GetSetSysTickSeconds(void)
{
    if (Q9K_SetSysTickSeconds == 0UL)
        Q9K_SetSysTickSeconds = Q9K_SETSYS_DEFAULT_TCKSEC;
    return Q9K_SetSysTickSeconds;
}

static Q9_u32 Q9K_GetSetSysTimeSlice(void)
{
    if (Q9K_SetSysTimeSlice == 0UL)
        Q9K_SetSysTimeSlice = Q9K_SETSYS_DEFAULT_TSLICE;
    return Q9K_SetSysTimeSlice;
}

/* E$UnkSvc -- s. Kopfkommentar. */
#define Q9K_ERR_UNKSVC 0x00D0UL

/* C5: Ausgabewert auf die gemeldete Groesse kuerzen (1, 2 oder 4 Byte). */
static Q9_u32 Q9K_SetSysTruncate(Q9_u32 value, Q9_u32 size)
{
    if (size == 1UL) return value & 0xFFUL;
    if (size == 2UL) return value & 0xFFFFUL;
    return value & 0xFFFFFFFFUL;
}

/* C5: darf der Aufrufer System-Globals aendern? 68k_tech.pdf S. 493: "Only a super-user can change system variables" -- Gruppe 0
 * (S. 91: "Users with a group ID of zero (super users)"). Ohne laufenden Prozess (Kernelkontext) ja. */
static int Q9K_SetSysCallerIsSuper(void)
{
    Q9_u32 desc = Q9K_GetU32(Q9K_SETSYS_ADDR(Q9K_SETSYS_VAR_PROC));

    if (desc == 0UL)
        return 1;
    return (Q9K_GetU32(desc + Q9K_SETSYS_USER_OFF) & 0xFFFF0000UL) == 0UL;
}

/* Q9K_ProcSetSys -- echte F$SetSys-Kernlogik (s. Kopfkommentar).
 *
 * C5 (2026-10-17), gegen das Handbuch (68k_tech.pdf S. 492 f.) nachgezogen:
 *   - d1.w ist die Groesse (1, 2 oder 4 Byte); 0 wird wie bisher als 4 gelesen (aeltere Aufrufer), alles andere ist E$Param.
 *   - Bit 31 in d1 = Lesen. Die Ausgabe d2 ist der VORHERIGE Wert -- auch beim Schreiben ("d2.l = original value"). Frueher kam
 *     beim Schreiben der neue Wert zurueck.
 *   - Schreiben darf nur ein Super-User (Gruppe 0), sonst E$Permit ($A4). Nur D_MinPty, D_MaxAge und die drei Q9-Werte unten sind
 *     veraenderbar ("The only useful variables that may be changed are D_MinPty and D_MaxAge"); jede andere bekannte Variable meldet
 *     beim Schreiben E$Param, eine unbekannte E$UnkSvc.
 *   - Lesen: nur Variablen, die dieser Kernel wirklich fuehrt (docs/KERNEL_SETSYS_VARIABLEN_de.md); sonst E$UnkSvc -- ein ehrlicher
 *     Fehlschlag statt eines geratenen Werts. Das ist eine bewusste Abweichung vom Handbuch ("Any system variable may be examined"),
 *     weil die echten Offsets in diesem System auf fremdem Speicher liegen (live gemessen: $55E ist nicht D_MinPty).
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
int Q9K_ProcSetSys(Q9_u32 varCode, Q9_u32 flags, Q9_u32 valueIn, Q9_u32 *outValue, Q9_u32 *outError)
{
    Q9_u32 size = flags & 0xFFFFUL;
    Q9_u32 old = 0UL;
    int known = 1;
    int writable = 0;

    *outError = 0UL;
    *outValue = 0UL;

    if (size == 0UL)
        size = 4UL;
    if (size != 1UL && size != 2UL && size != 4UL) {
        *outError = Q9K_ERR_PARAM;
        return 0;
    }

    /* aktueller Wert der Variable; writable = der Wert darf geaendert werden */
    if (varCode == Q9K_SETSYS_VAR_CSL_MALLOC_INCR) {
        old = Q9K_GetSetSysMallocIncrement(); writable = 1;
    } else if (varCode == Q9K_SETSYS_VAR_TCKSEC) {
        old = Q9K_GetSetSysTickSeconds(); writable = 1;
    } else if (varCode == Q9K_SETSYS_VAR_TSLICE || varCode == Q9K_SETSYS_VAR_TSLICE_HDR) {
        old = Q9K_GetSetSysTimeSlice(); writable = 1;
    } else if (varCode == Q9K_SETSYS_VAR_MINPTY) {
        old = Q9K_SchedGetMinPty(); writable = 1;
    } else if (varCode == Q9K_SETSYS_VAR_MAXAGE) {
        old = Q9K_SchedGetMaxAge(); writable = 1;
    } else if (varCode == Q9K_SETSYS_VAR_PROCSZ) {
        old = Q9K_SETSYS_PROCSZ_VALUE;
    } else if (Q9K_ReadSetSysReadonly(varCode, &old)) {
        /* Verifizierte System-Globals: lesbar, nicht veraenderbar. */
    } else {
        known = 0;
    }

    if (!known) {
        *outError = Q9K_ERR_UNKSVC;
        return 0;
    }

    if ((flags & Q9K_SETSYS_GETFLAG) != 0UL) {
        *outValue = Q9K_SetSysTruncate(old, size);
        return 1;
    }

    /* Schreiben */
    if (!Q9K_SetSysCallerIsSuper()) {
        *outError = Q9K_ERR_PERMIT;
        return 0;
    }
    if (!writable) {
        *outError = Q9K_ERR_PARAM;
        return 0;
    }
    valueIn = Q9K_SetSysTruncate(valueIn, size);
    if ((varCode == Q9K_SETSYS_VAR_CSL_MALLOC_INCR || varCode == Q9K_SETSYS_VAR_TCKSEC ||
         varCode == Q9K_SETSYS_VAR_TSLICE || varCode == Q9K_SETSYS_VAR_TSLICE_HDR) && valueIn == 0UL) {
        *outError = Q9K_ERR_UNKSVC;    /* wie bisher: 0 ist fuer diese Werte unsinnig */
        return 0;
    }
    if (varCode == Q9K_SETSYS_VAR_CSL_MALLOC_INCR)
        Q9K_SetSysMallocIncrement = valueIn;
    else if (varCode == Q9K_SETSYS_VAR_TCKSEC)
        Q9K_SetSysTickSeconds = valueIn;
    else if (varCode == Q9K_SETSYS_VAR_TSLICE || varCode == Q9K_SETSYS_VAR_TSLICE_HDR)
        Q9K_SetSysTimeSlice = valueIn;
    else if (varCode == Q9K_SETSYS_VAR_MINPTY)
        Q9K_SchedSetMinPty(valueIn);
    else if (varCode == Q9K_SETSYS_VAR_MAXAGE)
        Q9K_SchedSetMaxAge(valueIn);
    *outValue = Q9K_SetSysTruncate(old, size);          /* "d2.l = original value" */
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
    Q9_u32 error   = 0UL;
    int ok;

    ok = Q9K_ProcSetSys(varCode, flags, valueIn, &result, &error);
    Q9K_SetU32(Q9K_SetSysScratch_Value, result);
    Q9K_SetU32(Q9K_SetSysScratch_Error, error);
    Q9K_SetU32(Q9K_SetSysScratch_Success, (Q9_u32)ok);
}
