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
 * Offene Design-Frage fuer Schritt 6a (Init-Modul-Suche), noch NICHT
 * beantwortet: der reale Kernel bekommt seine Speicherregion-Liste zum
 * Durchsuchen aus Register D6 (Herkunft in dieser Runde nicht verfolgt,
 * s. Thema 01). Unser eigener Kernel hat keine aequivalente Quelle dafuer
 * -- muss vor der Implementierung geklaert werden, nicht hier geraten.
 */

#include "../q9sysglob.h"

typedef unsigned long  Q9_u32;

/* Schreibt einen 32-Bit-Wert an eine absolute Adresse (=Kernel-Global-
 * Offset, da Kernel-Globals-Basis bei diesem Kernel $000000 ist) */
static void Q9K_PutU32(Q9_u32 addr, Q9_u32 value)
{
    *(volatile Q9_u32 *)addr = value;
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

    /* TODO (Abschnitt 2, Punkt 3): Speichergroesse aus Q9_D_TOTRAM lesen,
     * echten freien Speicherblock im Arena-Kontrollblock registrieren --
     * die Ringliste oben ist nur der leere Ausgangszustand, nicht das
     * eigentliche Aufsetzen der Speicherverwaltung. */

    /* TODO (Abschnitt 2, Punkt 4): Exception-/Trap-Dispatch-Tabelle aus
     * kompakter Quelltabelle in die volle, direkt indizierbare Tabelle
     * expandieren (Q9_D_EXCJMP zeigt auf den vom Boot-ROM bereitgestellten
     * Speicherblock dafuer, s. q9kernel_entry.a). */

    /* TODO (Abschnitt 2, Punkt 6a): Init-Modul per Namenssuche ("init",
     * gross-/kleinschreibungsunabhaengig) finden -- Speicherregion-Liste
     * dafuer noch nicht entschieden, s. Kopfkommentar. Danach dessen
     * Konfigurationsfelder (M$SysConf etc.) in die Kernel-Globals
     * uebernehmen. */

    /* TODO (Abschnitt 2, Punkt 5/6): Prozess-/Pfad-Deskriptor-Tabellen mit
     * Freiliste einrichten. */

    /* TODO (Abschnitt 2, Punkt 7): ersten Ausfuehrungskontext konstruieren
     * und in den Scheduler springen -- existiert noch nicht. */

    return; /* -> Q9K_HaltLoop in q9kernel_entry.a */
}
