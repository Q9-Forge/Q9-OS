#!/bin/bash
#═════════════════════════════════════════════════════════════════════════════════════
# mk_init.sh -- init-Modul fuer den DHF-Bootbaum bauen
#═════════════════════════════════════════════════════════════════════════════════════
# Quelle ist der CB030-Port im MWOS-Baum: aus ihm stammt das init des heutigen OS9Boot
# (Ticker tkcb030) -- "orig"/"16" baut es byteidentisch nach (nachgeprueft 27.09.2026).
#
#   boot/mk_init.sh orig|16 [ausgabe]   unveraendert (16 MB, Standard-MemList)
#   boot/mk_init.sh <MB>    [ausgabe]   _RAMMax <MB> MB mit zwei Speicherbereichen (MB>16):
#        0-16 MB   Prioritaet 250 -- wird zuerst vergeben: die bash 1.12 ist nicht
#                  32-Bit-sauber und stirbt, sobald ihr Speicher oberhalb 16 MB liegt
#        16-<MB> MB Prioritaet 200 -- erst, wenn unten nichts mehr frei ist (grosse
#                  Compilerlaeufe)
# Dazu gehoeren "[board] ram = <MB>" in der .q9 (1..256, s. boardcfg.c) und ein ROM mit
# _RAMMax <MB> MB (Q9-Port systype.d, boot/mk_dhfrom.sh); eingesetzt wird es mit
# boot/mk_dhfboot.sh -i <datei> (oder Direkt-Patch von OS9Boot auf einem CF/RBF-Image --
# DHF ist nur ein bequemer Trageweg, siehe Session-Notiz, kein RAM-Mechanismus).
#
# WICHTIG: der OS-9-Modulname bleibt in JEDER Variante "init" (nam-Direktive in
# OS9/SRC/SYSMODS/INIT/init.a, portunabhaengig) -- nur der AUSGABE-Dateiname hier traegt
# die Groesse (init16/init64/init256). Der Austausch im Modulbaum (OS9Boot bzw. ROM) ist
# deshalb ein reiner Dateiersatz, kein Umbenennen in OS-9 noetig.
#
# Gebaut wird in einer Wegwerfkopie des Ports (Wine, os9make), der Port bleibt unberuehrt.
#═════════╤══════╤═══════════════════════════════════════════════════════════╤══════
# Datum   │ Ver. │ Aenderung                                                 │ Wer
#─────────┼──────┼───────────────────────────────────────────────────────────┼──────
# 27-09-26│ 1.00 │ Erster Wurf                                               │ Cld
# 03-10-26│ 2.00 │ Generalisiert: beliebiger MB-Wert statt nur orig/64       │ Cld
#═════════╧══════╧═══════════════════════════════════════════════════════════╧══════
set -u
V=${1:?Aufruf: mk_init.sh orig|<MB> [ausgabe]}
OUTF=${2:-$PWD/init_$V}
P=${MWOS:-/Volumes/SSD1TB/projects/MWOS}/OS9/68030/PORTS
T=$P/_mkinit_$$
trap 'rm -rf "$T"' EXIT
rsync -a --exclude Hardware --exclude CB030.zip --exclude .git "$P/CB030/" "$T/"
rm -f "$T/CMDS/BOOTOBJS/init_disk" "$T/CMDS/BOOTOBJS/init_rom"; find "$T/INIT/RELS" -type f -delete

if [ "$V" = orig ] || [ "$V" = 16 ]; then
    : # unveraendert -- Standard-MemList (16 MiB, eine Prioritaetsstufe)
else
    case "$V" in (*[!0-9]*|'') echo "mk_init: MB-Wert muss eine Zahl sein (oder 'orig'), nicht '$V'" >&2; exit 2;; esac
    [ "$V" -gt 16 ] || { echo "mk_init: MB-Wert muss > 16 sein (sonst 'orig' oder 16 verwenden)" >&2; exit 2; }
    python3 - "$T/systype.d" "$V" <<'EOF' || exit 2
import sys
p, mb = sys.argv[1], int(sys.argv[2])
s = open(p, encoding="latin1").read()
def rep(a, b):
    global s
    assert s.count(a) == 1, a
    s = s.replace(a, b)
rep("_RAMMax     equ (16*1024*1024)          * 16MiB (XXX 64 or 128MiB also possible)",
    f"_RAMMax     equ ({mb}*1024*1024)          * {mb}MiB (2026-10-03, Q9-Emulator [board] ram = {mb})\n"
    "_LoRAMTop   equ (16*1024*1024)          * bis hier zuerst vergeben (bash 1.12 nicht 32-Bit-sauber)")
rep("    MemType SYSRAM,250,B_USER,$1000,_RAMBase,_RAMMax,DRAMName,_RAMBase",
    "    MemType SYSRAM,250,B_USER,$1000,_RAMBase,_LoRAMTop,DRAMName,_RAMBase\n"
    "    MemType SYSRAM,200,B_USER,$1000,_LoRAMTop,_RAMMax,DRAMName,_RAMBase")
open(p, "w", encoding="latin1").write(s)
EOF
fi

WINE=${WINE:-"$HOME/.local/wine-stable/Wine Stable.app/Contents/Resources/wine/bin/wine"}
N=$(basename "$T")
( cd /tmp && WINEPREFIX="${WINEPREFIX:-$HOME/.wine}" WINEDEBUG=-all "$WINE" cmd /c \
    "set MWOS=M:&& set PATH=M:\\DOS\\BIN;%PATH%&& cd /d M:\\OS9\\68030\\PORTS\\$N && os9make.exe -e MWOS=M: GOAL=build INIT" ) >"$T/init.log" 2>&1
[ -f "$T/CMDS/BOOTOBJS/init_disk" ] || { tail -20 "$T/init.log"; echo "mk_init: Bau fehlgeschlagen" >&2; exit 1; }
cp "$T/CMDS/BOOTOBJS/init_disk" "$OUTF"
echo "init ($V, $(wc -c <"$OUTF") Byte) -> $OUTF"
