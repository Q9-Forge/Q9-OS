/*
 * q9kernel_tables.c -- Q9-OS eigener Kernel: System-/User-Dispatch-
 *                      Tabellen (SYSDIS/USRDIS), Modulverzeichnis,
 *                      Prozess-/Pfad-Deskriptor-Pools (Abschnitt 2,
 *                      Punkt 5/6).
 *
 * Init-Modul-Feldoffsets $38 (M$Procs), $3A (M$Paths), $62 (M$MDirSz)
 * stammen aus dem OFFIZIELLEN Manual (68k_tech.pdf, Table 2-4 "Init
 * Module Values") -- nicht reverse-engineert, direkt nachgeschlagen
 * (s. Andreas' Standing-Guidance: Manuals zuerst pruefen). Alle drei
 * liegen unterhalb von Offset $7C, sind also durch dieselbe Bounds-
 * Pruefung (initAvailableLen >= 0x7C) abgedeckt, die q9kernel_cinit.c
 * bereits fuer M$Compat/M$Compat2/M$SysConf durchfuehrt -- kein
 * zusaetzlicher Bounds-Check hier noetig, Aufrufer-Verantwortung.
 *
 * SYSDIS/USRDIS-Groesse (0x800 Byte je Tabelle) und Modulverzeichnis-
 * Eintragsgroesse (16 Byte) sind reale, per Disassemblierung verifizierte
 * Werte (docs/REVERSE_ENGINEERING.md, "Fund: Q9_disp_488" bzw. Thema 01
 * Modulverzeichnis-Nachtrag) -- keine Q9-Erfindung.
 *
 * Prozess-/Pfad-DESKRIPTOR-Groesse (Q9K_PROCDESC_SIZE/PATHDESC_SIZE)
 * dagegen IST eine eigene, bewusst vorlaeufige Q9-Festlegung: das echte
 * Deskriptor-Byte-Layout ist noch nicht reverse-engineert
 * (Q9_D_PROCSZ in q9sysglob.h ist selbst [PLATZHALTER]), und ein
 * Deskriptor-Layout ist ohnehin KEIN Kompat-Erfordernis (nur der externe
 * Modul-Header/Dreiklang ist es) -- deshalb hier bewusst ein einfacher,
 * generischer Slot-Pool mit Freiliste statt eines vorgetaeuschten
 * "echten" Layouts. Wird ueberarbeitet, sobald ein echtes Deskriptor-
 * Layout (Punkt 7, erster Ausfuehrungskontext) entworfen ist.
 *
 * Aufteilungsreihenfolge des EINEN grossen Arena-Blocks (SYSDIS ->
 * USRDIS -> Modulverzeichnis -> Prozess-Pool -> Pfad-Pool) folgt der
 * gleichen Grundidee wie der echte Kernel (Thema 01: ein Block, seriell
 * aufgeteilt), aber mit eigener Reihenfolge/eigenen Deskriptorgroessen.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

extern Q9_u32 Q9K_AllocMem(Q9_u32 requestedSize);

#ifndef Q9_D_SYSDIS
#define Q9_D_SYSDIS 0x3A4UL   /* s. q9sysglob.h, per #ifndef ueberschreibbar fuer Host-Tests */
#endif
#ifndef Q9_D_USRDIS
#define Q9_D_USRDIS 0x3A8UL
#endif
#ifndef Q9_D_MODDIR
#define Q9_D_MODDIR 0x03CUL   /* Start-Zeiger, s. q9sysglob.h */
#endif
/* Ende-Zeiger, real bei Q9_D_MODDIR+4 (0x40) -- NICHT hartkodiert "+4"
 * verwenden (gleicher Host-Test-Fund wie schon bei Q9K_ARENA_HEAD/TAIL
 * in q9kernel_arena.c): auf dem echten 32-Bit-Ziel ist Q9_u32 4 Byte
 * breit, die beiden Zeiger liegen also korrekt direkt hintereinander;
 * auf einem 64-Bit-Testhost wuerde ein Schreibzugriff auf Q9_D_MODDIR
 * (8 Byte) den bei "+4" beginnenden zweiten Zeiger ueberschreiben. Per
 * #ifndef ueberschreibbar, echte Ziel-Offsets unveraendert. */
#ifndef Q9_D_MODDIR_END
#define Q9_D_MODDIR_END (Q9_D_MODDIR + 4)
#endif

#define Q9K_SYSDIS_SIZE       0x800UL  /* real verifiziert, s. Kopfkommentar */
#define Q9K_USRDIS_SIZE       0x800UL
#define Q9K_MODDIR_ENTRY_SIZE 16UL     /* real verifiziert, s. Kopfkommentar */

#define Q9K_INIT_OFF_PROCS    0x38UL   /* M$Procs,  68k_tech.pdf Table 2-4 */
#define Q9K_INIT_OFF_PATHS    0x3AUL   /* M$Paths,  68k_tech.pdf Table 2-4 */
#define Q9K_INIT_OFF_MDIRSZ   0x62UL   /* M$MDirSz, 68k_tech.pdf Table 2-4 */

#define Q9K_PROCDESC_SIZE     128UL    /* PLATZHALTER, s. Kopfkommentar */
#define Q9K_PATHDESC_SIZE     32UL     /* PLATZHALTER, s. Kopfkommentar */

/* eigene Kernel-Global-Erweiterungen, direkt hinter Q9K_CpuCount
 * ($1200, s. q9kernel_cinit.c) -- kein Feld aus dem echten Kernel-
 * Layout, deshalb hier lokal statt in q9sysglob.h. Bewusst 8 statt 4
 * Byte Abstand (nicht bloss "naechstes Q9_u32-Feld") -- diese Adressen
 * sind eigene Erfindung ohne Kompat-Zwang, deshalb hier direkt
 * grosszuegig genug gewaehlt, um auch auf einem 64-Bit-Testhost (wo
 * Q9_u32 8 statt 4 Byte breit ist) ueberlappungsfrei zu bleiben --
 * spart die #ifndef-Override-Mechanik, die Q9_D_MODDIR_END oben
 * braucht (dort sind die Offsets echte, fixe Ziel-Werte, hier nicht). */
#ifndef Q9K_PROCPOOL_BASE_ADDR
#define Q9K_PROCPOOL_BASE_ADDR 0x1204UL
#endif
#ifndef Q9K_PROCPOOL_FREE_ADDR
#define Q9K_PROCPOOL_FREE_ADDR 0x120CUL
#endif
#ifndef Q9K_PATHPOOL_BASE_ADDR
#define Q9K_PATHPOOL_BASE_ADDR 0x1214UL
#endif
#ifndef Q9K_PATHPOOL_FREE_ADDR
#define Q9K_PATHPOOL_FREE_ADDR 0x121CUL
#endif

/* Byteweise Zusammensetzung statt Roh-Pointer-Cast -- WICHTIG (gleiche
 * Konvention wie q9kernel_cinit.c's M$SysConf-Lesezugriff): initMod
 * zeigt auf FREMDE, extern geschriebene Moduldaten (immer Big-Endian,
 * unabhaengig vom Host), ein roher "*(Q9_u16*)"-Cast waere auf einem
 * Little-Endian-Testhost falsch UND koennte auf echter 68000-Hardware
 * bei ungerader Adresse einen Address-Error ausloesen (Wortzugriffe
 * muessen dort geradzahlig ausgerichtet sein). Q9K_SetU32 betrifft
 * dagegen nur EIGENE, intern konsistente Kernel-Global-Felder (gleicher
 * Schreiber/Leser, native Zielordnung) -- dort bleibt der Roh-Cast
 * unveraendert, analog zu q9kernel_arena.c/q9kernel_exctable.c. */
static Q9_u16 Q9K_GetU16(Q9_u32 addr)
{
    const Q9_u8 *p = (const Q9_u8 *)addr;
    return (Q9_u16)((p[0] << 8) | p[1]);
}

static void Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }

/* Verkettet count gleich grosse Slots ab base zu einer einfachen
 * Freiliste (erster Q9_u32 jedes Slots = Zeiger auf naechsten freien
 * Slot, 0 = Ende) und schreibt den Listenkopf nach freeHeadAddr. Eigene
 * Erfindung (kein Kompat-Bezug), gleiches Prinzip wie das Arena-
 * Freiblock-Format (q9kernel_arena.c). */
static void Q9K_BuildFreeList(Q9_u32 base, Q9_u32 slotSize, Q9_u32 count, Q9_u32 freeHeadAddr)
{
    Q9_u32 i;

    if (count == 0) {
        Q9K_SetU32(freeHeadAddr, 0);
        return;
    }

    for (i = 0; i < count - 1; i++)
        Q9K_SetU32(base + i * slotSize, base + (i + 1) * slotSize);
    Q9K_SetU32(base + (count - 1) * slotSize, 0);

    Q9K_SetU32(freeHeadAddr, base);
}

/* Liest M$Procs/M$Paths/M$MDirSz aus dem gefundenen Init-Modul,
 * alloziert EINEN zusammenhaengenden Block ueber den Arena-Allokator
 * und teilt ihn auf. initMod muss bereits gegen Q9K_ValidModuleHeader
 * geprueft UND per initAvailableLen>=0x7C bounds-geprueft sein (Aufrufer-
 * Verantwortung, s. q9kernel_cinit.c). Rueckgabe 0 = Erfolg, 1 =
 * Allokation fehlgeschlagen (Arena zu klein/nicht initialisiert). */
Q9_u32 Q9K_SetupTables(const Q9_u8 *initMod)
{
    Q9_u32 initAddr  = (Q9_u32)(unsigned long)initMod;
    Q9_u16 procs     = Q9K_GetU16(initAddr + Q9K_INIT_OFF_PROCS);
    Q9_u16 paths     = Q9K_GetU16(initAddr + Q9K_INIT_OFF_PATHS);
    Q9_u16 mdirSz    = Q9K_GetU16(initAddr + Q9K_INIT_OFF_MDIRSZ);

    Q9_u32 moddirSize   = (Q9_u32)mdirSz * Q9K_MODDIR_ENTRY_SIZE;
    Q9_u32 procPoolSize = (Q9_u32)procs  * Q9K_PROCDESC_SIZE;
    Q9_u32 pathPoolSize = (Q9_u32)paths  * Q9K_PATHDESC_SIZE;
    Q9_u32 totalSize    = Q9K_SYSDIS_SIZE + Q9K_USRDIS_SIZE + moddirSize + procPoolSize + pathPoolSize;

    Q9_u32 block = Q9K_AllocMem(totalSize);
    Q9_u32 cursor;

    if (block == 0)
        return 1;

    cursor = block;

    /* SYSDIS/USRDIS: noch OHNE Inhalt (kein Syscall-Registrierungs-
     * mechanismus vorhanden, s. docs/REVERSE_ENGINEERING.md "Fund
     * (Korrektur einer Fehlannahme)") -- genullt statt uninitialisiert
     * gelassen, damit ein versehentlicher Aufruf ins Leere (Nullzeiger)
     * statt in Zufallsspeicher springt. TODO: echte Registrierungs-
     * funktion, sobald F$Link/Modul-Laden existiert. */
    {
        Q9_u32 i;
        for (i = 0; i < Q9K_SYSDIS_SIZE + Q9K_USRDIS_SIZE; i += 4)
            Q9K_SetU32(cursor + i, 0);
    }

    Q9K_SetU32(Q9_D_SYSDIS, cursor);
    cursor += Q9K_SYSDIS_SIZE;

    Q9K_SetU32(Q9_D_USRDIS, cursor);
    cursor += Q9K_USRDIS_SIZE;

    Q9K_SetU32(Q9_D_MODDIR, cursor);                     /* Start-Zeiger, s. Thema 01 */
    Q9K_SetU32(Q9_D_MODDIR_END, cursor + moddirSize);    /* Ende-Zeiger */
    cursor += moddirSize;

    Q9K_SetU32(Q9K_PROCPOOL_BASE_ADDR, cursor);
    Q9K_BuildFreeList(cursor, Q9K_PROCDESC_SIZE, procs, Q9K_PROCPOOL_FREE_ADDR);
    cursor += procPoolSize;

    Q9K_SetU32(Q9K_PATHPOOL_BASE_ADDR, cursor);
    Q9K_BuildFreeList(cursor, Q9K_PATHDESC_SIZE, paths, Q9K_PATHPOOL_FREE_ADDR);
    cursor += pathPoolSize;

    return 0;
}
