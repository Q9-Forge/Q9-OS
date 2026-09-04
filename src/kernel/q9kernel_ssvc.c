/*
 * q9kernel_ssvc.c -- Q9-OS eigener Kernel: F$SSvc (Abschnitt "IOMan-
 *                    Einbindung" 2026-08-31, im Anschluss an die CCR-/
 *                    A6-Umbauten -- IOMans ALLERERSTER Aufruf beim
 *                    Booten, echter Blocker fuer jeden weiteren
 *                    Fortschritt).
 *
 * Reale Register-/Verhaltenskonvention ECHT per Read gelesen (nicht
 * geraten), 68k_tech.pdf S. 508-510 (Callcode 0x32, s.
 * modules/SYSCALL_MODULE_MAP.md -- ZUSAETZLICH real per echtem
 * Kernel-Adress-Dump bestaetigt, docs/REVERSE_ENGINEERING.md "Komplette
 * Syscall-Tabelle": D_SysDis-Eintrag hat eine ECHTE Adresse (kein
 * Fehler-Stub), D_UsrDis-Eintrag ist der Fehler-Stub -- "nur
 * Supervisor-Tabelle", passt exakt zur dokumentierten "Attributes:
 * State: System"):
 *
 *   IN  (a1) = Zeiger auf eine Service-Request-Initialisierungstabelle
 *       (a3) = "user defined" globaler Datenzeiger
 *   OUT keine
 *   Fehler: cc=Carry, d1.w=Fehlercode (im Manual keine konkrete Liste
 *       genannt -- diese Implementierung schlaegt praktisch nie fehl,
 *       s. u.)
 *
 * Tabellenformat (woertlich aus dem Manual, Table-D-Beispiel):
 *   dc.w F$Service              ; Funktionscode (0-255)
 *   dc.w Routine-*-2            ; Offset zur Routine
 *     :
 *   dc.w F$Service+SysTrap      ; "Redefine system level request"
 *   dc.w SysRoutn-*-2
 *     :
 *   dc.w -1                     ; Tabellenende
 *
 * "The offset desired is the offset from the beginning of the table
 * entry to the routine minus 4. The minus 4 is to counteract
 * incrementing done in the kernel." -- d. h. gespeicherter Wert =
 * (RoutineAdresse - EintragAdresse) - 4, wobei EintragAdresse die
 * Adresse des CODE-Worts (nicht des Offset-Worts) ist. Umgekehrt beim
 * Registrieren: RoutineAdresse = EintragAdresse + gespeicherter Wert + 4.
 *
 * "If the sign bit of the function code word is set, only the system
 * table is updated. Otherwise, both the system and user tables are
 * updated." -- SysTrap = Bit 15 des Codeworts, realer Code selbst passt
 * in die unteren 8 Bit (0-255).
 *
 * "(a3) is intended to point to global static storage. This allows a
 * global data pointer to be associated with each installed system call.
 * When the system call is invoked, the data pointer is automatically
 * passed." -- LOEST das schon in der vorigen IOMan-Recherche-Runde
 * offen gelassene Raetsel um die "zweite 0x400-Byte-Haelfte" jeder
 * Syscall-Tabelle (s. q9kernel_entry.a Kopfkommentar
 * Q9K_TrapDispatch/IOMan-Trampolin-Fund): das ist genau dieser
 * per-Slot-(a3)-Datenzeiger, den F$SSvc hier schreibt und den ein
 * spaeterer Aufruf (egal ob echtes TRAP #0 oder IOMans eigener
 * A6-relativer Direktsprung) automatisch nach A3 laedt, bevor die
 * registrierte Routine erreicht wird.
 *
 * EIGENE ENTSCHEIDUNG, dokumentiert: real wird F$SSvc NUR in D_SysDis
 * registriert ("nur Supervisor-Tabelle", s. o.) -- unser eigener
 * Q9K_TrapDispatch unterscheidet aber (bekanntes, bereits an anderer
 * Stelle dokumentiertes TODO: "wir haben noch keine echte User-/
 * Supervisor-Prozesstrennung") NICHT zwischen den beiden Tabellen,
 * benutzt fuer JEDEN TRAP #0 IMMER Q9_D_USRDIS. Deshalb wird F$SSvc
 * SELBST (der Syscall, der F$SSvc bedient) in q9kernel_cinit.c bewusst
 * in BEIDE Tabellen eingetragen (wie alle unsere anderen Syscalls
 * bisher) -- sonst waere IOMans echter TRAP-#0-Aufruf (der bei uns
 * IMMER ueber USRDIS laeuft) gar nicht erreichbar. F$SSvc SELBST
 * registriert die IHM uebergebenen Eintraege dagegen weiterhin real-
 * konform (SysDis immer, UsrDis nur ohne SysTrap-Bit) -- das betrifft
 * die NEU registrierten Dienste, nicht F$SSvc als Aufruf selbst.
 *
 * Keine Fehlerpfad-Pruefung von (a1)/(a3) -- ein ungueltiger Zeiger
 * wuerde ohnehin nur die Tabellen korrumpieren, gleiche Begruendung wie
 * bei F$SRtMem (kein verstecktes Validierungs-Framework vorhanden).
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;

#ifndef Q9_D_USRDIS
#define Q9_D_USRDIS 0x3A8UL
#endif
#ifndef Q9_D_SYSDIS
#define Q9_D_SYSDIS 0x3A4UL
#endif

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }

/* Byteweise Grossgeschriebenes Lesen aus FREMDEN Moduldaten (IOMans
 * eigene Tabelle) -- gleiche Konvention/Begruendung wie ueberall in
 * diesem Kernel (Host-/Ziel-Endianness- und Alignment-Unabhaengigkeit,
 * s. z. B. q9kernel_moddir.c Q9K_ReadU32BE). */
static Q9_u16 Q9K_ReadHdrU16BE(Q9_u32 addr)
{
    const unsigned char *p = (const unsigned char *)addr;
    return (Q9_u16)((p[0] << 8) | p[1]);
}

/* Q9K_ProcSSvc -- echte F$SSvc-Kernlogik (s. Kopfkommentar). Wandert
 * durch die Tabelle bei tablePtr bis zum Endmarker (Codewort $ffff = -1
 * als Q9_u16), registriert jeden Eintrag in Q9_D_SYSDIS (immer) und
 * Q9_D_USRDIS (nur ohne SysTrap-Bit). */
/* Markierungstabelle "dieser Callcode wurde per F$SSvc EXTERN registriert",
 * ein Byte je Callcode (256 Byte, im genullten Global-Bereich). Externe
 * Module wie IOMan setzen die OS-9-Konvention "A4 = aktueller
 * Prozessdeskriptor" beim Handler-Eintritt voraus; unsere eigenen Handler
 * nicht. Q9K_TrapDispatch (q9kernel_entry.a) liest diese Tabelle, um A4
 * gezielt nur fuer die externen Handler umzusetzen. */
/* Wie Q9_D_SYSDIS/Q9_D_USRDIS per #define VOR dem #include auf einen
 * echten Testpuffer umlenkbar -- sonst wuerde der Hosttest auf die feste
 * Zieladresse schreiben und abstuerzen. */
#ifndef Q9K_SSVC_EXTERNAL_BASE
#define Q9K_SSVC_EXTERNAL_BASE 0x1400UL
#endif

void Q9K_ProcSSvc(Q9_u32 tablePtr, Q9_u32 dataPtr)
{
    Q9_u32 sysdisBase = Q9K_GetU32(Q9_D_SYSDIS);
    Q9_u32 usrdisBase = Q9K_GetU32(Q9_D_USRDIS);
    Q9_u32 entryAddr = tablePtr;

    for (;;) {
        Q9_u16 codeword = Q9K_ReadHdrU16BE(entryAddr);
        Q9_u16 rawOffset;
        Q9_u32 signExtOffset;
        Q9_u32 routineAddr;
        Q9_u32 realCode;
        int sysTrapOnly;

        if (codeword == 0xFFFFU)   /* -1 = Tabellenende */
            break;

        rawOffset = Q9K_ReadHdrU16BE(entryAddr + 2);
        /* Vorzeichenerweiterung des 16-Bit-Offsets auf 32 Bit -- OHNE
         * Annahme ueber die native "short"/"int"-Breite des Hosts (s.
         * bereits mehrfach dokumentierte Vorsicht in diesem Projekt). */
        signExtOffset = (rawOffset & 0x8000U) ? (0xFFFF0000UL | (Q9_u32)rawOffset) : (Q9_u32)rawOffset;

        realCode   = (Q9_u32)(codeword & 0x00FFU);
        sysTrapOnly = (codeword & 0x8000U) != 0;

        routineAddr = entryAddr + signExtOffset + 4UL;

        *(volatile unsigned char *)(Q9K_SSVC_EXTERNAL_BASE + realCode) = 1U;

        Q9K_SetU32(sysdisBase + realCode * 4UL, routineAddr);
        Q9K_SetU32(sysdisBase + 0x400UL + realCode * 4UL, dataPtr);

        if (!sysTrapOnly) {
            Q9K_SetU32(usrdisBase + realCode * 4UL, routineAddr);
            Q9K_SetU32(usrdisBase + 0x400UL + realCode * 4UL, dataPtr);
        }

        entryAddr += 4UL;   /* naechster 2-Wort-Eintrag (Code+Offset) */
    }
}

/* Eigene Kernel-Global-Erweiterungen fuer die ASM<->C-Uebergabe von
 * Q9K_SysFSSvc (q9kernel_entry.a) -- gleiches, etabliertes Muster wie
 * ueberall. Direkt hinter den SRtMem-Scratch-Feldern ($1344-$1348 + 4
 * Byte, q9kernel_sysmem.c) -- naechste freie Adresse $134C. */
#ifndef Q9K_SSVC_SCRATCH_TABLEPTR
#define Q9K_SSVC_SCRATCH_TABLEPTR 0x134CUL   /* Q9_u32, (a1) EIN */
#endif
#ifndef Q9K_SSVC_SCRATCH_DATAPTR
#define Q9K_SSVC_SCRATCH_DATAPTR  0x1350UL   /* Q9_u32, (a3) EIN */
#endif

/* Q9K_SysSSvcImpl -- duenne, PARAMETERLOSE Bruecke zwischen dem
 * Assembler-Trampolin und Q9K_ProcSSvc (echte Zweiparameter-C-Funktion,
 * s. oben) -- gleiches Muster wie ueberall (Q9K_SysForkImpl usw.). */
void Q9K_SysSSvcImpl(void)
{
    Q9_u32 tablePtr = Q9K_GetU32(Q9K_SSVC_SCRATCH_TABLEPTR);
    Q9_u32 dataPtr  = Q9K_GetU32(Q9K_SSVC_SCRATCH_DATAPTR);

    Q9K_ProcSSvc(tablePtr, dataPtr);
}
