/*
 * q9kernel_cinit.c -- Q9-OS eigener Kernel: C-Fortsetzung nach dem
 *                     Assembler-Einstieg (q9kernel_entry.a, Q9K_Entry).
 *
 * ERSTER ENTWURF, 2026-08-17 -- ERFOLGREICH gegen die komplette echte
 * Microware-C-Pipeline getestet (xcc -> cpfe -> ilink -> iopt -> be68k
 * -> opt68k -> r68, ueber ein minimales Makefile + mwos-build): alle
 * Stufen "exit status = 0", echtes 375-Byte-Objekt (.r) erzeugt. Nicht
 * mit dem Kernel-Modul selbst (q9kernel_entry.a) gelinkt -- dafuer
 * braeuchte es ein gemeinsames Makefile-Ziel, noch nicht angelegt.
 *
 * NACHTRAG 2026-08-18: bindet jetzt q9kernel_config.h ein, braucht also
 * zwingend -DQ9K_KERNEL_ATOMIC/-DQ9K_KERNEL_DEVELOPMENT PLUS
 * -DQ9K_ALLOC_STANDARD/-DQ9K_ALLOC_BUDDY beim Bauen (sonst #error,
 * s. q9kernel_config.h) -- inhaltlich noch ohne Wirkung, reine
 * Vorbereitung fuer kuenftigen variantenabhaengigen Code. Erneut real
 * kompiliert (Development+Standard), weiterhin exit status = 0.
 * BEWUSST klassische C-Typen (unsigned long/short/char) statt stdint.h/
 * uint32_t -- die xcc-Pipeline zeigt Defines wie _OSK/_MPF68000/_BIG_END,
 * die auf einen aelteren C-Sprachstand hindeuten; stdint.h-Verfuegbarkeit
 * fuer DIESE Toolchain nicht verifiziert (anders als q9moduleheader.c,
 * das explizit reines Host-gcc-Tooling ist, siehe dessen eigenen
 * Kopfkommentar -- diese Datei hier NICHT).
 *
 * Wenn Q9K_CInit zurueckkehrt, faengt Q9K_Entry's Q9K_HaltLoop das ab
 * (kein Absturz, aber auch kein Fortschritt) -- bewusst so belassen statt
 * hier zusaetzlich eine eigene Panik-Schleife zu duplizieren.
 *
 * Kernel-Globals-Zugriff: Basisadresse ist bei diesem Kernel fest $000000
 * (s. q9kernel_entry.a, Q9K_GlobBase) -- die Offset-Konstanten aus
 * src/q9sysglob.h sind dadurch fuer UNS direkt absolute Adressen, kein
 * A6-relativer Zugriff noetig wie beim echten 68K-Referenzkernel. Deshalb
 * hier reine Pointer-Casts auf absolute Adressen, keine A6-Simulation.
 *
 * In dieser Runde implementiert: die sechs leeren, zirkulaeren
 * Bereitschafts-/Warteschlangen (Kopf=Schwanz=sich selbst), exakt nach
 * dem verifizierten Fund aus docs/kernel-walkthrough/01-kernel-bootstrap/
 * README.md ("Zweiter Bonus-Fund"). ALLES Weitere aus der Boot-Reihenfolge
 * (Abschnitt 2 in OWN_KERNEL_INIT_PLAN.md: Speichergroesse/Arena richtig
 * aufsetzen -- nicht nur die leere Liste --, Dispatch-Tabelle, Init-Modul-
 * Suche/Schritt 6a, Prozess-/Pfad-Tabellen, Scheduler-Sprung) ist bewusst
 * NICHT implementiert, klar als TODO markiert, keine Attrappen/Fake-Logik.
 *
 * NACHTRAG 2026-08-18: die Speicherregion-Liste fuer Schritt 6a ist
 * inzwischen geklaert und in q9kernel_entry.a als Q9K_BootList gesichert
 * (die urspruengliche "keine Quelle"-Frage war ein Irrtum -- der Boot-
 * ROM uebergibt sie ueber sp beim Einsprung, s. Thema 01). Ebenfalls
 * fertig: Q9K_CheckSyncWord/Q9K_ValidModuleHeader (q9kernel_modcheck.c)
 * fuer die Kandidatenpruefung. Der eigentliche Such-/Vergleichs-Code
 * (Namensvergleich "init", Revisions-Tiebreak) fehlt noch.
 *
 * Ebenfalls neu: die Modulverzeichnis-Anlage (Q9_D_MODDIR) direkt nach
 * der Init-Suche haengt an M$MDirSz AUS dem gefundenen Init-Modul --
 * kann also erst NACH Schritt 6a passieren, nicht parallel dazu (echte
 * Reihenfolge-Zwangslaeufigkeit im Original, s. Thema 01, nicht nur
 * Design-Praeferenz). CPU-Anzahl fuer SMP kommt optional ueber dieselbe
 * Art Erweiterungsmechanismus (Q9K_GetCpuCount, q9kernel_initext.c) --
 * Default 1, falls das Init-Modul keine Q9-Erweiterung hat.
 */

#include "../q9sysglob.h"
#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned char  Q9_u8;

/* Aus q9kernel_modsearch.c/q9kernel_initext.c -- externe Deklarationen
 * statt gemeinsamer Header, gleiche bewusst schlanke Konvention wie
 * ueberall in diesem Verzeichnis. */
extern const Q9_u8 *Q9K_FindModuleByName(const Q9_u8 *regionList, const char *targetName, Q9_u32 *outAvailableLen);
extern Q9_u32 Q9K_GetCpuCount(const Q9_u8 *initModAddr, Q9_u32 availableLen);
extern void Q9K_ArenaInit(Q9_u32 freeBase, Q9_u32 freeSize);
extern Q9_u32 Q9K_BuildExcTable(void);
extern Q9_u32 Q9K_SetupTables(const Q9_u8 *initMod);
extern Q9_u32 Q9K_StartFirstProcess(void);
extern void   Q9K_JumpToFirstProc(void);   /* q9kernel_entry.a, kein Ruecksprung vorgesehen */

/* Eigene Kernel-Global-Erweiterungen (KEIN Feld aus dem echten Kernel-
 * Layout, deshalb hier lokal definiert statt in q9sysglob.h, s. dessen
 * eigenen Kopfkommentar) -- direkt hinter dem legacy-kompatiblen
 * Bereich (Q9_D_End=$1000):
 *   Q9K_BootList  ($1000, 64x8=512 Byte)  -- s. q9kernel_entry.a
 *   Q9K_CpuCount  ($1200, 4 Byte)          -- neu hier, Ergebnis von
 *                                             Q9K_GetCpuCount, noch von
 *                                             niemandem konsumiert
 *                                             (kein SMP-Scheduler
 *                                             existiert bisher) --
 *                                             trotzdem schon ein
 *                                             permanenter Platz dafuer,
 *                                             s. OWN_KERNEL_INIT_PLAN.md
 *                                             SMP-Abschnitt. */
#define Q9K_BOOTLIST_ADDR   0x1000UL
#define Q9K_CPUCOUNT_ADDR   0x1200UL

/* Freispeicher-Basis fuer die Arena (Abschnitt 2, Punkt 3) -- 2026-08-18
 * mit Andreas abgestimmt: fester Offset, VORLAEUFIG, unter der Annahme,
 * dass der Boot-ROM das Kernel-Abbild selbst oberhalb dieser Adresse
 * laedt (noch NICHT am echten/emulierten Boot-Pfad verifiziert -- der
 * neue Kernel wird testweise parallel zum echten dker030s ladbar
 * gemacht, s. build.sh/vendor, dort dann pruefen). Liegt bewusst deutlich
 * oberhalb von Q9K_CpuCount ($1200), damit spaeter noch Platz fuer
 * weitere eigene Kernel-Global-Erweiterungen bleibt, ohne die Arena-
 * Basis wieder verschieben zu muessen. */
#define Q9K_FREEMEM_BASE    0x2000UL

/* Schreibt einen 32-Bit-Wert an eine absolute Adresse (=Kernel-Global-
 * Offset, da Kernel-Globals-Basis bei diesem Kernel $000000 ist) */
static void Q9K_PutU32(Q9_u32 addr, Q9_u32 value)
{
    *(volatile Q9_u32 *)addr = value;
}

/* Wie Q9K_PutU32, aber fuer 1-/2-Byte-Felder -- WICHTIG, nicht einfach
 * Q9K_PutU32 fuer schmalere Felder wiederverwenden, das wuerde
 * benachbarte Kernel-Global-Bytes ueberschreiben (Q9_D_COMPAT/
 * Q9_D_COMPAT2 sind je 1 Byte breit, s. q9sysglob.h). */
static void Q9K_PutU8(Q9_u32 addr, Q9_u8 value)
{
    *(volatile Q9_u8 *)addr = value;
}

static void Q9K_PutU16(Q9_u32 addr, unsigned short value)
{
    *(volatile unsigned short *)addr = value;
}

/* Initialisiert eine leere, zirkulaere Warteschlange: Kopf- und Schwanz-
 * Zeiger (an queueBase+headOff bzw. queueBase+tailOff) zeigen beide auf
 * die Warteschlange selbst (queueBase) -- das beim echten Kernel
 * verifizierte Terminierungsmuster fuer "leer" (s. Kopfkommentar). */
static void Q9K_InitEmptyQueue(Q9_u32 queueBase, Q9_u32 headOff, Q9_u32 tailOff)
{
    Q9K_PutU32(queueBase + headOff, queueBase);
    Q9K_PutU32(queueBase + tailOff, queueBase);
}

void Q9K_CInit(void)
{
    /* Sechs leere Ringlisten -- exakte Offsets aus q9sysglob.h bzw. dem
     * verifizierten Fund in Thema 01 (Kopf-/Schwanz-Unteroffsets je
     * Warteschlangenart unterschiedlich, s. dortige Tabelle):
     *   Q9_D_ACTIVQ/Q9_D_SLEEPQ/Q9_D_WAITQ -- Prozess-Warteschlangen, +0x30/+0x34
     *   Q9_D_ARENA                        -- Speicher-Kontrollblock,  +0x8/+0xC
     *   Q9_D_ALMQ1/Q9_D_ALMQ2              -- F$Alarm-Warteschlangen,  +0xC/+0x10
     */
    Q9K_InitEmptyQueue(Q9_D_ACTIVQ, 0x30, 0x34);
    Q9K_InitEmptyQueue(Q9_D_SLEEPQ, 0x30, 0x34);
    Q9K_InitEmptyQueue(Q9_D_WAITQ,  0x30, 0x34);
    Q9K_InitEmptyQueue(Q9_D_ARENA,  0x08, 0x0C);
    Q9K_InitEmptyQueue(Q9_D_ALMQ1,  0x0C, 0x10);
    Q9K_InitEmptyQueue(Q9_D_ALMQ2,  0x0C, 0x10);

    /* Abschnitt 2, Punkt 3: Speichergroesse aus Q9_D_TOTRAM lesen (vom
     * Assembler-Einstieg schon aus D0 gesichert, s. q9kernel_entry.a),
     * echten freien Speicherblock im Arena-Kontrollblock registrieren --
     * die Ringliste oben war nur der leere Ausgangszustand.
     * Q9K_FREEMEM_BASE ist VORLAEUFIG (s. dortiger Kommentar) -- falls
     * Q9_D_TOTRAM kleiner als die Basis ist (unplausibel kleines RAM
     * oder falsch gelesener Wert), bewusst KEINE Initialisierung statt
     * mit einer negativ/riesig unterlaufenden Groesse zu rechnen. */
    {
        Q9_u32 totalRam = *(volatile Q9_u32 *)Q9_D_TOTRAM;

        if (totalRam > Q9K_FREEMEM_BASE) {
            Q9K_ArenaInit(Q9K_FREEMEM_BASE, totalRam - Q9K_FREEMEM_BASE);
        }
    }

    /* Abschnitt 2, Punkt 4: Exception-/Trap-Dispatch-Tabelle aus
     * kompakter Quelltabelle expandieren (q9kernel_exctable.c) --
     * Q9_D_EXCJMP zeigt auf den vom Boot-ROM bereitgestellten
     * Speicherblock dafuer (s. q9kernel_entry.a). Alle Vektoren zeigen
     * bisher auf denselben generischen Halt-Handler (kein IRQ-/Syscall-
     * Dispatcher existiert noch, s. dortige Kopfkommentare/TODOs).
     * Rueckgabewert (Konsistenzcheck der Quelltabelle) noch nicht
     * ausgewertet -- kein Panic-Mechanismus vorhanden, dem ein
     * Fehlschlag hier ohnehin mitgeteilt werden koennte (TODO, sobald
     * es einen gibt). */
    Q9K_BuildExcTable();

    /* Schritt 6a: Init-Modul per Namenssuche finden (2026-08-18,
     * verdrahtet nach einer Session-Pause -- Q9K_FindModuleByName/
     * Q9K_GetCpuCount existierten schon, waren aber noch nicht
     * aufgerufen). */
    {
        Q9_u32 initAvailableLen = 0;
        const Q9_u8 *initMod = Q9K_FindModuleByName((const Q9_u8 *)Q9K_BOOTLIST_ADDR, "init", &initAvailableLen);

        if (initMod != 0) {
            Q9K_PutU32(Q9_D_INIT, (Q9_u32)(unsigned long)initMod);

            /* Einfache Init-Modul-Konfigurationsfelder direkt uebernehmen
             * -- exakt wie beim echten Kernel verifiziert (Thema 01):
             * M$Compat->D_Compat, M$Compat2->D_Compat2, M$SysConf->
             * D_SysConf. Offsets sind Init-Modul-interne Felder (Manual
             * Table 2-4), NICHT Q9_MH68K_*-Header-Offsets -- liegen
             * ausserhalb des von Q9K_ValidModuleHeader geprueften
             * 0x30-Byte-Bereichs, deshalb Bounds-Check gegen
             * initAvailableLen zwingend vor jedem Zugriff. */
            if (initAvailableLen >= 0x7C) {
                Q9K_PutU8(Q9_D_COMPAT,  initMod[0x68]);
                Q9K_PutU8(Q9_D_COMPAT2, initMod[0x69]);
                Q9K_PutU16(Q9_D_SYSCONF, (unsigned short)((initMod[0x7A] << 8) | initMod[0x7B]));

                /* SMP-CPU-Anzahl: optional, Default 1 falls das
                 * Init-Modul keine Q9-Erweiterung hat (s.
                 * q9kernel_initext.c). Noch von niemandem konsumiert --
                 * kein SMP-Scheduler existiert bisher, aber der
                 * permanente Platz dafuer ist jetzt belegt. */
                Q9K_PutU32(Q9K_CPUCOUNT_ADDR, Q9K_GetCpuCount(initMod, initAvailableLen));

                /* Abschnitt 2, Punkt 5/6: M$Procs/M$Paths/M$MDirSz lesen
                 * (alle < 0x7C, durch denselben Bounds-Check oben
                 * abgedeckt), SYSDIS/USRDIS/Modulverzeichnis + Prozess-/
                 * Pfad-Pools ueber den Arena-Allokator aufsetzen
                 * (q9kernel_tables.c). Rueckgabewert (Allokationsfehler)
                 * noch nicht ausgewertet -- gleiche Begruendung wie bei
                 * Q9K_BuildExcTable (kein Panic-Mechanismus vorhanden). */
                Q9K_SetupTables(initMod);
            }
            /* TODO: initAvailableLen < 0x7C waere ein sehr kleines/
             * unplausibles Init-Modul -- bewusst KEIN Zugriff auf die
             * Konfigurationsfelder in diesem Fall, aber auch keine
             * explizite Fehlerbehandlung dafuer (noch offen). */
        }
        /* TODO: kein Init-Modul gefunden -- der echte Kernel gibt eine
         * feste Meldung aus ("kernel: can't find Init module", Thema 01)
         * und bricht vermutlich ab. Fuer uns noch nicht entschieden --
         * aktuell faellt die Funktion einfach durch bis zum return
         * unten, Q9K_HaltLoop faengt das ab (kein Fortschritt, aber
         * auch kein Absturz). */
    }

    /* Abschnitt 2, Punkt 7 (minimales Geruest, mit Andreas abgestimmt
     * 2026-08-18): ersten Ausfuehrungskontext konstruieren
     * (q9kernel_firstproc.c) und, falls erfolgreich, hineinspringen --
     * kein echter Scheduler (nur EIN Kontext existiert), kein echtes
     * geladenes Programm (F$Link existiert nicht, Sprungziel ist ein
     * reiner Platzhalter). Bei Fehlschlag (Pool/Arena erschoepft) fallen
     * wir bewusst durch bis zum return unten -- Q9K_HaltLoop faengt das
     * ab, kein Fake-Fortschritt. */
    if (Q9K_StartFirstProcess() == 0)
        Q9K_JumpToFirstProc(); /* kein Ruecksprung erwartet */

    return; /* -> Q9K_HaltLoop in q9kernel_entry.a (nur bei Fehlschlag oben) */
}
