#!/bin/bash
# leak_test.sh -- Langlauf und Ressourcenbilanz (Arbeitsplan A8):
#
#   lange Eingabezeilen "echo a;date;pd;..." = viele F$Fork/F$Exit-Zyklen (Laden, F$TLink, Beenden)
#
# Bootet wie login_shell_test.sh bis zur Anmeldung, tippt Zeilen mit insgesamt etwa CYCLES Zyklen
# (je echo/date/pd, 8 Zyklen je Zeile) und wertet die Speicherereignisse der Konsole aus (Developer-Kernel
# gibt jede Belegung "M A" und Freigabe "M F" aus). Eine Marke nach WARMUP Zyklen (Module sind
# geladen, Zwischenspeicher gefuellt), eine am Ende: Bloecke, die am Ende zusaetzlich belegt sind,
# gelten als Verlust. Ergebnis: Zahl der Zyklen und Prozessenden, belegte Bloecke/Byte an beiden Marken.
#
# Aufruf: tools/leak_test.sh [CYCLES] [WARMUP]      (Standard 100 und 7; 333 Zyklen = rund 1000 Forks)
# Umgebung: wie login_shell_test.sh, dazu Q9_LEAK_TIMEOUT (Sekunden fuer die Schleife, Standard 900),
#           Q9K_LEAK_MAX_BYTES (erlaubter Zuwachs zwischen den Marken, Standard 0).
# Exit 0 nur, wenn der Zuwachs nicht groesser ist.

set -u
CYCLES=${1:-100}
WARMUP=${2:-7}
VARIANT=development

HERE=$(cd "$(dirname "$0")/.." && pwd)
KDIR="${Q9K_KERNEL_SRC:-$HERE/Q9-KERNEL/68k/src/kernel}"
FLUX=${FLUX:-$HERE/../Q9-Flux/Q9-Flux-68k}
[ -x "$FLUX/build/macos/q9.exe" ] || FLUX=/Volumes/SSD1TB/projects/Q9-Forge/Q9-SDK/SRC/SYSTEM/Q9-Flux/Q9-Flux-68k
REFBOOT=${REFBOOT:-$FLUX/../.hide/OS9Boot.noprot.test}
OS9=${OS9:-/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9}
CMDSDIR=${CMDSDIR:-${MWOS:-/Volumes/SSD1TB/projects/MWOS}/OS9/68000/CMDS}

die() { echo "FAIL  $*"; exit 1; }
[ -x "$FLUX/build/macos/q9.exe" ] || die "Emulator fehlt: $FLUX/build/macos/q9.exe"
[ -f "$REFBOOT" ] || die "Referenz-Bootdatei fehlt: $REFBOOT"
[ -x "$OS9" ] || die "ToolShed os9 fehlt: $OS9"
for c in echo date pd dir setime; do [ -f "$CMDSDIR/$c" ] || die "Befehl fehlt: $CMDSDIR/$c"; done
command -v expect >/dev/null 2>&1 || die "expect fehlt"
[ "$WARMUP" -lt "$CYCLES" ] || die "WARMUP muss kleiner als CYCLES sein"

WORK="${TMPDIR:-/tmp}/q9_leak_$$"
rm -rf "$WORK"; mkdir -p "$WORK"
if [ "${Q9_KEEP_TEST_WORK:-0}" = 1 ]; then echo "KEEP_WORK=$WORK"; else trap 'rm -rf "$WORK"' EXIT; fi

Q9K_KERNEL_VARIANT=$VARIANT Q9K_BOOT=sysgo "$KDIR/build.sh" "$WORK/kb" > "$WORK/build.log" 2>&1
brc=$?
if [ $brc -ne 0 ] || [ ! -f "$WORK/kb/q9kernel" ] || grep -qE 'operand size|Errors: 0*[1-9]|FEHLER' "$WORK/build.log"; then
    tail -12 "$WORK/build.log" | sed 's/^/        /'
    die "[$VARIANT] Kernelbau fehlgeschlagen"
fi

IMG="$WORK/img.hda"
"$OS9" format -q -k -nQ9TEST -bs512 -l65536 -c32 "$IMG" > "$WORK/format.log" 2>&1 || die "os9 format"
Q9K_BUILD_DIR="$WORK/kb" "$HERE/tools/mkbootfile.sh" "$REFBOOT" "$IMG" > "$WORK/mkboot.log" 2>&1 \
    || { tail -5 "$WORK/mkboot.log"; die "mkbootfile.sh"; }
"$OS9" makdir "$IMG,CMDS" >/dev/null 2>&1 || die "makdir CMDS"
"$OS9" makdir "$IMG,SYS" >/dev/null 2>&1 || die "makdir SYS"
for c in echo date pd dir setime; do
    "$OS9" copy "$CMDSDIR/$c" "$IMG,CMDS/$c" >/dev/null 2>&1 || die "copy $c"
    "$OS9" attr -q -e -pe "$IMG,CMDS/$c" >/dev/null 2>&1 || true
done
# openleak: Testprogramm aus dem Kernelbau (A16: Prozessende mit offenem IOMan-Pfad), Q9K_LEAK_CMDS="openleak;"
"$OS9" copy "$WORK/kb/openleak" "$IMG,CMDS/openleak" >/dev/null 2>&1 || die "copy openleak"
"$OS9" attr -q -e -pe "$IMG,CMDS/openleak" >/dev/null 2>&1 || true
printf '* Q9 leak startup\rchx /dd/CMDS\rchd /dd\rsetime 26/10/15 12:00:00\r' > "$WORK/startup"
printf 'q9test,,0.0,128,/dd/CMDS,/dd,mshell -l\r' > "$WORK/password"
printf 'Q9 leak image\r' > "$WORK/motd"
for f in startup password motd; do
    "$OS9" copy "$WORK/$f" "$IMG,SYS/$f" >/dev/null 2>&1 || die "copy SYS/$f"
    "$OS9" attr -q -r -pr -w -pw "$IMG,SYS/$f" >/dev/null 2>&1 || true   # openleak (A16) oeffnet motd lesend
done

export Q9K_LEAK_CYCLES="$CYCLES"

dump="$FLUX/local_images/q9dbg_dump.txt"
rm -f "$dump"
( cd "$FLUX" && Q9_TEST_TIMEOUT="${Q9_TEST_TIMEOUT:-90}" \
    expect -f "$HERE/tools/run_leak_test.exp" "$FLUX" "$IMG" "$WORK/console.log" > "$WORK/expect.out" 2>&1 )
erc=$?
if [ $erc -ne 0 ]; then
    grep -a "LEAK-TEST" "$WORK/expect.out" "$WORK/console.log" 2>/dev/null | head -3 | sed 's/^[^:]*://'
    echo "        letzte Konsolenzeilen:"; tr -d '\r' < "$WORK/console.log" | grep -av '^\s*$' | tail -8 | cut -c1-120 | sed 's/^/        /'
    die "Boot/Login/Schleife ($erc)"
fi

# A16: openleak meldet einen misslungenen Open ("openleak: open failed XX"): ein vom Prozessende nicht
# geschlossener IOMan-Pfad sperrt die Datei, der naechste Open scheitert.
if grep -aq 'openleak: open failed' "$WORK/console.log"; then
    echo "FAIL  openleak: $(grep -ac 'openleak: open failed' "$WORK/console.log") misslungene Opens (IOMan-Pfad nach Prozessende offen)"
    exit 1
fi
python3 - "$WORK/console.log" "$CYCLES" "$WARMUP" "${Q9K_LEAK_MAX_BYTES:-0}" <<'PY' || exit 1
import re, sys
log, cycles, warm, maxbytes = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
raw = open(log, 'rb').read().decode('latin1').replace('\r', '')
EV = re.compile(r'^M ([APFLR]) r=([0-9A-F]+) a=([0-9A-F]+) s=([0-9A-F]+) e=([0-9A-F]+)', re.M)
# Ohne Zeilenanfang-Anker: bei verschraenkter Ausgabe mehrerer Prozesse steht die Marke mitten in der Zeile.
# Das Terminalecho der getippten Zeile endet ebenfalls so, die Ausgabe des Befehls folgt aber spaeter und gewinnt.
MARK = re.compile(r'zq(mid1|end1)\s*$', re.M)
events = [(m.start(), m.group(1), int(m.group(3), 16), int(m.group(4), 16), int(m.group(5), 16)) for m in EV.finditer(raw)]
marks = {m.group(1): m.start() for m in MARK.finditer(raw)}
if 'mid1' not in marks or 'end1' not in marks:
    print("FAIL  Marken fehlen im Log: %s" % sorted(marks)); sys.exit(1)
def snapshot(pos):
    live = {}
    for p, op, addr, size, err in events:
        if p >= pos: break
        # e= ist bei A/L der Fehlercode, bei F aber die Nachbaradresse im Freispeicher (kein Fehler)
        if op in 'AL':
            if err == 0: live[addr] = size
        elif op == 'F': live.pop(addr, None)
    return live
mid, end = snapshot(marks['mid1']), snapshot(marks['end1'])
# A8: Zeilen "M R r=<Link-Summe> a=<Moduleintraege> s=<Trap-Slots>" (je Prozessende); jeweils die letzte vor der Marke.
RES = re.compile(r'^M R r=([0-9A-F]+) a=([0-9A-F]+) s=([0-9A-F]+)', re.M)
resev = [(m.start(), int(m.group(1), 16), int(m.group(2), 16), int(m.group(3), 16)) for m in RES.finditer(raw)]
def lastres(pos):
    last = None
    for p, links, ent, slots in resev:
        if p >= pos: break
        last = (links, ent, slots)
    return last
r1, r2 = lastres(marks['mid1']), lastres(marks['end1'])
procs_total = sum(1 for p, op, *_ in events if op == 'P' and marks['mid1'] <= p < marks['end1'])
# Entscheidend sind die SUMMEN: ein Puffer, der an anderer Adresse neu angelegt wurde, ist kein Verlust.
gbytes = sum(end.values()) - sum(mid.values())
gblocks = len(end) - len(mid)
print("Zyklen %d (Marke nach %d), Prozessenden zwischen den Marken: %d" % (cycles, warm, procs_total))
print("belegt an Marke 1: %d Bloecke, %d Byte; am Ende: %d Bloecke, %d Byte" % (len(mid), sum(mid.values()), len(end), sum(end.values())))
print("Zuwachs: %d Bloecke, %d Byte" % (gblocks, gbytes))
extra = {}
for a, s in end.items():
    extra[s] = extra.get(s, 0) + 1
for a, s in mid.items():
    extra[s] = extra.get(s, 0) - 1
for s, n in sorted(extra.items()):
    if n: print("    Groesse %d Byte: %+d Bloecke" % (s, n))
ok = gbytes <= maxbytes
print("%s  Speicherbilanz: Zuwachs %d Byte (erlaubt %d)" % ("ok  " if ok else "FAIL", gbytes, maxbytes))
if r1 and r2:
    print("Modul-Linkzaehler (Summe): %d -> %d, Eintraege im Modulverzeichnis: %d -> %d, belegte Trap-Slots: %d -> %d" % (r1[0], r2[0], r1[1], r2[1], r1[2], r2[2]))
    maxlinks = int(__import__('os').environ.get('Q9K_LEAK_MAX_LINKS', '0'))
    okr = (r2[0] - r1[0] <= maxlinks) and (r2[2] - r1[2] <= 0)
    print("%s  Ressourcenbilanz: Linkzaehler %+d (erlaubt %d), Trap-Slots %+d (erlaubt 0)" % ("ok  " if okr else "FAIL", r2[0] - r1[0], maxlinks, r2[2] - r1[2]))
    ok = ok and okr
else:
    print("FAIL  keine 'M R'-Zeilen im Log (Ressourcenbilanz nicht messbar)")
    ok = False
sys.exit(0 if ok else 1)
PY
