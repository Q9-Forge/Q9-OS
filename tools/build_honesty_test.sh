#!/bin/bash
# build_honesty_test.sh -- Nachweis fuer Arbeitsplan A4 ("build.sh ehrlich machen"):
# ein absichtlich eingebauter Fehler muss den Kernelbau ABBRECHEN, auch wenn im
# Ausgabeverzeichnis noch Ergebnisse eines frueheren, erfolgreichen Laufs
# liegen (frueher taeuschte genau das Erfolg vor).
#
# Ablauf (alles in einer Wegwerfkopie des Quellbaums):
#   0  guter Bau                     -> muss gelingen ("BUILD OK")
#   A  Syntaxfehler in einer C-Datei -> Bau muss scheitern
#   B  unbekannter Befehl im Assembler (q9kernel_entry.a) -> muss scheitern
#   C  undefiniertes Symbol (Linkfehler) -> muss scheitern
#   Z  wieder gut                     -> muss wieder gelingen
# Nach jedem Fehlerlauf darf es kein Kernelmodul mehr im Ausgabeverzeichnis
# geben (sonst koennte ein Folgeschritt mit altem Stand weiterarbeiten).
#
# Aufruf: tools/build_honesty_test.sh [Szenarien]      (Standard: "0 A B C Z")
#   z. B. tools/build_honesty_test.sh 0 A     nur guter Bau + C-Fehler
# Umgebung: Q9K_HONESTY_BUILD=<pfad>  anderes build.sh verwenden (z. B. das alte
#   aus der Git-Historie, um zu zeigen, dass es den Fehler verschluckt haette).
#   Q9K_KEEP_TEST_WORK=1 behaelt das Arbeitsverzeichnis.
# Ein Bau dauert unter Wine rund 1-2 Minuten.

set -u
HERE=$(cd "$(dirname "$0")/.." && pwd)
KDIR="$HERE/Q9-KERNEL/68k/src/kernel"
SCEN=${*:-0 A B C Z}
WORK="${TMPDIR:-/tmp}/q9_honesty_$$"
rm -rf "$WORK"; mkdir -p "$WORK/src/Q9-KERNEL/68k/src" "$WORK/src/Q9-KERNEL/common"
if [ "${Q9K_KEEP_TEST_WORK:-0}" = 1 ]; then echo "KEEP_WORK=$WORK"; else trap 'rm -rf "$WORK"' EXIT; fi
cp -R "$KDIR" "$WORK/src/Q9-KERNEL/68k/src/"
rm -rf "$WORK/src/Q9-KERNEL/68k/src/kernel/build"
cp -R "$HERE/Q9-KERNEL/common/src" "$WORK/src/Q9-KERNEL/common/"
SRC="$WORK/src/Q9-KERNEL/68k/src/kernel"
if [ -n "${Q9K_HONESTY_BUILD:-}" ]; then cp "$Q9K_HONESTY_BUILD" "$SRC/build.sh"; fi
chmod +x "$SRC/build.sh"
OUT="$WORK/out"
cp "$SRC/q9kernel_clock.c" "$WORK/clock.orig"
cp "$SRC/q9kernel_entry.a" "$WORK/entry.orig"

pass=0; fail=0
verdict() { # <ok-Bedingung 0/1> <Text>
    if [ "$1" -eq 0 ]; then echo "ok    $2"; pass=$((pass+1)); else echo "FAIL  $2"; fail=$((fail+1)); fi
}
restore() { cp "$WORK/clock.orig" "$SRC/q9kernel_clock.c"; cp "$WORK/entry.orig" "$SRC/q9kernel_entry.a"; }
run_build() { # -> Exit-Status in $rc, Ausgabe in $WORK/b.log
    ( cd "$SRC" && Q9K_KERNEL_VARIANT=development ./build.sh "$OUT" ) > "$WORK/b.log" 2>&1
    rc=$?
}
expect_good() {
    run_build
    if [ $rc -eq 0 ] && grep -q 'BUILD OK' "$WORK/b.log" && [ -s "$OUT/q9kernel" ]; then verdict 0 "$1: guter Bau gelingt ($(wc -c < "$OUT/q9kernel" | tr -d ' ') Byte Kernel)"
    else verdict 1 "$1: guter Bau gelingt nicht (Exit $rc)"; tail -8 "$WORK/b.log" | sed 's/^/        /'; fi
}
expect_bad() { # <Name> <Hinweistext, der in der Ausgabe stehen soll (Regex)>
    run_build
    local stale=0
    [ -e "$OUT/q9kernel" ] && stale=1
    if [ $rc -ne 0 ]; then verdict 0 "$1: Bau bricht ab (Exit $rc)"
    else verdict 1 "$1: Bau meldete ERFOLG trotz Fehler (Exit 0)"; fi
    if grep -qE "$2" "$WORK/b.log"; then verdict 0 "$1: Fehler wird benannt ($2)"
    else verdict 1 "$1: Ausgabe nennt den Fehler nicht ($2)"; grep -aE 'FEHLER|rror' "$WORK/b.log" | head -3 | sed 's/^/        /'; fi
    if [ $stale -eq 0 ]; then verdict 0 "$1: kein altes Kernelmodul bleibt liegen"
    else verdict 1 "$1: altes Kernelmodul q9kernel liegt noch im Ausgabeverzeichnis"; fi
}

for s in $SCEN; do
    case "$s" in
        0) expect_good "0" ;;
        A) restore; printf '\nthis is not valid C ;;; @@\n' >> "$SRC/q9kernel_clock.c"
           expect_bad "A (C-Fehler)" 'q9kernel_clock|mwos-build' ;;
        B) restore; printf '\n        bogusop d0,d1\n' >> "$SRC/q9kernel_entry.a"
           expect_bad "B (Assemblerfehler)" 'r68|Errors|q9kernel_entry' ;;
        C) restore; printf '\nextern int Q9K_DoesNotExist(void);\nint Q9K_BreakLink(void) { return Q9K_DoesNotExist(); }\n' >> "$SRC/q9kernel_clock.c"
           expect_bad "C (Linkfehler)" 'l68|[Uu]ndefined|Q9K_DoesNotExist' ;;
        Z) restore; expect_good "Z" ;;
        *) echo "unbekanntes Szenario: $s" >&2 ;;
    esac
done

echo "----"
if [ "$fail" -eq 0 ]; then echo "ALLE TESTS OK (build_honesty_test.sh, $pass Pruefungen)"; exit 0; fi
echo "$fail von $((pass+fail)) Pruefungen ROT"; exit 1
