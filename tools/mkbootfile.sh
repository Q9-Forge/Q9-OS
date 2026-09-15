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
#          --disk  ergaenzt die per Q9_DISK_MODULES angegebenen
#                  Diskmodule. Bereits in der Referenz vorhandene Module
#                  werden anhand ihres Modulnamens nicht doppelt angehaengt.
#          Q9_BOOT_MODULES ergaenzt unabhaengige Bootmodule wie Trap-
#                  Bibliotheken (z. B. math), ebenfalls ohne Duplikate.
set -e

WITH_DISK=0
CONFIG=""
while [ "$#" -gt 0 ]; do
    case "$1" in
        --disk) WITH_DISK=1; shift ;;
        --config)
            [ "$#" -ge 2 ] || { echo "--config erwartet eine Datei" >&2; exit 1; }
            CONFIG="$2"; shift 2 ;;
        --) shift; break ;;
        *) break ;;
    esac
done
REF="$1"; IMG="$2"
if [ -z "$REF" ] || [ -z "$IMG" ]; then
    echo "Aufruf: $0 [--disk] <referenz-bootdatei> <ziel-image>" >&2
    exit 1
fi

HERE=$(cd "$(dirname "$0")/.." && pwd)
# NACHTRAG (2026-09-13): Pfad an die Repo-Reorganisation angepasst --
# der Kernel liegt jetzt unter Q9-KERNEL/68k/src/kernel/, nicht mehr
# direkt unter src/kernel/.
# Ein explizites Build-Verzeichnis erlaubt reproduzierbare Wegwerf-Boots,
# ohne das langlebige Standard-Artefakt im Quellbaum zu ueberschreiben.
BUILD=${Q9K_BUILD_DIR:-"$HERE/Q9-KERNEL/68k/src/kernel/build"}
if [ ! -f "$BUILD/q9kernel" ]; then
    echo "Kernel-Build fehlt: $BUILD/q9kernel" >&2
    exit 1
fi
OS9=${OS9:-/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9}
if [ -n "$CONFIG" ]; then
    [ -f "$CONFIG" ] || { echo "Boot-Konfiguration fehlt: $CONFIG" >&2; exit 1; }
    # shellcheck disable=SC1090
    . "$CONFIG"
fi
OUT=$(mktemp -t os9boot)

# Die Referenz-Bootdatei beginnt mit altem Kernel, init und forkchild. Die
# Grenzen werden aus M$Size gelesen; feste Offsets wurden bei jeder
# Kernelvergroesserung zu einer stillen und gefaehrlichen Fehlerquelle.
python3 - "$REF" "$BUILD" "$OUT" "$WITH_DISK" <<'PY'
import sys, os, struct
ref, build, out, withdisk = sys.argv[1:5]
d = open(ref, 'rb').read()

def module_size(offset):
    if offset + 8 > len(d) or d[offset:offset + 2] != b'\x4a\xfc':
        raise SystemExit('Referenz-Bootdatei: ungueltiger Modulanfang bei 0x%x' % offset)
    size = struct.unpack('>I', d[offset + 4:offset + 8])[0]
    if size < 16 or offset + size > len(d):
        raise SystemExit('Referenz-Bootdatei: ungueltige Modulgroesse 0x%x bei 0x%x' % (size, offset))
    return size

kernel_size = module_size(0)
init_size = module_size(kernel_size)
fork_offset = kernel_size + init_size
fork_size = module_size(fork_offset)
tail_start = fork_offset + fork_size
parts = [open(os.path.join(build, 'q9kernel'), 'rb').read(), d[kernel_size:tail_start]]
hello = os.path.join(build, 'hellosvc')
if os.path.exists(hello):
    parts.append(open(hello, 'rb').read())
parts.append(d[tail_start:])
def module_name(offset):
    name_offset = struct.unpack('>I', d[offset + 0x0c:offset + 0x10])[0]
    end = d.find(b'\0', offset + name_offset)
    if end < 0:
        raise SystemExit('Referenz-Bootdatei: nicht terminiertes Modulnamenfeld bei 0x%x' % offset)
    return d[offset + name_offset:end].decode('ascii', 'replace')

def module_name_bytes(module):
    if len(module) < 0x10 or module[:2] != b'\x4a\xfc':
        return None
    name_offset = struct.unpack('>I', module[0x0c:0x10])[0]
    end = module.find(b'\0', name_offset)
    if end < 0:
        return None
    return module[name_offset:end].decode('ascii', 'replace')

if withdisk == '1':
    extra = (os.environ.get('Q9_DISK_MODULES', '') + ' ' +
             os.environ.get('Q9_BOOT_MODULES', '')).split()
    present = set()
    offset = 0
    while offset < len(d) and d[offset:offset + 2] == b'\x4a\xfc':
        size = module_size(offset)
        present.add(module_name(offset).lower())
        offset += size
    for m in extra:
        module = open(m, 'rb').read()
        name = module_name_bytes(module)
        if name is not None:
            name = name.lower()
        if name is None or name not in present:
            parts.append(module)
            if name is not None:
                present.add(name)
open(out, 'wb').write(b''.join(parts))
print('Referenzgrenzen: kernel=0x%x init=0x%x forkchild=0x%x tail=0x%x' %
      (kernel_size, init_size, fork_size, tail_start))
print('Bootdatei:', sum(len(p) for p in parts), 'Byte')
PY

"$OS9" gen -b="$OUT" "$IMG" | head -1
rm -f "$OUT"
