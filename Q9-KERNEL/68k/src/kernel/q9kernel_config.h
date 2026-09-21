/*
 * q9kernel_config.h -- Q9-OS eigener Kernel: Build-Zeit-Variantenauswahl.
 *
 * Vorgabe, 2026-08-18: "Plan schon mal mit, denke frueher oder spaeter
 * wird es kommen" -- Grundlage fuer spaetere Atomic/Development- und
 * Allocator-Varianten, analog zur echten Microware-Namenskonvention
 * (intern dokumentiert: "[a|d]ker<CPU-Suffix>[s|b]").
 *
 * Kernel-Variante und Ziel-CPU sind unabhaengige Achsen. Die CPU wird vom
 * build.sh per Q9K_CPU_TYPE gesetzt; fuer Hosttests gilt 68000 als sicherer
 * Default. MMU/FPU werden nur als Q9K_HAS_MMU/Q9K_HAS_FPU definiert, wenn
 * das CPU-Profil oder ein explizites Build-Override sie freischaltet.
 * Zwei unabhaengige Kernel-Achsen, BEIDE muessen beim Bauen explizit per -D
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
 * Alle vier Kombinationen (Atomic/Development x Standard/Buddy), die CPU-
 * Profile 68000/68030 sowie der Fehlerfall ohne Flags sind real gegen die
 * echte xcc-Pipeline getestet
 * (2026-08-18, ueber q9kernel_modcheck.c als Traeger) -- alle vier
 * Kombinationen exit status = 0 durch, der Fehlerfall bricht mit genau
 * der erwarteten #error-Meldung ab ("catastrophic error: #error
 * directive"). Aufruf: -d<NAME> in CFLAGS wird von xcc zu -D<NAME> fuer
 * cpfe durchgereicht (bestaetigt in der Werkzeugkette-Ausgabe).
 */

#ifndef Q9KERNEL_CONFIG_H
#define Q9KERNEL_CONFIG_H

/* Standalone host tests do not run build.sh; keep them on the conservative
 * instruction subset unless a test explicitly selects another CPU. */
#if !defined(Q9K_CPU_68000) && !defined(Q9K_CPU_68010) && \
    !defined(Q9K_CPU_68020) && !defined(Q9K_CPU_68030) && \
    !defined(Q9K_CPU_68040) && !defined(Q9K_CPU_68060) && \
    !defined(Q9K_CPU_CPU32)
#define Q9K_CPU_68000 1
#endif

#if ((defined(Q9K_CPU_68000) ? 1 : 0) + \
     (defined(Q9K_CPU_68010) ? 1 : 0) + \
     (defined(Q9K_CPU_68020) ? 1 : 0) + \
     (defined(Q9K_CPU_68030) ? 1 : 0) + \
     (defined(Q9K_CPU_68040) ? 1 : 0) + \
     (defined(Q9K_CPU_68060) ? 1 : 0) + \
     (defined(Q9K_CPU_CPU32) ? 1 : 0)) != 1
#error "Genau ein Q9K_CPU_* Ziel muss gewaehlt werden"
#endif

#if defined(Q9K_HAS_MMU) && defined(Q9K_CPU_68000)
#error "Q9K_HAS_MMU ist auf einem 68000-Ziel nicht zulaessig"
#endif
#if defined(Q9K_HAS_MMU) && defined(Q9K_CPU_68010)
#error "Q9K_HAS_MMU ist auf einem 68010-Ziel nicht zulaessig"
#endif
#if defined(Q9K_HAS_FPU) && (defined(Q9K_CPU_68000) || defined(Q9K_CPU_68010))
#error "Q9K_HAS_FPU braucht ein CPU-Profil mit FPU-Unterstuetzung oder einen externen FPU-Build"
#endif

#if !defined(Q9K_MMU_FLAT) && !defined(Q9K_MMU_HARDWARE)
#define Q9K_MMU_FLAT 1
#endif
#if defined(Q9K_MMU_FLAT) && defined(Q9K_MMU_HARDWARE)
#error "Q9K_MMU_FLAT und Q9K_MMU_HARDWARE schliessen sich gegenseitig aus"
#endif
#if defined(Q9K_MMU_HARDWARE) && !defined(Q9K_HAS_MMU)
#error "Q9K_MMU_HARDWARE braucht Q9K_HAS_MMU"
#endif

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
