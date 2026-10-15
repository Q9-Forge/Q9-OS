#!/bin/bash
# kernel_regress.sh -- ein Kommando fuer den Q9-Kernel-Regressionslauf (Arbeitsplan A1).
#
#   tools/kernel_regress.sh [--variant atomic|development|both] [--quick]
#
# Stufen (jede meldet ok/FAIL, am Ende Summe und Exit-Code):
#   1. Hosttests     tools/hosttests.sh               (alle test_q9kernel_*.c, ~5 s)
#   2. Login-Test    tools/login_shell_test.sh <V>    (Boot -> sysgo -> startup -> tsmon -> login
#                                                      -> mshell -> echo/pd/dir/date, ~2 min je Variante)
#   3. Bestandstests tools/startup_shell_test.sh, tools/mgrpath_probe_test.sh
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
    [ -f "$REFBOOT" ] || { echo "FEHLER: REFBOOT fehlt: $REFBOOT" >&2; exit 2; }
    vars=$VARIANT; [ "$VARIANT" = both ] && vars="development atomic"
    for v in $vars; do
        stufe "Login-Test [$v]" bash "$HERE/login_shell_test.sh" "$v"
    done
    for t in startup_shell_test.sh mgrpath_probe_test.sh; do
        if [ -f "$HERE/$t" ]; then stufe "$t" bash "$HERE/$t"
        else echo "(uebersprungen, fehlt: $t)"; fi
    done
fi

echo
echo "KERNEL-REGRESSION: $pass Stufen ok, $fail fehlgeschlagen"
if [ "$fail" -gt 0 ]; then printf '  FAIL: %s\n' "${failed[@]}"; exit 1; fi
echo "ALLES GRUEN"
