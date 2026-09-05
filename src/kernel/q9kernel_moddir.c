/*
 * q9kernel_moddir.c -- Q9-OS eigener Kernel: Modulverzeichnis (Abschnitt
 *                      "F$Link/F$UnLink", 2026-08-21).
 *
 * Andreas: "fangen wir mit dem Link an und dementsprechend den unlink...
 * machen wir erst alles fertig ohne I/Os" -- dieser Schritt baut NUR
 * die In-Memory-Seite (F$Link durchsucht laut Manual, 68k_tech.pdf
 * S. 461, AUSSCHLIESSLICH das In-Memory-Modulverzeichnis, nie
 * Mass-Storage -- F$Load waere ein separater, spaeterer Schritt,
 * s. docs/kernel-walkthrough/11-programm-laden/).
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

        aLower = (a >= 'A' && a <= 'Z') ? (Q9_u8)(a + ('a' - 'A')) : a;
        bLower = (b >= 'A' && b <= 'Z') ? (Q9_u8)(b + ('a' - 'A')) : b;

        if (aLower != bLower)
            return 0;
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
 * $F00/$E00/$D00 (s. docs/kernel-walkthrough/03-dreiklang/):
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
Q9_u32 Q9K_ModDirLinkByName(Q9_u16 desiredTyLang, const char *name)
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
