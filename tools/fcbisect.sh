#!/bin/bash
# fcbisect.sh -- FARCALL-Bisektion (Arbeitsplan A2/A3).
#
#   tools/fcbisect.sh [--variant development|atomic] [--list] <Stellen>
#
# <Stellen>: "all", "none" oder Liste wie "1-21,30,33-35" (Nummern aus --list).
# Das Werkzeug kopiert die Kernelquellen in ein Wegwerfverzeichnis, ersetzt dort
# jedes gewaehlte `bsr <C-Funktion>` in q9kernel_entry.a durch `Q9K_FARCALL <C-Funktion>`
# und fuehrt tools/login_shell_test.sh gegen die Kopie aus (Quelle ueber Q9K_KERNEL_SRC).
# Die Arbeitskopie im Repo bleibt unberuehrt. Exit 0 = Login-Test gruen.
#
# "C-Funktion" = Ziel eines `bsr`, das in q9kernel_entry.a nicht als Label definiert ist.
set -u
HERE=$(cd "$(dirname "$0")/.." && pwd)
KSRC="$HERE/Q9-KERNEL/68k/src/kernel"
VARIANT=development; LIST=0; SEL=; MODE=farcall
while [ $# -gt 0 ]; do
    case "$1" in
        --variant) VARIANT=$2; shift 2;;
        --list) LIST=1; shift;;
        --mode) MODE=$2; shift 2;;   # farcall | far2 | pad: bsr + 4 NOPs (gleiche 8 Byte mehr, aber ohne FARCALL-Semantik)
        -h|--help) sed -n '2,13p' "$0"; exit 0;;
        *) SEL=$1; shift;;
    esac
done
[ "$LIST" = 1 ] || [ -n "$SEL" ] || { echo "Stellen angeben (all|none|1-21,...) oder --list" >&2; exit 2; }

# build.sh greift relativ auf ../../../common zu: die Kopie muss in gleicher Tiefe liegen
# (Geschwisterverzeichnis von src/kernel) und wird beim Beenden entfernt.
TMP="$HERE/Q9-KERNEL/68k/src/fcbisect_$$"
trap 'rm -rf "$TMP"' EXIT
cp -R "$KSRC" "$TMP"

python3 - "$TMP/q9kernel_entry.a" "$SEL" "$LIST" "$MODE" <<'PY'
import re, sys
path, sel, lst, mode = sys.argv[1], sys.argv[2], sys.argv[3] == "1", sys.argv[4]
L = open(path, encoding="latin-1").read().split("\n")
labels = set()
for l in L:
    m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):", l) or re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s+(equ|macro)\b", l)
    if m: labels.add(m.group(1))
sites = [(i, m.group(1)) for i, l in enumerate(L)
         for m in [re.match(r"^\s+bsr\s+([A-Za-z_][A-Za-z0-9_]*)", l)] if m and m.group(1) not in labels]
if lst:
    for n, (i, t) in enumerate(sites, 1): print("%2d  Z%-5d %s" % (n, i + 1, t))
    sys.exit(3)
chosen = set()
if sel == "all": chosen = set(range(1, len(sites) + 1))
elif sel != "none":
    for part in sel.split(","):
        a, _, b = part.partition("-"); chosen |= set(range(int(a), int(b or a) + 1))
bad = [n for n in chosen if not 1 <= n <= len(sites)]
if bad: sys.exit("Stellen ausserhalb 1..%d: %s" % (len(sites), sorted(bad)))
if mode == "far2":
    mi = next(k for k, l in enumerate(L) if l.startswith("Q9K_FARCALL macro"))
    ei = next(k for k in range(mi, len(L)) if L[k].strip() == "endm")
    macro = """Q9K_FARCALL2 macro
* registerneutraler Fernaufruf (fcbisect --mode far2): rettet a0 und springt per rts.
        pea     *+26(pc)            * Ruecksprungadresse = Befehl hinter dem rts
        move.l  a0,-(sp)            * a0 des Aufrufers retten
        lea     *(pc),a0
        adda.l  #\\1-*+4,a0          * a0 = Ziel
        move.l  a0,-(sp)
        move.l  4(sp),a0            * a0 des Aufrufers zurueck
        move.l  (sp)+,(sp)          * Ziel an die Stelle des geretteten a0
        rts                         * Sprung ins Ziel, Ruecksprung hinter dieses Makro
        endm""".split("\n")
    L[ei + 1:ei + 1] = macro
    # Zeilennummern der Aufrufstellen verschieben sich: neu bestimmen
    sites = [(i, m.group(1)) for i, l in enumerate(L)
             for m in [re.match(r"^\s+bsr\s+([A-Za-z_][A-Za-z0-9_]*)", l)] if m and m.group(1) not in labels]
for n in chosen:
    i, t = sites[n - 1]
    if mode == "far2":
        L[i] = re.sub(r"\bbsr\s+" + re.escape(t), "Q9K_FARCALL2 " + t, L[i], count=1)
    elif mode == "pad":
        L[i] = L[i] + "\n        nop\n        nop\n        nop\n        nop                 * fcbisect --mode pad"
    else:
        L[i] = re.sub(r"\bbsr\s+" + re.escape(t), "Q9K_FARCALL " + t, L[i], count=1)
open(path, "w", encoding="latin-1").write("\n".join(L))
print("%s an %d von %d Stellen: %s" % ({"pad": "PAD(bsr+4 NOP)", "far2": "FARCALL2"}.get(mode, "FARCALL"), len(chosen), len(sites), sel))
PY
rc=$?
[ "$rc" = 3 ] && exit 0
[ "$rc" = 0 ] || exit "$rc"
Q9K_KERNEL_SRC="$TMP" bash "$HERE/tools/login_shell_test.sh" "$VARIANT"
