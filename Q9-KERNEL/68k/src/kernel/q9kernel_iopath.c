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
 *   Nummerierung. Die native Schicht pflegt diese lokale P$Path-Tabelle
 *   inzwischen und trennt sie von der globalen Descriptor-Nummer; der
 *   Descriptor-Pool bleibt aus Platz-/Lifetime-Gruenden global.
 *
 * ECHTE Pfadaufloesung (Pathname -> Geraet/Dateisystem) existiert in diesem
 * Kernel noch NICHT -- die Microware-File-Manager bleiben dafuer zustaendig.
 * Der native Pfad erzwingt aber bereits die OS-9-Komponentensyntax und
 * liefert korrekte Fehlercodes, statt beliebige Speicherfolgen als Namen zu
 * akzeptieren. Gültige native Pfade werden vorerst mit der einzigen echten
 * Ausgabe verbunden, die dieser Kernel kennt -- dem DUART
 * (Q9K_DiagWriteD7-Mechanismus). Die neue Objektklassifikation wird bereits
 * im Descriptor festgehalten; die eigentliche Geraet-/Datei-Aufloesung und
 * das darauf aufbauende Read/Write/Status-Dispatch bleiben der naechste
 * Ausbau.
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
/* ECHTER BUG GEFUNDEN + GEFIXT (2026-09-05): hier stand 32. Der Pool legt
 * die Slots aber laengst mit 256 Byte an (q9kernel_tables.c, dort mit
 * ausfuehrlicher Begruendung) -- die beiden Definitionen widersprachen
 * einander. Folge: Q9K_ProcAllPD nullte von einem frisch vergebenen
 * Deskriptor nur das erste Achtel; alles ab +$20 blieb Altbestand.
 *
 * Das trifft ausgerechnet die Felder, die scf beim Lesen auswertet. Der
 * reale Pfaddeskriptor ist 256 Byte gross (PDSIZE in
 * internem Referenzmaterial), die ersten 128 Byte gehoeren dem File-Manager
 * ("do.b 128 File manager variables"), ab +$80 stehen die Optionen. scfs
 * ReadLn liest daraus u.a. +$81 (Grossschreibung -- danach wandelt es
 * $61..$7a um) und die neun Sonderzeichen ab +$89, und es holt den
 * Zielpuffer aus +$0e. Mit einem nur 32 Byte genullten Deskriptor sind
 * das samt und sonders Zufallswerte.
 */
#ifndef Q9K_PATHDESC_SIZE
#define Q9K_PATHDESC_SIZE 256UL
#endif

/* Pfad-Deskriptor-Layout (256 Byte, real PDSIZE aus io.a):
 *   +0x00 (4)  bei FREI: Next-Zeiger (Freilisten-Verkettung, gleiches
 *              Prinzip wie Q9K_ProcPoolAlloc); bei ALLOZIERT: die
 *              Deskriptornummer (Q9K_ProcAllPD legt sie dort ab, IOMan
 *              prueft sie gegen den Tabellenindex).
 *   +0x00..+0x7F: Bereich der File-Manager (scf/rbf), von IOMan und dem
 *              jeweiligen Manager belegt -- u.a. +$0e Zielpuffer.
 *   +0x06 (2)  Q9-native Objektklasse (aus dem Pfad abgeleitet)
 *   +0x08 (4)  Q9-native logische Dateiposition
 *   +0x80..+0xFF: Optionen (PD_OPT), beim Open aus dem Geraetedeskriptor
 *              gefuellt -- u.a. +$81 Grossschreibung, ab +$89 die
 *              Sonderzeichen.
 */
/* Kopf des Pfaddeskriptors, real belegt in internem Referenzmaterial:
 *   PD_PD  ($00, Wort) Pfadnummer
 *   PD_MOD ($02, Byte) Zugriffsmodus (read/write/update)
 *
 * ECHTER BUG GEFUNDEN + GEFIXT (2026-09-05): hier stand stattdessen ein
 * selbst erfundenes "Typ"-Langwort auf Offset 0 -- das ueberschrieb BEIDE
 * realen Felder mit 0. IOMan prueft aber vor JEDEM Lesen den Modus:
 *
 *     $ba46  moveq  #$5,d1        * verlangt Lesezugriff
 *     $ba4e  and.b  $2(a1),d1     * PD_MOD
 *     $ba52  bne    ...           * passt -> weiter zum File-Manager
 *     $ba56  move.w #$cb,d1       * sonst E_BMODE
 *
 * Mit PD_MOD = 0 brach IOMan deshalb ab, OHNE scf ueberhaupt zu rufen --
 * per PC-Zaehler bestaetigt: scfs ReadLn-Einstieg ($c224) wurde nie
 * erreicht, waehrend Write ($c502) und WritLn ($c4fc) normal ansprangen. */
#define Q9K_PATHDESC_NUM_OFF  0x00UL
#define Q9K_PATHDESC_MODE_OFF 0x02UL
#define Q9K_PATHDESC_REF_OFF  0x04UL       /* Q9-native open-reference count */
#define Q9K_PATHDESC_KIND_OFF 0x06UL       /* Q9-native resolved object kind */
#define Q9K_PATHDESC_POS_OFF  0x08UL       /* Q9-native logical file position */
#define Q9K_PATHDESC_OBJECT_OFF 0x0CUL     /* Q9-native backend object ID */

/* Native object kinds.  The first implementation deliberately keeps
 * UNRESOLVED as a valid state: accepting a pathname and dispatching it to a
 * real device/file manager are separate steps.  This lets the path layer
 * carry an explicit result instead of using "IOMan returned OK" as an
 * accidental type system. */
#define Q9K_PATH_KIND_UNRESOLVED 0U
#define Q9K_PATH_KIND_CONSOLE    1U
#define Q9K_PATH_KIND_FILESYSTEM 2U
#define Q9K_PATH_KIND_DIRECTORY  3U

#define Q9K_NATIVE_OBJECT_NONE   0U
#define Q9K_NATIVE_OBJECT_TERM   1U
#define Q9K_NATIVE_OBJECT_DD     2U
#define Q9K_NATIVE_OBJECT_SYS    3U
#define Q9K_NATIVE_OBJECT_MOTD   4U

/* Stable operation IDs for the native backend boundary.  The mask is kept
 * beside the descriptor logic so adding a file or directory backend cannot
 * silently broaden the console path. */
#define Q9K_NATIVE_OP_READ    1U
#define Q9K_NATIVE_OP_WRITE   2U
#define Q9K_NATIVE_OP_READLN  3U
#define Q9K_NATIVE_OP_WRITELN 4U
#define Q9K_NATIVE_OP_GETSTAT 5U
#define Q9K_NATIVE_OP_SETSTAT 6U
#define Q9K_NATIVE_OP_SEEK    7U
#define Q9K_NATIVE_OP_CLOSE   8U

#define Q9K_E_BPNAM  0x00D7U
#define Q9K_E_BMODE  0x00CBU
#define Q9K_E_BPNUM  0x00C9U
#define Q9K_E_UNKSVC 0x00D0U
#define Q9K_E_MNF    0x00DDU
#define Q9K_E_PTHFUL 0x00C8U

/* The current process owns the P$Path table.  Keep the offsets here in one
 * place so native I/O uses the same process layout as IOMan. */
#ifndef Q9_D_PROC
#define Q9_D_PROC              0x04CUL      /* current process descriptor */
#endif
#ifndef Q9K_PROCDESC_PATH_OFF
#define Q9K_PROCDESC_PATH_OFF  0x168UL      /* P$Path[0], 32 big-endian words */
#endif
#ifndef Q9K_PROCDESC_PATH_COUNT
#define Q9K_PROCDESC_PATH_COUNT 32UL
#endif

/* Scratchzellen fuer F$RetPD -- gleiche Konvention wie ueberall in diesem
 * Kernel (Assembler-Trampolin legt die Eingaben ab, holt die Ausgaben). */
#ifndef Q9K_RETPD_SCRATCH_DBTIN
#define Q9K_RETPD_SCRATCH_DBTIN   0x161CUL   /* Q9_u32, (a0) EIN = DBT-Basis   */
#define Q9K_RETPD_SCRATCH_NUMIN   0x1620UL   /* Q9_u32, d0.w EIN = Nummer      */
#define Q9K_RETPD_SCRATCH_ERROR   0x1624UL   /* Q9_u32, d1.w AUS bei Fehler    */
#define Q9K_RETPD_SCRATCH_SUCCESS 0x1628UL   /* Q9_u32, 0 = Fehlschlag / 1 = ok */
#endif

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

/* Gegenstueck zu Q9K_PathPoolAlloc: haengt den Deskriptor vorne in die
 * Freiliste zurueck. Der Next-Zeiger liegt bei FREIEN Deskriptoren an
 * Offset 0 (s. Layout oben) -- derselbe Platz, an dem ein ALLOZIERTER
 * seine Nummer bzw. seinen Typ traegt. Das ist kein Konflikt: die Nummer
 * wird beim Allozieren neu gesetzt, und ab hier gilt der Deskriptor als
 * frei. */
static void Q9K_PathPoolFree(Q9_u32 desc)
{
#ifdef Q9K_TEST_PATHPOOL_FREE_HOOK
    /* NUR IM HOSTTEST: dort ist dieser Zeiger nicht dereferenzierbar. Er
     * stammt aus einem DBT-Slot, und die sind echte 4 Byte breit
     * (68k-Zeigerbreite) -- auf einem 64-Bit-Host liegen die Testpuffer
     * oberhalb 4 GB, der zurueckgelesene Wert ist also abgeschnitten.
     * Niedrigen Speicher zu mappen geht auf macOS nicht (__PAGEZERO belegt
     * die unteren 4 GB). Der Hook protokolliert deshalb nur, WELCHER
     * Deskriptor freigegeben wurde; die beiden Zeilen darunter sind
     * strukturgleich zu Q9K_PathPoolAlloc und auf dem Ziel geprueft. */
    Q9K_TEST_PATHPOOL_FREE_HOOK(desc);
#else
    Q9K_SetU32(desc, Q9K_GetU32(Q9K_PATHPOOL_FREE_ADDR));
    Q9K_SetU32(Q9K_PATHPOOL_FREE_ADDR, desc);
#endif
}

/* Byteweises Big-Endian-Lesen/Schreiben von 16-Bit-Feldern in der
 * Pfad-Deskriptor-Blocktabelle (DBT). Byteweise aus demselben Grund wie
 * ueberall in diesem Kernel: der Hosttest laeuft little-endian, das Ziel
 * big-endian (vgl. Q9K_ReadU32BE in q9kernel_moddir.c). */
static Q9_u16 Q9K_ReadU16BE(Q9_u32 addr)
{
    const volatile unsigned char *p = (const volatile unsigned char *)addr;
    return (Q9_u16)(((Q9_u16)p[0] << 8) | (Q9_u16)p[1]);
}

static void Q9K_WriteU16BE(Q9_u32 addr, Q9_u16 value)
{
    volatile unsigned char *p = (volatile unsigned char *)addr;
    p[0] = (unsigned char)((value >> 8) & 0xFFU);
    p[1] = (unsigned char)(value & 0xFFU);
}

/* Die DBT-Zeigerslots sind EXAKT 4 Byte breit (fremde, von IOMan
 * angelegte Struktur) -- deshalb byteweise und NICHT ueber Q9K_SetU32:
 * Q9_u32 ist "unsigned long", auf dem 64-Bit-Hosttest also 8 Byte breit,
 * und wuerde den Nachbarslot mit ueberschreiben (im Test real
 * aufgefallen; auf dem 68k-Ziel waere es zufaellig gutgegangen). Gleiche
 * Begruendung wie beim vorhandenen Hinweis in test_q9kernel_ssvc.c. */
static Q9_u32 Q9K_ReadU32BE_At(Q9_u32 addr)
{
    const volatile unsigned char *p = (const volatile unsigned char *)addr;
    return ((Q9_u32)p[0] << 24) | ((Q9_u32)p[1] << 16)
         | ((Q9_u32)p[2] << 8)  | (Q9_u32)p[3];
}

static void Q9K_WriteU32BE_At(Q9_u32 addr, Q9_u32 value)
{
    volatile unsigned char *p = (volatile unsigned char *)addr;
    p[0] = (unsigned char)((value >> 24) & 0xFFU);
    p[1] = (unsigned char)((value >> 16) & 0xFFU);
    p[2] = (unsigned char)((value >> 8)  & 0xFFU);
    p[3] = (unsigned char)(value & 0xFFU);
}

static int Q9K_PrsNamIsNameChar(unsigned char c);
static Q9_u16 Q9K_NativeValidatePathname(Q9_u32 pathnamePtr,
                                         Q9_u32 *outPastName);
Q9_u32 Q9K_ProcPathDesc(Q9_u16 pathNum, Q9_u16 *outError);

static unsigned char Q9K_PathNameFold(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ? (unsigned char)(c - 'A' + 'a') : c;
}

static int Q9K_NativePathEquals(Q9_u32 pathnamePtr, const char *expected)
{
    const volatile Q9_u8 *p = (const volatile Q9_u8 *)pathnamePtr;
    Q9_u32 i = 0;

    if (pathnamePtr == 0 || expected == 0)
        return 0;
    while (expected[i] != 0) {
        if (p[i] != (Q9_u8)expected[i])
            return 0;
        i++;
    }
    return p[i] == 0;
}

/* Classify the first OS-9 pathname component after validating the complete
 * path.  This is intentionally a small native namespace, not a promise that
 * a file manager already exists behind every name:
 *
 *   term              -> native console device
 *   dd[/...]          -> native filesystem namespace
 *   any other name    -> unresolved (future device/file-manager lookup)
 *
 * A directory request is represented by the OS-9 directory access bit and is
 * kept distinct from an ordinary filesystem object.  The caller receives the
 * same updated past-name pointer as I$Open. */
static Q9_u16 Q9K_NativeClassifyPathname(Q9_u32 pathnamePtr, Q9_u32 mode,
                                         Q9_u32 *outPastName,
                                         Q9_u16 *outKind,
                                         Q9_u16 *outObject)
{
    const volatile Q9_u8 *p;
    Q9_u32 i = 0;
    Q9_u32 firstLen = 0;
    Q9_u8 first[8];
    Q9_u16 err;

    if (outPastName)
        *outPastName = pathnamePtr;
    if (outKind)
        *outKind = Q9K_PATH_KIND_UNRESOLVED;
    if (outObject)
        *outObject = Q9K_NATIVE_OBJECT_NONE;
    if (pathnamePtr == 0)
        return Q9K_E_BPNAM;

    p = (const volatile Q9_u8 *)pathnamePtr;
    while (p[i] == '/')
        i++;
    while (firstLen < sizeof(first) &&
           Q9K_PrsNamIsNameChar((unsigned char)(p[i] & 0x7fU))) {
        first[firstLen++] = Q9K_PathNameFold((unsigned char)(p[i] & 0x7fU));
        i++;
        if (p[i - 1] & 0x80U)
            break;
    }

    /* Reuse the canonical full-path validator so classification and open
     * cannot disagree about doubled components, empty names or termination. */
    err = Q9K_NativeValidatePathname(pathnamePtr, outPastName);
    if (err != 0)
        return err;

    if ((mode & 0x80UL) != 0) {
        if (Q9K_NativePathEquals(pathnamePtr, "/dd")) {
            *outKind = Q9K_PATH_KIND_DIRECTORY;
            *outObject = Q9K_NATIVE_OBJECT_DD;
            return 0;
        }
        if (Q9K_NativePathEquals(pathnamePtr, "/dd/SYS")) {
            *outKind = Q9K_PATH_KIND_DIRECTORY;
            *outObject = Q9K_NATIVE_OBJECT_SYS;
            return 0;
        }
        return Q9K_E_MNF;
    }
    if (Q9K_NativePathEquals(pathnamePtr, "/term")) {
        *outKind = Q9K_PATH_KIND_CONSOLE;
        *outObject = Q9K_NATIVE_OBJECT_TERM;
        return 0;
    }
    if (Q9K_NativePathEquals(pathnamePtr, "/dd/SYS/motd")) {
        *outKind = Q9K_PATH_KIND_FILESYSTEM;
        *outObject = Q9K_NATIVE_OBJECT_MOTD;
        return 0;
    }
    return Q9K_E_MNF;
}

/* Q9K_ProcAllPD -- echte F$AllPD-Kernlogik (Callcode $30, "Allocate
 * Process/Path Descriptor"; in OS-9/6809 hiess derselbe Dienst F$All64,
 * 1985 umbenannt, s. internem Referenzmaterial).
 *
 * Aufrufkonvention aus der IOMan-Analyse abgelesen (Modul-Offset
 * $135e ff.): IN a0 = Basis der Deskriptor-Blocktabelle (DBT), die IOMan
 * beim Init selbst anlegt und in D_PthDBT ($48) ablegt. OUT a1 = Zeiger
 * auf den neuen Deskriptor, d0.w = dessen Nummer, Carry bei Fehler.
 * IOMan holt a1 dabei NICHT aus dem Register, sondern aus dem
 * 44-Byte-Registerrahmen des Trampolin-Aufrufers (Slot +$24) -- genau wie
 * bei F$SRqMem, s. Q9K_SysFSRqMem in q9kernel_entry.a.
 *
 * DBT-Aufbau, ebenfalls aus IOMans Code belegt ($14d4 ff.):
 *   +0x00 (2) hoechster gueltiger Index
 *   +0x02 (2) von IOMan mit $0100 vorbelegt (Bedeutung noch offen)
 *   +idx*4    Zeiger auf den Deskriptor mit dieser Nummer (0 = frei)
 * Index 0 ist ungueltig -- IOman verwirft ihn ausdruecklich ("asl.w #2,d0
 * / beq" bei $14dc), weil Offset 0 der Kopf selbst ist. Der Deskriptor
 * traegt seine eigene Nummer an Offset 0; IOMan prueft das gegen den
 * Index ("cmp.w (a1),d0" bei $14ea), deshalb wird sie hier gesetzt.
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt).
 */
int Q9K_ProcAllPD(Q9_u32 dbtAddr, Q9_u32 *outDesc, Q9_u16 *outNum, Q9_u16 *outError)
{
    Q9_u16 maxIndex;
    Q9_u32 idx;
    Q9_u32 desc;
    Q9_u32 i;

    *outDesc  = 0;
    *outNum   = 0;
    *outError = 0;

    if (dbtAddr == 0) {
        *outError = 0x00D2U;            /* E_BPADDR, Bad Page Address */
        return 0;
    }

    maxIndex = Q9K_ReadU16BE(dbtAddr);

    for (idx = 1; idx <= (Q9_u32)maxIndex; idx++) {
        if (Q9K_ReadU32BE_At(dbtAddr + idx * 4UL) == 0)
            break;
    }

    if (idx > (Q9_u32)maxIndex) {
        *outError = 0x00C8U;            /* E_PTHFUL, Path Table full */
        return 0;
    }

    desc = Q9K_PathPoolAlloc();
    if (desc == 0) {
        *outError = 0x00C8U;            /* Pool erschoepft -- fuer den Aufrufer derselbe Fall */
        return 0;
    }

    for (i = 0; i < Q9K_PATHDESC_SIZE; i++)
        *(volatile unsigned char *)(desc + i) = 0;

    Q9K_WriteU16BE(desc, (Q9_u16)idx);
    Q9K_WriteU32BE_At(dbtAddr + idx * 4UL, desc);

    *outDesc = desc;
    *outNum  = (Q9_u16)idx;
    return 1;
}

/* Q9K_ProcRetPD -- echte F$RetPD-Kernlogik (Callcode $31, "Return
 * Process/Path Descriptor"), exaktes Gegenstueck zu Q9K_ProcAllPD oben.
 *
 * Aufrufkonvention aus der IOMan-Analyse abgelesen (2026-09-05,
 * Aufrufstelle Modul-Offset $1206 ff.), NICHT geraten:
 *     move.w  $0(a1),d0        * d0.w = Nummer, aus dem Deskriptor selbst
 *     movea.l $48(a6),a0       * a0   = D_PthDBT, die Blocktabelle
 *     ... Sprung in Dispatch-Slot $c4 = Callcode $31
 * Der Deskriptor traegt seine eigene Nummer an Offset 0 -- genau die legt
 * Q9K_ProcAllPD dort ab. OUT: nur Carry, kein Rueckgabewert.
 *
 * WARUM DAS HIER GEBRAUCHT WIRD: IOMan fordert diesen Dienst schon beim
 * Start an, unmittelbar bevor es "can't chgdir to system device" meldet.
 * Bis dahin lief der Aufruf in den Unimplemented-Stub und kam mit E_UNKSVC
 * zurueck (gefunden 2026-09-05 per Marker-Stub, s. docs/OWN_KERNEL_STATUS.md).
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt).
 */
int Q9K_ProcRetPD(Q9_u32 dbtAddr, Q9_u16 num, Q9_u16 *outError)
{
    Q9_u16 maxIndex;
    Q9_u32 desc;

    *outError = 0;

    if (dbtAddr == 0) {
        *outError = 0x00D2U;            /* E_BPADDR, Bad Page Address */
        return 0;
    }

    /* Index 0 ist ungueltig -- dort liegt der Tabellenkopf selbst, nicht
     * ein Deskriptorzeiger (gleiche Begruendung wie in Q9K_ProcAllPD). */
    maxIndex = Q9K_ReadU16BE(dbtAddr);
    if (num == 0 || (Q9_u32)num > (Q9_u32)maxIndex) {
        *outError = 0x00C9U;            /* E_BPNUM, Bad Path Number */
        return 0;
    }

    desc = Q9K_ReadU32BE_At(dbtAddr + (Q9_u32)num * 4UL);
    if (desc == 0) {
        *outError = 0x00C9U;            /* schon frei -- fuer den Aufrufer derselbe Fall */
        return 0;
    }

    /* The descriptor repeats its number in PD_PD.  IOMan relies on this
     * invariant when walking the DBT; checking it before returning the slot
     * also prevents a stale/corrupt DBT entry from returning an unrelated
     * pool object to the free list. */
#ifndef Q9K_TEST_PATHPOOL_FREE_HOOK
    if (Q9K_ReadU16BE(desc + Q9K_PATHDESC_NUM_OFF) != num) {
        *outError = 0x00C9U;            /* E_BPNUM -- inconsistent slot */
        return 0;
    }
#endif

    Q9K_WriteU32BE_At(dbtAddr + (Q9_u32)num * 4UL, 0);
    Q9K_PathPoolFree(desc);
    return 1;
}

/* ---------------------------------------------------------------------
 * F$PrsNam (Callcode $10, "Parse Pathlist Name")
 *
 * Zerlegt EIN Element eines Pfadnamens. Konvention aus dem realen
 * File-Manager scf abgelesen (Modul-Offset $00be ff.), nicht geraten:
 *
 *     movea.l $20(a5),a0    a0 = Pfadname (aus dem Registerrahmen)
 *     trap    #0 / $0010
 *     bcs.w   ...           Carry = Fehler
 *     tst.b   d0            d0.b = Trennzeichen HINTER dem Namen
 *     cmpi.b  #$d,d0        scf akzeptiert 0, CR und Leerzeichen
 *     cmpi.b  #$20,d0
 *     movea.l a1,a0         a1 = Zeiger HINTER das letzte Namenszeichen
 *
 * Vollstaendige Ausgabe (ECHTE Microware-Konvention -- 2026-09-08 durch
 * ECHTEN RBF-Level-2-Quellcode (SchDir/RBPNam, www.roug.org, l2sources/rbf)
 * zweifelsfrei belegt, nicht mehr nur aus dem Technical Manual geraten):
 * RBFs SchDir-Schleife ruft F$PrsNam via RBPNam GENAU EINMAL pro
 * Verzeichnisebene auf und braucht daraus GLEICHZEITIG zwei verschiedene
 * Zeiger -- das war der Denkfehler hinter der bisherigen "struktureller
 * Widerspruch, blackbox nicht loesbar"-Einschaetzung (Fortsetzung 23):
 *   a0 AUS = Anfang des Namens (hinter einem evtl. FUEHRENDEN '/', sonst
 *            unveraendert) -- RBF sichert dies SOFORT nach dem Trap
 *            (`pshs x`) als S.PathPt und benutzt es SPAETER direkt als
 *            Vergleichszeiger fuer F$CmpNam gegen die Verzeichniseintraege.
 *   a1 AUS = hinter dem letzten Namenszeichen, VOR einem evtl. Trenner
 *            (= Position DES Trenners selbst) -- RBF sichert dies als
 *            S.NextPt und ueberspringt den Trenner beim naechsten
 *            RBPNam-Aufruf SELBST per `leax 1,X` -- F$PrsNam darf hier
 *            NICHT vorgreifen.
 *   d0.b  = das Trennzeichen selbst
 *   d1.w  = Namenslaenge
 *   Carry gesetzt + d1.w = E_BPNAM ($D7), wenn kein gueltiger Name folgt.
 *
 * Der bisherige Code lieferte in a0 faelschlich "hinter Name UND Trenner"
 * (schon fast die Position des NAECHSTEN Elements) statt des Anfangs des
 * AKTUELLEN Namens -- dadurch verglich RBFs F$CmpNam bei jeder zweiten
 * Verzeichnisebene gegen die falsche (zu weit vorgerueckte) Adresse, was
 * die eigentliche Ursache des $D8-Bugs bei "/dd/startup" war. s.
 * docs/OWN_KERNEL_STATUS.md Fortsetzung 24 fuer die komplette Herleitung.
 *
 * Fuehrende '/' werden uebersprungen -- fuer "/term" liefert das den
 * Namen "term" (Laenge 4) mit Trennzeichen 0, genau was scf erwartet.
 * Gueltige Namenszeichen sind Buchstaben, Ziffern sowie '_', '.' und '$'
 * (OS-9-Konvention); das Trennzeichen ist alles andere.
 * --------------------------------------------------------------------- */
static int Q9K_PrsNamIsNameChar(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return 1;
    if (c >= 'a' && c <= 'z') return 1;
    if (c >= '0' && c <= '9') return 1;
    return (c == '_' || c == '.' || c == '$');
}

/* Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
int Q9K_ProcPrsNam(Q9_u32 pathPtr, Q9_u32 *outNameStart, Q9_u32 *outPastName,
                   Q9_u16 *outLen, Q9_u16 *outDelim, Q9_u16 *outError)
{
    const volatile unsigned char *p;
    Q9_u32 i = 0;
    Q9_u32 start;

    *outNameStart = 0;
    *outPastName  = pathPtr;
    *outLen       = 0;
    *outDelim     = 0;
    *outError     = 0;

    if (pathPtr == 0) {
        *outError = 0x00D7U;            /* E_BPNAM, Bad Path Name */
        return 0;
    }

    p = (const volatile unsigned char *)pathPtr;

    while (p[i] == '/')                  /* fuehrende Trenner ueberspringen */
        i++;

    start = i;
    /* OS-9 pathlists may mark the final character of a component with
     * bit 7 instead of appending a NUL.  Keep the marker out of the
     * lexical check, include that character in the component, and stop
     * immediately afterwards.  NUL-terminated host/native strings remain
     * unchanged. */
    while (Q9K_PrsNamIsNameChar((unsigned char)(p[i] & 0x7fU))) {
        unsigned char c = p[i++];
        if (c & 0x80U)
            break;
    }

    if (i == start) {                    /* leerer Name */
        *outError = 0x00D7U;
        return 0;
    }

    /* a1 (outNameStart): hinter dem letzten Namenszeichen, VOR einem
     * evtl. Trenner -- KEIN Trenner-Ueberspringen hier (s. Kopfkommentar,
     * durch echten RBF-Quellcode belegt). Numerisch unveraendert seit
     * Fortsetzung 16; nur die Rollenbeschreibung war dort noch falsch
     * begruendet (s. Fortsetzung 24). */
    *outNameStart = pathPtr + i;
    *outLen       = (Q9_u16)(i - start);
    *outDelim     = (p[i - 1] & 0x80U) ? 0U : (Q9_u16)p[i];

    /* ECHTER BUG GEFUNDEN + GEFIXT (2026-09-08, Fortsetzung 24, durch
     * echten RBF-Quellcode (SchDir/RBPNam) zweifelsfrei belegt): a0 muss
     * den ANFANG des aktuellen Namens liefern (hinter einem evtl.
     * fuehrenden '/', sonst unveraendert) -- RBF sichert genau diesen
     * Wert (`pshs x` direkt nach dem Trap) als seinen spaeteren
     * F$CmpNam-Vergleichszeiger. Der bisherige Code lieferte hier
     * faelschlich "hinter Name UND Trenner" (schon fast Position des
     * NAECHSTEN Elements) -- dadurch verglich RBF bei jeder zweiten
     * Verzeichnisebene gegen eine zu weit vorgerueckte Adresse, was die
     * eigentliche Ursache des $D8-Fehlers bei "/dd/startup" war. RBF
     * ueberspringt einen folgenden Trenner beim naechsten RBPNam-Aufruf
     * SELBST (`leax 1,X` auf *outNameStart, nicht auf *outPastName) --
     * F$PrsNam darf dem nicht vorgreifen. */
    *outPastName = pathPtr + start;
    return 1;
}

/* Q9K_ProcIOpen -- echte I$Open-Kernlogik (s. Kopfkommentar).
 * IN: mode (Zugriffsbits plus Directory-Bit fuer Klassifikation und spaetere
 *     Dispatch-Pruefung), pathnamePtr (Zeiger auf den NUL-terminierten
 *     Pfadnamen).
 * OUT: Q9_u32 -- 0 = Fehlschlag (Pool erschoepft, einziger bisher
 *     moeglicher Fehlerfall), sonst die Pfadnummer (immer >= 3, s. u.).
 *     *outPastName wird IMMER (auch im Fehlerfall, das Manual verlangt
 *     "(a0) = Updated past pathlist" nur fuer den Erfolgsfall, aber ein
 *     korrekt aktualisierter Zeiger schadet im Fehlerfall nicht und
 *     spart eine Sonderfall-Unterscheidung im Assembler-Trampolin).
 */
static Q9_u16 Q9K_NativeValidatePathname(Q9_u32 pathnamePtr, Q9_u32 *outPastName)
{
    const volatile Q9_u8 *p;
    Q9_u32 i = 0;
    Q9_u32 sawName = 0;
    Q9_u32 componentLen = 0;

    if (pathnamePtr == 0)
        return Q9K_E_BPNAM;
    p = (const volatile Q9_u8 *)pathnamePtr;
    while (i < 256UL) {
        Q9_u8 c = p[i++];
        if (c == 0) {
            if (componentLen == 0 || sawName == 0)
                return Q9K_E_BPNAM;
            if (outPastName)
                *outPastName = pathnamePtr + i;
            return 0;
        }
        if (c == '/') {
            if (componentLen == 0) {
                if (i == 1 && sawName == 0)
                    continue; /* absolute path root */
                return Q9K_E_BPNAM;
            }
            componentLen = 0;
            continue;
        }
        if (!Q9K_PrsNamIsNameChar((unsigned char)(c & 0x7fU)))
            return Q9K_E_BPNAM;
        componentLen++;
        sawName = 1;
    }
    return Q9K_E_BPNAM; /* unterminated/overlong path */
}

Q9_u32 Q9K_ProcIOpen(Q9_u32 mode, Q9_u32 pathnamePtr, Q9_u32 *outPastName,
                     Q9_u16 *outError)
{
    Q9_u32 slot;
    Q9_u32 pathNum;
    Q9_u32 descriptorNum;
    Q9_u32 procDesc;
    Q9_u32 pathIndex;
    Q9_u16 pathKind;
    Q9_u16 pathObject;

    *outError = 0;
    *outPastName = pathnamePtr;

    if ((mode & ~0xD7UL) != 0) {
        *outError = Q9K_E_BMODE;
        return 0;
    }
    *outError = Q9K_NativeClassifyPathname(pathnamePtr, mode, outPastName,
                                           &pathKind, &pathObject);
    if (*outError != 0)
        return 0;
    slot = Q9K_PathPoolAlloc();
    if (slot == 0) {
        *outError = Q9K_E_PTHFUL;
        return 0;
    }

    /* The pool index is global, while the number returned to the caller is
     * a process-local P$Path index.  Keep both values separate: this is
     * essential once two processes open paths concurrently. */
    descriptorNum = (slot - Q9K_GetU32(Q9K_PATHPOOL_BASE_ADDR)) / Q9K_PATHDESC_SIZE + 3UL;
    pathNum = descriptorNum;
    procDesc = Q9K_GetU32(Q9_D_PROC);
    if (procDesc != 0) {
        pathNum = 0;
        for (pathIndex = 3; pathIndex < Q9K_PROCDESC_PATH_COUNT; pathIndex++) {
            if (Q9K_ReadU16BE(procDesc + Q9K_PROCDESC_PATH_OFF + pathIndex * 2UL) == 0) {
                pathNum = pathIndex;
                break;
            }
        }
        if (pathNum == 0) {
            Q9K_PathPoolFree(slot);
            *outError = Q9K_E_PTHFUL;
            return 0;
        }
    }

    Q9K_WriteU16BE(slot + Q9K_PATHDESC_NUM_OFF, (Q9_u16)descriptorNum);
    Q9K_WriteU16BE(slot + Q9K_PATHDESC_KIND_OFF, pathKind);
    Q9K_WriteU16BE(slot + Q9K_PATHDESC_OBJECT_OFF, pathObject);
    Q9K_WriteU16BE(slot + Q9K_PATHDESC_REF_OFF, 1);
    Q9K_WriteU32BE_At(slot + Q9K_PATHDESC_POS_OFF, 0);

    /* Publish the descriptor number in the process table.  IOMan and the
     * native I/O handlers use this table as the authoritative path lookup. */
    if (procDesc != 0)
        Q9K_WriteU16BE(procDesc + Q9K_PROCDESC_PATH_OFF + pathNum * 2UL,
                       (Q9_u16)descriptorNum);

    /* Zugriffsmodus eintragen -- ohne ihn verweigert IOMan jeden Lese- und
     * Schreibzugriff auf diesen Pfad (s. Kopfkommentar oben). Faellt der
     * Aufrufer mit 0 herein, wird daraus Lesen+Schreiben: ein Pfad, auf dem
     * NICHTS erlaubt ist, waere in jedem Fall unbrauchbar, und die Konsole
     * kann real beides. */
    {
        Q9_u8 m = (Q9_u8)(mode & 0xFFUL);
        if (m == 0)
            m = 3;                      /* READ_ | WRITE_ */
        *(volatile Q9_u8 *)(slot + Q9K_PATHDESC_MODE_OFF) = m;
    }

    return pathNum;
}

/* Resolve a process-local P$Path number to the native descriptor.  All
 * native I/O operations must use this gate: the process table contains the
 * caller-visible path number, while the pool slot is global and can differ
 * between processes.  Returning the descriptor also validates the repeated
 * descriptor number, so stale or corrupted P$Path entries cannot silently
 * target another native object. */
Q9_u32 Q9K_ProcPathDesc(Q9_u16 pathNum, Q9_u16 *outError)
{
    Q9_u32 procDesc;
    Q9_u16 descriptorNum;
    Q9_u32 pathDesc;
    Q9_u32 poolBase;

    if (outError)
        *outError = 0;
    if (pathNum < 3 || pathNum >= Q9K_PROCDESC_PATH_COUNT) {
        if (outError)
            *outError = Q9K_E_BPNUM;
        return 0;
    }

    procDesc = Q9K_GetU32(Q9_D_PROC);
    if (procDesc == 0) {
        if (outError)
            *outError = Q9K_E_BPNUM;
        return 0;
    }
    descriptorNum = Q9K_ReadU16BE(procDesc + Q9K_PROCDESC_PATH_OFF +
                                   (Q9_u32)pathNum * 2UL);
    if (descriptorNum < 3) {
        if (outError)
            *outError = Q9K_E_BPNUM;
        return 0;
    }

    poolBase = Q9K_GetU32(Q9K_PATHPOOL_BASE_ADDR);
    if (poolBase == 0) {
        if (outError)
            *outError = Q9K_E_BPNUM;
        return 0;
    }
    pathDesc = poolBase + ((Q9_u32)descriptorNum - 3UL) * Q9K_PATHDESC_SIZE;
    if (Q9K_ReadU16BE(pathDesc + Q9K_PATHDESC_NUM_OFF) != descriptorNum) {
        if (outError)
            *outError = Q9K_E_BPNUM;
        return 0;
    }
    return pathDesc;
}

/* Read the native state needed by future data and status operations.  Keep
 * this as the single descriptor-layout reader so the later handlers do not
 * grow independent interpretations of kind, mode, references and position. */
int Q9K_ProcPathState(Q9_u16 pathNum, Q9_u16 *outKind, Q9_u8 *outMode,
                      Q9_u16 *outRefs, Q9_u32 *outPosition,
                      Q9_u16 *outError)
{
    Q9_u32 pathDesc;
    Q9_u16 err = 0;

    pathDesc = Q9K_ProcPathDesc(pathNum, &err);
    if (pathDesc == 0) {
        if (outError)
            *outError = err;
        return 0;
    }
    if (outKind)
        *outKind = Q9K_ReadU16BE(pathDesc + Q9K_PATHDESC_KIND_OFF);
    if (outMode)
        *outMode = *(volatile Q9_u8 *)(pathDesc + Q9K_PATHDESC_MODE_OFF);
    if (outRefs)
        *outRefs = Q9K_ReadU16BE(pathDesc + Q9K_PATHDESC_REF_OFF);
    if (outPosition)
        *outPosition = Q9K_ReadU32BE_At(pathDesc + Q9K_PATHDESC_POS_OFF);
    if (outError)
        *outError = 0;
    return 1;
}

/* Check only the data-access bits shared by native read/write operations.
 * Other I$Open mode bits (execute, append, non-sharable, directory) remain
 * available to their respective operation-specific rules. */
int Q9K_ProcPathCheckAccess(Q9_u16 pathNum, Q9_u8 requestedMode,
                            Q9_u16 *outError)
{
    Q9_u32 pathDesc;
    Q9_u8 grantedMode;
    Q9_u16 err = 0;

    pathDesc = Q9K_ProcPathDesc(pathNum, &err);
    if (pathDesc == 0) {
        if (outError)
            *outError = err;
        return 0;
    }
    grantedMode = *(volatile Q9_u8 *)(pathDesc + Q9K_PATHDESC_MODE_OFF);
    if ((requestedMode & 0x03U) != 0 &&
        (grantedMode & (requestedMode & 0x03U)) !=
            (requestedMode & 0x03U)) {
        if (outError)
            *outError = Q9K_E_BMODE;
        return 0;
    }
    if (outError)
        *outError = 0;
    return 1;
}

/* Decide whether an operation has a native backend for this object class.
 * Close is always available for an allocated native descriptor; the other
 * operations are currently implemented only for the console backend. */
int Q9K_ProcPathSupports(Q9_u16 pathNum, Q9_u8 operation,
                         Q9_u16 *outError)
{
    Q9_u16 kind = Q9K_PATH_KIND_UNRESOLVED;
    Q9_u16 object = Q9K_NATIVE_OBJECT_NONE;
    Q9_u32 pathDesc;
    Q9_u16 err = 0;

    pathDesc = Q9K_ProcPathDesc(pathNum, &err);
    if (pathDesc == 0) {
        if (outError)
            *outError = err;
        return 0;
    }
    kind = Q9K_ReadU16BE(pathDesc + Q9K_PATHDESC_KIND_OFF);
    object = Q9K_ReadU16BE(pathDesc + Q9K_PATHDESC_OBJECT_OFF);
    if (operation == Q9K_NATIVE_OP_CLOSE ||
        (kind == Q9K_PATH_KIND_CONSOLE && operation >= Q9K_NATIVE_OP_READ &&
         operation <= Q9K_NATIVE_OP_SEEK) ||
        (kind == Q9K_PATH_KIND_FILESYSTEM &&
         object == Q9K_NATIVE_OBJECT_MOTD &&
         operation == Q9K_NATIVE_OP_READ)) {
        if (outError)
            *outError = 0;
        return 1;
    }
    if (outError)
        *outError = Q9K_E_UNKSVC;
    return 0;
}

/* Parameterless bridge used by the 68k I$Read trap.  Keeping the register
 * ABI at the assembly boundary makes the backend testable on the host and
 * leaves the native descriptor/position logic in one place. */
int Q9K_ProcNativeRead(Q9_u16 pathNum, Q9_u32 bufferPtr, Q9_u32 count,
                       Q9_u32 *outCount, Q9_u16 *outError);

#ifndef Q9K_NATIVE_READ_SCRATCH_PATH
#define Q9K_NATIVE_READ_SCRATCH_PATH  0x1F38UL
#define Q9K_NATIVE_READ_SCRATCH_BUF   0x1F3CUL
#define Q9K_NATIVE_READ_SCRATCH_COUNT 0x1F40UL
#define Q9K_NATIVE_READ_SCRATCH_DONE  0x1F44UL
#define Q9K_NATIVE_READ_SCRATCH_ERROR 0x1F48UL
#define Q9K_NATIVE_READ_SCRATCH_OK    0x1F4CUL
#endif

void Q9K_SysNativeReadImpl(void)
{
    Q9_u32 done = 0;
    Q9_u16 err = 0;

    if (Q9K_ProcNativeRead(
            (Q9_u16)Q9K_GetU32(Q9K_NATIVE_READ_SCRATCH_PATH),
            Q9K_GetU32(Q9K_NATIVE_READ_SCRATCH_BUF),
            Q9K_GetU32(Q9K_NATIVE_READ_SCRATCH_COUNT),
            &done, &err)) {
        Q9K_SetU32(Q9K_NATIVE_READ_SCRATCH_DONE, done);
        Q9K_SetU32(Q9K_NATIVE_READ_SCRATCH_ERROR, 0UL);
        Q9K_SetU32(Q9K_NATIVE_READ_SCRATCH_OK, 1UL);
    } else {
        Q9K_SetU32(Q9K_NATIVE_READ_SCRATCH_DONE, 0UL);
        Q9K_SetU32(Q9K_NATIVE_READ_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_NATIVE_READ_SCRATCH_OK, 0UL);
    }
}

/* Minimal read-only file backend.  The namespace is deliberately tiny and
 * deterministic for now; it proves the file-manager boundary with real
 * position-aware reads without pretending that the CF/RBF layer is native. */
int Q9K_ProcNativeRead(Q9_u16 pathNum, Q9_u32 bufferPtr, Q9_u32 count,
                       Q9_u32 *outCount, Q9_u16 *outError)
{
    static const Q9_u8 motd[] = "Q9 native I/O\r\n";
    Q9_u32 pathDesc;
    Q9_u32 position;
    Q9_u32 available;
    Q9_u32 amount;
    Q9_u16 kind;
    Q9_u16 object;
    Q9_u8 mode;
    Q9_u16 refs;
    Q9_u16 err = 0;
    Q9_u32 i;

    if (outCount)
        *outCount = 0;
    pathDesc = Q9K_ProcPathDesc(pathNum, &err);
    if (pathDesc == 0) {
        if (outError)
            *outError = err;
        return 0;
    }
    kind = Q9K_ReadU16BE(pathDesc + Q9K_PATHDESC_KIND_OFF);
    object = Q9K_ReadU16BE(pathDesc + Q9K_PATHDESC_OBJECT_OFF);
    mode = *(volatile Q9_u8 *)(pathDesc + Q9K_PATHDESC_MODE_OFF);
    refs = Q9K_ReadU16BE(pathDesc + Q9K_PATHDESC_REF_OFF);
    position = Q9K_ReadU32BE_At(pathDesc + Q9K_PATHDESC_POS_OFF);
    (void)refs;
    if ((mode & 0x01U) == 0) {
        if (outError)
            *outError = Q9K_E_BMODE;
        return 0;
    }
    if (kind != Q9K_PATH_KIND_FILESYSTEM ||
        object != Q9K_NATIVE_OBJECT_MOTD) {
        if (outError)
            *outError = Q9K_E_UNKSVC;
        return 0;
    }
    if (bufferPtr == 0 && count != 0) {
        if (outError)
            *outError = 0x00D2U;
        return 0;
    }
    if (position >= (Q9_u32)(sizeof(motd) - 1U)) {
        if (outError)
            *outError = 0;
        return 1;
    }
    available = (Q9_u32)(sizeof(motd) - 1U) - position;
    amount = count < available ? count : available;
    for (i = 0; i < amount; i++)
        *(volatile Q9_u8 *)(bufferPtr + i) = motd[position + i];
    Q9K_WriteU32BE_At(pathDesc + Q9K_PATHDESC_POS_OFF, position + amount);
    if (outCount)
        *outCount = amount;
    if (outError)
        *outError = 0;
    return 1;
}

/* Eigene Kernel-Global-Erweiterungen fuer die ASM<->C-Uebergabe von
 * Q9K_SysIOpenImpl (q9kernel_entry.a) -- gleiches, etabliertes Muster
 * wie ueberall in diesem Kernel. Direkt hinter den SSvc-Scratch-Feldern
 * ($134C-$1354, q9kernel_entry.a/ssvc.c) -- naechste freie Adresse
 * $1360 (die davor genutzten temporaeren Debug-Adressen der
 * "!"-Raetsel-Sitzung sind inzwischen wieder frei). */
/* F$AllPD-Scratch (2026-09-02), direkt hinter Q9K_TrapA4Save ($13A8) und
 * vor der F$SSvc-Markierungstabelle ($1400) -- s. Belegungsuebersicht in
 * q9kernel_entry.a. Wie alle anderen per #define ueberschreibbar, damit
 * der Hosttest auf echte Puffer umlenken kann. */
#ifndef Q9K_ALLPD_SCRATCH_DBTIN
#define Q9K_ALLPD_SCRATCH_DBTIN   0x13ACUL   /* Q9_u32, (a0) EIN  = DBT-Basis */
#endif
#ifndef Q9K_ALLPD_SCRATCH_DESC
#define Q9K_ALLPD_SCRATCH_DESC    0x13B0UL   /* Q9_u32, (a1) AUS = Deskriptorzeiger */
#endif
#ifndef Q9K_ALLPD_SCRATCH_NUM
#define Q9K_ALLPD_SCRATCH_NUM     0x13B4UL   /* Q9_u32, d0.w AUS = Deskriptornummer */
#endif
#ifndef Q9K_ALLPD_SCRATCH_ERROR
#define Q9K_ALLPD_SCRATCH_ERROR   0x13B8UL   /* Q9_u32, d1.w AUS bei Fehler */
#endif
#ifndef Q9K_ALLPD_SCRATCH_SUCCESS
#define Q9K_ALLPD_SCRATCH_SUCCESS 0x13BCUL   /* Q9_u32, 0 = Fehlschlag / 1 = Erfolg */
#endif

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
#ifndef Q9K_IOpenScratch_Error
#define Q9K_IOpenScratch_Error    0x1370UL   /* Q9_u32, d1.w AUS bei Fehler */
#endif

/* Q9K_SysIOpenImpl -- duenne, PARAMETERLOSE Bruecke zwischen dem
 * Assembler-Trampolin und Q9K_ProcIOpen -- gleiches Muster wie ueberall
 * (Q9K_SysForkImpl usw.). */
/* Q9K_SysAllPDImpl -- duenne, PARAMETERLOSE Bruecke fuer F$AllPD,
 * gleiches Muster wie Q9K_SysIOpenImpl. */
void Q9K_SysAllPDImpl(void)
{
    Q9_u32 dbt  = Q9K_GetU32(Q9K_ALLPD_SCRATCH_DBTIN);
    Q9_u32 desc = 0;
    Q9_u16 num  = 0;
    Q9_u16 err  = 0;

    if (Q9K_ProcAllPD(dbt, &desc, &num, &err)) {
        Q9K_SetU32(Q9K_ALLPD_SCRATCH_DESC, desc);
        Q9K_SetU32(Q9K_ALLPD_SCRATCH_NUM, (Q9_u32)num);
        Q9K_SetU32(Q9K_ALLPD_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_ALLPD_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_ALLPD_SCRATCH_SUCCESS, 0UL);
    }
}

/* Q9K_SysRetPDImpl -- duenne, PARAMETERLOSE Bruecke fuer F$RetPD,
 * gleiches Muster wie Q9K_SysAllPDImpl daneben. */
void Q9K_SysRetPDImpl(void)
{
    Q9_u16 err = 0;
    Q9_u32 dbt = Q9K_GetU32(Q9K_RETPD_SCRATCH_DBTIN);
    Q9_u16 num = (Q9_u16)Q9K_GetU32(Q9K_RETPD_SCRATCH_NUMIN);

    if (Q9K_ProcRetPD(dbt, num, &err)) {
        Q9K_SetU32(Q9K_RETPD_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_RETPD_SCRATCH_SUCCESS, 0UL);
    }
    Q9K_SetU32(Q9K_RETPD_SCRATCH_ERROR, (Q9_u32)err);
}

/* Scratch-Bruecke fuer F$PrsNam, gleiches Muster wie ueberall. */
#ifndef Q9K_PRSNAM_SCRATCH_PATH
#define Q9K_PRSNAM_SCRATCH_PATH   0x13ECUL   /* Q9_u32, (a0) EIN  = Pfadname */
#define Q9K_PRSNAM_SCRATCH_NAME   0x13F0UL   /* Q9_u32, (a1) AUS = hinter dem letzten Namenszeichen (NICHT Namensanfang, s. Fix-Kommentar bei Q9K_ProcPrsNam) */
#define Q9K_PRSNAM_SCRATCH_PAST   0x13F4UL   /* Q9_u32, (a0) AUS = Anfang des Namens (fuer F$CmpNam), s. Fortsetzung 24 */
#define Q9K_PRSNAM_SCRATCH_LEN    0x13F8UL   /* Q9_u32, d1.w AUS = Laenge */
#define Q9K_PRSNAM_SCRATCH_DELIM  0x13FCUL   /* Q9_u32, d0.b AUS = Trennzeichen */
#define Q9K_PRSNAM_SCRATCH_ERROR  0x1600UL   /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_PRSNAM_SCRATCH_OK     0x1604UL   /* Q9_u32, 0 = Fehlschlag / 1 = Erfolg */
#endif

void Q9K_SysPrsNamImpl(void)
{
    Q9_u32 nameStart = 0, pastName = 0;
    Q9_u16 len = 0, delim = 0, err = 0;

    if (Q9K_ProcPrsNam(Q9K_GetU32(Q9K_PRSNAM_SCRATCH_PATH),
                       &nameStart, &pastName, &len, &delim, &err)) {
        Q9K_SetU32(Q9K_PRSNAM_SCRATCH_NAME, nameStart);
        Q9K_SetU32(Q9K_PRSNAM_SCRATCH_PAST, pastName);
        Q9K_SetU32(Q9K_PRSNAM_SCRATCH_LEN, (Q9_u32)len);
        Q9K_SetU32(Q9K_PRSNAM_SCRATCH_DELIM, (Q9_u32)delim);
        Q9K_SetU32(Q9K_PRSNAM_SCRATCH_OK, 1UL);
    } else {
        Q9K_SetU32(Q9K_PRSNAM_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_PRSNAM_SCRATCH_OK, 0UL);
    }
}

void Q9K_SysIOpenImpl(void)
{
    Q9_u32 mode     = Q9K_GetU32(Q9K_IOpenScratch_Mode);
    Q9_u32 namePtr  = Q9K_GetU32(Q9K_IOpenScratch_NamePtr);
    Q9_u32 pastName = 0;
    Q9_u32 pathNum;
    Q9_u32 pathDesc;
    Q9_u16 err = 0;
    Q9_u16 resolveErr = 0;

    pathNum = Q9K_ProcIOpen(mode, namePtr, &pastName, &err);
    if (pathNum != 0) {
        /* Exercise the same local-to-global lookup that future native I/O
         * handlers will use.  I$Open must never report a usable path whose
         * published P$Path entry cannot be resolved by that common gate. */
        pathDesc = Q9K_ProcPathDesc((Q9_u16)pathNum, &resolveErr);
        if (pathDesc == 0) {
            pathNum = 0;
            err = resolveErr;
        }
    }

    Q9K_SetU32(Q9K_IOpenScratch_PastName, pastName);
    Q9K_SetU32(Q9K_IOpenScratch_PathNum, pathNum);
    Q9K_SetU32(Q9K_IOpenScratch_Error, (Q9_u32)err);
}

/*
 * F$CmpNam (Callcode $11, "Compare Two Names")
 *
 * Verifizierte ABI (68k_tech.pdf S. 388):
 *
 *     IN   d1.w = Laenge der Musterzeichenkette
 *          (a0) = Zeiger auf das Muster
 *          (a1) = Zeiger auf den Zielnamen (NULL-terminiert)
 *     OUT  Carry geloescht, wenn die Namen uebereinstimmen
 *          sonst Carry gesetzt + d1.w = E$Differ ($A5)
 *
 * Gross-/Kleinschreibung gilt als gleich. Zwei Platzhalter im MUSTER:
 * '?' trifft genau ein Zeichen, '*' trifft eine beliebige Zeichenkette.
 *
 * Das Muster ist NICHT null-terminiert (deshalb die Laenge in d1.w) --
 * genau so ruft RBF den Call auf: F$PrsNam liefert Namensanfang und
 * Laenge mitten aus einem Pfadnamen heraus (s. Q9K_PrsNamScratch_PAST
 * oben), und beides geht unveraendert an F$CmpNam weiter.
 *
 * BEWUSSTE ABWEICHUNG: die reale Beschreibung nennt als zweiten Fehler
 * E$StkOvf ($A6) "Muster zu komplex" -- das ist eine Eigenschaft der
 * Original-Implementierung, die beim '*' rekursiv auf dem Stack sichert.
 * Diese Fassung vergleicht stattdessen iterativ mit einem einzigen
 * Rueckfallpunkt (konstanter Speicherbedarf), kann also kein Muster
 * ueberlaufen lassen und meldet E$StkOvf folgerichtig nie. Das ist
 * strikt vertraeglich: jedes Muster, das das Original akzeptiert, wird
 * auch hier akzeptiert.
 */
#define Q9K_E_DIFFER 0x00A5U /* errno.h: EOS_DIFFER, Namen unterschiedlich */

/* F$FindPD-Scratch (2026-09-18): hinter dem F$DelPrc-Block
 * ($1960-$1968, q9kernel_firstproc.c). */
#ifndef Q9K_FINDPD_SCRATCH_NUM
#define Q9K_FINDPD_SCRATCH_NUM     0x1970UL /* Q9_u32, d0.w EIN            */
#define Q9K_FINDPD_SCRATCH_DBT     0x1974UL /* Q9_u32, (a0) EIN            */
#define Q9K_FINDPD_SCRATCH_DESC    0x1978UL /* Q9_u32, (a1) AUS            */
#define Q9K_FINDPD_SCRATCH_ERROR   0x197CUL /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_FINDPD_SCRATCH_SUCCESS 0x1980UL /* Q9_u32, 0/1                 */
#endif

static unsigned char Q9K_CmpNamFold(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ? (unsigned char)(c - 'A' + 'a') : c;
}

int Q9K_ProcCmpNam(Q9_u32 patternPtr, Q9_u16 patternLen, Q9_u32 targetPtr,
                   Q9_u16 *outError)
{
    const volatile unsigned char *pat;
    const volatile unsigned char *tgt;
    Q9_u32 pi = 0, ti = 0;
    Q9_u32 starPat = 0, starTgt = 0;
    int haveStar = 0;

    *outError = 0;

    if (patternPtr == 0 || targetPtr == 0) {
        *outError = Q9K_E_DIFFER;
        return 0;
    }

    pat = (const volatile unsigned char *)patternPtr;
    tgt = (const volatile unsigned char *)targetPtr;

    while (tgt[ti] != 0) {
        if (pi < (Q9_u32)patternLen && pat[pi] == '*') {
            /* Rueckfallpunkt merken und zunaechst nichts verbrauchen. */
            haveStar = 1;
            starPat  = pi;
            starTgt  = ti;
            pi++;
        } else if (pi < (Q9_u32)patternLen &&
                   (pat[pi] == '?' ||
                    Q9K_CmpNamFold(pat[pi]) == Q9K_CmpNamFold(tgt[ti]))) {
            pi++;
            ti++;
        } else if (haveStar) {
            /* Letztes '*' ein Zeichen weiter fressen lassen. */
            starTgt++;
            ti = starTgt;
            pi = starPat + 1;
        } else {
            *outError = Q9K_E_DIFFER;
            return 0;
        }
    }

    /* Der Zielname ist zu Ende -- ein Restmuster darf nur noch aus '*'
     * bestehen. */
    while (pi < (Q9_u32)patternLen && pat[pi] == '*') {
        pi++;
    }

    if (pi != (Q9_u32)patternLen) {
        *outError = Q9K_E_DIFFER;
        return 0;
    }
    return 1;
}

/* Scratch-Bruecke fuer F$CmpNam (2026-09-17): hinter dem F$SPrior-Block
 * ($18A0-$18AC, q9kernel_procapi.c), der seinerseits hinter der
 * F$SRqMem-Eigentuemertabelle liegt (endet $1894, q9kernel_sysmem.c). */
#ifndef Q9K_CMPNAM_SCRATCH_PATTERN
#define Q9K_CMPNAM_SCRATCH_PATTERN 0x18B0UL /* Q9_u32, (a0) EIN = Muster       */
#define Q9K_CMPNAM_SCRATCH_TARGET  0x18B4UL /* Q9_u32, (a1) EIN = Zielname     */
#define Q9K_CMPNAM_SCRATCH_PATLEN  0x18B8UL /* Q9_u32, d1.w EIN = Musterlaenge */
#define Q9K_CMPNAM_SCRATCH_ERROR   0x18BCUL /* Q9_u32, d1.w AUS bei Fehler     */
#define Q9K_CMPNAM_SCRATCH_SUCCESS 0x18C0UL /* Q9_u32, 0 = ungleich / 1 = gleich */
#endif

void Q9K_SysCmpNamImpl(void)
{
    Q9_u16 err = 0;

    if (Q9K_ProcCmpNam(Q9K_GetU32(Q9K_CMPNAM_SCRATCH_PATTERN),
                       (Q9_u16)Q9K_GetU32(Q9K_CMPNAM_SCRATCH_PATLEN),
                       Q9K_GetU32(Q9K_CMPNAM_SCRATCH_TARGET),
                       &err)) {
        Q9K_SetU32(Q9K_CMPNAM_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_CMPNAM_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_CMPNAM_SCRATCH_SUCCESS, 0UL);
    }
}

/* Q9K_ProcFindPD -- echte F$FindPD-Kernlogik (Callcode $2F, "Find
 * Process/Path Descriptor"). Verifizierte ABI (68k_tech.pdf S. 425):
 * d0.w = Prozess-/Pfadnummer, (a0) = Tabellenzeiger;
 * AUS (a1) = Zeiger auf den Deskriptor. Systemzustand.
 *
 * Der Aufruf ist die reine Nachschlagehaelfte des Trios, dessen andere
 * beiden Teile schon hier stehen: F$AllPD vergibt eine Nummer, F$RetPD
 * gibt sie zurueck, F$FindPD uebersetzt sie in die Adresse. Deshalb
 * dieselbe, aus IOMans Code belegte DBT-Struktur wie dort (s.
 * Kopfkommentar von Q9K_ProcAllPD) und dieselben Fehlercodes -- Nummer 0
 * ist ungueltig, weil Offset 0 der Tabellenkopf selbst ist.
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
int Q9K_ProcFindPD(Q9_u32 dbtAddr, Q9_u16 num, Q9_u32 *outDesc, Q9_u16 *outError)
{
    Q9_u16 maxIndex;
    Q9_u32 desc;

    *outDesc  = 0;
    *outError = 0;

    if (dbtAddr == 0) {
        *outError = 0x00D2U;            /* E_BPADDR, Bad Page Address */
        return 0;
    }

    maxIndex = Q9K_ReadU16BE(dbtAddr);

    if (num == 0 || (Q9_u32)num > (Q9_u32)maxIndex) {
        *outError = 0x00C9U;            /* E_BPNUM, Bad Path Number */
        return 0;
    }

    desc = Q9K_ReadU32BE_At(dbtAddr + (Q9_u32)num * 4UL);
    if (desc == 0) {
        *outError = 0x00C9U;            /* Nummer gueltig, aber nicht vergeben */
        return 0;
    }

    *outDesc = desc;
    return 1;
}

void Q9K_SysFindPDImpl(void)
{
    Q9_u32 desc = 0;
    Q9_u16 err = 0;

    if (Q9K_ProcFindPD(Q9K_GetU32(Q9K_FINDPD_SCRATCH_DBT),
                       (Q9_u16)Q9K_GetU32(Q9K_FINDPD_SCRATCH_NUM),
                       &desc, &err)) {
        Q9K_SetU32(Q9K_FINDPD_SCRATCH_DESC, desc);
        Q9K_SetU32(Q9K_FINDPD_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_FINDPD_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_FINDPD_SCRATCH_SUCCESS, 0UL);
    }
}
