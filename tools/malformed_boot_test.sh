#!/bin/sh
# malformed_boot_test.sh -- negative emulator regression for
# Q9K_ModDirPopulateFromBootList's boot-module validation
# (Q9K_CheckSyncWord/Q9K_ValidModuleHeader, q9kernel_moddir.c).
#
# Warum dieses Skript existiert: Q9-KERNEL/STATUS.md, Abschnitt
# "Boot and integration hardening", nennt "malformed-image negative
# emulator tests" als offenen Punkt. Die Scan-Logik selbst (Sync-Wort
# + 24-Word-XOR-Pruefsumme, byteweises Vorruecken bei Fehlschlag statt
# der ungeprueften Modulgroesse zu vertrauen) sah beim Lesen bereits
# solide aus -- dieses Skript bestaetigt das EMPIRISCH statt es nur
# anzunehmen (s. docs/OWN_KERNEL_STATUS.md, Fortsetzung 99).
#
# Baut zwei absichtlich korrupte Testabbilder (Sync-Wort-Korruption bei
# "rbf", Pruefsummen-Korruption bei "cfide" -- beide sitzen bewusst in
# der von mkbootfile.sh unangetastet kopierten Modulkette, nicht in den
# explizit geparsten ersten drei Eintraegen kernel/init/forkchild) und
# prueft per Debug-Dump, dass GENAU das korrupte Modul aus der
# Moduldirectory-Kette fehlt, waehrend alle anderen Module (insbesondere
# die NACH dem korrupten Eintrag liegenden) weiterhin korrekt gefunden
# werden -- UND dass keine Exception auftritt.
#
# Aufruf: tools/malformed_boot_test.sh
# Voraussetzung: frischer Kernel-Build (build/q9kernel, build/forkchild),
# die Vendor-Module liegen unter /tmp/vendor_*.mod (s. docs/OWN_KERNEL_STATUS.md,
# Fortsetzung 94, fuer die Extraktion aus Q9-Flux/OS9Boot.noprot.test),
# und eine bereits aus den regulaeren Stresstests bekannte Referenz-
# Bootfile-Grundlage (dieses Skript baut sie bei Bedarf selbst neu).

set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
BUILD="$HERE/Q9-KERNEL/68k/src/kernel/build"
OS9=${OS9:-/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9}
FLUX=${FLUX:-/Volumes/SSD1TB/projects/Q9-Forge/Q9-Flux/Q9-Flux-68k}
VENDOR=${VENDOR:-/tmp}
WORK=$(mktemp -d -t malformed_boot)
trap 'rm -rf "$WORK"' EXIT

if [ ! -f "$BUILD/q9kernel" ] || [ ! -f "$BUILD/forkchild" ]; then
    echo "Kernel-Build fehlt -- erst $HERE/68k/src/kernel/build.sh laufen lassen" >&2
    exit 1
fi
for m in init ioman rbf cfide dd c0 scf sc68681 term; do
    [ -f "$VENDOR/vendor_$m.mod" ] || { echo "Vendor-Modul fehlt: $VENDOR/vendor_$m.mod" >&2; exit 1; }
done

fail=0

run_case() {
    name="$1"; corrupt_idx="$2"; corrupt_mode="$3"; expect_absent="$4"
    ref="$WORK/$name.boot"
    img="$WORK/$name.hda"
    dump="$FLUX/local_images/q9dbg_dump.txt"

    python3 - "$ref" "$corrupt_idx" "$corrupt_mode" "$BUILD" "$VENDOR" <<'PY'
import sys
ref, idx, mode, build, vendor = sys.argv[1:6]
idx = int(idx)
paths = [build + '/q9kernel', vendor + '/vendor_init.mod', build + '/forkchild',
          vendor + '/vendor_ioman.mod', vendor + '/vendor_rbf.mod', vendor + '/vendor_cfide.mod',
          vendor + '/vendor_dd.mod', vendor + '/vendor_c0.mod', vendor + '/vendor_scf.mod',
          vendor + '/vendor_sc68681.mod', vendor + '/vendor_term.mod']
parts = [bytearray(open(p, 'rb').read()) for p in paths]
assert parts[idx][:2] == b'\x4a\xfc'
if mode == 'sync':
    parts[idx][0] = 0x00
elif mode == 'checksum':
    parts[idx][0x10] ^= 0xFF
else:
    raise SystemExit('unknown mode ' + mode)
open(ref, 'wb').write(b''.join(parts))
PY

    "$OS9" format -q -k -nQ9TEST -bs512 -l32768 -c32 "$img" >/dev/null
    Q9K_BUILD_DIR="$BUILD" "$HERE/tools/mkbootfile.sh" "$ref" "$img" >/dev/null

    rm -f "$dump"
    ( cd "$FLUX" && Q9_TEST_TIMEOUT="${Q9_TEST_TIMEOUT:-15}" \
        expect -f "$HERE/tools/run_kernel_test.exp" "$FLUX" "$img" 'XXXNOMATCH' "$WORK/$name.log" \
        >"$WORK/$name.out" 2>&1 ) || true

    if [ ! -f "$dump" ]; then
        echo "FAIL  $name: kein Debug-Dump geschrieben"
        fail=1
        return
    fi
    if grep -q 'Vektor=[1-9]' "$dump" 2>/dev/null; then
        echo "FAIL  $name: Exception ausgeloest (erwartet: sauber uebersprungen)"
        grep 'Vektor=' "$dump" | head -1
        fail=1
        return
    fi
    if grep -q " Name=\"$expect_absent\"" "$dump"; then
        echo "FAIL  $name: korruptes Modul '$expect_absent' trotzdem in der Moduldirectory gefunden"
        fail=1
        return
    fi
    # Mindestens ein spaeter in der Kette liegendes Modul muss weiterhin da sein --
    # beweist, dass der Scan NACH der Korruption sauber weiterlief statt
    # aufzugeben oder falsch vorzuspringen.
    if ! grep -q 'Name="term"' "$dump"; then
        echo "FAIL  $name: nachfolgendes Modul 'term' fehlt -- Scan nach der Korruption gestoert"
        fail=1
        return
    fi
    echo "ok    $name: '$expect_absent' sauber uebersprungen, Scan lief korrekt bis 'term' weiter, keine Exception"
}

run_case syncword 4 sync rbf
# Kurze Pause: zwei q9.exe-Instanzen direkt nacheinander (Netzwerk-Listener,
# CF-Image-Handle) brauchen etwas Luft, sonst kann der zweite Lauf einen Dump
# vom ersten uebernehmen statt seinen eigenen frischen zu schreiben --
# live beobachtet, als beide Faelle ohne Pause direkt hintereinander liefen.
sleep 2
run_case checksum 5 checksum cfide

if [ "$fail" -eq 0 ]; then
    echo "ALLE TESTS OK (malformed_boot_test.sh)"
    exit 0
else
    echo "FEHLGESCHLAGEN (malformed_boot_test.sh)"
    exit 1
fi
