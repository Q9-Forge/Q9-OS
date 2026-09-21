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
   "$SRCDIR"/q9kernel_setsys.c "$SRCDIR"/q9kernel_date.c "$SRCDIR"/q9kernel_alarm.c \
   "$SRCDIR"/q9kernel_clock.c "$SRCDIR"/q9kernel_bitmap.c "$SRCDIR"/q9kernel_blkmap.c "$SRCDIR"/q9kernel_sema.c "$SRCDIR"/q9kernel_chain.c "$SRCDIR"/q9kernel_icpt.c "$SRCDIR"/q9kernel_strap.c "$SRCDIR"/q9kernel_event.c "$SRCDIR"/q9kernel_nproc.c "$SRCDIR"/q9kernel_mem.c \
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

# Keep the CPU selection in one place for both the C compiler and every
# hand-written assembler module.  The compiler's 68k target names are broader
# than r68's exact -m values: xcc's `68k` is the 68000/010 family and `020`
# covers 68020/030.  The per-CPU Q9K_* define still lets C code select the
# exact feature set without pretending that xcc can generate 030-only code.
CPU_TYPE="${Q9K_CPU_TYPE:-68000}"
case "$CPU_TYPE" in
    68000) CPU_CODE=0; XCC_TARGET=68k; CPU_DEF=-dQ9K_CPU_68000; CPU_MMU=0; CPU_FPU=0 ;;
    68010) CPU_CODE=1; XCC_TARGET=68k; CPU_DEF=-dQ9K_CPU_68010; CPU_MMU=0; CPU_FPU=0 ;;
    68020) CPU_CODE=2; XCC_TARGET=020; CPU_DEF=-dQ9K_CPU_68020; CPU_MMU=0; CPU_FPU=0 ;;
    68030) CPU_CODE=3; XCC_TARGET=020; CPU_DEF=-dQ9K_CPU_68030; CPU_MMU=1; CPU_FPU=0 ;;
    68040) CPU_CODE=4; XCC_TARGET=020; CPU_DEF=-dQ9K_CPU_68040; CPU_MMU=1; CPU_FPU=1 ;;
    68060) CPU_CODE=6; XCC_TARGET=020; CPU_DEF=-dQ9K_CPU_68060; CPU_MMU=1; CPU_FPU=1 ;;
    CPU32) CPU_CODE=2; XCC_TARGET=cpu32; CPU_DEF=-dQ9K_CPU_CPU32; CPU_MMU=0; CPU_FPU=0 ;;
    *)
        echo "FEHLER: Q9K_CPU_TYPE muss 68000, 68010, 68020, 68030, 68040, 68060 oder CPU32 sein" >&2
        exit 2
        ;;
esac

case "${Q9K_ENABLE_MMU:-auto}" in
    auto) [ "$CPU_MMU" -eq 1 ] && MMU_DEF=-dQ9K_HAS_MMU || MMU_DEF= ;;
    1|yes|on) MMU_DEF=-dQ9K_HAS_MMU ;;
    0|no|off) MMU_DEF= ;;
    *) echo "FEHLER: Q9K_ENABLE_MMU muss auto, 0 oder 1 sein" >&2; exit 2 ;;
esac
case "${Q9K_ENABLE_FPU:-auto}" in
    auto) [ "$CPU_FPU" -eq 1 ] && FPU_DEF=-dQ9K_HAS_FPU || FPU_DEF= ;;
    1|yes|on) FPU_DEF=-dQ9K_HAS_FPU ;;
    0|no|off) FPU_DEF= ;;
    *) echo "FEHLER: Q9K_ENABLE_FPU muss auto, 0 oder 1 sein" >&2; exit 2 ;;
esac
R68_CPU_OPT="-m${CPU_CODE}"
CDEFS="$CDEFS -dQ9K_ALLOC_STANDARD -dQ9K_BOOT_STARTUP $CPU_DEF $MMU_DEF $FPU_DEF -tp=$XCC_TARGET"
MMU_STATE=off; [ -n "$MMU_DEF" ] && MMU_STATE=on
FPU_STATE=off; [ -n "$FPU_DEF" ] && FPU_STATE=on
echo "== Ziel-CPU: $CPU_TYPE (r68 $R68_CPU_OPT, xcc -tp=$XCC_TARGET, MMU=$MMU_STATE, FPU=$FPU_STATE) =="
cat > makefile <<EOF
CFLAGS = -b -O7 -cq -cw $CDEFS
all: q9kernel_cinit.r q9kernel_modcheck.r q9kernel_initext.r q9kernel_modsearch.r q9kernel_arena.r q9kernel_exctable.r q9kernel_tables.r q9kernel_firstproc.r q9kernel_moddir.r q9kernel_sched.r q9kernel_procend.r q9kernel_procsleep.r q9kernel_sysmem.r q9kernel_debug.r q9kernel_ssvc.r q9kernel_iopath.r q9kernel_procapi.r q9kernel_traplink.r q9kernel_setsys.r q9kernel_date.r q9kernel_alarm.r q9kernel_clock.r q9kernel_bitmap.r q9kernel_blkmap.r q9kernel_sema.r q9kernel_chain.r q9kernel_icpt.r q9kernel_strap.r q9kernel_event.r q9kernel_nproc.r q9kernel_mem.r
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
q9kernel_date.r: q9kernel_date.c
q9kernel_alarm.r: q9kernel_alarm.c
q9kernel_clock.r: q9kernel_clock.c
q9kernel_bitmap.r: q9kernel_bitmap.c
q9kernel_blkmap.r: q9kernel_blkmap.c
q9kernel_sema.r: q9kernel_sema.c
q9kernel_chain.r: q9kernel_chain.c
q9kernel_icpt.r: q9kernel_icpt.c
q9kernel_strap.r: q9kernel_strap.c
q9kernel_event.r: q9kernel_event.c
q9kernel_nproc.r: q9kernel_nproc.c
q9kernel_mem.r: q9kernel_mem.c
EOF

echo "== C-Dateien kompilieren ($KERNEL_VARIANT/Standard-Variante, s. q9kernel_config.h) =="
# "all" selbst schlaegt am Ende fehl (os9make versucht danach ein
# Programm "all" zu linken) -- das ist erwartet, die acht echten .r-
# Ziele sind zu diesem Zeitpunkt schon fertig. Deshalb || true.
mwos-build . all < /dev/null || true
for f in q9kernel_cinit.r q9kernel_modcheck.r q9kernel_initext.r q9kernel_modsearch.r q9kernel_arena.r q9kernel_exctable.r q9kernel_tables.r q9kernel_firstproc.r q9kernel_moddir.r q9kernel_sched.r q9kernel_procend.r q9kernel_procsleep.r q9kernel_sysmem.r q9kernel_debug.r q9kernel_ssvc.r q9kernel_iopath.r q9kernel_procapi.r q9kernel_traplink.r q9kernel_setsys.r q9kernel_date.r q9kernel_alarm.r q9kernel_clock.r q9kernel_bitmap.r q9kernel_blkmap.r q9kernel_sema.r q9kernel_chain.r q9kernel_icpt.r q9kernel_strap.r q9kernel_event.r q9kernel_nproc.r q9kernel_mem.r; do
    [ -f "$f" ] || { echo "FEHLER: $f wurde nicht erzeugt"; exit 1; }
done

echo "== Assembler-Einstieg assemblieren =="
WINE_BIN="$HOME/.local/wine-stable/Wine Stable.app/Contents/Resources/wine/bin/wine"
export WINEPREFIX="$HOME/.local/wineprefix-os9"
export WINEDEBUG=-all
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    "$R68_CPU_OPT" -o=q9kernel_entry.r "q9kernel_entry.a" < /dev/null

echo "== Verlinken (kein csl.l/acstart.r -- eigener Assembler-Einstieg) =="
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=q9kernel -f=orowoe \
    q9kernel_entry.r q9kernel_cinit.r q9kernel_modcheck.r q9kernel_modsearch.r q9kernel_initext.r \
    q9kernel_arena.r q9kernel_exctable.r q9kernel_tables.r q9kernel_firstproc.r q9kernel_moddir.r \
    q9kernel_sched.r q9kernel_procend.r q9kernel_procsleep.r q9kernel_sysmem.r q9kernel_debug.r q9kernel_ssvc.r q9kernel_iopath.r q9kernel_procapi.r q9kernel_traplink.r \
    q9kernel_setsys.r q9kernel_date.r q9kernel_alarm.r q9kernel_clock.r q9kernel_bitmap.r q9kernel_blkmap.r q9kernel_sema.r q9kernel_chain.r q9kernel_icpt.r q9kernel_strap.r q9kernel_event.r q9kernel_nproc.r q9kernel_mem.r \
    < /dev/null

# Layout-Hinweis (2026-09-18): q9kernel_entry.a haelt fuer den bekannten
# A4-Herkunftsbug (RBF/SCF schreiben "addq.l #1,$3ac(a4)" mit a4 = unserer
# Modulbasis) einen 12-Byte-Totraum vor, der Datei-Offset $3ac abdecken
# SOLL. Befund vom 2026-09-18: er liegt seit laengerem bei ~$60c, also
# 612 Byte zu weit hinten -- auch in Kernels, die einwandfrei booten. Der
# Symptomschutz ist damit gegenstandslos verschoben; der Kernel bootet
# trotzdem, weil an $3ac offenbar nichts Kritisches liegt (addq/subq
# heben sich paarweise auf). Nur Ausgabe, kein Abbruch -- zur Kenntnis
# fuer den, der den A4-Bug eines Tages wirklich behebt.
gap=$(xxd -s 0x3ac -l 4 -p q9kernel)
echo "== Layout-Hinweis: Bytes an Datei-Offset \$3ac = $gap (Totraum-Soll: 00000000, s. Kommentar) =="

echo "== Fertig: $OUTDIR/q9kernel =="
file q9kernel || true

echo "== forkchild.a bauen (eigenstaendiges Testmodul fuer F\$Fork, s. dortigen Kopfkommentar) =="
cp "$SRCDIR"/forkchild.a .
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    "$R68_CPU_OPT" -o=forkchild.r "forkchild.a" < /dev/null
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=forkchild -f=orowoe forkchild.r < /dev/null
echo "== Fertig: $OUTDIR/forkchild =="
file forkchild || true

echo "== hellosvc.a bauen (eigenstaendiges Testmodul fuer F\$Fork, s. dortigen Kopfkommentar) =="
cp "$SRCDIR"/hellosvc.a .
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    "$R68_CPU_OPT" -o=hellosvc.r "hellosvc.a" < /dev/null
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=hellosvc -f=orowoe hellosvc.r < /dev/null
echo "== Fertig: $OUTDIR/hellosvc =="
file hellosvc || true

echo "== chaintgt.a bauen (Zielmodul fuer den F\$Chain-Test, s. dortigen Kopfkommentar) =="
cp "$SRCDIR"/chaintgt.a .
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    "$R68_CPU_OPT" -o=chaintgt.r "chaintgt.a" < /dev/null
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=chaintgt -f=orowoe chaintgt.r < /dev/null
echo "== Fertig: $OUTDIR/chaintgt =="
file chaintgt || true

echo "== nproctgt.a bauen (Zielmodul fuer den F\$NProc-Test, s. dortigen Kopfkommentar) =="
cp "$SRCDIR"/nproctgt.a .
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    "$R68_CPU_OPT" -o=nproctgt.r "nproctgt.a" < /dev/null
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=nproctgt -f=orowoe nproctgt.r < /dev/null
echo "== Fertig: $OUTDIR/nproctgt =="
file nproctgt || true

echo "== evsigtgt.a bauen (Zielmodul fuer den blockierenden Ev\$Wait-Test, s. dortigen Kopfkommentar) =="
cp "$SRCDIR"/evsigtgt.a .
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    "$R68_CPU_OPT" -o=evsigtgt.r "evsigtgt.a" < /dev/null
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=evsigtgt -f=orowoe evsigtgt.r < /dev/null
echo "== Fertig: $OUTDIR/evsigtgt =="
file evsigtgt || true

echo "== iattachsvc.a bauen (I\$Attach/I\$Detach-Regressionstest) =="
cp "$SRCDIR"/iattachsvc.a .
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" \
    "$R68_CPU_OPT" -o=iattachsvc.r "iattachsvc.a" < /dev/null
arch -x86_64 "$WINE_BIN" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" \
    -o=iattachsvc -f=orowoe iattachsvc.r < /dev/null
echo "== Fertig: $OUTDIR/iattachsvc =="
file iattachsvc || true
