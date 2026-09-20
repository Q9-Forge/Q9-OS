/*
 * test_q9kernel_moddir.c -- Regressionstest fuer q9kernel_moddir.c.
 *
 * NACHTRAG 2026-08-21: entstanden, nachdem ein echter Boot-Test
 * (F$Link ueber TRAP #0) einen offensichtlich unsinnigen Modulzeiger
 * lieferte ($FE01DE04). Der Verdacht fiel zunaechst auf einen echten
 * xcc/-O7-Compilerfehler (Register-Ueberschreibung bei einem inline
 * expandierten Aufruf) -- per echter opt68k-Inspektion wieder verworfen,
 * sobald klar war: $FE01DE04 liegt innerhalb der von Q9-Flux als
 * "ROM nach Remap" dokumentierten dritten Boot-Region ($FE000000/
 * $80000, s. Q9-Flux/docs/HWCONFIG.md) -- ein echter, statistisch
 * erwartbarer False-Positive-Treffer der Sync-Wort-/Pruefsummen-
 * Validierung auf rohem ROM-Inhalt, kein Logik- oder Compilerfehler.
 * Der eigentliche Fix (Regionen ausserhalb von Q9_D_TOTRAM ueberspringen)
 * ist ueber die Boot-Liste selbst nicht host-testbar (s. u.), dieser
 * Host-Test verifiziert deshalb weiterhin die reine Verzeichnis-Logik
 * (Add/Link/UnLink) unabhaengig von der xcc-Zielpipeline (gleiche
 * Konvention wie test_q9kernel_firstproc.c/tables.c).
 *
 * Q9K_CheckSyncWord/Q9K_ValidModuleHeader werden hier bewusst NICHT
 * real nachgebildet (das ist q9kernel_modcheck.c's eigener, bereits
 * eigenstaendig getesteter Zustaendigkeitsbereich) -- stattdessen
 * einfache Fakes, die den Sync-Wort-Bereich der Testmodule pruefen.
 * Q9K_MODDIR_FREE_ADDR/HEAD_ADDR werden per #define VOR dem #include
 * auf echte Testpuffer umgebogen, gleiches Muster wie ueberall sonst.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_moddir test_q9kernel_moddir.c && \
 *       ./test_q9kernel_moddir
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x2000];

#define Q9_D_MODDIR             ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9_D_MODDIR_END         ((unsigned long)(g_fakeGlobals + 0x008))
#define Q9K_MODDIR_FREE_ADDR    ((unsigned long)(g_fakeGlobals + 0x010))
#define Q9K_MODDIR_HEAD_ADDR    ((unsigned long)(g_fakeGlobals + 0x018))
/* Auf 0 gehalten (s. main()) -- Q9K_ModDirPopulateFromBootList behandelt
 * totalRam==0 als "Grenze unbekannt, nicht pruefen" (defensiv, falls das
 * Feld beim echten Boot je noch nicht gesetzt waere). Wird von diesem
 * Test ohnehin nicht direkt ausgeuebt (s. Kopfkommentar, Fall 1). */
#define Q9_D_TOTRAM             ((unsigned long)(g_fakeGlobals + 0x020))
/* F$VModul/F$DatMod legen ihre Rueckgabewerte hier ab -- ohne
 * Umlenkung greift der Test auf die echte Kerneladresse $1650 zu. */
#define Q9K_VMODUL_RETBUF       ((unsigned long)(g_fakeGlobals + 0x100))

/* Real nur 4/8/10/12 Byte auseinander (echtes 32-Bit-Ziel) -- auf
 * diesem 64-Bit-Testhost ist Q9_u32 8 Byte breit, deshalb hier
 * grosszuegig auf 32-Byte-Slots umgebogen (gleiches Muster wie
 * test_q9kernel_firstproc.c/tables.c). Betrifft NICHT das echte,
 * kompakte 16-Byte-Layout aus q9kernel_moddir.c. */
#define Q9K_MODDIR_NEXT_OFF     0x00UL
#define Q9K_MODDIR_HDRPTR_OFF   0x08UL
#define Q9K_MODDIR_TYLANG_OFF   0x10UL
#define Q9K_MODDIR_ATTREV_OFF   0x12UL
#define Q9K_MODDIR_LINKCNT_OFF  0x14UL
#define Q9K_TEST_SLOT_SIZE      32UL
/* F$GModDr kopiert ganze Eintraege -- im Test dieselbe
 * grosszuegige Slotgroesse wie oben, nicht die realen 16 Byte. */
#define Q9K_MODDIR_ENTRY_BYTES  32UL

typedef unsigned long  Q9_u32;
typedef unsigned char  Q9_u8;

/* Einfache Fakes statt der echten q9kernel_modcheck.c-Funktionen --
 * nur Sync-Wort-Pruefung, keine echte Pruefsumme (die ist NICHT
 * Gegenstand dieses Tests, s. Kopfkommentar). */
static int Q9K_CheckSyncWord(const Q9_u8 *addr, Q9_u32 availableLen)
{
    if (addr == 0 || availableLen < 2)
        return 0;
    return (addr[0] == 0x4A && addr[1] == 0xFC) ? 1 : 0;
}
static int Q9K_ValidModuleHeader(const Q9_u8 *addr, Q9_u32 availableLen)
{
    return Q9K_CheckSyncWord(addr, availableLen);
}

/* Speicherspur-Stubs (q9kernel_debug.c): reine Diagnose, ohne Einfluss
 * auf die hier geprueften Verzeichnislogik-Pfade. */
void Q9K_MemTraceSetModule(unsigned long header) { (void)header; }
void Q9K_MemTraceClearModule(void) {}
void Q9K_MemTraceEmit(unsigned long operation,
                      unsigned long requested,
                      unsigned long address,
                      unsigned long size,
                      unsigned long error,
                      unsigned long freeHead)
{
    (void)operation; (void)requested; (void)address;
    (void)size; (void)error; (void)freeHead;
}

/* Arena-Stubs (q9kernel_arena.c, dort eigenstaendig getestet): ein
 * einfacher Bump-Allocator reicht -- F$DatMod prueft hier den Modulaufbau,
 * nicht die Speicherverwaltung. */
static unsigned char g_datmodArena[8192];
static unsigned long g_datmodNext;
static int g_freeMemCalls;
unsigned long Q9K_AllocMem(unsigned long size)
{
    unsigned long a;
    if (g_datmodNext + size > sizeof(g_datmodArena))
        return 0;
    a = (unsigned long)(g_datmodArena + g_datmodNext);
    g_datmodNext += size;
    return a;
}
void Q9K_FreeMem(unsigned long addr, unsigned long size)
{
    (void)addr; (void)size; g_freeMemCalls++;
}

#include "q9kernel_moddir.c"

static int failures = 0;

static void checkU32(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-60s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-60s = %lu (erwartet %lu)\n", label, (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

/* Baut ein minimales, synthetisches 68K-Modulheader-Praefix in buf:
 * Sync ($4AFC), Groesse (moduleSize), Name (bei Offset nameOff, NUL-
 * terminiert), Typ/Sprache (tyLang), Revision (rev bei Offset 0x15).
 * buf muss mindestens 0x30 Byte gross sein (Standard-Headerlaenge). */
static void buildHeader(Q9_u8 *buf, Q9_u32 bufSize, Q9_u32 moduleSize,
                         const char *name, Q9_u16 tyLang, Q9_u8 rev)
{
    Q9_u32 nameOff = 0x20; /* fest, weit genug hinter dem Standard-Header */
    Q9_u32 i;

    memset(buf, 0, bufSize);
    buf[0x00] = 0x4A; buf[0x01] = 0xFC;                 /* Sync */
    buf[0x04] = (Q9_u8)(moduleSize >> 24); buf[0x05] = (Q9_u8)(moduleSize >> 16);
    buf[0x06] = (Q9_u8)(moduleSize >> 8);  buf[0x07] = (Q9_u8)moduleSize;
    buf[0x0C] = 0; buf[0x0D] = 0; buf[0x0E] = 0; buf[0x0F] = (Q9_u8)nameOff; /* M$Name-Offset (klein, passt in 1 Byte) */
    buf[0x12] = (Q9_u8)(tyLang >> 8);
    buf[0x13] = (Q9_u8)tyLang;
    buf[0x15] = rev;

    for (i = 0; name[i] != 0 && nameOff + i < bufSize - 1; i++)
        buf[nameOff + i] = (Q9_u8)name[i];
    buf[nameOff + i] = 0;
}

int main(void)
{
    static Q9_u8 modInit[64];
    static Q9_u8 modFoo[64];
    static Q9_u8 modInitRev2[64];
    static Q9_u8 bootList[3 * 8 + 4]; /* 3 Regionen + Nullterminator */
    static Q9_u8 region[3 * 64];      /* eine zusammenhaengende Region mit 3 Modulen */
    Q9_u32 i;

    memset(g_fakeGlobals, 0xCC, sizeof(g_fakeGlobals));
    memset(bootList, 0, sizeof(bootList));

    /* Freiliste ueber 8 Slots (echt 16 Byte, hier testweise
     * Q9K_TEST_SLOT_SIZE, s. o.) manuell aufbauen -- gleiches Muster wie
     * Q9K_BuildFreeList (q9kernel_tables.c, hier bewusst nicht
     * mitgelinkt, deshalb lokal nachgebaut). */
    {
        static Q9_u8 pool[8 * Q9K_TEST_SLOT_SIZE];
        Q9_u32 base = (Q9_u32)(unsigned long)pool;

        memset(pool, 0, sizeof(pool));
        for (i = 0; i < 7; i++)
            Q9K_SetU32(base + i * Q9K_TEST_SLOT_SIZE, base + (i + 1) * Q9K_TEST_SLOT_SIZE);
        Q9K_SetU32(base + 7 * Q9K_TEST_SLOT_SIZE, 0);
        Q9K_SetU32(Q9K_MODDIR_FREE_ADDR, base);
        Q9K_SetU32(Q9K_MODDIR_HEAD_ADDR, 0);
    }

    /* Region: drei echte, gueltige Testmodule direkt hintereinander
     * ("init" Rev 1, "foo" Rev 1, "init" Rev 2 -- prueft sowohl das
     * Auffinden mehrerer verschiedener Module als auch den Revisions-
     * Tiebreak bei gleichem Namen). */
    buildHeader(region + 0 * 64, 64, 64, "init", 0x0C01, 1);
    buildHeader(region + 1 * 64, 64, 64, "foo",  0x0C01, 1);
    buildHeader(region + 2 * 64, 64, 64, "init", 0x0C01, 2);

    /* Q9K_ModDirPopulateFromBootList selbst NICHT end-to-end testbar
     * (gleicher, bereits dokumentierter Host-Befund wie bei
     * Q9K_FindModuleByName, q9kernel_modsearch.c): Q9K_BootList-
     * Eintraege sind echte 4-Byte-Adressfelder (Q9K_ReadU32BE), auf
     * diesem 64-Bit-Testhost liegen normale Stack-/Static-Puffer aber
     * oft oberhalb von 4 GiB -- eine Adresse liesse sich dann gar nicht
     * verlustfrei in 4 Byte kodieren. Deshalb hier die eigentliche
     * Verzeichnislogik (Q9K_ModDirAdd/LinkByName/UnlinkByHeader) DIREKT
     * getestet, ohne den Umweg ueber die serialisierte Boot-Liste --
     * das ist ohnehin der Teil, der nach dem echten Boot-Test verdaechtig
     * war (s. Kopfkommentar), die Scan-Schleife selbst folgt demselben,
     * bereits durch Q9K_FindModuleByName abgedeckten Muster.
     * bootList/region bleiben oben deklariert (Byte-Layout-Dokumentation
     * fuer eine spaetere Zielumgebung/QEMU-Runde), hier nur ungenutzt. */
    (void)bootList;

    /* Fall 1: drei gueltige Testmodule direkt eintragen. */
    checkU32("Q9K_ModDirAdd(modInit Rev1) liefert einen Slot (!= 0)",
             (Q9_u32)(Q9K_ModDirAdd(region + 0 * 64) != 0), 1);
    checkU32("Q9K_ModDirAdd(modFoo) liefert einen Slot (!= 0)",
             (Q9_u32)(Q9K_ModDirAdd(region + 1 * 64) != 0), 1);
    checkU32("Q9K_ModDirAdd(modInit Rev2) liefert einen Slot (!= 0)",
             (Q9_u32)(Q9K_ModDirAdd(region + 2 * 64) != 0), 1);

    /* Fall 2: F$Link("init", beliebiger Typ) -- muss die HOEHERE
     * Revision (2) liefern, nicht die erste gefundene. */
    {
        Q9_u32 hdr = Q9K_ModDirLinkByName(0, "init");
        checkU32("F$Link(\"init\") liefert einen Treffer (nicht 0)", (Q9_u32)(hdr != 0), 1);
        checkU32("F$Link(\"init\") waehlt die HOEHERE Revision (modInitRev2)",
                 hdr, (Q9_u32)(unsigned long)(region + 2 * 64));
        (void)modInit; (void)modFoo; (void)modInitRev2;
    }

    /* Fall 3: F$Link("foo") -- anderer Name, muss den zweiten Kandidaten
     * finden. */
    checkU32("F$Link(\"foo\") findet das richtige Modul",
             Q9K_ModDirLinkByName(0, "foo"), (Q9_u32)(unsigned long)(region + 1 * 64));

    /* Fall 4: F$Link mit einem Namen, der nicht existiert -- muss 0
     * liefern (E_MNF-Fall, s. Q9K_SysFLink). */
    checkU32("F$Link(\"nichtvorhanden\") liefert 0", Q9K_ModDirLinkByName(0, "nichtvorhanden"), 0);

    /* Fall 5: Typfilter -- ein Filter, der zu KEINEM Eintrag passt,
     * muss ebenfalls 0 liefern, auch wenn der Name existiert. */
    checkU32("F$Link(\"init\", falscher Typfilter) liefert 0",
             Q9K_ModDirLinkByName(0x0D01, "init"), 0);

    /* Fall 5b (2026-09-02, Vorbereitung "Dreiklang"): Typ/Sprache werden
     * GETRENNT gefiltert, jeweils "0 = beliebig". Genau diese Semantik
     * braucht die Descriptor->Driver->File-Manager-Kette, die real mit den
     * Filtern $F00/$E00/$D00 gegen Module mit Sprache $01 linkt (Treiber
     * $0E01, File-Manager $0D01) -- ein exakter Wortvergleich fiele dort
     * durch. Die Testmodule oben tragen $0C01, also:
     *   - Filter $0C00 (Typ passt, Sprache "beliebig")  -> MUSS finden
     *   - Filter $0001 (Typ "beliebig", Sprache passt)  -> MUSS finden
     *   - Filter $0C02 (Typ passt, Sprache passt NICHT) -> darf NICHT finden
     *   - Filter $0E00 (Typ passt nicht)                -> darf NICHT finden */
    checkU32("F$Link(\"foo\", Filter $0C00: Typ passt, Sprache beliebig) findet das Modul",
             Q9K_ModDirLinkByName(0x0C00, "foo"), (Q9_u32)(unsigned long)(region + 1 * 64));
    checkU32("F$Link(\"foo\", Filter $0001: Typ beliebig, Sprache passt) findet das Modul",
             Q9K_ModDirLinkByName(0x0001, "foo"), (Q9_u32)(unsigned long)(region + 1 * 64));
    checkU32("F$Link(\"foo\", Filter $0C02: falsche Sprache) liefert 0",
             Q9K_ModDirLinkByName(0x0C02, "foo"), 0);
    checkU32("F$Link(\"foo\", Filter $0E00: falscher Typ) liefert 0",
             Q9K_ModDirLinkByName(0x0E00, "foo"), 0);

    /* Fall 6: F$UnLink -- muss den per F$Link gefundenen Header wieder
     * korrekt entfernen (Link-Zaehler auf 0 durch genau einen F$Link-
     * Aufruf oben), danach darf F$Link("init") nur noch die verbleibende
     * Revision-1-Instanz finden. */
    checkU32("F$UnLink(modInitRev2) findet den Eintrag (0 = Erfolg)",
             Q9K_ModDirUnlinkByHeader((Q9_u32)(unsigned long)(region + 2 * 64)), 0);
    checkU32("F$Link(\"init\") findet danach nur noch Rev 1",
             Q9K_ModDirLinkByName(0, "init"), (Q9_u32)(unsigned long)(region + 0 * 64));

    /* Fall 7: F$UnLink auf einen NICHT im Verzeichnis stehenden Header
     * (z.B. weil er schon entfernt wurde) muss sauber fehlschlagen (1). */
    checkU32("F$UnLink(bereits entferntes Modul) schlaegt sauber fehl (1)",
             Q9K_ModDirUnlinkByHeader((Q9_u32)(unsigned long)(region + 2 * 64)), 1);



    /* F$UnLoad (Callcode 0x1D). Kern der Sache: es muss denselben
     * Treffer waehlen wie F$Link und denselben Zaehler senken wie
     * F$UnLink -- ohne ihn vorher zu erhoehen. Genau das wird hier
     * nachgerechnet, weil die gemeinsame Suchroutine dafuer aus
     * Q9K_ModDirLinkByName herausgeloest wurde. */
    {
        Q9_u16 err;
        Q9_u32 slot;
        Q9_u32 before;

        /* Ausgangslage schaffen: "foo" zweimal verlinken, Zaehler ablesen. */
        checkU32("F$Link(\"foo\") als Vorbereitung",
                 Q9K_ModDirLinkByName(0, "foo") != 0, 1);
        slot = Q9K_ModDirFindSlotByName(0, "foo", 1);
        checkU32("Suchroutine findet \"foo\"", slot != 0, 1);
        before = Q9K_ModDirGetU16(slot + Q9K_MODDIR_LINKCNT_OFF);

        err = 0xFFFF;
        checkU32("F$UnLoad(\"foo\") meldet Erfolg",
                 Q9K_ModDirUnloadByName(0, "foo", &err), 1);
        checkU32("F$UnLoad meldet dabei keinen Fehler", (Q9_u32)err, 0);
        checkU32("F$UnLoad senkt den Linkzaehler um genau eins",
                 (Q9_u32)Q9K_ModDirGetU16(slot + Q9K_MODDIR_LINKCNT_OFF),
                 before - 1);

        err = 0;
        checkU32("F$UnLoad auf ein unbekanntes Modul schlaegt fehl",
                 Q9K_ModDirUnloadByName(0, "gibtsnicht", &err), 0);
        checkU32("F$UnLoad meldet dabei E_MNF", (Q9_u32)err, 0x00DDU);

        err = 0;
        checkU32("F$UnLoad mit Null-Namenszeiger schlaegt fehl",
                 Q9K_ModDirUnloadByName(0, 0, &err), 0);
        checkU32("F$UnLoad meldet dabei ebenfalls E_MNF", (Q9_u32)err, 0x00DDU);

        /* Ein falscher Typfilter darf denselben Namen NICHT treffen --
         * gleiche Filterregel wie bei F$Link. */
        err = 0;
        checkU32("F$UnLoad mit unpassendem Typfilter trifft nicht",
                 Q9K_ModDirUnloadByName(0x0101, "foo", &err), 0);
    }

    /* F$CRC (Callcode 0x17). Staerkster verfuegbarer Nachweis: der CRC
     * ueber ein ECHTES, von der Microware-Toolchain gebautes Modul --
     * einschliesslich seiner drei eigenen CRC-Bytes -- muss exakt die
     * dokumentierte Konstante CRCCon ($00800FE3, 68k_tech.pdf S.391 und
     * MWOS/OS9/SRC/DEFS/module.a) ergeben. Die Bytes unten sind eine
     * Momentaufnahme eines gebauten Moduls; sie muessen nicht aktuell
     * gehalten werden, da die Aussage fuer genau diese Bytes gilt. */
    {
        static const unsigned char realModule[] = {
    0x4A, 0xFC, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA6, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x48, 0x05, 0x55, 0x01, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB1, 0xB9,
    0x00, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x9A,
    0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x73, 0x76, 0x63, 0x00, 0x00, 0x70, 0x01,
    0x22, 0x3C, 0x00, 0x00, 0x00, 0x21, 0x41, 0xFA, 0x00, 0x14, 0x4E, 0x40,
    0x00, 0x8C, 0x65, 0x00, 0x00, 0x08, 0x72, 0x00, 0x4E, 0x40, 0x00, 0x06,
    0x4E, 0x40, 0x00, 0x06, 0x48, 0x61, 0x6C, 0x6C, 0x6F, 0x20, 0x61, 0x75,
    0x73, 0x20, 0x65, 0x69, 0x6E, 0x65, 0x6D, 0x20, 0x65, 0x63, 0x68, 0x74,
    0x65, 0x6E, 0x20, 0x50, 0x72, 0x6F, 0x67, 0x72, 0x61, 0x6D, 0x6D, 0x21,
    0x0D, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8F, 0xFF, 0x92,
        };
        static unsigned char patched[sizeof(realModule)];
        Q9_u32 accum;
        Q9_u32 half;
        unsigned i;

        accum = Q9K_CrcAccumulate(0xFFFFFFFFUL,
                                  (Q9_u32)(unsigned long)realModule,
                                  (Q9_u32)sizeof(realModule));
        checkU32("F$CRC ueber ein echtes Modul ergibt CRCCon", accum, 0x00800FE3UL);

        /* Der Akkumulator darf ueber mehrere Aufrufe fortgefuehrt werden
         * (ausdruecklich dokumentiert) -- zweigeteilt muss dasselbe
         * herauskommen. */
        half = Q9K_CrcAccumulate(0xFFFFFFFFUL,
                                 (Q9_u32)(unsigned long)realModule, 100);
        half = Q9K_CrcAccumulate(half,
                                 (Q9_u32)(unsigned long)(realModule + 100),
                                 (Q9_u32)sizeof(realModule) - 100);
        checkU32("F$CRC laesst sich ueber mehrere Aufrufe fortfuehren", half, accum);

        checkU32("F$CRC ueber 0 Bytes laesst den Akkumulator unveraendert",
                 Q9K_CrcAccumulate(0x00123456UL, (Q9_u32)(unsigned long)realModule, 0),
                 0x00123456UL);

        for (i = 0; i < sizeof(realModule); ++i)
            patched[i] = realModule[i];
        patched[40] ^= 0x01;
        checkU32("F$CRC erkennt ein einzelnes gekipptes Bit",
                 Q9K_CrcAccumulate(0xFFFFFFFFUL,
                                   (Q9_u32)(unsigned long)patched,
                                   (Q9_u32)sizeof(patched)) != 0x00800FE3UL,
                 1);
    }


    /* F$SetCRC (Callcode 0x26): setzt Kopfparitaet UND CRC neu. Nachweis
     * ohne Erwartungswert-Tabelle: ein absichtlich verfaelschtes Modul
     * wird von F$SetCRC wieder so hergerichtet, dass F$CRC ueber das
     * ganze Modul erneut CRCCon liefert und das XOR aller Kopfworte
     * wieder $FFFF ergibt -- also genau die beiden Bedingungen, an denen
     * echte OS-9-Werkzeuge ein Modul messen. */
    {
        static unsigned char mod[0x40];
        Q9_u16 err;
        Q9_u32 parity;
        Q9_u32 k;

        memset(mod, 0, sizeof(mod));
        mod[0x00] = 0x4A; mod[0x01] = 0xFC;              /* Sync */
        mod[0x04] = 0; mod[0x05] = 0; mod[0x06] = 0; mod[0x07] = sizeof(mod);
        mod[0x30] = 'x'; mod[0x31] = 'y';                 /* etwas Modulkoerper */
        mod[0x3A] = 0x5A;

        err = 0xFFFF;
        checkU32("F$SetCRC nimmt ein gueltiges Modulabbild an",
                 Q9K_ModSetCRC((Q9_u32)(unsigned long)mod, &err), 1);
        checkU32("F$SetCRC meldet dabei keinen Fehler", (Q9_u32)err, 0);

        parity = 0;
        for (k = 0; k < 0x30UL; k += 2)
            parity ^= ((Q9_u32)mod[k] << 8) | (Q9_u32)mod[k + 1];
        checkU32("F$SetCRC stellt die Kopfparitaet her", parity, 0xFFFFUL);

        checkU32("F$SetCRC macht den Modul-CRC gueltig (CRCCon)",
                 Q9K_CrcAccumulate(0xFFFFFFFFUL, (Q9_u32)(unsigned long)mod,
                                   (Q9_u32)sizeof(mod)),
                 0x00800FE3UL);

        /* Nach einer Aenderung im Koerper muss es erneut funktionieren. */
        mod[0x35] = 0x77;
        checkU32("F$SetCRC laesst sich nach einer Aenderung wiederholen",
                 Q9K_ModSetCRC((Q9_u32)(unsigned long)mod, &err), 1);
        checkU32("F$SetCRC stellt den CRC danach wieder her",
                 Q9K_CrcAccumulate(0xFFFFFFFFUL, (Q9_u32)(unsigned long)mod,
                                   (Q9_u32)sizeof(mod)),
                 0x00800FE3UL);

        /* Geprueft werden laut Beschreibung nur Sync und Groesse. */
        mod[0x00] = 0x00;
        err = 0;
        checkU32("F$SetCRC weist ein fehlendes Sync-Wort ab",
                 Q9K_ModSetCRC((Q9_u32)(unsigned long)mod, &err), 0);
        checkU32("F$SetCRC meldet dabei E_BMID", (Q9_u32)err, 0x00CDU);
        mod[0x00] = 0x4A;

        mod[0x07] = 0x10;   /* Groesse kleiner als Kopf+CRC */
        err = 0;
        checkU32("F$SetCRC weist eine unmoegliche Modulgroesse ab",
                 Q9K_ModSetCRC((Q9_u32)(unsigned long)mod, &err), 0);
        checkU32("F$SetCRC meldet auch dafuer E_BMID", (Q9_u32)err, 0x00CDU);

        err = 0;
        checkU32("F$SetCRC weist eine ungerade Moduladresse ab",
                 Q9K_ModSetCRC((Q9_u32)(unsigned long)mod + 1, &err), 0);
        err = 0;
        checkU32("F$SetCRC weist den Nullzeiger ab",
                 Q9K_ModSetCRC(0, &err), 0);
    }

    /* F$GModDr (Callcode 0x1A): kopiert das Verzeichnis heraus und
     * schneidet dabei auf ganze Eintraege ab. */
    {
        static unsigned char buf[8 * Q9K_TEST_SLOT_SIZE];
        Q9_u32 copied;
        Q9_u32 entries;
        Q9_u32 slot;

        /* Erwartete Eintragszahl direkt aus der aktiven Liste zaehlen. */
        entries = 0;
        for (slot = Q9K_GetU32(Q9K_MODDIR_HEAD_ADDR); slot != 0;
             slot = Q9K_GetU32(slot + Q9K_MODDIR_NEXT_OFF))
            entries++;

        memset(buf, 0xEE, sizeof(buf));
        copied = Q9K_ModDirCopyOut((Q9_u32)(unsigned long)buf, sizeof(buf));
        checkU32("F$GModDr kopiert jeden Verzeichniseintrag",
                 copied, entries * Q9K_MODDIR_ENTRY_BYTES);

        /* Ein zu kleiner Puffer liefert nur ganze Eintraege. */
        copied = Q9K_ModDirCopyOut((Q9_u32)(unsigned long)buf,
                                   Q9K_MODDIR_ENTRY_BYTES + Q9K_MODDIR_ENTRY_BYTES / 2);
        checkU32("F$GModDr schneidet auf ganze Eintraege ab",
                 copied, entries >= 1 ? Q9K_MODDIR_ENTRY_BYTES : 0);

        copied = Q9K_ModDirCopyOut((Q9_u32)(unsigned long)buf, 0);
        checkU32("F$GModDr mit Puffergroesse 0 kopiert nichts", copied, 0);

        copied = Q9K_ModDirCopyOut(0, sizeof(buf));
        checkU32("F$GModDr mit Null-Puffer kopiert nichts", copied, 0);
    }


    /* F$DatMod (Callcode 0x25): erzeugt ein echtes Datenmodul im Speicher
     * und traegt es ins Verzeichnis ein. Nachweis ohne Erwartungstabelle:
     * das erzeugte Modul muss anschliessend alle Pruefungen bestehen, die
     * dieser Kernel an ein Modul stellt -- Sync, Groesse, Kopfparitaet,
     * CRC gegen CRCCon -- und per F$Link auffindbar sein. */
    {
        static const char dmName[] = "shareddata";
        Q9_u32 hdr = 0, data = 0, past = 0;
        Q9_u16 err = 0xFFFF;
        Q9_u32 total;
        Q9_u32 parity;
        Q9_u32 k;

        printf("\n--- F$DatMod ---\n");
        g_datmodNext = 0;
        g_freeMemCalls = 0;

        checkU32("F$DatMod erzeugt ein Datenmodul",
                 Q9K_ModCreateData(64, 0x0001, 0x0555, (Q9_u16)(Q9K_MT_DATA << 8),
                                   (Q9_u32)(unsigned long)dmName,
                                   &hdr, &data, &past, &err), 1);
        checkU32("F$DatMod meldet dabei keinen Fehler", (Q9_u32)err, 0);
        checkU32("der Datenzeiger liegt hinter dem 48-Byte-Kopf",
                 data - hdr, Q9K_MH_HEADER_SIZE);
        checkU32("(a0) zeigt hinter den Namen",
                 past, (Q9_u32)(unsigned long)dmName + 10UL);

        total = Q9K_ReadU32BE((const Q9_u8 *)hdr + Q9K_MH_SIZE);
        checkU32("die Modulgroesse ist auf 16 Byte aufgerundet", total % 16UL, 0);
        checkU32("und fasst Kopf, Daten, Namen und CRC",
                 (Q9_u32)(total >= Q9K_MH_HEADER_SIZE + 64UL + 10UL + 3UL), 1);

        checkU32("das Sync-Wort steht am Anfang",
                 (Q9_u32)((Q9K_GetU8Raw(hdr) << 8) | Q9K_GetU8Raw(hdr + 1)), 0x4AFCUL);

        parity = 0;
        for (k = 0; k < Q9K_MH_PARITY + 2UL; k += 2)
            parity ^= ((Q9_u32)Q9K_GetU8Raw(hdr + k) << 8) | (Q9_u32)Q9K_GetU8Raw(hdr + k + 1);
        checkU32("die Kopfparitaet stimmt", parity, 0xFFFFUL);

        checkU32("der CRC des erzeugten Moduls stimmt (CRCCon)",
                 Q9K_CrcAccumulate(0xFFFFFFFFUL, hdr, total), 0x00800FE3UL);

        /* Der Datenbereich muss geloescht sein -- die Beschreibung
         * verlangt das ausdruecklich. */
        {
            Q9_u32 nonzero = 0;
            for (k = 0; k < 64UL; ++k)
                if (Q9K_GetU8Raw(data + k) != 0)
                    nonzero++;
            checkU32("der Datenbereich ist geloescht", nonzero, 0);
        }

        /* Und das Modul ist im Verzeichnis auffindbar. */
        checkU32("F$Link findet das erzeugte Datenmodul",
                 (Q9_u32)(Q9K_ModDirLinkByName(0, "shareddata") == hdr), 1);

        err = 0;
        checkU32("F$DatMod weist einen Nullzeiger als Namen ab",
                 Q9K_ModCreateData(16, 0, 0, 0x0400, 0, &hdr, &data, &past, &err), 0);
        checkU32("F$DatMod meldet dafuer E_BNAM", (Q9_u32)err, 0x00CEUL);

        err = 0;
        checkU32("F$DatMod weist einen leeren Namen ab",
                 Q9K_ModCreateData(16, 0, 0, 0x0400,
                                   (Q9_u32)(unsigned long)"", &hdr, &data, &past, &err), 0);

        err = 0;
        checkU32("F$DatMod meldet erschoepften Speicher sauber",
                 Q9K_ModCreateData(100000UL, 0, 0, 0x0400,
                                   (Q9_u32)(unsigned long)dmName,
                                   &hdr, &data, &past, &err), 0);
        checkU32("und nutzt dafuer E_MEMFUL", (Q9_u32)err, 0x00CFUL);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
