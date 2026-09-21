/*
 * q9kernel_exctable.c -- Q9-OS eigener Kernel: Exception-/Trap-Dispatch-
 *                        Tabelle aus kompakter Quelltabelle expandieren.
 *
 * Abschnitt 2, Punkt 4 (s. q9kernel_cinit.c). Mechanismus 1:1 wie am
 * echten Kernel verifiziert (intern dokumentiert, Fund:
 * "Trap-/Exception-Tabellen-Initialisierung"): Q9_D_EXCJMP zeigt auf
 * einen vom Boot-ROM bereitgestellten Speicherblock (256 Eintraege x
 * 4 Byte = 1024 Byte, s. Q9_T_END in q9sysglob.h); statt 256 einzelne
 * Adressen im Modul zu speichern, wird eine KOMPAKTE Quelltabelle
 * (Gruppen aus {Anzahl, Ziel}) zur Boot-Zeit in die volle, direkt
 * indizierbare Tabelle expandiert.
 *
 * Die Gruppierung selbst ist KEINE Q9-Erfindung, sondern die reale
 * MC68030-Standard-Exception-Vektortabelle (per Skript gegen den echten Original-Kernel
 * verifiziert, s. o.) -- deshalb hier unveraendert uebernommen, inkl.
 * der beiden reservierten Reset-Vektoren (0/1: initialer SSP/PC), die
 * NICHT Teil der Quelltabelle sind (der echte Kernel startet seinen
 * Fuellzaehler ebenfalls bei 2, nicht bei 0 -- diese zwei Slots werden
 * separat/hardwareseitig behandelt, hier also bewusst nicht angefasst).
 *
 * INHALTLICH zeigen die nicht speziell verdrahteten Gruppen weiterhin auf
 * den generischen Halt-Handler (Q9K_ExcDefault). TRAP #0, TRAP #1-15 und
 * die registrierten IRQ-/Autovektor-Pfade werden nach dem Aufbau gezielt auf
 * ihre echten Dispatcher umgebogen; FPU-/MMU-Fehlerbehandlung bleibt offen.
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
 * absoluter Adressen speichert (intern dokumentiert) -- keine
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
extern void Q9K_ExcTrap(void);   /* q9kernel_entry.a, s. Kommentar bei defaultHandler */
extern void Q9K_TrapDispatch(void);
extern void Q9K_TCallDispatch(void);   /* q9kernel_entry.a, TRAP #1-15 (F$TLink-Trap-Bibliotheken) */

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
extern void Q9K_IRQDispatch(void);      /* q9kernel_entry.a -- Zustellung fuer F$IRQ-Eintraege */
extern void Q9K_TraceHandler(void);      /* q9kernel_entry.a -- F$DExec single-step */

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
/* ---------------------------------------------------------------------
 * F$IRQ (Callcode $2A, "Enter IRQ Polling Table")
 *
 * Konvention aus dem realen Treiber sc68681 abgelesen (Modul-Offset
 * $00ec ff., nicht geraten):
 *     move.b $34(a1),d0     d0.b = Vektornummer  (aus dem Descriptor)
 *     move.b $36(a1),d1     d1.b = Prioritaet
 *     lea    $572(pc),a0    a0   = Interrupt-Service-Routine
 *     trap   #0 / $002a
 * Zusaetzlich gilt die OS-9-Konvention a2 = statischer Speicher des
 * Treibers, a3 = Geraete-Portadresse; beide werden mitgefuehrt, damit
 * der spaetere IRQ-Dispatch sie der ISR wieder vorlegen kann.
 * a0 = 0 bedeutet "Eintrag entfernen" (Treiber-Terminate).
 *
 * Die Registrierung und die Zustellung laufen inzwischen getrennt: F$IRQ
 * trägt den Eintrag ein, und Q9K_IRQDispatch fragt die passenden ISRn beim
 * Geräte- oder Autovektor-Interrupt ab. Offen bleibt die vollständige
 * Abdeckung aller Hardware-Level und Treiberkonventionen.
 * --------------------------------------------------------------------- */
/* ECHTER BUG GEFUNDEN + GEFIXT (2026-09-11, Fortsetzung 51): bei 16
 * Eintraegen a 20 Byte reicht die Tabelle von $1500 bis $1640 -- das
 * ueberlappt GLEICH VIERFACH mit spaeter angelegten Scratch-Zellen
 * anderer Syscalls, die "$16xx" faelschlich fuer frei hielten:
 * Q9K_SEND_SCRATCH_ERROR/SUCCESS ($1610/$1614, q9kernel_procsleep.c,
 * Slot 13), Q9K_RETPD_SCRATCH_* ($161C-$1628, q9kernel_iopath.c, Slot
 * 14), UND Q9K_VMODUL_SCRATCH_HDR/SIZE/ENTRY/ERROR/SUCCESS ($162C-
 * $163C, q9kernel_moddir.c, Slot 15) -- BYTE-GENAU deckungsgleich mit
 * den Feldern von Slot 15 (Vektor/Prioritaet/ISR/Static/Port). Live
 * gefunden per Instruktionsspur-Freeze (`Q9_FREEZE_PC`) + gezieltem
 * `Q9_WATCH_ADDR` auf die gesamte Tabelle: nach einem `F$VModul`-Aufruf
 * (z.B. beim Laden von "echo") hinterlaesst dessen Rueckgabewert
 * ($1650, ein reiner Datenpuffer) in `Q9K_VMODUL_SCRATCH_ENTRY`
 * ($1634 = Slot 15s ISR-Feld) einen Wert, der Slot 15 wie einen
 * gueltigen, registrierten Eintrag aussehen laesst (solange dessen
 * "Vektor"-Feld, alias `Q9K_VMODUL_SCRATCH_HDR`, zufaellig nicht 0
 * ist) -- die IRQ-Dispatch-Schleife (`q9kernel_entry.a`) ruft dann
 * `jsr (a1)` mit `a1=$1650` auf und stuerzt ab (Vektor 4, `PC=$7002`
 * nach Sprung durch Nullwoerter, s. docs/OWN_KERNEL_STATUS.md
 * Fortsetzung 51). Fix: auf 12 Slots verkleinert -- endet bei $15F0,
 * VOR der ersten real belegten Nachbarzelle ($1600,
 * Q9K_PRSNAM_SCRATCH_ERROR/OK) -- keine Ueberlappung mehr moeglich.
 * Kollisionspruefung bei neuen Scratch-Feldern MUSS ab sofort den
 * GESAMTEN belegten Bereich einer Tabelle einschliessen, nicht nur
 * deren Anfangsadresse. */
#ifndef Q9K_IRQTAB_BASE
#define Q9K_IRQTAB_BASE   0x1500UL   /* 12 Eintraege a 20 Byte -- endet bei $15F0, s. Kommentar oben */
#endif
#define Q9K_IRQTAB_SLOTS  12UL
#define Q9K_IRQTAB_ENTSZ  20UL
#define Q9K_IRQ_OFF_VECTOR  0UL      /* 0 = Slot frei */
#define Q9K_IRQ_OFF_PRIO    4UL
#define Q9K_IRQ_OFF_ISR     8UL
#define Q9K_IRQ_OFF_STATIC  12UL
#define Q9K_IRQ_OFF_PORT    16UL

/* Die Tabelle ist ein 32-Bit-OS-9-ABI-Objekt.  Auf 64-Bit-Hosttests ist
 * unsigned long breiter; deshalb hier explizit vier Byte lesen/schreiben,
 * sonst wuerden benachbarte Felder ueberlappt. */
static Q9_u32 Q9K_IRQGet(Q9_u32 addr) { return (Q9_u32)*(volatile unsigned int *)addr; }
static void   Q9K_IRQSet(Q9_u32 addr, Q9_u32 v) { *(volatile unsigned int *)addr = (unsigned int)v; }

/* Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
int Q9K_ProcIRQ(Q9_u32 vector, Q9_u32 prio, Q9_u32 isr,
                Q9_u32 statics, Q9_u32 port, Q9_u16 *outError)
{
    Q9_u32 i;
    Q9_u32 slot;
    Q9_u32 freeSlot = 0;

    *outError = 0;

    if (isr == 0) {
        /* Entfernen: Eintrag mit passendem Vektor UND statischem Speicher
         * suchen (mehrere Geraete koennen sich denselben Vektor teilen). */
        for (i = 0; i < Q9K_IRQTAB_SLOTS; i++) {
            slot = Q9K_IRQTAB_BASE + i * Q9K_IRQTAB_ENTSZ;
            if (Q9K_IRQGet(slot + Q9K_IRQ_OFF_VECTOR) == vector &&
                Q9K_IRQGet(slot + Q9K_IRQ_OFF_STATIC) == statics) {
                Q9K_IRQSet(slot + Q9K_IRQ_OFF_VECTOR, 0);
                Q9K_IRQSet(slot + Q9K_IRQ_OFF_ISR, 0);
                Q9K_IRQSet(slot + Q9K_IRQ_OFF_PRIO, 0);
                Q9K_IRQSet(slot + Q9K_IRQ_OFF_STATIC, 0);
                Q9K_IRQSet(slot + Q9K_IRQ_OFF_PORT, 0);

                /* Do not leave a stale dispatcher installed after the last
                 * device on a vector has gone away.  This matters for
                 * spurious interrupts: the normal exception handler should
                 * regain control instead of entering an empty IRQ scan. */
                {
                    Q9_u32 j;
                    int sameVector = 0;
                    Q9_u32 tableBase = *(volatile Q9_u32 *)Q9_D_EXCJMP;
                    for (j = 0; j < Q9K_IRQTAB_SLOTS; j++) {
                        Q9_u32 other = Q9K_IRQTAB_BASE + j * Q9K_IRQTAB_ENTSZ;
                        if (Q9K_IRQGet(other + Q9K_IRQ_OFF_VECTOR) == vector) {
                            sameVector = 1;
                            break;
                        }
                    }
                    if (!sameVector && tableBase != 0 && vector < Q9K_EXCTABLE_TOTAL)
                        *(Q9K_ExcHandler *)(tableBase + vector * sizeof(Q9K_ExcHandler)) = Q9K_ExcTrap;

                    /* Autovectors are shared by all polled devices.  Restore
                     * their defaults only when the complete polling table is
                     * empty; vector 30 remains the board timer. */
                    if (!sameVector && vector >= 25 && vector <= 31) {
                        int anyEntry = 0;
                        for (j = 0; j < Q9K_IRQTAB_SLOTS; j++) {
                            Q9_u32 other = Q9K_IRQTAB_BASE + j * Q9K_IRQTAB_ENTSZ;
                            if (Q9K_IRQGet(other + Q9K_IRQ_OFF_VECTOR) != 0) {
                                anyEntry = 1;
                                break;
                            }
                        }
                        if (!anyEntry && tableBase != 0) {
                            Q9_u32 av;
                            for (av = 25; av <= 31; av++) {
                                if (av != 30)
                                    *(Q9K_ExcHandler *)(tableBase + av * sizeof(Q9K_ExcHandler)) = Q9K_ExcTrap;
                            }
                        }
                    }
                }
                return 1;
            }
        }
        *outError = 0x00E1U;            /* E_PARAM, kein solcher Eintrag */
        return 0;
    }

    if (vector < Q9K_EXCTABLE_RESERVED || vector >= Q9K_EXCTABLE_TOTAL || vector == 30) {
        *outError = 0x00E1U;            /* E_PARAM, reservierter/ungueltiger Vektor */
        return 0;
    }

    for (i = 0; i < Q9K_IRQTAB_SLOTS; i++) {
        slot = Q9K_IRQTAB_BASE + i * Q9K_IRQTAB_ENTSZ;
        if (Q9K_IRQGet(slot + Q9K_IRQ_OFF_VECTOR) == 0) {
            if (freeSlot == 0)
                freeSlot = slot;
        } else if (Q9K_IRQGet(slot + Q9K_IRQ_OFF_VECTOR) == vector &&
                   Q9K_IRQGet(slot + Q9K_IRQ_OFF_STATIC) == statics) {
            /* Derselbe Treiber registriert denselben Vektor erneut --
             * als Aktualisierung behandeln, nicht als Fehler. */
            freeSlot = slot;
            break;
        }
    }

    if (freeSlot == 0) {
        *outError = 0x00CAU;            /* E_POLL, Polling Table Full */
        return 0;
    }

    Q9K_IRQSet(freeSlot + Q9K_IRQ_OFF_PRIO, prio);
    Q9K_IRQSet(freeSlot + Q9K_IRQ_OFF_ISR, isr);
    Q9K_IRQSet(freeSlot + Q9K_IRQ_OFF_STATIC, statics);
    Q9K_IRQSet(freeSlot + Q9K_IRQ_OFF_PORT, port);
    Q9K_IRQSet(freeSlot + Q9K_IRQ_OFF_VECTOR, vector);   /* zuletzt: macht den Slot gueltig */

    /* NEU 2026-09-03: Zustellung einschalten. Bisher wurde die Tabelle nur
     * gefuehrt -- ein eintreffender Geraete-Interrupt lief in den
     * generischen Halt-Handler, der Treiber bekam nie sein TxRDY/RxRDY.
     * Der Vektorslot wird erst HIER umgebogen (nicht pauschal beim Boot):
     * so bleiben alle Vektoren, fuer die sich kein Treiber registriert hat,
     * weiterhin auf dem Halt-Handler und melden einen echten Fehler, statt
     * still ins Leere zu laufen.
     * Ein einziger Dispatcher bedient alle Vektoren -- er liest seine
     * Vektornummer aus dem Exception-Frame (s. Q9K_IRQDispatch). */
    if (vector < Q9K_EXCTABLE_TOTAL) {
        Q9_u32 tableBase = *(volatile Q9_u32 *)Q9_D_EXCJMP;

        if (tableBase != 0) {
            Q9K_ExcHandler *slotPtr =
                (Q9K_ExcHandler *)(tableBase + vector * sizeof(Q9K_ExcHandler));
            *slotPtr = Q9K_IRQDispatch;

            /* Zusaetzlich die Autovektoren auf den Dispatcher legen (Vektor
             * 25-31 = Level 1-7). Grund: der Interrupt eines Geraets kommt
             * nicht zwingend unter seinem per IVR gesetzten Vektor an -- beim
             * DUART liefert das Interrupt-Acknowledge real einen Autovektor
             * (27 = Level 3). Der Dispatcher erkennt diesen Fall und fragt
             * dann die gesamte Tabelle ab, statt nach Vektornummer zu filtern.
             * Vektor 30 bleibt ausgespart: dort haengt der Board-Timer mit
             * eigenem Handler (s. Q9K_TimerIRQHandler weiter unten). */
            {
                Q9_u32 av;

                for (av = 25; av <= 31; av++) {
                    Q9K_ExcHandler *avSlot;

                    if (av == 30) {
                        continue;
                    }
                    avSlot = (Q9K_ExcHandler *)(tableBase + av * sizeof(Q9K_ExcHandler));
                    *avSlot = Q9K_IRQDispatch;
                }
            }
        }
    }
    return 1;
}

/* Scratch-Bruecke fuer den Assembler-Handler, gleiches Muster wie ueberall. */
#ifndef Q9K_IRQ_SCRATCH_VECTOR
#define Q9K_IRQ_SCRATCH_VECTOR  0x13C8UL
#define Q9K_IRQ_SCRATCH_PRIO    0x13CCUL
#define Q9K_IRQ_SCRATCH_ISR     0x13D0UL
#define Q9K_IRQ_SCRATCH_STATIC  0x13D4UL
#define Q9K_IRQ_SCRATCH_PORT    0x13D8UL
#define Q9K_IRQ_SCRATCH_ERROR   0x13DCUL
#define Q9K_IRQ_SCRATCH_SUCCESS 0x13E0UL
#endif

void Q9K_SysIRQImpl(void)
{
    Q9_u16 err = 0;
    int ok = Q9K_ProcIRQ(Q9K_IRQGet(Q9K_IRQ_SCRATCH_VECTOR),
                         Q9K_IRQGet(Q9K_IRQ_SCRATCH_PRIO),
                         Q9K_IRQGet(Q9K_IRQ_SCRATCH_ISR),
                         Q9K_IRQGet(Q9K_IRQ_SCRATCH_STATIC),
                         Q9K_IRQGet(Q9K_IRQ_SCRATCH_PORT), &err);
    Q9K_IRQSet(Q9K_IRQ_SCRATCH_ERROR, (Q9_u32)err);
    Q9K_IRQSet(Q9K_IRQ_SCRATCH_SUCCESS, ok ? 1UL : 0UL);
}

Q9_u32 Q9K_BuildExcTable(void)
{
    Q9_u32 tableBase = *(volatile Q9_u32 *)Q9_D_EXCJMP;
    Q9_u32 vectorIndex = Q9K_EXCTABLE_RESERVED;
    /* 2026-09-02: Q9K_ExcTrap (Assembler, q9kernel_entry.a) statt der
     * C-Funktion Q9K_ExcDefault -- letztere bekommt vom Compiler einen
     * Stack-Check-Prolog, der bei einer Exception im Kontext eines
     * externen Moduls (A6=0) selbst zuschlaegt und die eigentliche
     * Exception verdeckt. Q9K_ExcTrap haelt Vektor, PC und SR fest. */
    Q9K_ExcHandler defaultHandler = (Q9K_ExcHandler)Q9K_ExcTrap;
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

    /* Vector 9 is the 68000 trace exception used by F$DExec. */
    {
        Q9K_ExcHandler *traceSlot =
            (Q9K_ExcHandler *)(tableBase + 9 * sizeof(Q9K_ExcHandler));
        *traceSlot = Q9K_TraceHandler;
    }

    /* NACHTRAG 2026-09-11 (Abschnitt "F$TLink/User Trap Handlers"):
     * Vektoren 33-47 (TRAP #1-15, "tcall N,Funktion") auf den echten
     * Trap-Bibliotheks-Dispatcher umbiegen -- gleiches Vorgehen wie
     * eben bei Vektor 32, nur als Schleife ueber alle 15 Vektoren statt
     * eines einzelnen Eintrags. */
    {
        unsigned int v;
        for (v = 33; v <= 47; v++) {
            Q9K_ExcHandler *tcallSlot = (Q9K_ExcHandler *)(tableBase + v * sizeof(Q9K_ExcHandler));
            *tcallSlot = Q9K_TCallDispatch;
        }
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
