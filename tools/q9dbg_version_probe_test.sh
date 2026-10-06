#!/bin/sh
# q9dbg_version_probe_test.sh -- emulator regression for F$Q9Dbg
# Unterfunktion 0 (Version/Faehigkeiten), s.
# docs/OWN_KERNEL_STATUS.md, Fortsetzung 118.
#
# Warum dieses Skript existiert: die Live-Anbindung von F$Q9Dbg
# (Dispatcher-Registrierung, Eintritts-/Rueckkehr-Haken,
# Trace-Ringpuffer) war bisher nur per Hosttest verifiziert -- ein
# echter Syscall ueber TRAP #0 auf echtem (emuliertem) 68k-Code war
# noch nie gelaufen. Q9K_TestProcA (q9kernel_entry.a, ganz am Anfang,
# unter "ifne Q9K_DEBUG") ruft bereits unbedingt F$Q9Dbg Unterfunktion 0
# auf und meldet das Ergebnis ueber die Marker 'V' (Erfolg, Puffergroesse
# > 0) bzw. 'v' (Fehlschlag) -- dieses Skript baut den regulaeren
# Developer-Kernel (KEIN Sonder-Schalter noetig) und prueft, dass 'V'
# tatsaechlich auf der Konsole erscheint, BEVOR IOMans eigener F$Link
# laeuft (der Probe-Block steht davor).
#
# Aufruf: tools/q9dbg_version_probe_test.sh
# Umgebung (optional): FLUX, OS9, REFBOOT, Q9_TEST_TIMEOUT

set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
FLUX=${FLUX:-$HERE/../Q9-Flux/Q9-Flux-68k}
REFBOOT=${REFBOOT:-$HERE/../Q9-Flux/.hide/OS9Boot.noprot.test}
OS9=${OS9:-/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9}
WORK=$(mktemp -d -t q9dbg_version_probe)
echo WORK=$WORK

[ -x "$FLUX/build/macos/q9.exe" ] || { echo "Emulator fehlt: $FLUX/build/macos/q9.exe" >&2; exit 1; }
[ -f "$REFBOOT" ] || { echo "Referenz-Bootdatei fehlt: $REFBOOT" >&2; exit 1; }

# 1. Regulaeren Developer-Kernel bauen (Kopie, Quellbaum unberuehrt --
#    kein Sonder-Schalter noetig, die Sonde ist bereits Teil des
#    Standard-Developer-Testablaufs).
mkdir -p "$WORK/src/Q9-KERNEL/68k/src" "$WORK/src/Q9-KERNEL/common"
cp -R "$HERE/Q9-KERNEL/68k/src/kernel" "$WORK/src/Q9-KERNEL/68k/src/"
rm -rf "$WORK/src/Q9-KERNEL/68k/src/kernel/build"
cp -R "$HERE/Q9-KERNEL/common/src" "$WORK/src/Q9-KERNEL/common/"
( cd "$WORK/src/Q9-KERNEL/68k/src/kernel" && Q9K_KERNEL_VARIANT=development Q9K_BOOT=test ./build.sh "$WORK/kb" ) > "$WORK/build.log" 2>&1 || true
if grep -qE 'operand size|Errors: 0*[1-9]' "$WORK/build.log" || [ ! -f "$WORK/kb/q9kernel" ]; then
    echo "FAIL  Kernelbau fehlgeschlagen (s. $WORK/build.log)"; tail -40 "$WORK/build.log"; exit 1
fi

# 2. Vendor-Module linear aus der Referenz-Bootdatei holen (exakt dasselbe
#    Verfahren wie tools/mgrpath_probe_test.sh).
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

# 3. Abbild: Bootkette auf ein frisch formatiertes Abbild schreiben.
V="$WORK"
cat "$WORK/kb/q9kernel" "$V/vendor_init.mod" "$WORK/kb/forkchild" "$V/vendor_ioman.mod" \
    "$V/vendor_rbf.mod" "$V/vendor_cfide.mod" "$V/vendor_dd.mod" "$V/vendor_c0.mod" \
    "$V/vendor_scf.mod" "$V/vendor_sc68681.mod" "$V/vendor_term.mod" > "$WORK/ref.boot"
"$OS9" format -q -k -nQ9TEST -bs512 -l32768 -c32 "$WORK/img.hda" >/dev/null
Q9K_BUILD_DIR="$WORK/kb" "$HERE/tools/mkbootfile.sh" "$WORK/ref.boot" "$WORK/img.hda" >/dev/null

# 4. Laufen lassen, bis die F$Q9Dbg-Sonde erreicht ist (oder ein
#    Fehlschlag-Marker kommt -- expect matcht nur EXAKT 'V', ein
#    Fehlschlag faellt deshalb in den Timeout-Zweig, danach wird der
#    Dump trotzdem geprueft).
dump="$FLUX/local_images/q9dbg_dump.txt"
rm -f "$dump"
( cd "$FLUX" && Q9_TEST_TIMEOUT="${Q9_TEST_TIMEOUT:-30}" \
    expect -f "$HERE/tools/run_kernel_test.exp" "$FLUX" "$WORK/img.hda" 'V' "$WORK/console.log" \
    > "$WORK/expect.out" 2>&1 ) || true

echo "--- Konsolenausgabe (roh) ---"
cat "$WORK/console.log" 2>/dev/null | tr -d '\000' | head -c 2000
echo
echo "--- Ende Konsolenausgabe ---"

if [ -f "$dump" ]; then
    if ! grep -q 'Vektor=0 (Fmt' "$dump"; then
        echo "FAIL  Exception ausgeloest:"; grep -m1 'Vektor=' "$dump"; exit 1
    fi
fi

python3 - "$WORK/console.log" <<'PY'
import sys
data = open(sys.argv[1], 'rb').read()
# WICHTIG: nur den Bereich NACH dem echten Boot-Start pruefen -- die
# "spawn ..."-Echozeile davor enthaelt den vollen Pfad inkl. des
# zufaelligen mktemp-Arbeitsverzeichnisnamens, der selbst zufaellig
# ein 'V' oder 'v' enthalten kann (genau das fuehrte in einer frueheren
# Fassung dieses Skripts zu einem falsch-positiven "ok", waehrend der
# echte Kernel tatsaechlich haengengeblieben war -- s. Fortsetzung 119).
marker = b'bootfile was found.'
idx = data.find(marker)
if idx < 0:
    print('FAIL  Boot-Start-Marker ("bootfile was found.") nicht in der Konsolenausgabe gefunden -- Boot kam nicht so weit')
    sys.exit(1)
s = data[idx:].decode('latin1', errors='replace')
if 'v' in s and 'V' not in s:
    print('FAIL  F$Q9Dbg Unterfunktion 0 meldete Fehlschlag (Marker lowercase v)')
    sys.exit(1)
if 'V' not in s:
    print('FAIL  Marker V (F$Q9Dbg-Erfolg) nicht in der Konsolenausgabe gefunden (nach dem Boot-Start)')
    sys.exit(1)
print('ok    F$Q9Dbg Unterfunktion 0 (Version/Faehigkeiten): Marker V gefunden -- Puffergroesse > 0, kein Fehlschlag')
# Fortsetzung 119: Lebensbeweis fuer das Protokollieren selbst (s. Probe-Block
# in Q9K_TestProcA): "VW<Saetze>R<Byte>:<Satzkopf 12 Byte>".
import re
m = re.search(r'VW([0-9A-F]{8})R([0-9A-F]{8}):([0-9A-F]{24})', s)
if not m:
    print('FAIL  Trace-Probe "VW...R...:..." nicht gefunden'); sys.exit(1)
cnt = int(m.group(1), 16); nread = int(m.group(2), 16); hdr = bytes.fromhex(m.group(3))
if cnt < 6:
    print('FAIL  Trace: nur %d Saetze, erwartet >= 6 (3x F$ID Eintritt+Rueckkehr)' % cnt); sys.exit(1)
print('ok    Trace an, 3x F$ID, Trace aus: %d Saetze geschrieben' % cnt)
if nread != 12:
    print('FAIL  Ringpuffer lesen lieferte %d statt 12 Byte' % nread); sys.exit(1)
rtype, rlen, code, flags = hdr[0], hdr[1], hdr[2], hdr[3]
pid = (hdr[4] << 8) | hdr[5]
if rtype != 1 or code != 0x0C:
    print('FAIL  erster Satz: Typ %d Code $%02X, erwartet Eintritt (1) F$ID ($0C)' % (rtype, code)); sys.exit(1)
if pid == 0:
    print('FAIL  erster Satz traegt keine Prozess-ID'); sys.exit(1)
print('ok    erster Satz: Eintritt F$ID, Laenge %d, PID %d, Tick %d -- F$Q9Dbg selbst nicht protokolliert' % (rlen, pid, int.from_bytes(hdr[6:10], 'big')))
PY

echo "ALLE TESTS OK (q9dbg_version_probe_test.sh)"
