/*
 * q9kernel_config.h -- Q9-OS eigener Kernel: Build-Zeit-Variantenauswahl.
 *
 * Vorgabe, 2026-08-18: "Plan schon mal mit, denke frueher oder spaeter
 * wird es kommen" -- Grundlage fuer spaetere Atomic/Development- und
 * Allocator-Varianten, analog zur echten Microware-Namenskonvention
 * (intern dokumentiert: "[a|d]ker<CPU-Suffix>[s|b]").
 *
 * Zwei unabhaengige Achsen, BEIDE muessen beim Bauen explizit per -D
 * gesetzt werden -- kein stiller Default, um nicht aus Versehen die
 * falsche Variante zu bauen (z.B. versehentlich Atomic ohne Schutz-
 * mechanismen fuer ein Multi-User-Ziel):
 *
 *   Q9K_KERNEL_ATOMIC       -- schlank: kein Debugging (F$DFork/F$DExec/
 *                              F$DExit), keine Multi-User-Schutz-
 *                              mechanismen, keine MMU-Speicherschutz,
 *                              keine Modul/User-ID-Pruefung.
 *   Q9K_KERNEL_DEVELOPMENT  -- voll: alles davon vorhanden. Aktueller
 *                              Q9-Flux/CB030-Entwicklungsstand zielt auf
 *                              das reale Original-Kernel-Aequivalent, also ist
 *                              das hier der aktuelle Standardfall.
 *
 *   Q9K_ALLOC_STANDARD      -- klassischer 16-Byte-Aufloesungs-Allocator.
 *   Q9K_ALLOC_BUDDY         -- Binary-Buddy (Zweierpotenz-Bloecke),
 *                              deterministischer, speicherineffizienter,
 *                              typisch fuer Atomic/Echtzeit.
 *
 * Development und Atomic unterscheiden sich inzwischen beim Diagnosepfad:
 * Development kompiliert die DUART-Memory-/Modultrace aus, Atomic nicht.
 * Die eigentlichen Speicher- und Schutzvarianten bleiben davon getrennt.
 * Jede Datei, die variantenabhaengigen Code bekommt, bindet diesen Header
 * ein und verwendet Q9K_MEMTRACE_COMPILETIME fuer reine Diagnoseausgaben.
 *
 * Alle vier Kombinationen (Atomic/Development x Standard/Buddy) UND der
 * Fehlerfall ohne Flags real gegen die echte xcc-Pipeline getestet
 * (2026-08-18, ueber q9kernel_modcheck.c als Traeger) -- alle vier
 * Kombinationen exit status = 0 durch, der Fehlerfall bricht mit genau
 * der erwarteten #error-Meldung ab ("catastrophic error: #error
 * directive"). Aufruf: -d<NAME> in CFLAGS wird von xcc zu -D<NAME> fuer
 * cpfe durchgereicht (bestaetigt in der Werkzeugkette-Ausgabe).
 */

#ifndef Q9KERNEL_CONFIG_H
#define Q9KERNEL_CONFIG_H

#if !defined(Q9K_KERNEL_ATOMIC) && !defined(Q9K_KERNEL_DEVELOPMENT)
#error "Kernel-Variante nicht gewaehlt -- -DQ9K_KERNEL_ATOMIC oder -DQ9K_KERNEL_DEVELOPMENT beim Bauen setzen"
#endif

/* Select the first real startup experiment.  Set to 0 to use the
 * established scheduler/diagnostic processes instead. */
#ifndef Q9K_BOOT_STARTUP
#define Q9K_BOOT_STARTUP 1
#endif
#if defined(Q9K_KERNEL_ATOMIC) && defined(Q9K_KERNEL_DEVELOPMENT)
#error "Q9K_KERNEL_ATOMIC und Q9K_KERNEL_DEVELOPMENT schliessen sich gegenseitig aus"
#endif

#if !defined(Q9K_ALLOC_STANDARD) && !defined(Q9K_ALLOC_BUDDY)
#error "Allocator-Variante nicht gewaehlt -- -DQ9K_ALLOC_STANDARD oder -DQ9K_ALLOC_BUDDY beim Bauen setzen"
#endif
#if defined(Q9K_ALLOC_STANDARD) && defined(Q9K_ALLOC_BUDDY)
#error "Q9K_ALLOC_STANDARD und Q9K_ALLOC_BUDDY schliessen sich gegenseitig aus"
#endif

/* Development-only diagnostics are compiled out of Atomic kernels. */
#if defined(Q9K_KERNEL_DEVELOPMENT)
#define Q9K_MEMTRACE_COMPILETIME 1
#else
#define Q9K_MEMTRACE_COMPILETIME 0
#endif

#endif /* Q9KERNEL_CONFIG_H */
