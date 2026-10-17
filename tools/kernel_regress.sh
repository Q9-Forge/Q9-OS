#!/bin/bash
# kernel_regress.sh -- ein Kommando fuer den Q9-Kernel-Regressionslauf (Arbeitsplan A1).
#
#   tools/kernel_regress.sh [--variant atomic|development|both] [--quick]
#
# Stufen (jede meldet ok/FAIL, am Ende Summe und Exit-Code):
#   1. Hosttests     tools/hosttests.sh               (alle test_q9kernel_*.c, ~5 s)
#   2. Login-Test    tools/login_shell_test.sh <V>    (Boot -> sysgo -> startup -> tsmon -> login
#                                                      -> mshell -> echo/pd/dir/date, ~2 min je Variante)
#   3. Langlauf      tools/leak_test.sh 64 8          (192 Forks, Speicherbilanz; ~2 min)
#   4. Parallellauf  tools/leak_test.sh 24 2 mit 5 gleichzeitigen Prozessen (~1,5 min)
#   5. Offener Pfad tools/leak_test.sh 12 4 mit openleak (A16: Prozessende schliesst IOMan-Pfade; ~1 min)
#   6. Interrupt-Injektion (A15): Langlauf, Parallellauf und Login-Test mit Q9_TIMER_PERIOD_MS=1 (zehnfacher Takt; ~5 min)
#   7. F$TLink-Fehler tools/leak_test.sh 12 4 mit tlinkerr (B2: Rueckbau bei M$Init-Fehler und Speichermangel; ~1 min)
#   8. Debugger-Aufrufe tools/leak_test.sh 8 2 mit dbgtest (C2: F$DFork/F$DExec/F$DExit, Ressourcenbilanz; ~1 min)
#   9. Semaphore/Alarme tools/leak_test.sh mit semtest (8/2) und alarmtest (3/1) (C3: F$Sema P/V, F$Alarm-Matrix; ~3 min)
#  10. Bestandstests tools/startup_shell_test.sh, tools/mgrpath_probe_test.sh
#                    (nur Standardvariante, laufen mit dem Kernel, den build.sh ohne Schalter baut)
# --quick: nur Stufe 1 (ohne Emulator, ohne Wine-Bau).
#
# Umgebung (sonst kanonische Pfade auf dem Mac):
#   FLUX    Q9-Flux-68k-Verzeichnis (build/macos/q9.exe, ROM)
#   REFBOOT Referenz-Bootdatei mit tsmon/login/mshell (OS9Boot.noprot.test)
# Wine-Bauten duerfen nicht parallel laufen -- die Stufen laufen deshalb nacheinander.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
VARIANT=both; QUICK=0
while [ $# -gt 0 ]; do
    case "$1" in
        --variant) VARIANT=${2:-}; shift 2;;
        --quick)   QUICK=1; shift;;
        -h|--help) sed -n '2,20p' "$0"; exit 0;;
        *) echo "unbekannte Option: $1" >&2; exit 2;;
    esac
done
case "$VARIANT" in atomic|development|both) ;; *) echo "--variant: atomic|development|both" >&2; exit 2;; esac

CANON=/Volumes/SSD1TB/projects/Q9-Forge/Q9-SDK/SRC/SYSTEM
[ -n "${FLUX:-}" ]    || export FLUX=$CANON/Q9-Flux/Q9-Flux-68k
[ -n "${REFBOOT:-}" ] || export REFBOOT=$CANON/Q9-Flux/.hide/OS9Boot.noprot.test

pass=0; fail=0; failed=()
stufe() {   # stufe <Name> <Befehl...>
    local name=$1; shift
    local t0=$SECONDS
    echo "=== $name"
    if "$@"; then pass=$((pass+1)); echo "--- ok   $name ($((SECONDS-t0)) s)"
    else          fail=$((fail+1)); failed+=("$name"); echo "--- FAIL $name ($((SECONDS-t0)) s)"; fi
}

stufe "Hosttests (alle test_q9kernel_*)" bash "$HERE/hosttests.sh"

if [ "$QUICK" = 0 ]; then
    [ -x "$FLUX/build/macos/q9.exe" ] || { echo "FEHLER: Emulator fehlt: $FLUX/build/macos/q9.exe (FLUX setzen oder --quick)" >&2; exit 2; }
    # Eigenes Exemplar des Emulators unter anderem Namen: auf diesem Rechner arbeiten mehrere Sitzungen mit q9.exe, und ein
    # "pkill q9.exe" einer davon beendete die Emulatoren dieses Laufs mitten im Test ("Emulator endet ..." an wechselnden Stellen,
    # kein Absturzbericht). Ein Exemplar mit anderem Namen wird davon nicht getroffen und ist gegen einen Neubau im Flux-Baum
    # waehrend des Laufs geschuetzt.
    EMU_DIR=$(mktemp -d "${TMPDIR:-/tmp}/q9kreg.XXXXXX")
    cp "$FLUX/build/macos/q9.exe" "$EMU_DIR/kernelregress-emu" && export Q9K_EMU="$EMU_DIR/kernelregress-emu"
    trap 'rm -rf "$EMU_DIR"' EXIT
    [ -f "$REFBOOT" ] || { echo "FEHLER: REFBOOT fehlt: $REFBOOT" >&2; exit 2; }
    vars=$VARIANT; [ "$VARIANT" = both ] && vars="development atomic"
    for v in $vars; do
        stufe "Login-Test [$v]" bash "$HERE/login_shell_test.sh" "$v"
    done
    # Langlauf (A8/A12): 192 Forks mit Speicherbilanz; stuerzte vor der RaceRing-Korrektur in 60-80 % der Laeufe ab.
    stufe "Langlauf 192 Forks, Speicherbilanz" bash "$HERE/leak_test.sh" 64 8
    # Parallellauf (A6): fuenf gleichzeitige Prozesse je Zyklus (zwei dir mit Dateizugriff ueber IOMan/RBF/Treiber,
    # date, pd, echo; "&" und "wait") bei laufendem Timer. 24 Zyklen = 120 Forks; die lange Fassung (96 Zyklen) steht
    # im Arbeitsplan.
    stufe "Parallellauf 5 Prozesse, Speicherbilanz" env Q9K_LEAK_PER=2 \
        'Q9K_LEAK_CMDS=dir /dd/CMDS&dir /dd/SYS&date&pd&echo a&wait;' bash "$HERE/leak_test.sh" 24 2
    # Offener Pfad (A16): openleak oeffnet /dd/SYS/motd und endet ohne I$Close. Bleibt der Pfad in IOMan stehen,
    # scheitert schon der zweite Open (Fehler $D6, Meldung "open failed"); leak_test.sh wertet die Meldungen aus.
    stufe "Prozessende mit offenem IOMan-Pfad" env Q9K_LEAK_PER=2 'Q9K_LEAK_CMDS=openleak;openleak;' \
        bash "$HERE/leak_test.sh" 12 4
    # Interrupt-Injektion (A15): Q9_TIMER_PERIOD_MS=1 (Q9-Flux, timer_irq.c) verkuerzt den Board-Takt von 10 auf 1 ms. Die
    # IRQ-Fenster aus A7 (Queue- und Kontextuebergabe, Rueckweg des Trap-Pfads) werden dadurch zehnmal so oft getroffen.
    # Die Q9-Uhr laeuft entsprechend schneller. Setzt einen Emulator mit dieser Variablen voraus; ein aelterer q9.exe
    # ignoriert sie, die Stufen laufen dann nur mit 10 ms (kein Fehlalarm, aber auch kein Beleg).
    stufe "Interrupt-Injektion: Langlauf, Takt 1 ms" env Q9_TIMER_PERIOD_MS=1 bash "$HERE/leak_test.sh" 64 8
    stufe "Interrupt-Injektion: Parallellauf, Takt 1 ms" env Q9_TIMER_PERIOD_MS=1 Q9K_LEAK_PER=2 \
        'Q9K_LEAK_CMDS=dir /dd/CMDS&dir /dd/SYS&date&pd&echo a&wait;' bash "$HERE/leak_test.sh" 24 2
    stufe "Interrupt-Injektion: Login-Test, Takt 1 ms" env Q9_TIMER_PERIOD_MS=1 bash "$HERE/login_shell_test.sh" development
    # F$TLink-Fehlerpfade (B2): tlinkerr laesst M$Init eines Trap-Moduls scheitern (zweimal) und einen F$TLink am
    # Speichermangel scheitern. Zwischen den Marken duerfen Link-Zaehler, Modulverzeichnis, Trap-Slots und Freispeicher nicht
    # wachsen; leak_test.sh prueft ausserdem die gemeldeten Fehlercodes (A5, A5, CF).
    stufe "F\$TLink-Fehlerpfade, Ressourcenbilanz" env Q9K_LEAK_PER=2 'Q9K_LEAK_CMDS=tlinkerr;tlinkerr;' \
        bash "$HERE/leak_test.sh" 12 4
    # Debugger-Aufrufe (C2): dbgtest startet dbgchild per F$DFork, steuert es per F$DExec (Einzelschritt, Haltepunkte) und raeumt
    # mit F$DExit auf; die Werte prueft der Login-Test (Block 10). Hier die Ressourcenbilanz ueber viele Laeufe: nach F$DExit
    # muessen Link-Zaehler, Modulverzeichnis und Freispeicher unveraendert sein (frueher blieb der Modul-Link des Kindes stehen).
    # Voraussetzung: ein Emulator mit eingeschaltetem Trace-Bit (Q9-Flux Makefile, M68K_EMULATE_TRACE).
    stufe "Debugger-Aufrufe, Ressourcenbilanz" env Q9K_LEAK_PER=2 'Q9K_LEAK_CMDS=dbgtest;' bash "$HERE/leak_test.sh" 8 2
    # Semaphore und Alarme (C3): Werte und Reihenfolge pruefen die Login-Test-Bloecke 11/12; hier die Ressourcenbilanz ueber mehrere
    # Laeufe (blockierte Kinder, Wartequeue, Alarmtabelle duerfen nichts zurueckhalten). alarmtest dauert ~30 s je Lauf (Messung in
    # Sekunden der Systemuhr), deshalb nur drei Laeufe.
    stufe "Semaphore (F\$Sema P/V), Ressourcenbilanz" env Q9K_LEAK_PER=2 'Q9K_LEAK_CMDS=semtest;' bash "$HERE/leak_test.sh" 8 2
    stufe "Alarme (F\$Alarm-Matrix), Ressourcenbilanz" env Q9K_LEAK_PER=1 Q9_LEAK_TIMEOUT=400 'Q9K_LEAK_CMDS=alarmtest;' bash "$HERE/leak_test.sh" 3 1
    for t in startup_shell_test.sh mgrpath_probe_test.sh; do
        if [ -f "$HERE/$t" ]; then stufe "$t" bash "$HERE/$t"
        else echo "(uebersprungen, fehlt: $t)"; fi
    done
fi

echo
echo "KERNEL-REGRESSION: $pass Stufen ok, $fail fehlgeschlagen"
if [ "$fail" -gt 0 ]; then printf '  FAIL: %s\n' "${failed[@]}"; exit 1; fi
echo "ALLES GRUEN"
