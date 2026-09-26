#!/bin/bash
#═════════════════════════════════════════════════════════════════════════════════════
# mk_dhfboot.sh -- DHF-Bootbaum bauen: Host-Verzeichnis, von dem Q9 Flux bootet
#═════════════════════════════════════════════════════════════════════════════════════
# Ablauf beim Booten (ROM Claude_cb030_DHF_BIOS.BIN, Konfiguration dhf_boot.q9):
#   1. ROM-Booter bootdhf (Q9-Port ROM_CBOOT/io_dhf.c) liest <bootbaum>/OS9Boot ueber das
#      DHF-Fenster $FFFF4000 -- nur wenn die .q9 "[dhf0] hostpath" setzt, sonst CF.
#   2. Dieses OS9Boot enthaelt dhfmgr, dhfdrv und den DHF-Deskriptor "dd" (statt cfide-dd),
#      init nennt /dd als Systemlaufwerk -> /dd ist wieder <bootbaum>.
#   3. sysgo: chx CMDS, SYS/startup, tsmon /term -- alles vom Host-Verzeichnis.
#
# Das Skript:
#   - baut dhfmgr/dhfdrv/dd mit qr68k/ql68k,
#   - legt <bootbaum> als APFS-Klon von Q9-Images/cf_images/OS9SYS an (nur wenn er fehlt;
#     -n = neu anlegen, vorhandenen Baum vorher loeschen),
#   - setzt <bootbaum>/OS9Boot aus dem OS9Boot von OS9SYS zusammen (cfide-"dd" ersetzt,
#     dhfmgr/dhfdrv dahinter),
#   - nimmt in <bootbaum>/SYS/startup die CF-Laufwerke aus dem iniz (dd ist jetzt DHF).
#
# Aufruf: boot/mk_dhfboot.sh [-n] [bootbaum]    (Standard: Q9-Images/dhf_root/boot)
#═════════╤══════╤═══════════════════════════════════════════════════════════╤══════
# Datum   │ Ver. │ Aenderung                                                 │ Wer
#─────────┼──────┼───────────────────────────────────────────────────────────┼──────
# 26-09-26│ 1.00 │ Erster Wurf                                               │ Cld
#═════════╧══════╧═══════════════════════════════════════════════════════════╧══════
set -u
NEU=0
[ "${1:-}" = "-n" ] && { NEU=1; shift; }

HERE=$(cd "$(dirname "$0")" && pwd)
DHF=$(cd "$HERE/.." && pwd)
FORGE=$(cd "$DHF/../.." && pwd)
QR68K=${QR68K:-$FORGE/Q9-QCC/Q9-BACKEND-68K/q9-qr68k/build/qr68k}
QL68K=${QL68K:-$FORGE/Q9-QCC/Q9-BACKEND-68K/q9-ql68k/build/ql68k}
DEFS=$FORGE/Q9-QCC/runtime/os9/q9defs.d
SRC=$FORGE/Q9-Images/cf_images/OS9SYS
BAUM=${1:-$FORGE/Q9-Images/dhf_root/boot}

die() { echo "mk_dhfboot: $*" >&2; exit 2; }
for f in "$QR68K" "$QL68K" "$DEFS" "$SRC/OS9Boot"; do [ -e "$f" ] || die "fehlt: $f"; done

W=$(mktemp -d "${TMPDIR:-/tmp}/dhfboot.XXXXXX")
trap 'rm -rf "$W"' EXIT

#── 1. Module bauen ────────────────────────────────────────────────────────────────────
cp "$DEFS" "$W/"
for m in manager/dhfmgr_68k.a:dhfmgr driver/dhfdrv_68k.a:dhfdrv descriptor/dd_dhf.a:dd; do
    src=${m%%:*}; nam=${m##*:}; b=$(basename "$src" .a)
    cp "$DHF/$src" "$W/"
    ( cd "$W" && "$QR68K" "$b.a" -o="$b.r" >"$b.log" 2>&1 && "$QL68K" "$b.r" -n="$nam" -gu=0.0 -O="$nam.mod" >>"$b.log" 2>&1 ) \
        || { cat "$W/$b.log"; die "Bauen $src"; }
done

#── 2. Bootbaum ────────────────────────────────────────────────────────────────────────
if [ $NEU = 1 ] && [ -d "$BAUM" ]; then
    chmod -R u+rwx "$BAUM"; rm -rf "$BAUM"
fi
if [ ! -d "$BAUM" ]; then
    mkdir -p "$(dirname "$BAUM")"
    cp -cRp "$SRC" "$BAUM" 2>/dev/null || cp -Rp "$SRC" "$BAUM" || die "Kopie von $SRC"
    echo "Bootbaum angelegt: $BAUM (Klon von $SRC)"
fi

#── 3. OS9Boot ─────────────────────────────────────────────────────────────────────────
python3 - "$SRC/OS9Boot" "$W" "$BAUM/OS9Boot" <<'EOF' || die "OS9Boot zusammensetzen"
import struct, sys
src, w, out = sys.argv[1:]
d = open(src, "rb").read()
def mods(d):
    i = 0
    while i + 16 <= len(d) and d[i:i+2] == b"\x4a\xfc":
        size = struct.unpack_from(">I", d, i + 4)[0]
        nm = struct.unpack_from(">I", d, i + 12)[0]
        yield d[i+nm:i+nm+32].split(b"\0")[0].decode("latin1"), d[i:i+size]
        i += size
new = {n: open(f"{w}/{n}.mod", "rb").read() for n in ("dd", "dhfmgr", "dhfdrv")}
res, seen = [], []
for n, m in mods(d):
    if n in ("dhfmgr", "dhfdrv"):
        continue                       # aeltere DHF-Fassung ersetzen
    if n == "dd":
        res += [new["dd"], new["dhfmgr"], new["dhfdrv"]]
    else:
        res.append(m)
    seen.append(n)
if "dd" not in seen:
    sys.exit("kein dd im Quell-OS9Boot")
b = b"".join(res)
open(out, "wb").write(b)
print(f"OS9Boot: {len(b)} Byte, {len(res)} Module (dd -> DHF, +dhfmgr {len(new['dhfmgr'])}, +dhfdrv {len(new['dhfdrv'])})")
EOF

#── 4. SYS/startup: CF-Laufwerke nicht mehr initialisieren ─────────────────────────────
ST=$BAUM/SYS/startup
if grep -q "iniz dd c0 c0_fmt r0" "$ST"; then
    [ -e "$ST.vor_dhfboot" ] || cp "$ST" "$ST.vor_dhfboot"
    sed -i '' 's/iniz dd c0 c0_fmt r0/iniz dd r0/' "$ST"
    echo "SYS/startup: iniz dd r0 (Sicherung SYS/startup.vor_dhfboot)"
fi
echo "fertig: $BAUM"
