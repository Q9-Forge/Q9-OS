#!/bin/bash
#═════════════════════════════════════════════════════════════════════════════════════
# mk_dhfrom.sh -- ROM Claude_cb030_DHF_BIOS.BIN bauen (Claude-ROM + Booter "boot from DHF")
#═════════════════════════════════════════════════════════════════════════════════════
# Der ROM besteht aus romboot (CBOOT-Booter mit rombug, Q9-Port ROM_CBOOT, gelinkt auf
# $FE000000) und dahinter den ROM-Modulen. Neu ist nur der Booter: ROM_CBOOT/io_dhf.c +
# Eintrag in syscon.c. Die Module (kernel ... unzip) werden 1:1 aus dem bisherigen ROM
# uebernommen (ab dem Ende des alten romboot), der Rest bis 512 KB mit $FF aufgefuellt.
#
# Gebaut wird in einer WEGWERFKOPIE des Q9-Ports im MWOS-Baum (os9make loest Pfade relativ
# zu $(MWOS) auf) -- die Build-Artefakte des echten Ports (CMDS/BOOTOBJS) bleiben unberuehrt.
# Toolchain: xcc/r68/l68 des SDK unter Wine (s. Q9-Port README.md).
#
# Aufruf: boot/mk_dhfrom.sh [alter-rom [neuer-rom]]
#   Standard: Q9-Images/rom_images/Claude_cb030_BIOS.BIN -> .../Claude_cb030_DHF_BIOS.BIN
#═════════╤══════╤═══════════════════════════════════════════════════════════╤══════
# Datum   │ Ver. │ Aenderung                                                 │ Wer
#─────────┼──────┼───────────────────────────────────────────────────────────┼──────
# 26-09-26│ 1.00 │ Erster Wurf                                               │ Cld
#═════════╧══════╧═══════════════════════════════════════════════════════════╧══════
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
FORGE=$(cd "$HERE/../../.." && pwd)
MWOS=${MWOS:-/Volumes/SSD1TB/projects/MWOS}
PORT=$MWOS/OS9/68030/PORTS/Q9
ALT=${1:-$FORGE/Q9-Images/rom_images/Claude_cb030_BIOS.BIN}
NEU=${2:-$FORGE/Q9-Images/rom_images/Claude_cb030_DHF_BIOS.BIN}
WINE=${WINE:-"$HOME/.local/wine-stable/Wine Stable.app/Contents/Resources/wine/bin/wine"}
# Laenge des romboot im alten ROM = Beginn der Module (Claude-ROM: $158AC)
ALT_BOOT=${ALT_BOOT:-0x158AC}

die() { echo "mk_dhfrom: $*" >&2; exit 2; }
for f in "$ALT" "$PORT/ROM_CBOOT/io_dhf.c" "$WINE"; do [ -e "$f" ] || die "fehlt: $f"; done
grep -q bootdhf "$PORT/ROM_CBOOT/syscon.c" || die "syscon.c ohne bootdhf"

TMPNAME=_dhfrom_$$
TMP=$MWOS/OS9/68030/PORTS/$TMPNAME
trap 'rm -rf "$TMP"' EXIT
rsync -a --exclude Hardware --exclude CB030.zip --exclude .git --exclude SPF --exclude build \
    "$PORT/" "$TMP/" || die "Kopie des Ports"

echo "── romboot bauen (Wine, alle vier ROM_CBOOT-Varianten; gebraucht wird ROMBUG)"
LOG=$(mktemp "${TMPDIR:-/tmp}/dhfrom.XXXXXX")
WINEPREFIX="${WINEPREFIX:-$HOME/.wine}" WINEDEBUG=-all "$WINE" cmd /c \
    "set MWOS=M:&& set PATH=M:\\DOS\\BIN;%PATH%&& cd /d M:\\OS9\\68030\\PORTS\\$TMPNAME && os9make.exe -e MWOS=M: GOAL=build ROM_CBOOT" \
    >"$LOG" 2>&1
BOOT=$TMP/CMDS/BOOTOBJS/ROMBUG/romboot
grep -q "io_dhf.c" "$LOG" && [ -s "$BOOT" ] && [ "$BOOT" -nt "$TMP/ROM_CBOOT/io_dhf.c" ] \
    || { tail -30 "$LOG"; die "Bauen fehlgeschlagen (Log $LOG)"; }
grep -i "error" "$LOG" | grep -v "error_code" && die "Fehler im Bau (Log $LOG)"
rm -f "$LOG"

echo "── ROM zusammensetzen"
python3 - "$BOOT" "$ALT" "$NEU" "$ALT_BOOT" <<'EOF' || die "Zusammensetzen"
import struct, sys
boot, alt, neu, altboot = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4], 0)
b = open(boot, "rb").read()
r = open(alt, "rb").read()
if r[altboot:altboot+2] != b"\x4a\xfc":
    sys.exit(f"im alten ROM beginnt bei {altboot:#x} kein Modul")
i, names = altboot, []
while i + 16 <= len(r) and r[i:i+2] == b"\x4a\xfc":
    size = struct.unpack_from(">I", r, i + 4)[0]
    nm = struct.unpack_from(">I", r, i + 12)[0]
    names.append(r[i+nm:i+nm+32].split(b"\0")[0].decode("latin1"))
    i += size
mods = r[altboot:i]
img = b + mods
if len(img) > len(r):
    sys.exit(f"zu gross: {len(img)} > {len(r)} Byte -- Module entfernen oder ROM vergroessern")
if len(b) % 2:
    sys.exit("romboot ungerade lang")
img += b"\xff" * (len(r) - len(img))
open(neu, "wb").write(img)
print(f"romboot {len(b)} Byte (alt {altboot}), {len(names)} Module {len(mods)} Byte, "
      f"frei {len(r) - len(b) - len(mods)} Byte -> {neu}")
EOF
