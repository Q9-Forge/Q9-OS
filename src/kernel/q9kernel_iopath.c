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

/* Q9K_ProcAllPD -- echte F$AllPD-Kernlogik (Callcode $30, "Allocate
 * Process/Path Descriptor"; in OS-9/6809 hiess derselbe Dienst F$All64,
 * 1985 umbenannt, s. MWOS/OS9/SRC/DEFS/funcs.a Zeile 35).
 *
 * Aufrufkonvention aus der IOMan-Disassemblierung abgelesen (Modul-Offset
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
 *     movea.l a1,a0         a1 = Zeiger auf den Namensanfang
 *
 * Vollstaendige Ausgabe (klassische OS-9-Konvention):
 *   a1    = erstes Zeichen des Namens
 *   a0    = hinter dem Namen (auf das Trennzeichen)
 *   d0.b  = das Trennzeichen selbst
 *   d1.w  = Namenslaenge
 *   Carry gesetzt + d1.w = E_BPNAM ($D7), wenn kein gueltiger Name folgt.
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
    while (Q9K_PrsNamIsNameChar(p[i]))
        i++;

    if (i == start) {                    /* leerer Name */
        *outError = 0x00D7U;
        return 0;
    }

    *outNameStart = pathPtr + start;
    *outPastName  = pathPtr + i;
    *outLen       = (Q9_u16)(i - start);
    *outDelim     = (Q9_u16)p[i];
    return 1;
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

/* Scratch-Bruecke fuer F$PrsNam, gleiches Muster wie ueberall. */
#ifndef Q9K_PRSNAM_SCRATCH_PATH
#define Q9K_PRSNAM_SCRATCH_PATH   0x13ECUL   /* Q9_u32, (a0) EIN  = Pfadname */
#define Q9K_PRSNAM_SCRATCH_NAME   0x13F0UL   /* Q9_u32, (a1) AUS = Namensanfang */
#define Q9K_PRSNAM_SCRATCH_PAST   0x13F4UL   /* Q9_u32, (a0) AUS = hinter dem Namen */
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

    pathNum = Q9K_ProcIOpen(mode, namePtr, &pastName);

    Q9K_SetU32(Q9K_IOpenScratch_PastName, pastName);
    Q9K_SetU32(Q9K_IOpenScratch_PathNum, pathNum);
}
