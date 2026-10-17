#!/bin/bash
# hosttests.sh -- alle Kernel-Hosttests uebersetzen und ausfuehren.
#
# Jede Datei Q9-KERNEL/68k/src/kernel/test_q9kernel_*.c ist eine eigene
# Suite (docs/KERNEL_WERKZEUGE_de.md, Abschnitt "Hosttests"). Uebersetzt wird
# in einem Wegwerfverzeichnis; der Quellbaum bleibt unberuehrt.
#
# Die Schalter -DQ9K_KERNEL_DEVELOPMENT und -DQ9K_ALLOC_STANDARD sind
# Pflicht (die Debug-Suiten brauchen zusaetzlich -DQ9K_DEBUG, s. unten): q9kernel_config.h verlangt die Variantenwahl per #error. Ein
# Uebersetzungsfehler zaehlt hier als FAIL, nie als "uebersprungen".
#
# Aufruf: tools/hosttests.sh [-v] [muster]
#   -v       zeigt die Ausgabe jeder Suite
#   muster   nur Suiten, deren Name das Muster enthaelt (z. B. sched)
# Ausgabe: je Suite "ok"/"FAIL", am Ende eine Summe. Exit 0 = alles gruen.
# Umgebung (optional): CC (Standard: gcc), Q9K_HOSTTEST_KEEP=1 behaelt das
#   Arbeitsverzeichnis.

set -u
HERE=$(cd "$(dirname "$0")/.." && pwd)
KDIR="$HERE/Q9-KERNEL/68k/src/kernel"
COMMON="$HERE/Q9-KERNEL/common/src"
CC=${CC:-gcc}
VERBOSE=0
if [ "${1:-}" = "-v" ]; then VERBOSE=1; shift; fi
PATTERN=${1:-}

WORK=$(mktemp -d "${TMPDIR:-/tmp}/q9_hosttests.XXXXXX")
if [ "${Q9K_HOSTTEST_KEEP:-0}" = 1 ]; then
    echo "KEEP_WORK=$WORK"
else
    trap 'rm -rf "$WORK"' EXIT
fi

command -v "$CC" >/dev/null 2>&1 || { echo "FEHLER: Uebersetzer '$CC' nicht gefunden" >&2; exit 2; }

# Quellbaum kopieren: Tests und Kernelquellen liegen im selben Verzeichnis,
# einige Tests erwarten ausserdem q9sysglob.h aus common/src.
cp "$KDIR"/*.c "$KDIR"/*.h "$WORK"/ 2>/dev/null
cp "$COMMON"/*.h "$WORK"/ 2>/dev/null
cp "$COMMON"/*.c "$WORK"/ 2>/dev/null

pass=0; fail=0; failed=""
for src in "$KDIR"/test_q9kernel_*.c; do
    [ -f "$src" ] || continue
    name=$(basename "$src" .c)
    case "$name" in *"$PATTERN"*) ;; *) continue ;; esac
    exe="$WORK/$name"
    log="$WORK/$name.log"
    # Die Debug-Suiten pruefen Code, den build.sh nur im Developer-Kernel
    # mit -dQ9K_DEBUG uebersetzt; ohne den Schalter fehlen ihnen die
    # Funktionen (Uebersetzungsfehler "undeclared function").
    EXTRA=""
    case "$name" in *dbg*|*trace*|*ringbuf*) EXTRA="-DQ9K_DEBUG" ;; esac
    if ! ( cd "$WORK" && $CC -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD $EXTRA \
            -o "$exe" "$name.c" ) > "$log" 2>&1; then
        echo "FAIL  $name  (Uebersetzung)"
        [ "$VERBOSE" = 1 ] && tail -15 "$log" | sed 's/^/        /'
        fail=$((fail+1)); failed="$failed $name"; continue
    fi
    if ( cd "$WORK" && "$exe" ) > "$log" 2>&1; then
        echo "ok    $name"
        [ "$VERBOSE" = 1 ] && sed 's/^/        /' "$log"
        pass=$((pass+1))
    else
        rc=$?
        echo "FAIL  $name  (Exit $rc)"
        tail -12 "$log" | sed 's/^/        /'
        fail=$((fail+1)); failed="$failed $name"
    fi
done

total=$((pass+fail))
echo "----"
if [ "$total" -eq 0 ]; then echo "FAIL  keine Suite gefunden (Muster '$PATTERN')"; exit 2; fi
if [ "$fail" -eq 0 ]; then
    echo "Hosttests: $pass/$total gruen"
    exit 0
fi
echo "Hosttests: $pass/$total gruen, $fail ROT:$failed"
exit 1
