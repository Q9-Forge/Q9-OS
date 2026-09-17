/*
 * q9kernel_moddir.c -- Q9-OS eigener Kernel: Modulverzeichnis (Abschnitt
 *                      "F$Link/F$UnLink", 2026-08-21).
 *
 * Vorgabe: "fangen wir mit dem Link an und dementsprechend den unlink...
 * machen wir erst alles fertig ohne I/Os" -- dieser Schritt baut NUR
 * die In-Memory-Seite (F$Link durchsucht laut Manual, 68k_tech.pdf
 * S. 461, AUSSCHLIESSLICH das In-Memory-Modulverzeichnis, nie
 * Mass-Storage -- F$Load waere ein separater, spaeterer Schritt,
 * s. intern dokumentiert).
 *
 * Speicherformat: eigene, KEIN-Kompat-Erfordernis-Festlegung (wie schon
 * bei den Prozess-/Pfad-Deskriptoren, q9kernel_firstproc.c) -- externe
 * Module sehen das Verzeichnis-interne Byte-Layout nie, nur die reale
 * F$Link/F$UnLink-Registerkonvention nach aussen muss stimmen (die
 * setzen die Assembler-Trampoline Q9K_SysFLink/Q9K_SysFUnLink in
 * q9kernel_entry.a um). Eintragsgroesse (Q9K_MODDIR_ENTRY_SIZE=16 Byte)
 * ist dagegen ECHT/verifiziert (q9kernel_tables.c) -- das eigene
 * 16-Byte-Layout unten passt exakt hinein:
 *   +0x00  Next     (4) -- Freilisten-ODER-Verzeichnisliste (je nach
 *                          Zustand, gleiches Wiederverwendungsprinzip
 *                          wie bei den Prozess-/Pfad-Pools), 0 = Ende
 *   +0x04  HdrPtr   (4) -- Zeiger auf den echten Modulkopf
 *   +0x08  TyLang   (2) -- Typ(hi)/Sprache(lo), Kopie aus dem Modulkopf
 *   +0x0A  AttRev   (2) -- Attribut(hi)/Revision(lo), Kopie
 *   +0x0C  LinkCnt  (2) -- F$Link-Zaehler
 *   +0x0E  Reserved (2)
 *
 * Zwei getrennte, durch dieselben Next-Felder verkettete Listen ueber
 * demselben Slot-Pool (Q9_D_MODDIR/Q9_D_MODDIR_END, von
 * q9kernel_tables.c ueber Q9K_BuildFreeList vorbereitet): eine
 * Freiliste (Kopf Q9K_MODDIR_FREE_ADDR) und die eigentliche, aktive
 * Verzeichnisliste (Kopf Q9K_MODDIR_HEAD_ADDR) -- ein Slot ist immer
 * nur in genau einer der beiden.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

/* Aus q9kernel_modcheck.c -- externe Deklarationen statt gemeinsamer
 * Header, gleiche schlanke Konvention wie ueberall in diesem Verzeichnis. */
extern int Q9K_CheckSyncWord(const Q9_u8 *addr, Q9_u32 availableLen);
extern int Q9K_ValidModuleHeader(const Q9_u8 *addr, Q9_u32 availableLen);
#if Q9K_MEMTRACE_COMPILETIME
extern void Q9K_MemTraceSetModule(Q9_u32 header);
extern void Q9K_MemTraceClearModule(void);
extern void Q9K_MemTraceEmit(Q9_u32 operation, Q9_u32 requested,
                             Q9_u32 address, Q9_u32 size, Q9_u32 error,
                             Q9_u32 freeHead);
#define Q9K_MEMTRACE_OP_MODULE_LOAD 7UL
#endif

/* Modulheader-Offsets (s. src/q9moduleheader.h Q9_MH68K_*) -- lokal
 * dupliziert, gleiche Konvention wie q9kernel_modsearch.c. */
#define Q9K_MH_SIZE     0x04UL
#define Q9K_MH_NAME     0x0CUL
#define Q9K_MH_TYLANG   0x12UL   /* Typ+Sprache als EIN Wort, s. Kopfkommentar */

#ifndef Q9_D_MODDIR
#define Q9_D_MODDIR 0x03CUL
#endif
#ifndef Q9_D_MODDIR_END
#define Q9_D_MODDIR_END (Q9_D_MODDIR + 4)
#endif

/* Real, feste Ziel-Offsets innerhalb eines 16-Byte-Verzeichniseintrags
 * (s. Kopfkommentar) -- per #ifndef ueberschreibbar, gleicher Grund wie
 * an jeder anderen Stelle dieses Kernels: Q9_u32 ist auf dem echten
 * 32-Bit-Ziel 4 Byte breit (Next+HdrPtr liegen dann korrekt exakt 4
 * Byte auseinander), auf einem 64-Bit-Testhost aber 8 Byte -- ein
 * Q9K_SetU32-Schreibzugriff wuerde dort in die Nachbarfelder
 * hineinschreiben. */
#ifndef Q9K_MODDIR_NEXT_OFF
#define Q9K_MODDIR_NEXT_OFF    0x00UL
#endif
#ifndef Q9K_MODDIR_HDRPTR_OFF
#define Q9K_MODDIR_HDRPTR_OFF  0x04UL
#endif
#ifndef Q9K_MODDIR_TYLANG_OFF
#define Q9K_MODDIR_TYLANG_OFF  0x08UL
#endif
#ifndef Q9K_MODDIR_ATTREV_OFF
#define Q9K_MODDIR_ATTREV_OFF  0x0AUL
#endif
#ifndef Q9K_MODDIR_LINKCNT_OFF
#define Q9K_MODDIR_LINKCNT_OFF 0x0CUL
#endif
#ifndef Q9K_MODDIR_FLAGS_OFF
/* $0E ist der letzte freie Platz im 16-Byte-Slot (NEXT $00, HDRPTR $04,
 * TYLANG $08, ATTREV $0A, LINKCNT $0C). Bit 0 = "permanent": Eintrag darf
 * nie aus dem Verzeichnis verschwinden. */
#define Q9K_MODDIR_FLAGS_OFF   0x0EUL
#define Q9K_MODDIR_FLAG_PERM   0x0001U
#endif

/* Eigene Kernel-Global-Erweiterungen, direkt hinter Q9K_TrapHandlerScratch
 * ($1230, s. q9kernel_entry.a) -- kein Feld aus dem echten Kernel-Layout. */
#ifndef Q9K_MODDIR_FREE_ADDR
#define Q9K_MODDIR_FREE_ADDR 0x1234UL
#endif
#ifndef Q9K_MODDIR_HEAD_ADDR
#define Q9K_MODDIR_HEAD_ADDR 0x1238UL
#endif

/* F$VModul-Rueckgabepuffer (2026-09-11, s. Kopfkommentar
 * Q9K_ModDirValidateAndAdd) -- 20 Byte, fest/wiederverwendet, freier
 * Bereich hinter Q9K_SRqCMemFrameScratch ($1640, q9kernel_entry.a). */
#ifndef Q9K_VMODUL_RETBUF
#define Q9K_VMODUL_RETBUF 0x1650UL
#endif

#define Q9K_E_MNF 0x00DDU /* errno.h: Module Not Found, wie in q9kernel_firstproc.c */

/* F$UnLoad-/F$CRC-Scratch (2026-09-17): hinter dem F$CpyMem-Block
 * ($18D0-$18E4, q9kernel_procapi.c). Alle Zellen 32 Bit breit, der
 * Assembler liest Wortwerte als unteres Wort ("+2"). */
#ifndef Q9K_UNLOAD_SCRATCH_TYLANG
#define Q9K_UNLOAD_SCRATCH_TYLANG  0x18E8UL /* Q9_u32, d0.w EIN            */
#define Q9K_UNLOAD_SCRATCH_NAME    0x18ECUL /* Q9_u32, (a0) EIN            */
#define Q9K_UNLOAD_SCRATCH_ERROR   0x18F0UL /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_UNLOAD_SCRATCH_SUCCESS 0x18F4UL /* Q9_u32, 0/1                 */
#endif
#ifndef Q9K_CRC_SCRATCH_COUNT
#define Q9K_CRC_SCRATCH_COUNT      0x18F8UL /* Q9_u32, d0.l EIN            */
#define Q9K_CRC_SCRATCH_ACCUM      0x18FCUL /* Q9_u32, d1.l EIN/AUS        */
#define Q9K_CRC_SCRATCH_ADDR       0x1900UL /* Q9_u32, (a0) EIN            */
#endif

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }

/* Byteweise Zusammensetzung statt Roh-Pointer-Cast fuer FREMDE Moduldaten
 * -- gleiche Begruendung/Konvention wie q9kernel_tables.c/Q9K_GetU16
 * (Big-Endian unabhaengig vom Host, keine ungerade-Adresse-Gefahr). */
static Q9_u16 Q9K_GetU16BE(Q9_u32 addr)
{
    const Q9_u8 *p = (const Q9_u8 *)addr;
    return (Q9_u16)((p[0] << 8) | p[1]);
}

static Q9_u32 Q9K_ReadU32BE(const Q9_u8 *p)
{
    return ((Q9_u32)p[0] << 24) | ((Q9_u32)p[1] << 16) |
           ((Q9_u32)p[2] << 8) | (Q9_u32)p[3];
}

static void Q9K_ModDirSetU16(Q9_u32 addr, Q9_u16 value)
{
    *(volatile Q9_u16 *)addr = value;
}

static Q9_u16 Q9K_ModDirGetU16(Q9_u32 addr)
{
    return *(volatile Q9_u16 *)addr;
}

/* Gross-/kleinschreibungsunabhaengiger Namensvergleich -- lokal
 * dupliziert, gleiche Logik wie Q9K_NamesMatch (q9kernel_modsearch.c,
 * dort static, deshalb keine gemeinsame Nutzung). moduleName darf
 * innerhalb von nameMaxLen liegen -- bricht sicher ab. */
/* Gueltiges Zeichen INNERHALB eines Modulnamens -- gleiche Menge wie in
 * F$PrsNam (Q9K_ProcPrsNam, q9kernel_iopath.c): Buchstaben, Ziffern sowie
 * '_', '.' und '$'. Alles andere beendet den Namen. */
static int Q9K_ModDirIsNameChar(Q9_u8 c)
{
    return (c >= '0' && c <= '9') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           c == '_' || c == '.' || c == '$';
}

static int Q9K_ModDirNamesMatch(const Q9_u8 *moduleName, Q9_u32 nameMaxLen, const char *targetName)
{
    Q9_u32 i;

    for (i = 0; i < nameMaxLen; i++) {
        Q9_u8 a = moduleName[i];
        Q9_u8 b = (Q9_u8)targetName[i];
        Q9_u8 aEnd = (Q9_u8)(a & 0x80U);
        Q9_u8 aLower;
        Q9_u8 bLower;

        /* ECHTER BUG GEFUNDEN + GEFIXT (2026-09-06): Ist der MODULNAME zu
         * Ende, galt bisher nur ein ebenfalls beendeter Zielname als Treffer.
         * IOMan sucht aber mit dem REST DES PFADES: beim Open von
         * "/dd/startup" fragt es nach dem Modul "dd/startup" -- der
         * Geraetename endet fuer den Kernel am '/'. Real endet ein Modulname
         * im F$Link-Aufruf am ersten Zeichen, das kein Namenszeichen ist;
         * dass danach noch Text folgt, ist ausdruecklich erlaubt (F$Link
         * liefert in a0 genau deshalb den Zeiger HINTER den Namen zurueck).
         * Symptom vorher: I$Open meldete E_MNF, obwohl F$Link("dd") den
         * Deskriptor per Einzeltest sauber fand. */
        if (a == 0)
            return Q9K_ModDirIsNameChar(b) ? 0 : 1;

        a = (Q9_u8)(a & 0x7fU);
        b = (Q9_u8)(b & 0x7fU);
        aLower = (a >= 'A' && a <= 'Z') ? (Q9_u8)(a + ('a' - 'A')) : a;
        bLower = (b >= 'A' && b <= 'Z') ? (Q9_u8)(b + ('a' - 'A')) : b;

        if (aLower != bLower)
            return 0;

        if (aEnd) {
            b = (Q9_u8)targetName[i + 1U];
            return b == 0 || !Q9K_ModDirIsNameChar((Q9_u8)(b & 0x7fU));
        }
    }
    return 0;
}

/* Holt EINEN Slot aus der Freiliste (0 = Pool erschoepft) -- gleiches
 * Muster wie Q9K_ProcPoolAlloc (q9kernel_firstproc.c). */
static Q9_u32 Q9K_ModDirPoolAlloc(void)
{
    Q9_u32 head = Q9K_GetU32(Q9K_MODDIR_FREE_ADDR);

    if (head == 0)
        return 0;

    Q9K_SetU32(Q9K_MODDIR_FREE_ADDR, Q9K_GetU32(head + Q9K_MODDIR_NEXT_OFF));
    return head;
}

/* Gibt slot an die Freiliste zurueck (LIFO, wie Q9K_ProcPoolAlloc's
 * Gegenstueck -- ein explizites "Free" existierte dort bisher nicht,
 * wird hier neu gebraucht fuer F$UnLink). */
static void Q9K_ModDirPoolFree(Q9_u32 slot)
{
    Q9K_SetU32(slot + Q9K_MODDIR_NEXT_OFF, Q9K_GetU32(Q9K_MODDIR_FREE_ADDR));
    Q9K_SetU32(Q9K_MODDIR_FREE_ADDR, slot);
}

/* Haengt slot vorne an die aktive Verzeichnisliste (Kopf-Insert --
 * guenstiger als Anhaengen, da wir hier keinen Schwanz-Zeiger fuehren;
 * Reihenfolge der Verzeichnisliste hat keine funktionale Bedeutung). */
static void Q9K_ModDirListPush(Q9_u32 slot)
{
    Q9K_SetU32(slot + Q9K_MODDIR_NEXT_OFF, Q9K_GetU32(Q9K_MODDIR_HEAD_ADDR));
    Q9K_SetU32(Q9K_MODDIR_HEAD_ADDR, slot);
}

/* Fuegt EIN bereits validiertes Modul (hdr) ins Verzeichnis ein --
 * Rueckgabe: Slot-Adresse oder 0 (Pool erschoepft, TODO: kein
 * Panic-Mechanismus vorhanden, dem das hier ohnehin mitgeteilt werden
 * koennte, gleiche Begruendung wie an anderen Stellen dieses Kernels). */
static Q9_u32 Q9K_ModDirAdd(const Q9_u8 *hdr)
{
    Q9_u32 hdrAddr = (Q9_u32)(unsigned long)hdr;
    Q9_u32 slot = Q9K_ModDirPoolAlloc();

    if (slot == 0)
        return 0;

    Q9K_SetU32(slot + Q9K_MODDIR_HDRPTR_OFF, hdrAddr);
    Q9K_ModDirSetU16(slot + Q9K_MODDIR_TYLANG_OFF, Q9K_GetU16BE(hdrAddr + Q9K_MH_TYLANG));
    Q9K_ModDirSetU16(slot + Q9K_MODDIR_ATTREV_OFF, Q9K_GetU16BE(hdrAddr + Q9K_MH_TYLANG + 2));
    Q9K_ModDirSetU16(slot + Q9K_MODDIR_LINKCNT_OFF, 0);
    Q9K_ModDirSetU16(slot + Q9K_MODDIR_FLAGS_OFF, 0);

    Q9K_ModDirListPush(slot);
    return slot;
}

/* Markiert einen Eintrag als permanent -- s. Q9K_MODDIR_FLAG_PERM. */
static void Q9K_ModDirMarkPermanent(Q9_u32 slot)
{
    Q9K_ModDirSetU16(slot + Q9K_MODDIR_FLAGS_OFF,
                     (Q9_u16)(Q9K_ModDirGetU16(slot + Q9K_MODDIR_FLAGS_OFF) |
                              Q9K_MODDIR_FLAG_PERM));
}

/* Abschnitt 2, Punkt "F$Link/F$UnLink": durchsucht Q9K_BootList (s.
 * q9kernel_entry.a) EIN EINZIGES Mal beim Boot nach JEDEM gueltigen
 * Modul (nicht nur "init" wie Q9K_FindModuleByName in Schritt 6a) und
 * traegt jeden Treffer ins Verzeichnis ein -- damit F$Link ueberhaupt
 * etwas zum Finden hat. Gleiche Scan-/Validierungslogik wie
 * Q9K_FindModuleByName (q9kernel_modsearch.c), aber OHNE Namensfilter
 * und OHNE Revisions-Tiebreak (jedes gueltige Modul wird eingetragen,
 * auch mehrere Revisionen desselben Namens -- F$Link selbst waehlt
 * spaeter beim Suchen die hoechste Revision aus, s. Q9K_ModDirLinkByName).
 *
 * ECHTER BUG GEFUNDEN + GEFIXT (2026-08-21): F$Link lieferte ueber TRAP
 * #0 real einen offensichtlich unsinnigen Modulzeiger ($FE01DE04).
 * Ursache war KEIN Compiler-/Logikfehler (per Host-Test, echter opt68k-
 * Inspektion UND der jetzt hier eingebauten totalRam-Grenze
 * ausgeschlossen) -- Q9K_BootList enthaelt neben echtem RAM auch eine
 * dritte Region bei $FE000000/$80000, die laut Q9-Flux/docs/HWCONFIG.md
 * + MMU_SSM_WORKFLOW_de.md das per Remap eingeblendete BOOT-ROM ist,
 * nicht RAM. Auf 512 KByte rohem ROM-Code/-Daten erzeugt die Sync-Wort-
 * plus-24-Word-XOR-Pruefsumme frueher oder spaeter einen echten
 * False-Positive-Treffer (statistisch erwartbar, kein Einzelfall) --
 * Q9K_FindModuleByName (Schritt 6a) ist dagegen unempfindlich, weil es
 * zusaetzlich auf den Namen "init" filtert; diese Funktion traegt aber
 * bewusst JEDES gueltig aussehende Modul ein, ist also viel anfaelliger.
 * Fix: Regionen, deren Basis ausserhalb des tatsaechlich installierten
 * RAM (Q9_D_TOTRAM, Boot-Zeit-Register D0) liegt, gar nicht erst
 * scannen -- ROM/MMIO-Bereiche enthalten grundsaetzlich keine echten,
 * ladbaren OS-9-Module. Rueckgabe: Anzahl eingetragener Module. */
#ifndef Q9_D_TOTRAM
#define Q9_D_TOTRAM 0x06CUL
#endif
Q9_u32 Q9K_ModDirPopulateFromBootList(const Q9_u8 *bootList)
{
    Q9_u32 added = 0;
    Q9_u32 regionIndex;
    Q9_u32 totalRam;

    if (bootList == 0)
        return 0;

    totalRam = Q9K_GetU32(Q9_D_TOTRAM);

    for (regionIndex = 0; regionIndex < 64; regionIndex++) {
        const Q9_u8 *entry = bootList + (regionIndex * 8);
        Q9_u32 regionBase = Q9K_ReadU32BE(entry);
        Q9_u32 regionLen  = Q9K_ReadU32BE(entry + 4);
        Q9_u32 offset = 0;

        if (regionBase == 0)
            break;

        if (totalRam != 0 && regionBase >= totalRam)
            continue; /* ausserhalb des echten RAM (z.B. ROM-Remap) -- ueberspringen, s. Kopfkommentar */

        while (offset + 0x30UL <= regionLen) {
            const Q9_u8 *candidate = (const Q9_u8 *)(regionBase + offset);
            Q9_u32 remaining = regionLen - offset;
            Q9_u32 moduleSize;

            if (!Q9K_CheckSyncWord(candidate, remaining)) {
                offset += 2;
                continue;
            }
            if (!Q9K_ValidModuleHeader(candidate, remaining)) {
                offset += 2;
                continue;
            }

            moduleSize = Q9K_ReadU32BE(candidate + Q9K_MH_SIZE);
            if (moduleSize == 0) {
                offset += 2;
                continue;
            }

            /* offset bewusst VOR dem Add-Aufruf aktualisiert (nicht erst
             * danach) -- moduleSize wird dahinter nicht mehr gebraucht,
             * einfacher zu lesen. War waehrend der Fehlersuche kurzzeitig
             * als Compiler-Bug-Workaround verdaechtigt (per echter opt68k-
             * Inspektion prompt wieder verworfen, sobald der wahre Befund
             * -- s. u. -- feststand) -- bleibt trotzdem als die klarere
             * Reihenfolge stehen. */
            offset += moduleSize;

            {
                /* Module aus der Bootdatei liegen PERMANENT im Speicher --
                 * sie koennen gar nicht verschwinden. Ihr Verzeichniseintrag
                 * darf deshalb nie entfernt werden (s. Q9K_ModDirUnlinkByHeader). */
                Q9_u32 newSlot = Q9K_ModDirAdd(candidate);
                if (newSlot != 0) {
                    Q9K_ModDirMarkPermanent(newSlot);
                    added++;
                }
            }
        }
    }

    return added;
}

/* Sucht in der aktiven Verzeichnisliste nach Typ/Sprache-Filter + Namen.
 *
 * FILTER-SEMANTIK (KORRIGIERT 2026-09-02, Vorbereitung "Dreiklang"): Typ
 * (High-Byte) und Sprache (Low-Byte) werden GETRENNT geprueft, jeweils mit
 * "0 = beliebig". Vorher stand hier ein exakter WORT-Vergleich
 * ("desiredTyLang == 0 || tyLang == desiredTyLang") -- der war falsch und
 * haette den Dreiklang (Descriptor->Driver->File-Manager) sofort brechen
 * lassen. Beleg, empirisch an den ECHTEN Modulen aus dem Original-Image
 * (CMDS/BOOTOBJS) nachgemessen, gegen die real verwendeten Filterwerte
 * $F00/$E00/$D00 (s. intern dokumentiert):
 *
 *   Rolle          Modul              M$TyLang   Filter   exakt?
 *   Descriptor     term/t1/c0          $0F00      $F00     ja (zufaellig)
 *   Driver         sc68681/cfide       $0E01      $E00     NEIN
 *   File-Manager   scf/rbf             $0D01      $D00     NEIN
 *
 * Treiber und File-Manager tragen Sprache $01 (Maschinencode), die real
 * benutzten Filter aber Sprache $00 -- mit exaktem Wortvergleich fiele
 * die Kette also schon beim Treiber aus. Da es sich um ausgelieferte,
 * nachweislich funktionierende Microware-Software handelt, MUSS die
 * Sprache-0 im Filter "beliebig" bedeuten. Das Manual (68k_tech.pdf
 * S. 461) ist an der Stelle unscharf ("Desired module type/language byte
 * (0 = any)" -- Singular "byte", obwohl d0.w ein Wort ist); die
 * byteweise Lesart ist die einzige, die zu den realen Modulen passt.
 *
 * Bei mehreren Treffern (mehrere
 * Revisionen desselben Namens) wird die hoechste M$Rev behalten --
 * gleiches Prinzip wie Q9K_FindModuleByName (Thema 01, Revisions-
 * Tiebreak statt "ersten Treffer nehmen"). Bei Erfolg: Link-Zaehler
 * des gewaehlten Eintrags erhoehen, Headerzeiger zurueckgeben.
 * Rueckgabe 0 = kein Treffer (E_MNF, s. Q9K_SysFLink). */
/* Reine Suche ohne jede Nebenwirkung: liefert den BESTEN passenden
 * Verzeichnis-Slot (hoechste Revision) oder 0. Herausgeloest, damit
 * F$UnLoad denselben Treffer bestimmen kann wie F$Link, ohne dabei den
 * Link-Zaehler zu erhoehen (was es sofort wieder zuruecknehmen muesste). */
static Q9_u32 Q9K_ModDirFindSlotByName(Q9_u16 desiredTyLang, const char *name)
{
    Q9_u32 slot = Q9K_GetU32(Q9K_MODDIR_HEAD_ADDR);
    Q9_u32 bestSlot = 0;
    Q9_u32 bestRevision = 0;

    while (slot != 0) {
        Q9_u32 hdrAddr = Q9K_GetU32(slot + Q9K_MODDIR_HDRPTR_OFF);
        Q9_u16 tyLang  = Q9K_ModDirGetU16(slot + Q9K_MODDIR_TYLANG_OFF);

        /* Typ und Sprache getrennt, jeweils "0 = beliebig" (s. Kopfkommentar).
         * Deckt den bisherigen Fall desiredTyLang==0 unveraendert mit ab:
         * dann sind beide Teilfilter 0 und damit beide "beliebig". */
        Q9_u16 wantType = (Q9_u16)((desiredTyLang >> 8) & 0x00FFU);
        Q9_u16 wantLang = (Q9_u16)(desiredTyLang & 0x00FFU);
        Q9_u16 haveType = (Q9_u16)((tyLang >> 8) & 0x00FFU);
        Q9_u16 haveLang = (Q9_u16)(tyLang & 0x00FFU);

        if ((wantType == 0 || wantType == haveType) &&
            (wantLang == 0 || wantLang == haveLang)) {
            Q9_u32 nameOffset = Q9K_ReadU32BE((const Q9_u8 *)hdrAddr + Q9K_MH_NAME);
            Q9_u32 moduleSize = Q9K_ReadU32BE((const Q9_u8 *)hdrAddr + Q9K_MH_SIZE);

            if (nameOffset < moduleSize &&
                Q9K_ModDirNamesMatch((const Q9_u8 *)(hdrAddr + nameOffset), moduleSize - nameOffset, name)) {
                Q9_u32 revision = ((const Q9_u8 *)hdrAddr)[0x15];

                if (bestSlot == 0 || revision > bestRevision) {
                    bestSlot = slot;
                    bestRevision = revision;
                }
            }
        }

        slot = Q9K_GetU32(slot + Q9K_MODDIR_NEXT_OFF);
    }

    return bestSlot;
}

Q9_u32 Q9K_ModDirLinkByName(Q9_u16 desiredTyLang, const char *name)
{
    Q9_u32 bestSlot = Q9K_ModDirFindSlotByName(desiredTyLang, name);

    if (bestSlot == 0)
        return 0;

    Q9K_ModDirSetU16(bestSlot + Q9K_MODDIR_LINKCNT_OFF,
                      (Q9_u16)(Q9K_ModDirGetU16(bestSlot + Q9K_MODDIR_LINKCNT_OFF) + 1));

    return Q9K_GetU32(bestSlot + Q9K_MODDIR_HDRPTR_OFF);
}


/* Sucht den Verzeichniseintrag zu einer gegebenen Modulkopfadresse
 * (reale F$UnLink-Eingabe, (a2)) und dekrementiert dessen Link-Zaehler.
 * Erreicht er 0, wird der Eintrag aus der aktiven Liste entfernt und
 * der Slot an die Freiliste zurueckgegeben (KEIN Speicher-Deallozieren
 * -- die Modul-Bytes selbst gehoeren uns nicht, s. Kopfkommentar; nur
 * die eigene 16-Byte-Verzeichnisbuchhaltung wird freigegeben).
 * Rueckgabe 0 = Erfolg, 1 = Header nicht im Verzeichnis gefunden. */
Q9_u32 Q9K_ModDirUnlinkByHeader(Q9_u32 hdrAddr)
{
    Q9_u32 slot = Q9K_GetU32(Q9K_MODDIR_HEAD_ADDR);
    Q9_u32 prev = 0;

    while (slot != 0) {
        if (Q9K_GetU32(slot + Q9K_MODDIR_HDRPTR_OFF) == hdrAddr) {
            Q9_u16 linkCnt = Q9K_ModDirGetU16(slot + Q9K_MODDIR_LINKCNT_OFF);

            if (linkCnt > 0)
                linkCnt--;
            Q9K_ModDirSetU16(slot + Q9K_MODDIR_LINKCNT_OFF, linkCnt);

            /* ECHTER BUG GEFUNDEN + GEFIXT (2026-09-06): hier wurde der
             * Eintrag entfernt, sobald der Zaehler 0 erreichte -- auch bei
             * Modulen aus der Bootdatei. IOMan linkt und unlinkt beim Start
             * reihum; danach waren rbf, cfide und dd aus dem Verzeichnis
             * verschwunden, obwohl sie unveraendert im Speicher lagen, und
             * I$Open meldete E_MNF. Real bleibt ein Modul im Verzeichnis,
             * solange es im Speicher liegt; entfernt wird der Eintrag erst,
             * wenn auch der Speicher freigegeben wird -- was bei
             * Bootdatei-Modulen nie passiert. */
            if (linkCnt == 0 &&
                (Q9K_ModDirGetU16(slot + Q9K_MODDIR_FLAGS_OFF) & Q9K_MODDIR_FLAG_PERM) == 0) {
                Q9_u32 next = Q9K_GetU32(slot + Q9K_MODDIR_NEXT_OFF);

                if (prev == 0)
                    Q9K_SetU32(Q9K_MODDIR_HEAD_ADDR, next);
                else
                    Q9K_SetU32(prev + Q9K_MODDIR_NEXT_OFF, next);

                Q9K_ModDirPoolFree(slot);
            }

            return 0;
        }

        prev = slot;
        slot = Q9K_GetU32(slot + Q9K_MODDIR_NEXT_OFF);
    }

    return 1;
}

/* Q9K_ModDirValidateAndAdd -- F$VModul-Kernlogik (Callcode 0x2e,
 * "Validate Module", 68k_tech.pdf S. 532f, echt per Read gelesen). IN
 * (hier als Parameter durchgereicht): (a0)=Modulzeiger, d1.l=Modulgroesse
 * (d0.l=Modulgruppen-ID wird laut Manual nur fuer eine spaetere,
 * gruppenbezogene Sonderbehandlung gebraucht -- unser Kernel kennt keine
 * "Modulgruppen", deshalb hier ungenutzt). OUT (Erfolg): "Verzeichnis-
 * eintragszeiger" -- s. u., ist bei uns der VALIDIERTE MODULKOPF SELBST,
 * NICHT unser eigener 16-Byte-Slot. Rueckgabe 0 = Fehlschlag
 * (*outError gesetzt).
 *
 * ECHTER BUG GEFUNDEN + GEFIXT (2026-09-11, per Rueckspringadressen-
 * Forensik in IOMans F$Load-Wrapper, s. docs/OWN_KERNEL_STATUS.md):
 * anfangs wurde hier -- dem Kopfkommentar von F$Link/F$UnLink folgend
 * ("Verzeichnis-internes Layout sehen externe Module nie") -- unser
 * eigener 16-Byte-Slot zurueckgegeben. Live per Instruktionsspur
 * nachgewiesen, dass IOMans F$Load-Wrapper (ioman+$6d6) DIREKT nach dem
 * Aufruf zwei ECHTE Modulheader-Felder aus (a2) liest:
 *     move.w $12(a0), d0      * $12 = M$TypLang (Q9K_MH_TYLANG)
 *     adda.l $c(a0), a0       * $c  = M$Name-Offset (Q9K_MH_NAME)
 * -- exakt die Standard-68K-Modulheader-Offsets, NICHT unser eigenes
 * Slot-Layout (das bei $0E endet). Mit dem Slot-Zeiger las IOMan damit
 * Datenmuell als "Modulname" und rief intern F$Link darauf auf, das
 * folgerichtig E$MNF ($DD) meldete -- OBWOHL F$VModul selbst laengst
 * erfolgreich validiert+eingetragen hatte. Anders als bei F$Link/
 * F$UnLink (deren reale OUT-Konvention laut Manual "(a2) = Module
 * pointer" ist -- das war schon vorher korrekt) verlangt F$VModul also
 * TROTZ der Bezeichnung "Directory entry pointer" denselben echten
 * Modulkopfzeiger. Fix: `hdr` zurueckgeben statt des Slots; der Slot
 * bleibt intern (Q9K_ModDirAdd traegt ihn wie bisher ins Verzeichnis
 * ein, nur sein Zeiger wird nicht mehr nach aussen gereicht).
 *
 * ECHTER BUG GEFUNDEN + BEHOBEN (2026-09-10): dieser Dienst fehlte
 * komplett (lief in Q9K_SysUnimplemented) -- IOMans eigene F$Load-
 * Implementierung braucht ihn beim Laden eines neuen Moduls von
 * Mass-Storage, s. docs/OWN_KERNEL_STATUS.md.
 *
 * Pruefung in drei real belegten Stufen (internem Referenzmaterial, per
 * E$UnkSvc/E$BPAddr/E$BPNam als Anker ausgezaehlt, s.
 * docs/OWN_KERNEL_STATUS.md):
 *   1. Sync-Wort ($4AFC)                -> sonst E$BMID  ($CD)
 *   2. 24-Word-XOR-Kopfpruefsumme       -> sonst E$BMHP  ($EC)
 *   3. 24-Bit-Modul-CRC ueber das GANZE Modul (inkl. des CRC-Feldes
 *      selbst) -- Polynom $800063, Akkumulator-Start $FFFFFF, muss am
 *      Ende genau CRCCon ($00800FE3, internem Referenzmaterial Zeile
 *      248) ergeben. NICHT aus der Doku geraten (die nennt nur den
 *      Algorithmus in Worten, keinen Code) -- empirisch gegen sechs
 *      echte, unveraenderte Microware-Module verifiziert (rbf/cfide/
 *      ioman/scf/dd/c0.mod aus dem F$Load-Testkorpus dieser Sitzung):
 *      alle sechs ergeben exakt $800FE3, ein einzelnes verfaelschtes
 *      Byte ergibt nachweislich etwas anderes.
 *                                        -> sonst E$BMCRC ($E8)
 *
 * Vereinfachung: KEINE Namens-/Revisions-Deduplizierung -- ein zweiter
 * F$VModul-Aufruf fuer denselben Namen legt einen weiteren Eintrag an,
 * statt die bessere Revision auszuwaehlen (das erledigt
 * Q9K_ModDirLinkByName beim SUCHEN ohnehin schon, s. dort: es waehlt
 * unter mehreren Treffern die hoechste Revision). Fuer den F$Load-
 * Regelfall (neues Modul, noch nicht im Verzeichnis) korrekt; echtes
 * TODO, sobald mehrfaches Laden derselben Datei ueblich wird.
 *
 * ECHTER BUG GEFUNDEN + GEFIXT (2026-09-10): das per Parameter (d1.l)
 * hereingereichte `size` stimmt bei einem echten F$Load-Aufruf (IOMan,
 * Mass-Storage-Pfad) NACHWEISLICH NICHT mit der wahren Modulgroesse
 * ueberein -- live gemessen mit "/CMDS/echo" ($C8E laut os9-Toolshed-
 * ident, "Good CRC"): IOMan uebergab $C90, zwei Byte zu viel (Ursache
 * auf IOMan-Seite nicht weiterverfolgt, ausserhalb der Kernel-
 * Zustaendigkeit). Die CRC-Pruefung ueber die FALSCHEN zwei
 * Zusatzbyte hinweg schlug deshalb reproduzierbar mit E$BMCRC fehl,
 * obwohl das Modul selbst unversehrt war (Sync-Wort am Pufferanfang
 * exakt korrekt gemessen). Fix: die Modulgroesse NACH bestandener
 * Kopfpruefsumme aus dem Header selbst lesen (M$Size, Offset $04) --
 * der Wert ist zu diesem Zeitpunkt bereits durch die 24-Word-XOR-
 * Pruefsumme abgesichert (sie deckt Offset $00-$2F, M$Size liegt
 * darin), also vertrauenswuerdig, sobald Q9K_ValidModuleHeader
 * erfolgreich war. `size` (Aufrufer-Parameter) bleibt nur noch als
 * OBERGRENZE fuer die Bounds-Pruefung der beiden billigen Vorstufen
 * (Sync-Wort, Kopfpruefsumme) in Gebrauch, s. u. */
Q9_u32 Q9K_ModDirValidateAndAdd(const Q9_u8 *hdr, Q9_u32 size, Q9_u16 *outError)
{
    Q9_u32 crc;
    Q9_u32 i;
    Q9_u32 realSize;
    Q9_u16 tyLang;

    *outError = 0;

    if (!Q9K_CheckSyncWord(hdr, size)) {
        *outError = 0x00CDU;            /* E$BMID, Bad Module ID */
        return 0;
    }
    if (!Q9K_ValidModuleHeader(hdr, size)) {
        *outError = 0x00ECU;            /* E$BMHP, Bad Module Header Parity */
        return 0;
    }

    /* Reale Groesse aus dem (jetzt pruefsummengesicherten) Header lesen
     * statt dem moeglicherweise ungenauen Aufrufer-Parameter zu
     * vertrauen (s. Kopfkommentar). Trotzdem nie ueber die vom Aufrufer
     * zugesicherte Pufferlaenge hinaus lesen -- ein beschaedigter Header
     * koennte theoretisch eine zu grosse Groesse behaupten. */
    realSize = Q9K_ReadU32BE(hdr + Q9K_MH_SIZE);
    if (realSize == 0 || realSize > size)
        realSize = size;
    tyLang = Q9K_GetU16BE((Q9_u32)(unsigned long)hdr + Q9K_MH_TYLANG);

    crc = 0xFFFFFFUL;
    for (i = 0; i < realSize; i++) {
        Q9_u32 bit;

        crc = (crc ^ ((Q9_u32)hdr[i] << 16)) & 0xFFFFFFUL;
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x800000UL)
                crc = ((crc << 1) ^ 0x800063UL) & 0xFFFFFFUL;
            else
                crc = (crc << 1) & 0xFFFFFFUL;
        }
    }
    if (crc != 0x00800FE3UL) {
        *outError = 0x00E8U;            /* E$BMCRC, Bad Module CRC */
        return 0;
    }

    /* Slot wird intern angelegt (Buchhaltung, F$Link/F$UnLink finden das
     * Modul kuenftig darueber) -- Slot-Erschoepfung wird hier bewusst
     * NICHT als Fehler behandelt: das validierte Modul ist uneinge-
     * schraenkt benutzbar, es fehlt nur der Verzeichniseintrag fuer eine
     * SPAETERE Namenssuche -- dafuer gibt es (noch) keinen eigenen, real
     * belegten Fehlercode-Fall. */
    (void)Q9K_ModDirAdd(hdr);

    /* ECHTER BUG GEFUNDEN + GEFIXT (2026-09-11, zweite Runde, per
     * Instruktionsspur/Registerfreeze in IOMans F$Load-Wrapper): weder
     * unser eigener 16-Byte-Slot NOCH der rohe Modulkopfzeiger sind das
     * richtige (a2). IOMans Wrapper (ioman+$8f8 ff.) inkrementiert/
     * dekrementiert `+$0C(a2)` als Link-Zaehler (live gemessen: ADDQ.W
     * dann spaeter SUBQ.W) und liest `+$12(a2)` als Typ/Sprache-Wort --
     * das sind FESTE Offsets eines ECHTEN Microware-Verzeichniseintrags,
     * die weder mit unserem 16-Byte-Slot (endet bei $0E) noch mit dem
     * Modulheader (dessen $0C/$12 M$Name/M$TypLang sind -- Schreiben
     * DORT haette den Header selbst beschaedigt, live bestaetigt: `+$0C`
     * als Kopfzeiger interpretiert korrumpierte M$Name) uebereinstimmen.
     * Ausserdem liest derselbe Wrapper `+$00(a2)` und macht daraus am
     * Ende SEINEN EIGENEN Rueckgabewert (a2) -- muss also der Modulkopf-
     * zeiger sein, damit der AUFRUFER dieses Aufrufers (unser Testcode)
     * am Ende einen sinnvollen Modulzeiger bekommt.
     *
     * Fix: ein eigener, NUR FUER DIESEN ZWECK reservierter 20-Byte-
     * Rueckgabe-Puffer (Q9K_VMODUL_RETBUF, fest/wiederverwendet -- muss
     * nur bis zum naechsten F$VModul-Aufruf ueberleben, IOMan liest ihn
     * unmittelbar nach dem Rueckkehren):
     *   +0x00 (4) Modulkopfzeiger (wird am Ende IOMans eigener
     *             Rueckgabewert)
     *   +0x0C (2) Link-Zaehler-Platzhalter (wird inkrementiert, dann im
     *             selben Aufruf wieder dekrementiert -- Endwert 0,
     *             daher als reiner Scratch ausreichend)
     *   +0x12 (2) Typ/Sprache, Kopie aus dem Modulheader
     * Weder unser eigener Slot noch der Modulkopf werden dadurch
     * angetastet -- der Puffer liegt vollstaendig ausserhalb beider. */
    Q9K_SetU32(Q9K_VMODUL_RETBUF + 0x00UL, (Q9_u32)(unsigned long)hdr);
    Q9K_ModDirSetU16(Q9K_VMODUL_RETBUF + 0x0CUL, 0);
    Q9K_ModDirSetU16(Q9K_VMODUL_RETBUF + 0x12UL, tyLang);
    return (Q9_u32)Q9K_VMODUL_RETBUF;
}

/* Scratch-Bruecke fuer F$VModul, gleiches Muster wie ueberall in diesem
 * Kernel -- freier Bereich direkt hinter den F$RetPD-Scratchzellen
 * ($161C-$1628, q9kernel_iopath.c) und vor der F$SSvc-Markierungstabelle
 * ($1700, q9kernel_ssvc.c). */
#ifndef Q9K_VMODUL_SCRATCH_HDR
#define Q9K_VMODUL_SCRATCH_HDR     0x162CUL   /* Q9_u32, (a0) EIN = Modulzeiger */
#define Q9K_VMODUL_SCRATCH_SIZE    0x1630UL   /* Q9_u32, d1.l EIN = Modulgroesse */
#define Q9K_VMODUL_SCRATCH_ENTRY   0x1634UL   /* Q9_u32, (a2) AUS = Verzeichniseintrag */
#define Q9K_VMODUL_SCRATCH_ERROR   0x1638UL   /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_VMODUL_SCRATCH_SUCCESS 0x163CUL   /* Q9_u32, 0 = Fehlschlag / 1 = Erfolg */
#endif

/* Q9K_SysVModulImpl -- duenne, PARAMETERLOSE Bruecke zwischen dem
 * Assembler-Trampolin (q9kernel_entry.a, Q9K_SysFVModul) und
 * Q9K_ModDirValidateAndAdd oben -- gleiches, etabliertes Muster wie
 * Q9K_SysRetPDImpl (q9kernel_iopath.c). */
void Q9K_SysVModulImpl(void)
{
    Q9_u32 hdrAddr = Q9K_GetU32(Q9K_VMODUL_SCRATCH_HDR);
    Q9_u32 size    = Q9K_GetU32(Q9K_VMODUL_SCRATCH_SIZE);
    Q9_u32 entry;
    Q9_u16 err = 0;

    entry = Q9K_ModDirValidateAndAdd((const Q9_u8 *)hdrAddr, size, &err);
#if Q9K_MEMTRACE_COMPILETIME
    Q9K_MemTraceSetModule(hdrAddr);
    Q9K_MemTraceEmit(Q9K_MEMTRACE_OP_MODULE_LOAD, size, hdrAddr, size,
                     entry != 0 ? 0UL : (Q9_u32)err, 0UL);
    Q9K_MemTraceClearModule();
#endif
    if (entry != 0) {
        Q9K_SetU32(Q9K_VMODUL_SCRATCH_ENTRY, entry);
        Q9K_SetU32(Q9K_VMODUL_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_VMODUL_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_VMODUL_SCRATCH_SUCCESS, 0UL);
    }
}

/* Q9K_ModDirUnloadByName -- echte F$UnLoad-Kernlogik (Callcode $1D,
 * "Unlink Module by Name"). Verifizierte ABI (68k_tech.pdf S. 531):
 * d0.w = Modultyp/-sprache, (a0) = Zeiger auf den Modulnamen; (a0) wird
 * hinter den Namen fortgeschrieben. Carry + d1.w im Fehlerfall.
 *
 * Der Unterschied zu F$UnLink ist ausschliesslich die Eingabe: dort die
 * Kopfadresse, hier der Name. Gesucht wird deshalb mit exakt derselben
 * Regel wie bei F$Link (inkl. Revisions-Tiebreak), heruntergezaehlt mit
 * exakt derselben Routine wie bei F$UnLink -- F$UnLoad ist genau die
 * Verbindung dieser beiden und fuehrt bewusst keine eigene dritte
 * Variante ein.
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
Q9_u32 Q9K_ModDirUnloadByName(Q9_u16 desiredTyLang, const char *name,
                              Q9_u16 *outError)
{
    Q9_u32 slot;
    Q9_u32 hdrAddr;

    *outError = 0;

    if (name == 0) {
        *outError = Q9K_E_MNF;
        return 0;
    }

    slot = Q9K_ModDirFindSlotByName(desiredTyLang, name);
    if (slot == 0) {
        *outError = Q9K_E_MNF;
        return 0;
    }

    hdrAddr = Q9K_GetU32(slot + Q9K_MODDIR_HDRPTR_OFF);
    if (Q9K_ModDirUnlinkByHeader(hdrAddr) != 0) {
        *outError = Q9K_E_MNF;
        return 0;
    }
    return 1;
}

void Q9K_SysUnloadImpl(void)
{
    Q9_u16 err = 0;

    if (Q9K_ModDirUnloadByName((Q9_u16)Q9K_GetU32(Q9K_UNLOAD_SCRATCH_TYLANG),
                               (const char *)Q9K_GetU32(Q9K_UNLOAD_SCRATCH_NAME),
                               &err)) {
        Q9K_SetU32(Q9K_UNLOAD_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_UNLOAD_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_UNLOAD_SCRATCH_SUCCESS, 0UL);
    }
}

/* Q9K_CrcAccumulate -- echte F$CRC-Kernlogik (Callcode $17, "Generate
 * CRC"). Verifizierte ABI (68k_tech.pdf S. 390f): d0.l = Byteanzahl,
 * d1.l = CRC-Akkumulator, (a0) = Datenzeiger; AUS: d1.l = fortgefuehrter
 * Akkumulator.
 *
 * Es ist der 24-Bit-OS-9-Modul-CRC. Der Akkumulator wird vor dem ERSTEN
 * Aufruf auf -1 gesetzt und darf ueber mehrere Aufrufe fortgefuehrt
 * werden; die oberen 8 Bit bleiben dabei ungenutzt. Rechnet man ueber
 * ein vollstaendiges Modul EINSCHLIESSLICH seiner drei CRC-Bytes, muss
 * am Ende die dokumentierte Konstante $00800FE3 stehen (CRCCon) -- genau
 * das prueft der Host-Test an einem echten, gebauten Modul.
 *
 * Die Bitschritte sind aus der im Projekt bereits gegen echte Module
 * erprobten Referenzfassung uebernommen (Q9-Flux/tools, os9_crc), nicht
 * aus einer Polynombeschreibung nachempfunden. */
Q9_u32 Q9K_CrcAccumulate(Q9_u32 accum, Q9_u32 addr, Q9_u32 count)
{
    const volatile Q9_u8 *p = (const volatile Q9_u8 *)addr;
    Q9_u32 c0 = (accum >> 16) & 0xFFUL;
    Q9_u32 c1 = (accum >> 8) & 0xFFUL;
    Q9_u32 c2 = accum & 0xFFUL;
    Q9_u32 i;

    for (i = 0; i < count; ++i) {
        Q9_u32 a = ((Q9_u32)p[i] ^ c0) & 0xFFUL;

        c0 = c1;
        c1 = c2;
        c1 ^= (a >> 7) & 0xFFUL;
        c2 = (a << 1) & 0xFFUL;
        c1 ^= (a >> 2) & 0xFFUL;
        c2 ^= (a << 6) & 0xFFUL;
        a ^= (a << 1) & 0xFFUL;
        a ^= (a << 2) & 0xFFUL;
        a ^= (a << 4) & 0xFFUL;
        a &= 0xFFUL;
        if (a & 0x80UL) {
            c0 ^= 0x80UL;
            c2 ^= 0x21UL;
        }
    }

    return (c0 << 16) | (c1 << 8) | c2;
}

void Q9K_SysCrcImpl(void)
{
    Q9K_SetU32(Q9K_CRC_SCRATCH_ACCUM,
               Q9K_CrcAccumulate(Q9K_GetU32(Q9K_CRC_SCRATCH_ACCUM),
                                 Q9K_GetU32(Q9K_CRC_SCRATCH_ADDR),
                                 Q9K_GetU32(Q9K_CRC_SCRATCH_COUNT)));
}
