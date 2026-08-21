/*
 * q9kernel_exctable.c -- Q9-OS eigener Kernel: Exception-/Trap-Dispatch-
 *                        Tabelle aus kompakter Quelltabelle expandieren.
 *
 * Abschnitt 2, Punkt 4 (s. q9kernel_cinit.c). Mechanismus 1:1 wie am
 * echten Kernel verifiziert (docs/REVERSE_ENGINEERING.md, "Fund:
 * Trap-/Exception-Tabellen-Initialisierung"): Q9_D_EXCJMP zeigt auf
 * einen vom Boot-ROM bereitgestellten Speicherblock (256 Eintraege x
 * 4 Byte = 1024 Byte, s. Q9_T_END in q9sysglob.h); statt 256 einzelne
 * Adressen im Modul zu speichern, wird eine KOMPAKTE Quelltabelle
 * (Gruppen aus {Anzahl, Ziel}) zur Boot-Zeit in die volle, direkt
 * indizierbare Tabelle expandiert.
 *
 * Die Gruppierung selbst ist KEINE Q9-Erfindung, sondern die reale
 * MC68030-Standard-Exception-Vektortabelle (per Skript gegen dker030s
 * verifiziert, s. o.) -- deshalb hier unveraendert uebernommen, inkl.
 * der beiden reservierten Reset-Vektoren (0/1: initialer SSP/PC), die
 * NICHT Teil der Quelltabelle sind (der echte Kernel startet seinen
 * Fuellzaehler ebenfalls bei 2, nicht bei 0 -- diese zwei Slots werden
 * separat/hardwareseitig behandelt, hier also bewusst nicht angefasst).
 *
 * INHALTLICH aber noch ohne echte Handler: alle Gruppen zeigen bisher
 * auf denselben generischen Halt-Handler (Q9K_ExcDefault) -- es gibt
 * noch keinen IRQ-Dispatcher, keinen Syscall-Dispatcher (TRAP #0,
 * Gruppe unten eigens kommentiert) und keine FPU-/MMU-Fehlerbehandlung.
 * Das ist kein Fake-Zustand, sondern ehrlich das, was heute existiert --
 * TODOs an den betroffenen Gruppen markiert.
 *
 * Konsistenzpruefung repliziert den echten Kernel: die Summe aller
 * counts MUSS exakt 254 ergeben (256 minus die 2 reservierten Reset-
 * Vektoren), sonst Panic beim Original (`bsr 0x7f6`). Hier (noch kein
 * eigener Panic-Mechanismus vorhanden) stattdessen ein Fehlercode
 * (Rueckgabe 1) -- Aufrufer entscheidet, was damit passiert.
 *
 * WICHTIG (per Host-Test gefunden, 2026-08-18, gleiche Lehre wie schon
 * bei q9kernel_arena.c): der Slot-Abstand ist sizeof(Q9K_ExcHandler),
 * NICHT hartkodiert "4" -- auf dem echten 32-Bit-Zielsystem ist ein
 * 68K-Funktionszeiger 4 Byte breit (deckt sich zufaellig mit der
 * Eintragsgroesse der echten CPU-Hardware-Vektortabelle), auf einem
 * 64-Bit-Host beim Testen aber 8 Byte. Ein erster Entwurf mit
 * hartkodiertem "*4" liess benachbarte Eintraege beim Schreiben
 * ueberlappen (echter, durch den Host-Test aufgedeckter Bug -- auf dem
 * 32-Bit-Ziel waere er unbemerkt geblieben, weil sizeof(Handler)==4 dort
 * zufaellig stimmt, genau wie beim Arena-Freiblock-Header).
 *
 * WICHTIGER ECHTER LINKER-FUND (per l68 gefunden, 2026-08-18): eine
 * erste Fassung hatte eine `static const`-Quelltabelle mit Funktions-
 * ZEIGERN als Feldinhalt (Struct {count, target}). l68 verweigerte den
 * Link mit "initialized data (or jumptable) allowed only on program or
 * trap handler modules" -- ein Systm-Modul (unser Kernel) darf laut
 * echtem Linker KEINE vorab gebackenen, relozierten Adressen als Daten
 * enthalten (muss ROM-faehig/positionsunabhaengig bleiben). Genau DAS
 * ist der eigentliche Grund, warum der echte Kernel in seiner
 * Quelltabelle bei 0x3802 vorzeichenbehaftete PC-relative OFFSETS statt
 * absoluter Adressen speichert (s. docs/REVERSE_ENGINEERING.md) -- keine
 * Design-Vorliebe, sondern dieselbe harte Modultyp-Beschraenkung.
 * Deshalb hier umgebaut: die Quelltabelle enthaelt NUR noch Integer-
 * Zaehler (echte, unproblematische const-Daten), keine Zeiger. Die
 * einzige heute existierende Zieladresse (Q9K_ExcDefault) wird zur
 * LAUFZEIT per Code (nicht als Daten-Initialisierer) ermittelt und in
 * jeden Slot geschrieben. Sobald es echte, unterschiedliche Handler pro
 * Gruppe gibt (IRQ-/Syscall-Dispatcher), MUESSEN deren Adressen genauso
 * zur Laufzeit in ein beschreibbares (nicht const-initialisiertes)
 * Array eingetragen werden -- eine `static const`-Zeigertabelle wie im
 * ersten Entwurf wird beim naechsten Link wieder denselben Fehler
 * auslösen.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef void (*Q9K_ExcHandler)(void);

#ifndef Q9_D_EXCJMP
#define Q9_D_EXCJMP 0x068UL   /* s. q9sysglob.h -- per #ifndef ueberschreibbar fuer Host-Tests */
#endif

#define Q9K_EXCTABLE_TOTAL    256  /* 256 CPU-Vektoren, s. Q9_T_END=0x400 (256*4) in q9sysglob.h */
#define Q9K_EXCTABLE_RESERVED 2    /* Vektor 0/1 (ColdSP/ColdPC) -- hier nicht Teil der Quelltabelle */

static void Q9K_ExcDefault(void)
{
    for (;;) { } /* keine Rueckkehr moeglich/vorgesehen -- gleiche Philosophie wie Q9K_HaltLoop */
}

/* q9kernel_entry.a -- der echte TRAP-#0-Syscall-Dispatcher (Abschnitt
 * "F$Link/F$UnLink", 2026-08-21). KEIN normaler C-aufrufbarer Handler
 * (RTE statt RTS, eigene Registerkonvention, direkt als Vektor-Ziel
 * gedacht) -- hier nur die ADRESSE gebraucht, fuer den Tabelleneintrag. */
extern void Q9K_TrapDispatch(void);

/* q9kernel_entry.a -- setzt VBR (movec, privilegiert, in C nicht
 * ausdrueckbar). ECHTER BUG GEFUNDEN (2026-08-21, per Diagnose-Ausgabe):
 * ohne diesen Aufruf bleibt VBR auf $00000000 stehen (Boot-ROM-Default),
 * waehrend unsere Tabelle bei *Q9_D_EXCJMP liegt -- TRAP #0 vektorisierte
 * dadurch durch die genullte Adresse-0-Region statt durch die von uns
 * befuellte Tabelle. S. ausfuehrlichen Kommentar bei Q9K_SetVBR. */
extern void Q9K_SetVBR(Q9_u32 tableBase);

/* q9kernel_entry.a -- echter Interrupt-Handler fuer Autovector 30 (Level
 * 6, Board-Timer, 10ms, s. Q9-Flux/src/kernel/m68krt.c q9_m68krt_attach_quicc
 * -- s. ausfuehrlichen Bugfix-Kommentar unten, docs/BOARD.md ist hier
 * noch veraltet), Abschnitt "Scheduler" (2026-08-21). Wie Q9K_TrapDispatch
 * KEIN normaler C-aufrufbarer Handler (RTE statt RTS, eigene
 * Registerkonvention). */
extern void Q9K_TimerIRQHandler(void);

/* NUR Integer-Zaehler, KEINE Zeiger -- s. Kopfkommentar (echter l68-
 * Linker-Fund: Zeiger als const-Daten sind in Systm-Modulen verboten).
 * Reihenfolge/Werte = die reale MC68030-Standard-Vektorgruppierung
 * (s. Kopfkommentar), Summe MUSS 254 ergeben. */
static const Q9_u16 Q9K_ExcGroupCounts[] = {
    2,    /* 2-3:    Bus/Address Error */
    5,    /* 4-8:    Illegal Instr/ZeroDiv/CHK/TRAPV/Priv.Violation */
    1,    /* 9:      Trace */
    5,    /* 10-14:  Line-A/F-Emulator, reserviert, Koprozessor, Format-Fehler */
    1,    /* 15:     Uninitialized Interrupt */
    8,    /* 16-23:  reserviert */
    1,    /* 24:     Spurious Interrupt */
    7,    /* 25-31:  Autovektor-Interrupts Level 1-7 -- TODO: eigener IRQ-Dispatcher */
    1,    /* 32:     TRAP #0 -- TODO: eigener Syscall-Dispatcher, jetzt Platzhalter */
    15,   /* 33-47:  TRAP #1-15 */
    7,    /* 48-54:  FPU-Exceptions */
    2,    /* 55-56:  FP Unimpl. Datatype, MMU Config Error */
    7,    /* 57-63:  MMU-Fehler + reserviert */
    192   /* 64-255: User-Defined Vectors */
};

#define Q9K_EXCSOURCE_COUNT (sizeof(Q9K_ExcGroupCounts) / sizeof(Q9K_ExcGroupCounts[0]))

/* Expandiert Q9K_ExcGroupCounts in die volle Tabelle bei *Q9_D_EXCJMP
 * (der Kernel-Global enthaelt den ZEIGER auf den Boot-ROM-Block, nicht
 * den Block selbst -- s. q9kernel_entry.a, "move.l a5,Q9_D_ExcJmp(a6)").
 * defaultHandler wird per Code (nicht als Daten-Initialisierer)
 * ermittelt -- s. Kopfkommentar zum Linker-Fund. Rueckgabe 0 = Erfolg,
 * 1 = Quelltabelle inkonsistent (Summe != 254). */
Q9_u32 Q9K_BuildExcTable(void)
{
    Q9_u32 tableBase = *(volatile Q9_u32 *)Q9_D_EXCJMP;
    Q9_u32 vectorIndex = Q9K_EXCTABLE_RESERVED;
    Q9K_ExcHandler defaultHandler = Q9K_ExcDefault;
    unsigned int i;

    for (i = 0; i < Q9K_EXCSOURCE_COUNT; i++) {
        Q9_u16 n;

        for (n = 0; n < Q9K_ExcGroupCounts[i]; n++) {
            Q9K_ExcHandler *slot;

            if (vectorIndex >= Q9K_EXCTABLE_TOTAL)
                return 1; /* Quelltabelle liefert mehr als die 254 erwarteten Eintraege */

            slot = (Q9K_ExcHandler *)(tableBase + vectorIndex * sizeof(Q9K_ExcHandler));
            *slot = defaultHandler;
            vectorIndex++;
        }
    }

    if (vectorIndex != Q9K_EXCTABLE_TOTAL)
        return 1;

    /* NACHTRAG 2026-08-21 (Abschnitt "F$Link/F$UnLink"): Vektor 32
     * (TRAP #0, s. Quelltabelle oben) jetzt gezielt auf den echten
     * Syscall-Dispatcher umbiegen, statt auf dem generischen Halt-
     * Handler zu bleiben -- einzelner, gezielter Nachtrag statt die
     * bestehende, konsistenzgeprueften Schleife umzubauen. */
    {
        Q9K_ExcHandler *trapSlot = (Q9K_ExcHandler *)(tableBase + 32 * sizeof(Q9K_ExcHandler));
        *trapSlot = Q9K_TrapDispatch;
    }

    /* NACHTRAG 2026-08-21 (Abschnitt "Scheduler"): Vektor 30 (Autovector
     * Level 6, Board-Timer, s. Quelltabelle oben, Gruppe "25-31") jetzt
     * gezielt auf den echten Timer-Interrupt-Handler umbiegen -- gleiches
     * Vorgehen wie eben bei Vektor 32.
     *
     * ECHTER BUG GEFUNDEN + GEFIXT (2026-08-21, per Boot-Test: kein
     * Prozesswechsel fand je statt, obwohl F$Link/F$UnLink/Q9K_SchedRun
     * alle sauber liefen): urspruenglich hier Vektor 27 (Level 3)
     * eingetragen, gestuetzt auf Q9-Flux/docs/BOARD.md ("Der 100Hz-Timer
     * ... laeuft ueber Autovector 27 (= Level 3)") UND den Session-
     * Notizen von vor diesem Abschnitt. TATSAECHLICH lag der Board-Timer
     * zu diesem Zeitpunkt aber schon auf LEVEL 6 (s. Q9-Flux/src/kernel/
     * m68krt.c, q9_m68krt_attach_quicc: "d.irq_level = 6;" fuer
     * "timer_irq") -- eine NOCH TAGESAKTUELLE, in BOARD.md noch nicht
     * nachgezogene "Hardware-Vereinheitlichung" (selber Tag) hatte den
     * Timer von Level 3 auf Level 6 verschoben (Grund laut dortigem
     * Kommentar: Level 6 muss in der IRQ-Registrierungsreihenfolge NACH
     * QUICC/Level 5 stehen). Mit Vektor 27 verdrahtet liess der
     * tatsaechlich auf Vektor 30 (24+6) einlaufende Interrupt den
     * generischen Halt-Handler (Q9K_ExcDefault, reines "for(;;){}")
     * laufen -- KEIN Absturz, KEINE Ausgabe, einfach eine fuer immer
     * unterbrochene Rueckkehr aus der Interrupt-Behandlung, die dem
     * unterbrochenen Prozess (der selbst weiterhin druckte, bis ER
     * getroffen wurde) von aussen wie ein irgendwann eintretendes
     * Verstummen erschien -- per verkuerztem Testverzoegerungswert
     * (viele schnelle "AAAA..."-Wiederholungen statt einzelner
     * Zeichen) zweifelsfrei von einem echten Haenger IN Q9K_TestProcA
     * selbst unterschieden. Fix: Vektor 30 statt 27. Alle anderen
     * fuenf Eintraege dieser Gruppe (Level 1/2/3/4/7) bleiben bewusst
     * auf dem generischen Halt-Handler -- es gibt fuer sie noch keine
     * Hardware/keinen Anwendungsfall in diesem Kernel (TODO, s.
     * Quelltabellen-Kommentar). */
    {
        Q9K_ExcHandler *timerSlot = (Q9K_ExcHandler *)(tableBase + 30 * sizeof(Q9K_ExcHandler));
        *timerSlot = Q9K_TimerIRQHandler;
    }

    /* ECHTER BUG GEFUNDEN + GEFIXT (2026-08-21): ohne dies bleibt VBR auf
     * dem Boot-ROM-Default $00000000 stehen -- die CPU vektorisiert dann
     * durch die genullte Adresse-0-Region statt durch tableBase. S.
     * Kommentar bei Q9K_SetVBR (q9kernel_entry.a). */
    Q9K_SetVBR(tableBase);

    return 0;
}
