#!/bin/bash
#═════════════════════════════════════════════════════════════════════════════════════
# mk_dhfboot.sh -- DHF-Bootbaum bauen: Host-Verzeichnis, von dem Q9 Flux bootet
#═════════════════════════════════════════════════════════════════════════════════════
# Ablauf beim Booten (ROM Claude_cb030_DHF_BIOS.BIN, Konfiguration dhf_boot.q9/q9sys.q9):
#   1. ROM-Booter bootdhf (Q9-Port ROM_CBOOT/io_dhf.c) liest <bootbaum>/OS9Boot ueber das
#      DHF-Fenster $FFFF4000 -- nur wenn die .q9 "[dhf0] hostpath" setzt, sonst CF.
#   2. Dieses OS9Boot enthaelt dhfmgr, dhfdrv und den DHF-Deskriptor "dd" (statt cfide-dd),
#      init nennt /dd als Systemlaufwerk -> /dd ist wieder <bootbaum>.
#   3. sysgo: chx CMDS, SYS/startup, tsmon /term -- alles vom Host-Verzeichnis.
#
# Das Skript:
#   - baut dhfmgr/dhfdrv/dd (und ggf. den DHF-Zweitnamen) mit qr68k/ql68k,
#   - legt <bootbaum> als APFS-Klon von Q9-Images/cf_images/OS9SYS an (nur wenn er fehlt;
#     -n = neu anlegen, vorhandenen Baum vorher loeschen),
#   - setzt <bootbaum>/OS9Boot aus dem OS9Boot von OS9SYS zusammen (cfide-"dd" ersetzt,
#     dhfmgr/dhfdrv dahinter),
#   - passt das iniz in <bootbaum>/SYS/startup an (Original in SYS/startup.vor_dhfboot).
#
# Optionen fuer die Laufwerksnamen (Standard: nur dd, keine CF):
#   -a NAME  zweiter Name fuer das DHF-Systemlaufwerk (gleicher Port wie dd, IOMan teilt das
#            Geraet), z.B. -a c0 -> /c0 == /dd
#   -i DATEI init-Modul aus DATEI statt des init aus OS9SYS, z.B. das von boot/mk_init.sh
#            gebaute (64 MB, zwei Speicherbereiche) fuer "[board] ram = 64"
#   -f NAME  CF-Laufwerk (Master, cfide) unter diesem Namen, z.B. -f d0 -> /d0 = CF-Image aus
#            [c0] der .q9. Die cfide-Deskriptoren c0/c0_fmt fallen weg; der Deskriptor ist
#            das cfide-"c0" aus OS9SYS mit neuem Namen und neu berechneter Modul-CRC.
#
# Aufruf: boot/mk_dhfboot.sh [-n] [-a NAME] [-f NAME] [-i DATEI] [bootbaum]
#         (Standard-Bootbaum: Q9-Images/dhf_root/boot)
# Beispiel Q9SYS: boot/mk_dhfboot.sh -a c0 -f d0 ../../Q9-Images/cf_images/Q9SYS
#═════════╤══════╤═══════════════════════════════════════════════════════════╤══════
# Datum   │ Ver. │ Aenderung                                                 │ Wer
#─────────┼──────┼───────────────────────────────────────────────────────────┼──────
# 26-09-26│ 1.00 │ Erster Wurf                                               │ Cld
# 26-09-26│ 1.01 │ -a (DHF-Zweitname) und -f (CF-Laufwerk umbenannt)          │ Cld
# 27-09-26│ 1.02 │ -i (init aus Datei, z.B. 64-MB-init von mk_init.sh)         │ Cld
#═════════╧══════╧═══════════════════════════════════════════════════════════╧══════
set -u
NEU=0; ALIAS=; CFNAME=; INITF=
while [ $# -gt 0 ]; do
    case "$1" in
        -n) NEU=1; shift;;
        -a) ALIAS=$2; shift 2;;
        -f) CFNAME=$2; shift 2;;
        -i) INITF=$2; shift 2;;
        -*) echo "unbekannte Option $1" >&2; exit 2;;
        *) break;;
    esac
done

HERE=$(cd "$(dirname "$0")" && pwd)
DHF=$(cd "$HERE/.." && pwd)
FORGE=$(cd "$DHF/../.." && pwd)
QR68K=${QR68K:-$FORGE/Q9-QCC/Q9-BACKEND-68K/q9-qr68k/build/qr68k}
QL68K=${QL68K:-$FORGE/Q9-QCC/Q9-BACKEND-68K/q9-ql68k/build/ql68k}
DEFS=$FORGE/Q9-QCC/runtime/os9/q9defs.d
SRC=$FORGE/Q9-Images/cf_images/OS9SYS
BAUM=${1:-$FORGE/Q9-Images/dhf_root/boot}

die() { echo "mk_dhfboot: $*" >&2; exit 2; }
for f in "$QR68K" "$QL68K" "$DEFS" "$SRC/OS9Boot" ${INITF:+"$INITF"}; do [ -e "$f" ] || die "fehlt: $f"; done
for n in "$ALIAS" "$CFNAME"; do
    [ -z "$n" ] || [[ "$n" =~ ^[a-z][a-z0-9]?$ ]] || die "Laufwerksname '$n': 1-2 Zeichen (Deskriptor wird im Modul umbenannt)"
done
[ -z "$ALIAS" ] || [ "$ALIAS" != "$CFNAME" ] || die "-a und -f gleich"

W=$(mktemp -d "${TMPDIR:-/tmp}/dhfboot.XXXXXX")
trap 'rm -rf "$W"' EXIT

#── 1. Module bauen ────────────────────────────────────────────────────────────────────
cp "$DEFS" "$W/"
MODS="manager/dhfmgr_68k.a:dhfmgr driver/dhfdrv_68k.a:dhfdrv descriptor/dd_dhf.a:dd"
if [ -n "$ALIAS" ]; then   # Zweitname: dd_dhf.a mit anderem Modulnamen
    sed -e "s/^        nam     dd/        nam     $ALIAS/" -e "s/psect   dd,/psect   $ALIAS,/" \
        "$DHF/descriptor/dd_dhf.a" >"$W/alias_dhf.a"
    grep -q "psect   $ALIAS," "$W/alias_dhf.a" || die "Zweitname nicht einsetzbar"
fi
for m in $MODS ${ALIAS:+alias_dhf.a:$ALIAS}; do
    src=${m%%:*}; nam=${m##*:}; b=$(basename "$src" .a)
    [ -e "$W/$src" ] || cp "$DHF/$src" "$W/"
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
python3 - "$SRC/OS9Boot" "$W" "$BAUM/OS9Boot" "$ALIAS" "$CFNAME" "$INITF" <<'EOF' || die "OS9Boot zusammensetzen"
import os, struct, sys
src, w, out, alias, cfname, initf = sys.argv[1:]
d = open(src, "rb").read()

def crc24(data):                       # OS-9-Modul-CRC (an allen OS9Boot-Modulen geprueft)
    crc = 0xFFFFFF
    for b in data:
        crc ^= b << 16
        for _ in range(8):
            crc <<= 1
            if crc & 0x1000000:
                crc ^= 0x1800063
    return crc & 0xFFFFFF

def mods(d):
    i = 0
    while i + 16 <= len(d) and d[i:i+2] == b"\x4a\xfc":
        size = struct.unpack_from(">I", d, i + 4)[0]
        nm = struct.unpack_from(">I", d, i + 12)[0]
        yield d[i+nm:i+nm+32].split(b"\0")[0].decode("latin1"), d[i:i+size]
        i += size

def rename(m, old, new):               # gleich lange Namen: Bytes tauschen, CRC neu
    m = bytearray(m)
    nm = struct.unpack_from(">I", m, 12)[0]
    assert m[nm:nm+len(old)+1] == old.encode() + b"\0" and len(new) == len(old)
    m[nm:nm+len(new)] = new.encode()
    m[-3:] = (crc24(m[:-3]) ^ 0xFFFFFF).to_bytes(3, "big")
    return bytes(m)

names = ["dd", "dhfmgr", "dhfdrv"] + ([alias] if alias else [])
new = {n: open(f"{w}/{n}.mod", "rb").read() for n in names}
drop = {"dhfmgr", "dhfdrv"}            # aeltere DHF-Fassung ersetzen
if alias: drop.add(alias)
if cfname: drop |= {"c0", "c0_fmt", cfname}
res, seen, cf = [], [], None
for n, m in mods(d):
    seen.append(n)
    if n == "c0":
        cf = m
    if n in drop:
        continue
    if n == "dd":
        res += [new[x] for x in names]
        if cfname:
            res.append(None)           # Platz fuer das CF-Laufwerk
    elif n == "init" and initf:
        im = open(initf, "rb").read()
        if im[:2] != b"\x4a\xfc" or list(mods(im))[0][0] != "init":
            sys.exit("-i: kein init-Modul: " + initf)
        res.append(im)
    else:
        res.append(m)
if "dd" not in seen:
    sys.exit("kein dd im Quell-OS9Boot")
if cfname:
    if cf is None:
        sys.exit("kein cfide-c0 im Quell-OS9Boot")
    res[res.index(None)] = rename(cf, "c0", cfname)
b = b"".join(res)
for n, m in mods(b):                   # Kontrolle: jede CRC stimmt
    if crc24(m[:-3]) ^ 0xFFFFFF != int.from_bytes(m[-3:], "big"):
        sys.exit(f"CRC falsch in {n}")
open(out, "wb").write(b)
extra = (f", {alias} = DHF-Zweitname" if alias else "") + (f", {cfname} = CF (cfide)" if cfname else "") + (f", init aus {os.path.basename(initf)}" if initf else "")
print(f"OS9Boot: {len(b)} Byte, {len(res)} Module (dd -> DHF{extra})")
EOF

#── 4. SYS/startup: iniz anpassen ──────────────────────────────────────────────────────
ST=$BAUM/SYS/startup
[ -e "$ST.vor_dhfboot" ] || cp "$ST" "$ST.vor_dhfboot"
INIZ="iniz dd${ALIAS:+ $ALIAS}${CFNAME:+ $CFNAME} r0"
# Ausgangszeile aus der Sicherung (Original von OS9SYS), damit ein zweiter Lauf wieder greift
tr '\r' '\n' <"$ST.vor_dhfboot" | grep -q "^iniz dd c0 c0_fmt r0$" || die "SYS/startup: iniz-Zeile nicht gefunden"
sed "s/iniz dd c0 c0_fmt r0/$INIZ/" "$ST.vor_dhfboot" >"$ST"
echo "SYS/startup: $INIZ (Original SYS/startup.vor_dhfboot)"
echo "fertig: $BAUM"
