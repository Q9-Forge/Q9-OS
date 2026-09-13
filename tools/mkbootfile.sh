#!/bin/sh
# mkbootfile.sh -- baut eine Bootdatei aus dem eigenen Kernel plus den
# unveraenderten Microware-Modulen und schreibt sie in ein Testimage.
#
# Warum es dieses Skript gibt: das Rezept steckte bisher in
# Wegwerf-Kommandozeilen. Es enthaelt aber mehrere Details, die man nicht
# jedes Mal neu herleiten will -- und zwei davon haben schon Stunden
# gekostet (s. docs/OWN_KERNEL_STATUS.md):
#
#   * Die Microware-Module werden LINEAR aus einer Referenz-Bootdatei
#     uebernommen, nie per "os9 copy" einzeln geholt: die Bootdatei ist im
#     Image fragmentiert, der Bootloader liest sie aber linear.
#   * Zum Zurueckschreiben ist "os9 gen -b=" richtig -- es legt die Datei
#     unfragmentiert an und zieht Start-LSN und Laenge im
#     Identification-Sektor korrekt mit.
#   * Die Modulreihenfolge bleibt wie in der Referenz; nur der Kernel wird
#     ersetzt und eigene Module werden angehaengt. Wer die Reihenfolge
#     aendert, verschiebt alle Ladeadressen -- und damit jede Adresse in
#     Messnotizen (auch das hat schon in die Irre gefuehrt).
#
# Aufruf:  tools/mkbootfile.sh [--disk] <referenz-bootdatei> <ziel-image>
#          --disk  nimmt zusaetzlich RBF, den CompactFlash-Treiber und die
#                  Geraetedeskriptoren auf (fuer F$Load von Platte)
set -e

WITH_DISK=0
if [ "$1" = "--disk" ]; then WITH_DISK=1; shift; fi
REF="$1"; IMG="$2"
if [ -z "$REF" ] || [ -z "$IMG" ]; then
    echo "Aufruf: $0 [--disk] <referenz-bootdatei> <ziel-image>" >&2
    exit 1
fi

HERE=$(cd "$(dirname "$0")/.." && pwd)
# NACHTRAG (2026-09-13): Pfad an die Repo-Reorganisation angepasst --
# der Kernel liegt jetzt unter Q9-KERNEL/68k/src/kernel/, nicht mehr
# direkt unter src/kernel/.
BUILD="$HERE/Q9-KERNEL/68k/src/kernel/build"
OS9=${OS9:-/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9}
OUT=$(mktemp -t os9boot)

# Offsets der Referenz-Bootdatei: davor steht der (zu ersetzende) Kernel,
# dahinter die unveraenderten Microware-Module.
REF_TAIL_START=${REF_TAIL_START:-0x3092}   # hinter dem Kernel
REF_TAIL_SPLIT=${REF_TAIL_SPLIT:-0x328e}   # hinter init/forkchild

python3 - "$REF" "$BUILD" "$OUT" "$WITH_DISK" "$REF_TAIL_START" "$REF_TAIL_SPLIT" <<'PY'
import sys, os
ref, build, out, withdisk, a, b = sys.argv[1:7]
a, b = int(a, 0), int(b, 0)
d = open(ref, 'rb').read()
parts = [open(os.path.join(build, 'q9kernel'), 'rb').read(), d[a:b]]
hello = os.path.join(build, 'hellosvc')
if os.path.exists(hello):
    parts.append(open(hello, 'rb').read())
parts.append(d[b:])
if withdisk == '1':
    extra = os.environ.get('Q9_DISK_MODULES', '')
    for m in extra.split():
        parts.append(open(m, 'rb').read())
open(out, 'wb').write(b''.join(parts))
print('Bootdatei:', sum(len(p) for p in parts), 'Byte')
PY

"$OS9" gen -b="$OUT" "$IMG" | head -1
rm -f "$OUT"
