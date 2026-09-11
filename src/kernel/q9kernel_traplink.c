/*
 * q9kernel_traplink.c -- Q9-OS eigener Kernel: F$TLink (Callcode 0x21,
 *                        "Install User Trap Handler Module").
 *
 * Anlass (2026-09-11, Abschnitt "F$Load-Meilenstein"/Folgetest): ein per
 * F$Load geladenes echtes Kommandomodul ("/dd/CMDS/echo") per F$Fork
 * ausgefuehrt -- es startet, laeuft echten Microware-Code, versucht
 * dann per F$TLink(13,"csl") seine C-Laufzeitbibliothek "csl" zu
 * installieren (per Rueckspringadressen-/Byte-Muster-Forensik in
 * echo.mod direkt gefunden: trap 13, Name "csl", KEIN F$STrap/T$Math-
 * Bezug trotz aehnlichem Namen) und scheitert daran mit "**** can't
 * install csl ****", weil F$TLink bisher komplett fehlte.
 *
 * Reale Konvention (68k_tech.pdf S. 524f, "F$TLink -- Install User Trap
 * Handler Module", echt per Read gelesen -- UND per komplettem
 * Beispielcode in Kapitel 5 "User Trap Handlers" gegengeprueft):
 *
 *   IN:  d0.w = User-Trap-Nummer (1-15)
 *        d1.l = optionale Speichergroesse (0 = Vorgabe aus M$Mem nehmen)
 *        (a0) = Modulnamenzeiger (0 oder leerer String = Trap entfernen
 *               -- HIER NOCH NICHT IMPLEMENTIERT, s. u.)
 *   OUT (Erfolg): (a0) = hinter den Namen aktualisiert,
 *        (a1) = Trap-Ausfuehrungs-Einsprung (M$Exec des Trap-Moduls),
 *        (a2) = Trap-Modulzeiger, Carry geloescht.
 *   OUT (Fehlschlag): Carry gesetzt, d1.w = Fehlercode.
 *
 * F$TLink selbst macht laut Handbuch drei Dinge: das Modul linken/
 * laden, dessen statischen Speicher bereitstellen (falls noetig), und
 * dessen Initialisierungsroutine (M$Init, Offset $48) GENAU EINMAL
 * aufrufen. Der dritte Schritt (echter Fremdaufruf mit einem speziellen,
 * von M$Init selbst per "movem.l (a7),a6 / addq.l #8,a7 / rts"
 * konsumierten Stack-Rahmen) passiert in Q9K_SysFTLink
 * (q9kernel_entry.a) -- diese Datei liefert nur die Buchhaltung (Modul
 * finden, Speicher anfordern, Trap-Tabelle des AKTUELLEN Prozesses
 * eintragen) und reicht Modulzeiger/Ausfuehrungs-/Init-Einsprung/
 * Speicherzeiger an die Scratch-Zellen weiter.
 *
 * NEU: pro-Prozess-Trap-Tabelle (15 Eintraege, s. Q9K_PROCDESC_TRAPTBL_OFF
 * unten) -- eigene Erweiterung OHNE OS-9-Entsprechung an bekannter
 * Stelle (direkt hinter Q9K_PROCDESC_ENTRYPC_OFF=$1C8, s.
 * q9kernel_firstproc.c), da kein fremdes Modul diese Tabelle je liest
 * (nur unser eigener TRAP-#1-15-Dispatcher, s. q9kernel_exctable.c).
 * Jeder Eintrag 12 Byte: +0 Modulzeiger (0 = frei), +4 Ausfuehrungs-
 * Einsprung, +8 statischer Speicherzeiger (0 = keiner).
 *
 * Vereinfachung: KEIN F$Load-Fallback, falls das Modul noch nicht im
 * Verzeichnis steht (reines F$Link-Verhalten wie bei F$Link selbst) --
 * das Handbuch beschreibt zwar "link, or load", aber ein disk-ladender
 * Aufruf aus einem C-Handler heraus wuerde denselben trampolinartigen
 * Fremdaufruf-Mechanismus brauchen wie F$Load selbst (IOMans eigene
 * Implementierung) -- fuer den Regelfall dieser Sitzung (csl vorher
 * explizit per F$Load bereitgestellt) nicht noetig. Echtes TODO, falls
 * ein Trap-Modul je NUR ueber F$TLink (ohne vorheriges F$Load) verfuegbar
 * sein soll.
 *
 * Ebenfalls NICHT implementiert: Entfernen eines Traps (namePtr=0),
 * die "bereits installiert"-Kollisionspruefung nutzt einen plausiblen,
 * aber nicht im Handbuch konkret benannten Fehlercode (E$ModBsy, wie
 * bei anderen "schon belegt"-Faellen in diesem Kernel).
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

/* Aus q9kernel_moddir.c/q9kernel_sysmem.c -- externe Deklarationen statt
 * gemeinsamer Header, gleiche schlanke Konvention wie ueberall in diesem
 * Kernel. */
extern Q9_u32 Q9K_ModDirLinkByName(Q9_u16 desiredTyLang, const char *name);
extern int    Q9K_ProcSRqMem(Q9_u32 requestedSize, Q9_u32 *outAddr, Q9_u32 *outSize, Q9_u16 *outError);

/* Modulheader-Offsets (s. src/q9moduleheader.h Q9_MH68K_*) -- lokal
 * dupliziert, gleiche Konvention wie q9kernel_moddir.c/q9kernel_modsearch.c. */
#define Q9K_MH_EXEC   0x30UL
#define Q9K_MH_MEM    0x38UL
#define Q9K_MH_INIT   0x48UL
/* NACHTRAG (2026-09-11, Fortsetzung 49) -- s. ausfuehrliche Begruendung
 * bei Q9K_ApplyInitializedData unten: "csl" (wie jedes echte, compilierte
 * C-Modul) hat SELBST initialisierte globale/statische Daten, die bisher
 * NIRGENDS angewendet wurden (Q9K_ProcFork deckt nur GEFORKTE Prozesse
 * ab, F$TLink lief bisher komplett daran vorbei). */
#define Q9K_MH_IDATA  0x40UL
#define Q9K_MH_IREFS  0x44UL

/* Kernel-Global D_Proc -- echte, verifizierte Adresse (s. jede andere
 * Datei dieses Kernels, die den aktuellen Prozessdeskriptor braucht). */
#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif

/* Eigene Erweiterung des Prozessdeskriptors (s. Kopfkommentar) -- naechste
 * freie Adresse nach Q9K_PROCDESC_ENTRYPC_OFF ($1C8+4=$1CC,
 * q9kernel_firstproc.c). 15 Eintraege x 12 Byte = 180 Byte ($B4),
 * endet bei $280 -- weit innerhalb der echten Deskriptorgroesse ($400,
 * Q9K_PROCDESC_SIZE). */
#ifndef Q9K_PROCDESC_TRAPTBL_OFF
#define Q9K_PROCDESC_TRAPTBL_OFF 0x1CCUL
#endif
#define Q9K_TRAPTBL_ENTRY_SIZE   12UL
#define Q9K_TRAPTBL_OFF_MODPTR    0UL
#define Q9K_TRAPTBL_OFF_EXECENTRY 4UL
#define Q9K_TRAPTBL_OFF_STATICPTR 8UL

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
static Q9_u8  Q9K_GetU8(Q9_u32 addr) { return *(volatile Q9_u8 *)addr; }
static void   Q9K_SetU8(Q9_u32 addr, Q9_u8 value) { *(volatile Q9_u8 *)addr = value; }

/* Byteweises Big-Endian-Lesen eines Langworts -- gleiche Begruendung wie
 * ueberall (Host-Test laeuft little-endian, Ziel big-endian). */
static Q9_u32 Q9K_TLinkReadU32BE(Q9_u32 addr)
{
    const volatile Q9_u8 *p = (const volatile Q9_u8 *)addr;
    return ((Q9_u32)p[0] << 24) | ((Q9_u32)p[1] << 16) |
           ((Q9_u32)p[2] << 8)  | (Q9_u32)p[3];
}

/* Wie Q9K_TLinkReadU32BE, nur 16 Bit -- fuer M$IRefs (s.
 * Q9K_ApplyInitializedData unten). */
static Q9_u16 Q9K_TLinkReadU16BE(Q9_u32 addr)
{
    const volatile Q9_u8 *p = (const volatile Q9_u8 *)addr;
    return (Q9_u16)(((Q9_u32)p[0] << 8) | (Q9_u32)p[1]);
}

/* NACHTRAG (2026-09-11, Fortsetzung 49) -- M$IData/M$IRefs (68k_tech.pdf
 * Table 1-8), 1:1 dieselbe Logik wie Q9K_ApplyInitializedData in
 * q9kernel_firstproc.c (dort ausfuehrlicher Kopfkommentar samt Byte-
 * Ebenen-Verifikation gegen echo.mod) -- HIER EIGENSTAENDIG DUPLIZIERT
 * (gleiche, im ganzen Kernel etablierte Konvention: keine gemeinsamen
 * Header fuer interne Offsets/Helfer, s. Kopfkommentar dort).
 *
 * Anlass: NICHT nur GEFORKTE Prozesse (Q9K_ProcFork) brauchen das --
 * "csl" selbst (M$IData=$afa0, M$IRefs=$bb08 im real vermessenen
 * csl.mod) hat GENAU DIESELBEN zwei Felder, aber F$TLink lief bisher
 * komplett daran vorbei. Live beobachtet (2026-09-11): nach erfolgreichem
 * F$TLink+F$Fork stuerzt "echo" beim ERSTEN echten Aufruf einer
 * "csl"-Funktion mit Vektor 4 (Illegal Instruction) bei einer winzigen
 * PC-Adresse ab ($6c) -- klassisches Symptom eines Sprungs durch einen
 * NICHT relozierten (weil nie initialisierten) Zeiger, genau wie beim
 * juengst geloesten "echo"-eigenen Fall, nur diesmal in "csl"s EIGENEM,
 * von F$TLink bereitgestelltem statischem Speicher (staticPtr, s.
 * Q9K_ProcTLink -- wird spaeter als a6 an M$Init uebergeben, exakt die
 * Rolle, die "block"/a6 bei Q9K_ProcFork spielt). */
static void Q9K_ApplyInitializedData(Q9_u32 hdrAddr, Q9_u32 block)
{
    Q9_u32 idataOff = Q9K_TLinkReadU32BE(hdrAddr + Q9K_MH_IDATA);
    Q9_u32 irefsOff = Q9K_TLinkReadU32BE(hdrAddr + Q9K_MH_IREFS);
    Q9_u32 p, end;
    Q9_u32 group;
    Q9_u32 relocBase;

    if (idataOff == 0 && irefsOff == 0)
        return;   /* kein initialisierter Speicher deklariert -- unveraendert */

    if (idataOff != 0) {
        p = hdrAddr + idataOff;
        end = hdrAddr + irefsOff;
        while (p < end) {
            Q9_u32 dstOff = Q9K_TLinkReadU32BE(p);
            Q9_u32 count  = Q9K_TLinkReadU32BE(p + 4);
            Q9_u32 i;
            for (i = 0; i < count; i++)
                Q9K_SetU8(block + dstOff + i, Q9K_GetU8(p + 8 + i));
            p += 8 + count;
        }
    }

    if (irefsOff != 0) {
        p = hdrAddr + irefsOff;
        for (group = 0; group < 2; group++) {
            Q9_u32 count;
            Q9_u32 j;

            (void)Q9K_TLinkReadU16BE(p);      /* MS-Wort -- bisher immer 0, verworfen */
            count = Q9K_TLinkReadU16BE(p + 2);
            p += 4;
            relocBase = (group == 0) ? hdrAddr : block;
            for (j = 0; j < count; j++) {
                Q9_u32 fieldOff = Q9K_TLinkReadU16BE(p);
                Q9_u32 fieldAddr = block + fieldOff;
                Q9_u32 newVal;
                p += 2;
                newVal = Q9K_TLinkReadU32BE(fieldAddr) + relocBase;
                Q9K_SetU8(fieldAddr + 0, (Q9_u8)(newVal >> 24));
                Q9K_SetU8(fieldAddr + 1, (Q9_u8)(newVal >> 16));
                Q9K_SetU8(fieldAddr + 2, (Q9_u8)(newVal >> 8));
                Q9K_SetU8(fieldAddr + 3, (Q9_u8)newVal);
            }
            p += 4;   /* Terminierungspaar MS=0/Anzahl=0 der Gruppe ueberspringen */
        }
    }
}

/* Real belegte Fehlercodes (MWOS/OS9/SRC/DEFS/funcs.a, per E$UnkSvc/
 * E$BPAddr/E$BPNam als Anker ausgezaehlt, s. docs/OWN_KERNEL_STATUS.md). */
#define Q9K_ERR_MODBSY 0x00D1U   /* E$ModBsy, Module Busy */
#define Q9K_ERR_MNF    0x00DDU   /* E$MNF, Module Not Found */
#define Q9K_ERR_PARAM  0x00E1U   /* E$Param, Impossible parameter specified */

/* Q9K_ProcTLink -- echte F$TLink-Kernlogik (s. Kopfkommentar).
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
int Q9K_ProcTLink(Q9_u32 trapNum, Q9_u32 memOverride, Q9_u32 namePtr,
                  Q9_u32 *outPastName, Q9_u32 *outModPtr, Q9_u32 *outExecEntry,
                  Q9_u32 *outInitEntry, Q9_u32 *outStaticPtr, Q9_u16 *outError)
{
    Q9_u32 p;
    Q9_u32 curProc;
    Q9_u32 slotAddr;
    Q9_u32 hdr;
    Q9_u32 size;
    Q9_u32 staticPtr = 0;

    *outPastName  = 0;
    *outModPtr    = 0;
    *outExecEntry = 0;
    *outInitEntry = 0;
    *outStaticPtr = 0;
    *outError     = 0;

    /* Namensende bestimmen -- NUL-terminiert, wie bei jedem eigenen
     * Testaufruf dieses Kernels (echte Pfadname-Trennzeichen-Erkennung
     * wie bei F$PrsNam braucht F$TLink laut Manual nicht: der Name ist
     * ein reiner Modulname, kein Pfad). */
    p = namePtr;
    while (*(volatile Q9_u8 *)p != 0)
        p++;
    p++;
    *outPastName = p;

    if (trapNum < 1UL || trapNum > 15UL) {
        *outError = Q9K_ERR_PARAM;
        return 0;
    }

    curProc = Q9K_GetU32(Q9_D_PROC);
    slotAddr = curProc + Q9K_PROCDESC_TRAPTBL_OFF + (trapNum - 1UL) * Q9K_TRAPTBL_ENTRY_SIZE;

    if (Q9K_GetU32(slotAddr + Q9K_TRAPTBL_OFF_MODPTR) != 0) {
        *outError = Q9K_ERR_MODBSY;   /* fuer diesen Trap ist schon etwas installiert */
        return 0;
    }

    hdr = Q9K_ModDirLinkByName(0, (const char *)(unsigned long)namePtr);
    if (hdr == 0) {
        *outError = Q9K_ERR_MNF;
        return 0;
    }

    *outExecEntry = hdr + Q9K_TLinkReadU32BE(hdr + Q9K_MH_EXEC);
    *outInitEntry = hdr + Q9K_TLinkReadU32BE(hdr + Q9K_MH_INIT);
    *outModPtr    = hdr;

    /* Statischen Speicher bereitstellen -- Aufrufer-Override, sonst die
     * vom Modul selbst deklarierte Groesse (M$Mem). 0 bei beidem heisst
     * "kein eigener Speicher noetig" (reale Trap-Handler koennen das,
     * s. das Beispiel in Kapitel 5 -- die dortige TrapInit tut nichts
     * mit ihrem Speicher). */
    size = memOverride;
    if (size == 0)
        size = Q9K_TLinkReadU32BE(hdr + Q9K_MH_MEM);

    if (size != 0) {
        Q9_u32 grantedSize = 0;
        Q9_u16 memErr = 0;

        if (!Q9K_ProcSRqMem(size, &staticPtr, &grantedSize, &memErr)) {
            *outError = memErr;
            return 0;
        }
        /* NACHTRAG 2026-09-11 (Fortsetzung 49): M$IData/M$IRefs des
         * Trap-Moduls selbst anwenden -- s. ausfuehrlichen Kopfkommentar
         * bei Q9K_ApplyInitializedData oben. NUR wenn wirklich eigener
         * Speicher bereitgestellt wurde (staticPtr!=0, s. "size==0"-Fall
         * oben) -- ohne eigenen Speicher gibt es kein Ziel zum Kopieren. */
        Q9K_ApplyInitializedData(hdr, staticPtr);
    }
    *outStaticPtr = staticPtr;

    Q9K_SetU32(slotAddr + Q9K_TRAPTBL_OFF_MODPTR, hdr);
    Q9K_SetU32(slotAddr + Q9K_TRAPTBL_OFF_EXECENTRY, *outExecEntry);
    Q9K_SetU32(slotAddr + Q9K_TRAPTBL_OFF_STATICPTR, staticPtr);

    return 1;
}

/* Scratch-Bruecke fuer den Assembler-Trampolin (Q9K_SysFTLink,
 * q9kernel_entry.a) -- gleiches, etabliertes Muster wie ueberall in
 * diesem Kernel. Freier Bereich hinter Q9K_VMODUL_RETBUF ($1650-$1663,
 * q9kernel_moddir.c). */
#ifndef Q9K_TLINK_SCRATCH_TRAPNUM
#define Q9K_TLINK_SCRATCH_TRAPNUM   0x1690UL   /* Q9_u32, d0.w EIN */
#define Q9K_TLINK_SCRATCH_MEMOVR    0x1694UL   /* Q9_u32, d1.l EIN */
#define Q9K_TLINK_SCRATCH_NAMEPTR   0x1698UL   /* Q9_u32, (a0) EIN */
#define Q9K_TLINK_SCRATCH_PASTNAME  0x169CUL   /* Q9_u32, (a0) AUS */
#define Q9K_TLINK_SCRATCH_MODPTR    0x16A0UL   /* Q9_u32, (a2) AUS */
#define Q9K_TLINK_SCRATCH_EXECENTRY 0x16A4UL   /* Q9_u32, (a1) AUS */
#define Q9K_TLINK_SCRATCH_INITENTRY 0x16A8UL   /* Q9_u32, AUS -- nur fuer den Assembler-Sprung zu M$Init */
#define Q9K_TLINK_SCRATCH_STATICPTR 0x16ACUL   /* Q9_u32, AUS -- wird a6 beim M$Init-Aufruf */
#define Q9K_TLINK_SCRATCH_ERROR     0x16B0UL   /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_TLINK_SCRATCH_SUCCESS   0x16B4UL   /* Q9_u32, 0 = Fehlschlag / 1 = Erfolg */
#endif

void Q9K_SysTLinkImpl(void)
{
    Q9_u32 trapNum = Q9K_GetU32(Q9K_TLINK_SCRATCH_TRAPNUM);
    Q9_u32 memOvr  = Q9K_GetU32(Q9K_TLINK_SCRATCH_MEMOVR);
    Q9_u32 namePtr = Q9K_GetU32(Q9K_TLINK_SCRATCH_NAMEPTR);
    Q9_u32 pastName = 0, modPtr = 0, execEntry = 0, initEntry = 0, staticPtr = 0;
    Q9_u16 err = 0;

    if (Q9K_ProcTLink(trapNum, memOvr, namePtr, &pastName, &modPtr,
                      &execEntry, &initEntry, &staticPtr, &err)) {
        Q9K_SetU32(Q9K_TLINK_SCRATCH_PASTNAME, pastName);
        Q9K_SetU32(Q9K_TLINK_SCRATCH_MODPTR, modPtr);
        Q9K_SetU32(Q9K_TLINK_SCRATCH_EXECENTRY, execEntry);
        Q9K_SetU32(Q9K_TLINK_SCRATCH_INITENTRY, initEntry);
        Q9K_SetU32(Q9K_TLINK_SCRATCH_STATICPTR, staticPtr);
        Q9K_SetU32(Q9K_TLINK_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_TLINK_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_TLINK_SCRATCH_SUCCESS, 0UL);
    }
}

/* ***************************************************************
 * EXPERIMENT (2026-09-11, Fortsetzung 48) -- dritter Anlauf, ersetzt
 * die in Fortsetzung 46 (nachtraeglich verschieben -- brach die schon
 * laufende F$TLink-Registrierung) und Fortsetzung 47 (vorab an eine
 * VORHERGESAGTE Adresse legen -- ueberlappte mit echos eigenem, bereits
 * geladenem Modul) verworfenen Ansaetze.
 *
 * Diesmal KEINE Vorhersage/Nachtraeglichkeit: EINE einzige, kombinierte
 * Q9K_AllocMem-Allokation reserviert csl UND echos kuenftigen
 * Prozessblock gemeinsam, garantiert ueberlappungsfrei mit ALLEM
 * anderen (der Allocator selbst buergt dafuer -- keine Pruefung gegen
 * die Moduldirectory noetig, keine Kollisionsmoeglichkeit MEHR, weil
 * nichts anderes zwischen dieser Allokation und ihrer Nutzung
 * dazwischenkommen kann).
 *
 * Layout der Allokation (Groesse = $E33C + echos Speicherbedarf):
 *   [combinedBlock .. combinedBlock+cslModSize)   -- csl-Kopie
 *   [combinedBlock+cslModSize .. combinedBlock+$E33C)  -- bewusster Leerraum
 *   [combinedBlock+$E33C .. Ende)                 -- echos kuenftiger
 *                                                     Prozessblock (A6)
 * Die Konstante $E33C ist die in Fortsetzung 42/44 hergeleitete
 * Differenz "echos A6 minus benoetigte csl-Basis" -- fest fuer DIESE
 * eine echo.mod/csl.mod-Kombination, keine allgemeine Konstante.
 *
 * Q9K_FORK_BLOCK_OVERRIDE (q9kernel_firstproc.c) sorgt dafuer, dass der
 * NAECHSTE F$Fork (hier: fuer "echo") exakt combinedBlock+$E33C statt
 * einer neuen Q9K_AllocMem-Allokation bekommt.
 * *************************************************************** */
#ifndef Q9K_COMB_MODDIR_HEAD
#define Q9K_COMB_MODDIR_HEAD    0x1238UL
#endif
#define Q9K_COMB_MODDIR_NEXT_OFF   0x00UL
#define Q9K_COMB_MODDIR_HDRPTR_OFF 0x04UL
#define Q9K_COMB_MH_NAME  0x0CUL
#define Q9K_COMB_MH_SIZE  0x04UL
#define Q9K_COMB_MH_MEM   0x38UL
#define Q9K_COMB_MH_STACK 0x3CUL
#define Q9K_COMB_CSL_A6_DELTA 0xE33CUL

#ifndef Q9K_COMB_SCRATCH_RESULT
#define Q9K_COMB_SCRATCH_RESULT 0x16E4UL   /* Q9_u32, AUS -- 0=ok, sonst Fehlercode */
#endif

/* = Q9K_FORK_BLOCK_OVERRIDE (q9kernel_firstproc.c) -- lokal dupliziert,
 * gleiche Konvention wie ueberall in diesem Kernel (keine Cross-File-
 * Konstante). MUSS mit dem dortigen Wert uebereinstimmen. */
#ifndef Q9K_COMB_FORK_BLOCK_OVERRIDE
#define Q9K_COMB_FORK_BLOCK_OVERRIDE 0x16F0UL
#endif

extern Q9_u32 Q9K_AllocMem(Q9_u32 requestedSize);   /* q9kernel_arena.c */

static Q9_u8 Q9K_CombGetU8(Q9_u32 addr) { return *(volatile Q9_u8 *)addr; }
static void  Q9K_CombSetU8(Q9_u32 addr, Q9_u8 value) { *(volatile Q9_u8 *)addr = value; }

static void Q9K_CombFindModule4(const char *name4, Q9_u32 *outSlot, Q9_u32 *outHdr)
{
    Q9_u32 slot = Q9K_GetU32(Q9K_COMB_MODDIR_HEAD);

    *outSlot = 0;
    *outHdr = 0;
    while (slot != 0) {
        Q9_u32 hdrAddr = Q9K_GetU32(slot + Q9K_COMB_MODDIR_HDRPTR_OFF);
        Q9_u32 nameOff = Q9K_TLinkReadU32BE(hdrAddr + Q9K_COMB_MH_NAME);
        Q9_u32 modSize = Q9K_TLinkReadU32BE(hdrAddr + Q9K_COMB_MH_SIZE);

        if (nameOff < modSize) {
            Q9_u32 np = hdrAddr + nameOff;
            if ((Q9K_CombGetU8(np)     & 0x7FU) == (Q9_u8)name4[0] &&
                (Q9K_CombGetU8(np + 1) & 0x7FU) == (Q9_u8)name4[1] &&
                (Q9K_CombGetU8(np + 2) & 0x7FU) == (Q9_u8)name4[2] &&
                (Q9K_CombGetU8(np + 3) & 0x7FU) == (Q9_u8)name4[3]) {
                *outSlot = slot;
                *outHdr = hdrAddr;
                return;
            }
        }
        slot = Q9K_GetU32(slot + Q9K_COMB_MODDIR_NEXT_OFF);
    }
}

void Q9K_ExperimentalCombinedAlloc(void)
{
    Q9_u32 cslSlot, cslHdr, echoSlot, echoHdr;

    Q9K_CombFindModule4("csl\0", &cslSlot, &cslHdr);   /* 4. Byte wird eh maskiert, "csl"+Fuellbyte passt */
    if (cslSlot == 0) { Q9K_SetU32(Q9K_COMB_SCRATCH_RESULT, 1UL); return; }

    Q9K_CombFindModule4("echo", &echoSlot, &echoHdr);
    if (echoSlot == 0) { Q9K_SetU32(Q9K_COMB_SCRATCH_RESULT, 2UL); return; }

    {
        Q9_u32 cslModSize = Q9K_TLinkReadU32BE(cslHdr + Q9K_COMB_MH_SIZE);
        Q9_u32 echoMem    = Q9K_TLinkReadU32BE(echoHdr + Q9K_COMB_MH_MEM);
        Q9_u32 echoStack  = Q9K_TLinkReadU32BE(echoHdr + Q9K_COMB_MH_STACK);
        Q9_u32 echoNeeded = echoMem + echoStack;   /* addMem=0, paramSize=0 -- unser Testaufruf */
        Q9_u32 combinedSize = Q9K_COMB_CSL_A6_DELTA + echoNeeded;
        Q9_u32 combinedBlock;

        if (cslModSize >= Q9K_COMB_CSL_A6_DELTA) {
            Q9K_SetU32(Q9K_COMB_SCRATCH_RESULT, 4UL);  /* csl passt nicht mehr vor echos A6 -- Konstante pruefen */
            return;
        }

        combinedBlock = Q9K_AllocMem(combinedSize);
        if (combinedBlock == 0) {
            Q9K_SetU32(Q9K_COMB_SCRATCH_RESULT, 5UL);
            return;
        }

        {
            Q9_u32 oldCslBase = cslHdr;
            Q9_u32 i;

            /* Kopierrichtung: combinedBlock ist eine BRANDNEUE, exklusive
             * Allokation -- kein Ueberlappungsrisiko mit oldCslBase, also
             * genuegt eine einfache Vorwaertskopie (kein memmove-Aufwand
             * wie in Fortsetzung 46 noetig). */
            for (i = 0; i < cslModSize; i++)
                Q9K_CombSetU8(combinedBlock + i, Q9K_CombGetU8(oldCslBase + i));

            Q9K_SetU32(cslSlot + Q9K_COMB_MODDIR_HDRPTR_OFF, combinedBlock);
            Q9K_SetU32(Q9K_COMB_FORK_BLOCK_OVERRIDE, combinedBlock + Q9K_COMB_CSL_A6_DELTA);
            Q9K_SetU32(Q9K_COMB_SCRATCH_RESULT, 0UL);
        }
    }
}
