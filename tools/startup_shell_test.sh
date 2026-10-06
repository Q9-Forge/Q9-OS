#!/bin/sh
# startup_shell_test.sh -- emulator regression for the Q9K_TestStartup chain
# in Q9K_TestProcA (see docs/OWN_KERNEL_STATUS.md, Fortsetzung 108):
#
#   F$Load /dd/CMDS/mshell        (IOMan F$Load -> kernel F$Link)    marker T
#   I$Dup 0, I$Close 0            (native /term path)
#   I$Open /dd/SYS/startup        (IOMan, device attach via F$Link)
#   F$Fork mshell                                                     marker K
#   F$Wait                        (mshell ran and exited)             marker W
#
# Fehlerfaelle melden sich mit l/d/c/o/f + Fehlercode statt K/W.
# Baut eine Kernelkopie mit Q9K_TestStartup=1 (Quellbaum unberuehrt), haengt
# csl und cio an die Bootdatei, legt CMDS/mshell und SYS/startup ins Abbild.
#
# Aufruf: tools/startup_shell_test.sh
# Umgebung (optional): FLUX, OS9, REFBOOT, Q9_TEST_TIMEOUT

set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
FLUX=${FLUX:-$HERE/../Q9-Flux/Q9-Flux-68k}
REFBOOT=${REFBOOT:-$HERE/../Q9-Flux/.hide/OS9Boot.noprot.test}
OS9=${OS9:-/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9}
# Nur Ziffern im Verzeichnisnamen: der Emulator gibt den Abbildpfad aus, und
# ein zufaelliges grosses "W" darin (mktemp) loeste den Marker zu frueh aus.
WORK="${TMPDIR:-/tmp}/q9_startup_shell_$$"
rm -rf "$WORK"; mkdir -p "$WORK"
trap 'rm -rf "$WORK"' EXIT

[ -x "$FLUX/build/macos/q9.exe" ] || { echo "Emulator fehlt: $FLUX/build/macos/q9.exe" >&2; exit 1; }
[ -f "$REFBOOT" ] || { echo "Referenz-Bootdatei fehlt: $REFBOOT" >&2; exit 1; }

# 1. Kernel mit Q9K_TestStartup=1 bauen (Kopie).
mkdir -p "$WORK/src/Q9-KERNEL/68k/src" "$WORK/src/Q9-KERNEL/common"
cp -R "$HERE/Q9-KERNEL/68k/src/kernel" "$WORK/src/Q9-KERNEL/68k/src/"
rm -rf "$WORK/src/Q9-KERNEL/68k/src/kernel/build"
cp -R "$HERE/Q9-KERNEL/common/src" "$WORK/src/Q9-KERNEL/common/"
ENTRY="$WORK/src/Q9-KERNEL/68k/src/kernel/q9kernel_entry.a"
sed -i.bak 's/^Q9K_TestStartup equ 0/Q9K_TestStartup equ 1/' "$ENTRY"
grep -q '^Q9K_TestStartup equ 1' "$ENTRY" || { echo "Schalter Q9K_TestStartup nicht gefunden" >&2; exit 1; }
( cd "$WORK/src/Q9-KERNEL/68k/src/kernel" && ./build.sh "$WORK/kb" ) > "$WORK/build.log" 2>&1 || true
if grep -qE 'operand size|Errors: 0*[1-9]' "$WORK/build.log" || [ ! -f "$WORK/kb/q9kernel" ]; then
    echo "FAIL  Kernelbau fehlgeschlagen"; exit 1
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

# 3. Abbild: Bootkette zuerst, dann Verzeichnisse und Dateien.
V="$WORK"
cat "$WORK/kb/q9kernel" "$V/vendor_init.mod" "$WORK/kb/forkchild" "$V/vendor_ioman.mod" \
    "$V/vendor_rbf.mod" "$V/vendor_cfide.mod" "$V/vendor_dd.mod" "$V/vendor_c0.mod" \
    "$V/vendor_scf.mod" "$V/vendor_sc68681.mod" "$V/vendor_term.mod" \
    "$V/vendor_csl.mod" "$V/vendor_cio.mod" > "$WORK/ref.boot"
"$OS9" format -q -k -nQ9TEST -bs512 -l32768 -c32 "$WORK/img.hda" >/dev/null
Q9K_BUILD_DIR="$WORK/kb" "$HERE/tools/mkbootfile.sh" "$WORK/ref.boot" "$WORK/img.hda" >/dev/null
"$OS9" makdir "$WORK/img.hda,CMDS"
"$OS9" makdir "$WORK/img.hda,SYS"
"$OS9" copy "$V/vendor_mshell.mod" "$WORK/img.hda,CMDS/mshell" >/dev/null
"$OS9" attr -q -e -pe "$WORK/img.hda,CMDS/mshell" >/dev/null 2>&1 || true
printf '* Q9 startup test\rchd /dd\r' > "$WORK/startup"
"$OS9" copy "$WORK/startup" "$WORK/img.hda,SYS/startup" >/dev/null

# 4. Laufen lassen bis "W" (F$Wait der Startup-Shell zurueck). K und W stehen
#    nicht zwingend nebeneinander: dazwischen schreibt mshell seine Ausgabe.
#    Ein grosses W kommt in den Boot- und Emulatormeldungen sonst nicht vor.
dump="$FLUX/local_images/q9dbg_dump.txt"
rm -f "$dump"
( cd "$FLUX" && Q9_TEST_TIMEOUT="${Q9_TEST_TIMEOUT:-60}" \
    expect -f "$HERE/tools/run_kernel_test.exp" "$FLUX" "$WORK/img.hda" 'W' "$WORK/console.log" \
    > "$WORK/expect.out" 2>&1 ) || true

[ -f "$dump" ] || { echo "FAIL  kein Debug-Dump geschrieben"; exit 1; }
if ! grep -q 'Vektor=0 (Fmt' "$dump"; then
    echo "FAIL  Exception ausgeloest:"; grep -m1 'Vektor=' "$dump"; exit 1
fi
python3 - "$WORK/console.log" <<'PY'
import re, sys
s = open(sys.argv[1], 'rb').read().decode('latin1').replace('\r', '').replace('\n', '')
s = re.sub(r'\]F\$S[A-Za-z]+ [^]]*?C=[0-9]( D0-L=0x[0-9A-F]+ A2-P=0x[0-9A-F]+)?', '', s)
after = s[s.find('RT'):] if 'RT' in s else ''
if not after:
    print('FAIL  Marker T (Startup-Zweig) nicht erreicht'); sys.exit(1)
k = after.find('K')
if k < 0 or after.find('W', k) < 0:
    what = {'l': 'F$Load', 'd': 'I$Dup', 'c': 'I$Close', 'o': 'I$Open', 'f': 'F$Fork'}
    m = re.search(r'([ldcof])([0-9A-F]{8})', after)
    if m:
        print(f'FAIL  {what[m.group(1)]} fehlgeschlagen, d1={m.group(2)}')
    else:
        print('FAIL  K/W nicht erreicht')
    sys.exit(1)
print('ok    mshell geladen, stdin umgeleitet, geforkt, beendet (T ... K W), keine Exception')
PY
echo "ALLE TESTS OK (startup_shell_test.sh)"
