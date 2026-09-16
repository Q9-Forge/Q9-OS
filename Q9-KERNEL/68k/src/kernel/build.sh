#!/usr/bin/env bash
# build.sh -- baut und verlinkt alle Q9-eigenen Kernel-Quelldateien zu
#             einem echten OS-9/68K-Modul.
#
# 2026-08-18: erster erfolgreicher End-zu-Ende-Link ueberhaupt. Vorher
# wurde jede Datei nur EINZELN gegen die echte Toolchain getestet (nie
# zusammen gelinkt) -- dabei kam ans Licht, dass die vom Compiler
# eingebaute Stack-Ueberlauf-Pruefung zwei Laufzeitsymbole braucht
# (_stkhandler/_stklimit), die normalerweise aus der echten OS-9-C-
# Runtime (csl.l) kommen -- die haben wir als Kernel nicht (wir SIND die
# Runtime), deshalb jetzt selbst in q9kernel_entry.a definiert.
#
# Ergebnis (2026-08-18): echtes, gueltiges OS-9/68K-Modul erzeugt --
# von macOS' eigenem `file`-Kommando UND unserem eigenen q9ident (s.
# src/q9ident.c) unabhaengig korrekt erkannt (Format OS-9/68K, Typ
# Systm, Name "q9kernel", Attribut REENT|SUPER).
#
# Bewusst kein os9make-Makefile fuer den Link-Schritt -- os9make hat
# beim Versuch, eine "all"-Sammelregel zu definieren, automatisch
# versucht, ein Programm "all" ueber die normale C-Programm-Kette
# (inkl. acstart.r/csl.l) zu bauen, was wir explizit NICHT wollen (wir
# haben keinen C-Runtime-Start, unser eigener Assembler-Einstieg
# uebernimmt das). Direkter l68-Aufruf ohne jede Bibliothek ist
# einfacher und war das, was tatsaechlich funktioniert hat.
#
# Aufruf: ./build.sh [ausgabeverzeichnis] (Default: ./build)

set -euo pipefail

OUTDIR="${1:-$(dirname "$0")/build}"
SRCDIR="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$OUTDIR"
cd "$OUTDIR"

cp "$SRCDIR"/q9kernel_entry.a "$SRCDIR"/q9kernel_cinit.c "$SRCDIR"/q9kernel_modcheck.c \
   "$SRCDIR"/q9kernel_modsearch.c "$SRCDIR"/q9kernel_initext.c "$SRCDIR"/q9kernel_arena.c \
   "$SRCDIR"/q9kernel_exctable.c "$SRCDIR"/q9kernel_tables.c "$SRCDIR"/q9kernel_firstproc.c \
   "$SRCDIR"/q9kernel_moddir.c "$SRCDIR"/q9kernel_sched.c "$SRCDIR"/q9kernel_procend.c \
   "$SRCDIR"/q9kernel_procsleep.c "$SRCDIR"/q9kernel_sysmem.c "$SRCDIR"/q9kernel_debug.c \
   "$SRCDIR"/q9kernel_ssvc.c "$SRCDIR"/q9kernel_iopath.c "$SRCDIR"/q9kernel_procapi.c "$SRCDIR"/q9kernel_traplink.c \
   "$SRCDIR"/q9kernel_setsys.c \
   "$SRCDIR"/q9kernel_config.h .
# NACHTRAG (2026-09-13): Pfad an die Repo-Reorganisation angepasst --
# q9sysglob.h liegt jetzt unter Q9-KERNEL/common/src/ (fuer den
# geplanten x86-Port gemeinsam genutzt), nicht mehr direkt eine Ebene
# ueber src/kernel/.
cp "$SRCDIR"/../../../common/src/q9sysglob.h .
sed -i.bak 's#"../q9sysglob.h"#"q9sysglob.h"#' q9kernel_cinit.c && rm q9kernel_cinit.c.bak

source /Volumes/SSD1TB/projects/MWOS/tools/macos/env/os9-toolchain.sh

KERNEL_VARIANT="${Q9K_KERNEL_VARIANT:-development}"
case "$KERNEL_VARIANT" in
    development)
        CDEFS="-dQ9K_KERNEL_DEVELOPMENT -dQ9K_MEMTRACE_ARENA"
        ;;
    atomic)
        CDEFS="-dQ9K_KERNEL_ATOMIC"
        ;;
    *)
        echo "FEHLER: Q9K_KERNEL_VARIANT muss development oder atomic sein" >&2
        exit 2
        ;;
esac
CDEFS="$CDEFS -dQ9K_ALLOC_STANDARD -dQ9K_BOOT_STARTUP"
cat > makefile <<EOF
CFLAGS = -b -O7 -cq -cw $CDEFS
all: q9kernel_cinit.r q9kernel_modcheck.r q9kernel_initext.r q9kernel_modsearch.r q9kernel_arena.r q9kernel_exctable.r q9kernel_tables.r q9kernel_firstproc.r q9kernel_moddir.r q9kernel_sched.r q9kernel_procend.r q9kernel_procsleep.r q9kernel_sysmem.r q9kernel_debug.r q9kernel_ssvc.r q9kernel_iopath.r q9kernel_procapi.r q9kernel_traplink.r q9kernel_setsys.r
q9kernel_cinit.r: q9kernel_cinit.c
q9kernel_modcheck.r: q9kernel_modcheck.c
q9kernel_initext.r: q9kernel_initext.c
q9kernel_modsearch.r: q9kernel_modsearch.c
q9kernel_arena.r: q9kernel_arena.c
q9kernel_exctable.r: q9kernel_exctable.c
q9kernel_tables.r: q9kernel_tables.c
q9kernel_firstproc.r: q9kernel_firstproc.c
q9kernel_moddir.r: q9kernel_moddir.c
q9kernel_sched.r: q9kernel_sched.c
q9kernel_procend.r: q9kernel_procend.c
q9kernel_procsleep.r: q9kernel_procsleep.c
q9kernel_sysmem.r: q9kernel_sysmem.c

q9kernel_debug.r: q9kernel_debug.c
q9kernel_ssvc.r: q9kernel_ssvc.c
q9kernel_iopath.r: q9kernel_iopath.c
q9kernel_procapi.r: q9kernel_procapi.c
q9kernel_traplink.r: q9kernel_traplink.c
q9kernel_setsys.r: q9kernel_setsys.c
EOF

echo "== C-Dateien kompilieren ($KERNEL_VARIANT/Standard-Variante, s. q9kernel_config.h) =="
# "all" selbst schlaegt am Ende fehl (os9make versucht danach ein
# Programm "all" zu linken) -- das ist erwartet, die acht echten .r-
# Ziele sind zu diesem Zeitpunkt schon fertig. Deshalb || true.
mwos-build . all < /dev/null || true
for f in q9kernel_cinit.r q9kernel_modcheck.r q9kernel_initext.r q9kernel_modsearch.r q9kernel_arena.r q9kernel_exctable.r q9kernel_tables.r q9kernel_firstproc.r q9kernel_moddir.r q9kernel_sched.r q9kernel_procend.r q9kernel_procsleep.r q9kernel_sysmem.r q9kernel_debug.r q9kernel_ssvc.r q9kernel_iopath.r q9kernel_procapi.r q9kernel_traplink.r q9kernel_setsys.r; do
    [ -f "$f" ] || { echo "FEHLER: $f wurde nicht erzeugt"; exit 1; }
done

echo "== Assembler-Einstieg assemblieren =="
WINE_BIN="$HOME/.local/wine-stable/Wine Stable.app/Contents/Resources/wine/bin/wine"
export WINEPREFIX="$HOME/.local/wineprefix-os9"
export WINEDEBUG=-all
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    -o=q9kernel_entry.r "q9kernel_entry.a" < /dev/null

echo "== Verlinken (kein csl.l/acstart.r -- eigener Assembler-Einstieg) =="
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=q9kernel -f=orowoe \
    q9kernel_entry.r q9kernel_cinit.r q9kernel_modcheck.r q9kernel_modsearch.r q9kernel_initext.r \
    q9kernel_arena.r q9kernel_exctable.r q9kernel_tables.r q9kernel_firstproc.r q9kernel_moddir.r \
    q9kernel_sched.r q9kernel_procend.r q9kernel_procsleep.r q9kernel_sysmem.r q9kernel_debug.r q9kernel_ssvc.r q9kernel_iopath.r q9kernel_procapi.r q9kernel_traplink.r \
    q9kernel_setsys.r \
    < /dev/null

echo "== Fertig: $OUTDIR/q9kernel =="
file q9kernel || true

echo "== forkchild.a bauen (eigenstaendiges Testmodul fuer F\$Fork, s. dortigen Kopfkommentar) =="
cp "$SRCDIR"/forkchild.a .
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    -o=forkchild.r "forkchild.a" < /dev/null
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=forkchild -f=orowoe forkchild.r < /dev/null
echo "== Fertig: $OUTDIR/forkchild =="
file forkchild || true

echo "== hellosvc.a bauen (eigenstaendiges Testmodul fuer F\$Fork, s. dortigen Kopfkommentar) =="
cp "$SRCDIR"/hellosvc.a .
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    -o=hellosvc.r "hellosvc.a" < /dev/null
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=hellosvc -f=orowoe hellosvc.r < /dev/null
echo "== Fertig: $OUTDIR/hellosvc =="
file hellosvc || true

echo "== iattachsvc.a bauen (I\$Attach/I\$Detach-Regressionstest) =="
cp "$SRCDIR"/iattachsvc.a .
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    -o=iattachsvc.r "iattachsvc.a" < /dev/null
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=iattachsvc -f=orowoe iattachsvc.r < /dev/null
echo "== Fertig: $OUTDIR/iattachsvc =="
file iattachsvc || true
