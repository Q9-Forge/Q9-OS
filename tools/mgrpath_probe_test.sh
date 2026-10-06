#!/bin/sh
# mgrpath_probe_test.sh -- emulator regression for the IOMan manager path
# (I$Open/I$Read/I$Close through Q9K_TrapDispatch), see
# docs/OWN_KERNEL_STATUS.md, Fortsetzung 107.
#
# Warum dieses Skript existiert: vier Fehler in diesem Pfad (falsche
# IOMan-Routine registriert, Aufruferregister ueberschrieben, falsches
# Halbwort von R$d0, globaler Callcode nach verschachtelten Traps) fielen
# im Standardboot nie auf, weil dort niemand nach der IOMan-Anmeldung
# I$Read/I$Close per Trap aufruft. Dieses Skript baut den Kernel mit der
# Messsonde Q9K_TestMgrPathProbe=1 (Quellen in ein Wegwerfverzeichnis
# kopiert, der Quellbaum bleibt unveraendert), legt eine startup-Datei mit
# bekanntem Inhalt ins Abbild und prueft die Sondenausgabe:
#
#   I$Open("/dd/startup") liefert eine lokale Pfadnummer N >= 3,
#   P$Path[N] ist als Managerpfad markiert (Bit 15),
#   I$Read(N,16) gelingt mit 16 Byte "ABCDEFGH...",
#   I$Close(N) gelingt und gibt P$Path[N] frei,
#   keine Exception (Vektor=0).
#
# Aufruf: tools/mgrpath_probe_test.sh
# Umgebung (optional): FLUX, OS9, REFBOOT, Q9_TEST_TIMEOUT

set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
FLUX=${FLUX:-$HERE/../Q9-Flux/Q9-Flux-68k}
REFBOOT=${REFBOOT:-$HERE/../Q9-Flux/.hide/OS9Boot.noprot.test}
OS9=${OS9:-/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9}
WORK=$(mktemp -d -t mgrpath_probe)
trap 'rm -rf "$WORK"' EXIT

[ -x "$FLUX/build/macos/q9.exe" ] || { echo "Emulator fehlt: $FLUX/build/macos/q9.exe" >&2; exit 1; }
[ -f "$REFBOOT" ] || { echo "Referenz-Bootdatei fehlt: $REFBOOT" >&2; exit 1; }

# 1. Kernel mit eingeschalteter Sonde bauen (Kopie, Quellbaum unberuehrt).
mkdir -p "$WORK/src/Q9-KERNEL/68k/src" "$WORK/src/Q9-KERNEL/common"
cp -R "$HERE/Q9-KERNEL/68k/src/kernel" "$WORK/src/Q9-KERNEL/68k/src/"
rm -rf "$WORK/src/Q9-KERNEL/68k/src/kernel/build"
cp -R "$HERE/Q9-KERNEL/common/src" "$WORK/src/Q9-KERNEL/common/"
ENTRY="$WORK/src/Q9-KERNEL/68k/src/kernel/q9kernel_entry.a"
sed -i.bak 's/^Q9K_TestMgrPathProbe equ 0/Q9K_TestMgrPathProbe equ 1/' "$ENTRY"
grep -q '^Q9K_TestMgrPathProbe equ 1' "$ENTRY" || { echo "Schalter Q9K_TestMgrPathProbe nicht gefunden" >&2; exit 1; }
( cd "$WORK/src/Q9-KERNEL/68k/src/kernel" && ./build.sh "$WORK/kb" ) > "$WORK/build.log" 2>&1 || true
if grep -qE 'operand size|Errors: 0*[1-9]' "$WORK/build.log" || [ ! -f "$WORK/kb/q9kernel" ]; then
    echo "FAIL  Kernelbau mit Sonde fehlgeschlagen (s. $WORK/build.log)"; exit 1
fi

# 2. Vendor-Module linear aus der Referenz-Bootdatei holen.
python3 - "$REFBOOT" "$WORK" <<'PY'
import sys, struct
data = open(sys.argv[1], 'rb').read(); out = sys.argv[2]; off = 0
while off + 0x30 <= len(data):
    if data[off:off+2] != b'\x4a\xfc':
        off += 2; continue
    size = struct.unpack('>I', data[off+4:off+8])[0]
    noff = struct.unpack('>I', data[off+12:off+16])[0]
    name = data[off+noff:off+noff+32].split(b'\0')[0].decode('latin1')
    open(f"{out}/vendor_{name}.mod", 'wb').write(data[off:off+size]); off += size
PY

# 3. Abbild: Bootkette zuerst auf das frisch formatierte Abbild, dann startup.
V="$WORK"
cat "$WORK/kb/q9kernel" "$V/vendor_init.mod" "$WORK/kb/forkchild" "$V/vendor_ioman.mod" \
    "$V/vendor_rbf.mod" "$V/vendor_cfide.mod" "$V/vendor_dd.mod" "$V/vendor_c0.mod" \
    "$V/vendor_scf.mod" "$V/vendor_sc68681.mod" "$V/vendor_term.mod" > "$WORK/ref.boot"
"$OS9" format -q -k -nQ9TEST -bs512 -l32768 -c32 "$WORK/img.hda" >/dev/null
Q9K_BUILD_DIR="$WORK/kb" "$HERE/tools/mkbootfile.sh" "$WORK/ref.boot" "$WORK/img.hda" >/dev/null
printf 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\r' > "$WORK/startup"
"$OS9" copy "$WORK/startup" "$WORK/img.hda,startup" >/dev/null

# 4. Laufen lassen, bis der Testablauf hinter der Sonde erreicht ist.
dump="$FLUX/local_images/q9dbg_dump.txt"
rm -f "$dump"
( cd "$FLUX" && Q9_TEST_TIMEOUT="${Q9_TEST_TIMEOUT:-40}" \
    expect -f "$HERE/tools/run_kernel_test.exp" "$FLUX" "$WORK/img.hda" 'Hallo von Q9-OS!' "$WORK/console.log" \
    > "$WORK/expect.out" 2>&1 ) || true

[ -f "$dump" ] || { echo "FAIL  kein Debug-Dump geschrieben"; exit 1; }
if ! grep -q 'Vektor=0 (Fmt' "$dump"; then
    echo "FAIL  Exception ausgeloest:"; grep -m1 'Vektor=' "$dump"; exit 1
fi

# 5. Sondenausgabe pruefen.
python3 - "$WORK/console.log" <<'PY'
import re, sys
s = open(sys.argv[1], 'rb').read().decode('latin1').replace('\r', '').replace('\n', '')
# Speicherspur-Zeilen von F$SRqMem/F$SRtMem unterbrechen die Sondenausgabe.
s = re.sub(r'\]F\$S[A-Za-z]+ [^]]*?C=[0-9]( D0-L=0x[0-9A-F]+ A2-P=0x[0-9A-F]+)?', '', s)
H = '([0-9A-F]{8})'
m = re.search('O%' + H + '&' + H*6 + '#' + H + H + '"' + H + H + '&' + H*6 + '!' + H + H + '&' + H*6, s)
if not m:
    print('FAIL  Sondenausgabe nicht gefunden'); sys.exit(1)
v = [int(x, 16) for x in m.groups()]
path = v[0] & 0xffff
pre, rd_c, rd_n, buf0, buf1 = v[1:7], v[7], v[8], v[9], v[10]
mid, cl_c, post = v[11:17], v[17], v[19:25]
fails = []
if not 3 <= path <= 5: fails.append(f'I$Open lieferte Pfad {path} (erwartet lokale Nummer 3..5)')
elif not pre[path] & 0x8000: fails.append(f'P$Path[{path}]={pre[path]:04x} nicht als Managerpfad markiert')
if rd_c: fails.append(f'I$Read Carry gesetzt, d1={rd_n:#x}')
elif rd_n != 16: fails.append(f'I$Read lieferte {rd_n} statt 16 Byte')
if (buf0, buf1) != (0x41424344, 0x45464748): fails.append(f'Puffer {buf0:08x}{buf1:08x} statt "ABCDEFGH"')
if 3 <= path <= 5 and not mid[path] & 0x8000: fails.append(f'P$Path[{path}] nach I$Read nicht wieder markiert')
if cl_c: fails.append('I$Close Carry gesetzt')
if 3 <= path <= 5 and post[path] != 0: fails.append(f'P$Path[{path}]={post[path]:04x} nach I$Close nicht frei')
if fails:
    for f in fails: print('FAIL  ' + f)
    sys.exit(1)
print(f'ok    Managerpfad {path}: Open markiert, Read 16 Byte "ABCDEFGH", Close gibt frei, keine Exception')
PY
echo "ALLE TESTS OK (mgrpath_probe_test.sh)"
