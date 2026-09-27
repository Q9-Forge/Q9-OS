#!/bin/bash
#═════════════════════════════════════════════════════════════════════════════════════
# mk_init.sh -- init-Modul fuer den DHF-Bootbaum bauen
#═════════════════════════════════════════════════════════════════════════════════════
# Quelle ist der CB030-Port im MWOS-Baum: aus ihm stammt das init des heutigen OS9Boot
# (Ticker tkcb030) -- "orig" baut es byteidentisch nach (nachgeprueft 27.09.2026).
#
#   boot/mk_init.sh orig [ausgabe]   unveraendert (16 MB)
#   boot/mk_init.sh 64   [ausgabe]   _RAMMax 64 MB mit zwei Speicherbereichen:
#        0-16 MB  Prioritaet 250 -- wird zuerst vergeben: die bash 1.12 ist nicht
#                 32-Bit-sauber und stirbt, sobald ihr Speicher oberhalb 16 MB liegt
#        16-64 MB Prioritaet 200 -- erst, wenn unten nichts mehr frei ist (grosse
#                 Compilerlaeufe)
# Dazu gehoeren "[board] ram = 64" in der .q9 und ein ROM mit _RAMMax 64 MB (Q9-Port
# systype.d, boot/mk_dhfrom.sh); eingesetzt wird es mit boot/mk_dhfboot.sh -i <datei>.
#
# Gebaut wird in einer Wegwerfkopie des Ports (Wine, os9make), der Port bleibt unberuehrt.
#═════════╤══════╤═══════════════════════════════════════════════════════════╤══════
# Datum   │ Ver. │ Aenderung                                                 │ Wer
#─────────┼──────┼───────────────────────────────────────────────────────────┼──────
# 27-09-26│ 1.00 │ Erster Wurf                                               │ Cld
#═════════╧══════╧═══════════════════════════════════════════════════════════╧══════
set -u
V=${1:?Aufruf: mk_init.sh orig|64 [ausgabe]}
OUTF=${2:-$PWD/init_$V}
P=${MWOS:-/Volumes/SSD1TB/projects/MWOS}/OS9/68030/PORTS
T=$P/_mkinit_$$
trap 'rm -rf "$T"' EXIT
rsync -a --exclude Hardware --exclude CB030.zip --exclude .git "$P/CB030/" "$T/"
rm -f "$T/CMDS/BOOTOBJS/init_disk" "$T/CMDS/BOOTOBJS/init_rom"; find "$T/INIT/RELS" -type f -delete
if [ "$V" = 64 ]; then
python3 - "$T/systype.d" <<'EOF' || exit 2
import sys; p = sys.argv[1]; s = open(p, encoding="latin1").read()
def rep(a, b):
    global s; assert s.count(a) == 1, a; s = s.replace(a, b)
rep("_RAMMax     equ (16*1024*1024)          * 16MiB (XXX 64 or 128MiB also possible)",
    "_RAMMax     equ (64*1024*1024)          * 64MiB (2026-09-27, Q9-Emulator [board] ram = 64)\n"
    "_LoRAMTop   equ (16*1024*1024)          * bis hier zuerst vergeben (bash 1.12 nicht 32-Bit-sauber)")
rep("    MemType SYSRAM,250,B_USER,$1000,_RAMBase,_RAMMax,DRAMName,_RAMBase",
    "    MemType SYSRAM,250,B_USER,$1000,_RAMBase,_LoRAMTop,DRAMName,_RAMBase\n"
    "    MemType SYSRAM,200,B_USER,$1000,_LoRAMTop,_RAMMax,DRAMName,_RAMBase")
open(p, "w", encoding="latin1").write(s)
EOF
elif [ "$V" != orig ]; then
    echo "mk_init: orig oder 64" >&2; exit 2
fi
WINE=${WINE:-"$HOME/.local/wine-stable/Wine Stable.app/Contents/Resources/wine/bin/wine"}
N=$(basename "$T")
( cd /tmp && WINEPREFIX="${WINEPREFIX:-$HOME/.wine}" WINEDEBUG=-all "$WINE" cmd /c \
    "set MWOS=M:&& set PATH=M:\\DOS\\BIN;%PATH%&& cd /d M:\\OS9\\68030\\PORTS\\$N && os9make.exe -e MWOS=M: GOAL=build INIT" ) >"$T/init.log" 2>&1
[ -f "$T/CMDS/BOOTOBJS/init_disk" ] || { tail -20 "$T/init.log"; echo "mk_init: Bau fehlgeschlagen" >&2; exit 1; }
cp "$T/CMDS/BOOTOBJS/init_disk" "$OUTF"
echo "init ($V, $(wc -c <"$OUTF") Byte) -> $OUTF"
