#!/bin/bash
#═══════════════════════════════════════════════════════════════════════════════════════
# File:   run_dhfregr.sh                                                     Ver. 1.00
# Owner:  Claude
# Desc.:  DHF-Regressionstest, komplett: baut Manager/Treiber/Deskriptoren/Testprogramm
#         aus dem AKTUELLEN Quellstand, spielt sie in eine APFS-KOPIE des Claude-Images
#         ein (das echte Image bleibt unberuehrt -- ein gleichzeitig laufender Emulator
#         darauf stoert nicht), bootet den Emulator mit einer Kopie von
#         Q9-Images/emu_config/dhf_claude.q9 (dessen /SYS/startup haengt /d0 und /d1
#         selbst an), laesst test/dhfregr_68k.a laufen, prueft danach die
#         Host-Seite unter Q9-Images/dhf_root, laesst zusaetzlich die ECHTEN OS-9-Kommandos
#         list/attr/makdir/copy/del/deldir -q/rename/free/dsave, bash-getwd und eine Shell-Umleitung (echo >) auf /d0 los und
#         fasst alles zusammen.
#
# Aufruf: Q9_LOGIN_PASS=<Passwort fuer "super"> test/run_dhfregr.sh [-k] [-i]
#         -k  Emulator-Protokoll und Arbeitsverzeichnis behalten
#         -i  wenn alles gruen: die frisch gebauten Module auch ins ECHTE Image
#             einspielen (nur, wenn gerade kein Emulator mit dhf_claude.q9 laeuft)
# Exit:   0 = alles gruen, 1 = mindestens ein FEHLER, 2 = Test lief nicht durch
#
# Umgebung (alle optional ausser Q9_LOGIN_PASS):
#   Q9_LOGIN_USER (super)   QR68K / QL68K / OS9TOOL (Standardpfade s.u.)
#   Q9_TIMEOUT (120)        Sekunden je Schritt im expect-Skript
#
# Edition History
#─────────┬──────┬───────────────────────────────────────────────────────────┬──────
# Date    │ Ver. │ Description                                               │ By
#─────────┼──────┼───────────────────────────────────────────────────────────┼──────
# 26-09-26│ 1.00 │ Erster Wurf                                               │ Cld
#═════════╧══════╧═══════════════════════════════════════════════════════════╧══════
set -u
KEEP=0; INSTALL=0
for a in "$@"; do case "$a" in -k) KEEP=1;; -i) INSTALL=1;; *) echo "unbekannte Option $a" >&2; exit 2;; esac; done

HERE=$(cd "$(dirname "$0")" && pwd)
DHF=$(cd "$HERE/.." && pwd)
FORGE=$(cd "$DHF/../.." && pwd)
QR68K=${QR68K:-$FORGE/Q9-QCC/Q9-BACKEND-68K/q9-qr68k/build/qr68k}
QL68K=${QL68K:-$FORGE/Q9-QCC/Q9-BACKEND-68K/q9-ql68k/build/ql68k}
OS9TOOL=${OS9TOOL:-/Volumes/SSD1TB/projects/MWOS/tools/macos/bin/os9}
DEFS=$FORGE/Q9-QCC/runtime/os9/q9defs.d
EMU=$FORGE/Q9-Flux/Q9-Flux-68k
CFG=$FORGE/Q9-Images/emu_config/dhf_claude.q9
IMG=$FORGE/Q9-Images/cf_images/OS9SYS_Claude.hda
ROOT=$FORGE/Q9-Images/dhf_root
USER_=${Q9_LOGIN_USER:-super}
TMO=${Q9_TIMEOUT:-120}

die() { echo "run_dhfregr: $*" >&2; exit 2; }
[ -n "${Q9_LOGIN_PASS:-}" ] || die "Q9_LOGIN_PASS nicht gesetzt (Passwort fuer $USER_)"
for f in "$QR68K" "$QL68K" "$OS9TOOL" "$DEFS" "$CFG" "$IMG" "$EMU/build/macos/q9.exe"; do
    [ -e "$f" ] || die "fehlt: $f"
done
command -v expect >/dev/null || die "expect fehlt"

W=$(mktemp -d "${TMPDIR:-/tmp}/dhfregr.XXXXXX")
cleanup() { [ $KEEP = 1 ] && echo "Arbeitsverzeichnis: $W" || rm -rf "$W"; }
trap cleanup EXIT

#── 1. Bauen ───────────────────────────────────────────────────────────────────────────
# Quelle | interner Modulname | Name im Image unter /CMDS/dhf
MODS="manager/dhfmgr_68k.a:dhfmgr:dhfmgr_68k
driver/dhfdrv_68k.a:dhfdrv:dhfdrv_68k
descriptor/d0_dhf.a:d0:d0
descriptor/d1_dhf.a:d1:d1
test/dhfregr_68k.a:dhfregr:dhfregr
test/dhfrenfree_68k.a:dhfrfr:dhfrenfree"
cp "$DEFS" "$W/"
echo "── Bauen"
while IFS=: read -r src nam dst; do
    cp "$DHF/$src" "$W/"
    b=$(basename "$src" .a)
    ( cd "$W" && "$QR68K" "$b.a" -o="$b.r" >"$b.qr.log" 2>&1 ) || { cat "$W/$b.qr.log"; die "qr68k $src"; }
    ( cd "$W" && "$QL68K" "$b.r" -n="$nam" -gu=0.0 -O="$b.mod" >"$b.ql.log" 2>&1 ) || { cat "$W/$b.ql.log"; die "ql68k $src"; }
    printf "   %-24s -> %-8s %5d Byte\n" "$src" "$nam" "$(wc -c <"$W/$b.mod")"
done <<<"$MODS"

#── 2. Einspielen (in eine APFS-Kopie) ─────────────────────────────────────────────────
REAL_IMG=$IMG
IMG=$W/img.hda
cp -c "$REAL_IMG" "$IMG" 2>/dev/null || cp "$REAL_IMG" "$IMG" || die "Image-Kopie fehlgeschlagen"
sed -e "s|^image *=.*|image = $IMG|" -e "s|^rom *= *\.\./|rom  = $FORGE/Q9-Images/|" "$CFG" >"$W/emu.q9"
CFG=$W/emu.q9
echo "── Einspielen nach Kopie von $(basename "$REAL_IMG"),/CMDS/dhf"
while IFS=: read -r src nam dst; do
    b=$(basename "$src" .a)
    "$OS9TOOL" del "$IMG,/CMDS/dhf/$dst" >/dev/null 2>&1
    "$OS9TOOL" copy "$W/$b.mod" "$IMG,/CMDS/dhf/$dst" >/dev/null 2>&1 || die "os9 copy $dst"
    "$OS9TOOL" attr -e "$IMG,/CMDS/dhf/$dst" >/dev/null 2>&1
    rm -f "$W/chk"; "$OS9TOOL" copy "$IMG,/CMDS/dhf/$dst" "$W/chk" >/dev/null 2>&1
    cmp -s "$W/$b.mod" "$W/chk" || die "Rueckvergleich $dst im Image fehlgeschlagen"
done <<<"$MODS"

#── 3. Host-Seite vorbereiten ──────────────────────────────────────────────────────────
mkdir -p "$ROOT/d0"
for d in rt ut dd; do chmod -R u+rwx "$ROOT/d0/$d" 2>/dev/null; rm -rf "$ROOT/d0/$d"; done
rm -f "$ROOT/d0/rn2.txt"; cp "$ROOT/d0/hello.txt" "$ROOT/d0/rn.txt"   # fuer "rename"
rm -rf "$ROOT/d0/dsq" "$ROOT/d0/dsz" "$ROOT/d0/zt" "$ROOT/d0/nm"      # fuer "dsave"/Zeit/Namen
mkdir -p "$ROOT/d0/zt"; echo t >"$ROOT/d0/zt/zeit.txt"; touch -t 202001020304 "$ROOT/d0/zt/zeit.txt"
mkdir -p "$ROOT/d0/nm"; for n in ok.txt .DS_Store ._ok.txt abcdefghijklmnopqrstuvwxyz1234 "$(printf "\303\274mlaut.txt")"; do echo x >"$ROOT/d0/nm/$n"; done
mkdir -p "$ROOT/d0/dsq/a/b"; printf "eins\r" >"$ROOT/d0/dsq/f1"; printf "zwei\r" >"$ROOT/d0/dsq/a/f2"
cp "$ROOT/d0/hello.txt" "$ROOT/d0/dsq/a/b/f3"
# Koeder fuer Fall 86: Nachbarverzeichnis, dessen Name mit dem d0-Basispfad BEGINNT
mkdir -p "$ROOT/d0_nachbar"; echo geheim >"$ROOT/d0_nachbar/geheim.txt"
D1ROOT=$FORGE/Q9-Images/cf_images/OS9SYS
HELLO_SUM=$(md5 -q "$ROOT/d0/hello.txt" 2>/dev/null || md5sum "$ROOT/d0/hello.txt" | cut -d' ' -f1)

#── 4. Emulator ────────────────────────────────────────────────────────────────────────
# Prompt-Muster "# " am Pufferende: das fruehere [#$] ?$ griff auf jedes "$" der
# Testausgabe (Hexwerte, "E$CEF") und beendete den Emulator mitten im Lauf (s. STATUS.md,
# Nachtrag SS_Rename/SS_Free); "ROOT# " allein passt nach "chd" nicht mehr (Prompt "/d0/x# ").
LOG=$W/emu.log
cat >"$W/run.exp" <<EOF
log_user 0
log_file -a $LOG
set timeout $TMO
set prompt {# \$}
cd "$EMU"
spawn ./build/macos/q9.exe "$CFG"
# ACHTUNG: expect-Musterlisten in {} MUESSEN mehrzeilig sein -- einzeilig liest expect das
# Ganze als EIN Glob-Muster, das nie passt, und kehrt nach dem Timeout still zurueck (so
# kostete frueher jeder Schritt exakt Q9_TIMEOUT Sekunden, ohne je "exit" zu erreichen).
expect {
    "devices online" {}
    timeout { exit 3 }
}
send "\r"
set ok 0
for {set i 0} {\$i < 10 && !\$ok} {incr i} {
    expect {
        "User name?:" { send "$USER_\r"; exp_continue }
        -re {Password[^\r\n]*:} { send "\$env(Q9_LOGIN_PASS)\r"; exp_continue }
        -re \$prompt { set ok 1 }
        timeout { send "\r" }
    }
}
if {!\$ok} { exit 4 }
proc run {cmd} {
    global prompt
    send_log "\nRUNNER [clock format [clock seconds] -format %T] \$cmd\n"
    send "\$cmd\r"
    expect {
        -re \$prompt {}
        timeout { send_log "\nRUNNER: TIMEOUT bei \$cmd\n"; exit 5 }
    }
}
run "/dd/CMDS/dhf/dhfregr"
run "list /d0/hello.txt"
run "attr /d0"
run "makdir /d0/ut"
run "copy /d0/hello.txt /d0/ut/k.txt"
run "list /d0/ut/k.txt"
run "echo hallo >/d0/ut/e.txt"
run "del /d0/ut/k.txt"
run "list /d1/cfboot_os9.bl"
run "makdir /d0/dd"
run "makdir /d0/dd/unter"
run "copy /d0/hello.txt /d0/dd/f.txt"
run "copy /d0/hello.txt /d0/dd/unter/g.txt"
run "deldir -q /d0/dd"
run "rename /d0/rn.txt rn2.txt"
run "free /d0"
run "chd /d0/ut"
run "bash -c pwd"
run "makdir /d0/dsz"
run "chd /d0/dsq"
run "dsave -s /d0/dsz | mshell"
run "chd /dd/HOME/ROOT"
run "dir -e /d0/zt"
run "copy /d0/hello.txt /d1/rotest.txt"
run "makdir /d1/rotest"
run "dir /d0/nm"
# Alle Pfade sind jetzt geschlossen -> der Emulator darf KEINE Datei unter dhf_root mehr
# offen halten (Beleg dafuer, dass I\$Close den Host wirklich erreicht, s. Mgr_Close/PD_COUNT)
catch {exec lsof -p [exp_pid] > $W/lsof.txt}
send_log "\nRUNNER [clock format [clock seconds] -format %T] Ende\n"
send "\x1d"
expect eof
EOF
echo "── Emulator: $(basename "$CFG")"
expect "$W/run.exp"; RC=$?
tr -d '\r' <"$LOG" >"$W/emu.txt"
[ $RC = 0 ] || { tail -40 "$W/emu.txt"; KEEP=1; die "expect-Abbruch (Code $RC)"; }

#── 5. Auswertung Gast ─────────────────────────────────────────────────────────────────
sed -n '/=== dhfregr/,/DHFREGR-ENDE/p' "$W/emu.txt" >"$W/gast.txt"
grep -q "DHFREGR-ENDE" "$W/gast.txt" || { tail -40 "$W/emu.txt"; KEEP=1; die "dhfregr lief nicht bis zum Ende"; }
echo "── Gast (dhfregr)"
grep -E "^(OK|FEHLER) " "$W/gast.txt" | sed 's/^/   /'
G_OK=$(grep -c "^OK " "$W/gast.txt"); G_FAIL=$(grep -c "^FEHLER " "$W/gast.txt")

#── 6. Auswertung Host + Standard-Utilities ────────────────────────────────────────────
H_OK=0; H_FAIL=0
host() { if eval "$2"; then echo "   OK     H$1"; H_OK=$((H_OK+1)); else echo "   FEHLER H$1"; H_FAIL=$((H_FAIL+1)); fi; }
echo "── Zeitablauf"; grep "^RUNNER " "$W/emu.txt" | sed 's/^RUNNER /   /'
echo "── Host / Standard-Utilities"
host "1 d0/rt existiert als Verzeichnis"          '[ -d "$ROOT/d0/rt" ]'
host "2 d0/rt ist fuer den Besitzer rwx"          '[ -r "$ROOT/d0/rt" ] && [ -w "$ROOT/d0/rt" ] && [ -x "$ROOT/d0/rt" ]'
host "3 d0/rt ist danach leer"                    '[ -z "$(ls -A "$ROOT/d0/rt" 2>/dev/null)" ]'
host "4 hello.txt unveraendert"                   '[ "$HELLO_SUM" = "$(md5 -q "$ROOT/d0/hello.txt" 2>/dev/null || md5sum "$ROOT/d0/hello.txt" | cut -d" " -f1)" ]'
host "5 nichts ausserhalb von d0 angelegt"        '[ -z "$(ls -A "$ROOT" | grep -v "^d0$" | grep -v "^d0_nachbar$")" ] && [ "$(cat "$ROOT/d0_nachbar/geheim.txt")" = geheim ]'
host "6 list /d0/hello.txt (Standard-Utility)"    'grep -q "^Hallo von Claude, DHF-Testdatei" "$W/emu.txt"'
host "7 attr /d0 zeigt Verzeichnis"               'grep -qE "^d[-a-z]+ +/d0$" "$W/emu.txt"'
host "8 makdir /d0/ut legt Host-Verzeichnis an"   '[ -d "$ROOT/d0/ut" ]'
host "9 copy+list: Kopie lesbar (2x Inhalt)"      '[ "$(grep -c "^Hallo von Claude, DHF-Testdatei" "$W/emu.txt")" = 2 ]'
host "10 del /d0/ut/k.txt entfernt die Kopie"     '[ ! -e "$ROOT/d0/ut/k.txt" ]'
host "11 keine offenen Host-Dateien unter d0/d1"  '[ -s "$W/lsof.txt" ] && ! grep -q -e "$ROOT" -e "$D1ROOT" "$W/lsof.txt"'
host "12 echo >/d0/ut/e.txt: Inhalt kommt an"     '[ "$(od -An -c "$ROOT/d0/ut/e.txt" 2>/dev/null | tr -d " ")" = "hallo\r" ]'

# d1 (zweites DHF-Laufwerk -> cf_images/OS9SYS): eine Zeile, die nur in dieser Datei steht
# (nicht im Bootprotokoll), muss per "list" angekommen sein
host "13 d1: list /d1/cfboot_os9.bl = Host-Datei" 'grep -qx "/c0/CMDS/BOOTOBJS/cache030" "$W/emu.txt"'

host "14 deldir -q /d0/dd loescht rekursiv"     '[ ! -e "$ROOT/d0/dd" ]'
host "16 free /d0 meldet freien Platz"           'grep -q "free on media" "$W/emu.txt" && ! grep -q "free: can" "$W/emu.txt"'
host "17 bash getwd in /d0/ut (pwd)"            'grep -qx "/d0/ut" "$W/emu.txt"'
host "18 dsave | mshell: Baum 1:1 kopiert"          'diff -r "$ROOT/d0/dsq" "$ROOT/d0/dsz" >/dev/null 2>&1'
host "15 rename /d0/rn.txt rn2.txt"               '[ ! -e "$ROOT/d0/rn.txt" ] && [ -f "$ROOT/d0/rn2.txt" ]'

host "19 Ortszeit: dir -e zeigt 20/01/02 0304"   'grep -q "20/01/02 0304.* zeit.txt" "$W/emu.txt"'
host "20 /d1 nur lesbar: copy/makdir abgelehnt"  '[ ! -e "$D1ROOT/rotest.txt" ] && [ ! -e "$D1ROOT/rotest" ]'
host "21 dir blendet .DS_Store/._/lange/UTF-8 aus" 'sed -n "/Directory of \/d0\/nm/,/# *\$/p" "$W/emu.txt" | grep -q "ok.txt" && ! grep -q -e DS_Store -e "_ok.txt" -e abcdefghijklmnopqrstuvwxyz -e mlaut "$W/emu.txt"'
rm -f "$D1ROOT/rotest.txt"; rmdir "$D1ROOT/rotest" 2>/dev/null

TOTAL_FAIL=$((G_FAIL+H_FAIL))
echo "══ ERGEBNIS: Gast $G_OK OK / $G_FAIL FEHLER, Host $H_OK OK / $H_FAIL FEHLER"
for d in rt ut dd; do chmod -R u+rwx "$ROOT/d0/$d" 2>/dev/null; rm -rf "$ROOT/d0/$d"; done
rm -rf "$ROOT/d0_nachbar" "$ROOT/d0/dsq" "$ROOT/d0/dsz" "$ROOT/d0/zt" "$ROOT/d0/nm"; rm -f "$ROOT/d0/rn.txt" "$ROOT/d0/rn2.txt"
[ $TOTAL_FAIL = 0 ] || exit 1
if [ $INSTALL = 1 ]; then
    if pgrep -f "q9.exe .*dhf_claude.q9" >/dev/null; then
        echo "── -i: NICHT eingespielt -- ein Emulator laeuft gerade auf $(basename "$REAL_IMG")"
    else
        echo "── -i: Module ins echte Image $(basename "$REAL_IMG")"
        while IFS=: read -r src nam dst; do
            b=$(basename "$src" .a)
            "$OS9TOOL" del "$REAL_IMG,/CMDS/dhf/$dst" >/dev/null 2>&1
            "$OS9TOOL" copy "$W/$b.mod" "$REAL_IMG,/CMDS/dhf/$dst" >/dev/null 2>&1 || die "os9 copy $dst (echt)"
            "$OS9TOOL" attr -e "$REAL_IMG,/CMDS/dhf/$dst" >/dev/null 2>&1
            echo "   $dst"
        done <<<"$MODS"
    fi
fi
exit 0
