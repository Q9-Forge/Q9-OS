#!/bin/bash
#═════════════════════════════════════════════════════════════════════════════════════
# run_dhfboot.sh -- Boottest: Q9 Flux bootet OS-9 aus einem DHF-Host-Verzeichnis
#═════════════════════════════════════════════════════════════════════════════════════
# Prueft den ganzen Weg: ROM-Booter bootdhf -> OS9Boot vom Host -> /dd = DHF -> Login ->
# Befehle aus /dd/CMDS -> Schreiben auf /dd landet im Host-Verzeichnis. Zweiter Lauf: DHF-ROM
# OHNE [dhf0] muss wie bisher von CompactFlash booten (Rueckfall).
#
# Gebootet wird von einem APFS-KLON des Bootbaums (Q9-Images/dhf_root/boot, s.
# boot/mk_dhfboot.sh), das CF-Image fuer den Rueckfall ist ebenfalls ein Klon.
#
# Aufruf: Q9_LOGIN_PASS=<Passwort fuer "super"> test/run_dhfboot.sh [-k] [-v]
#   -k  Arbeitsverzeichnis behalten   -v  Emulator-Ausgabe live zeigen
# Ergebnis: Zeile "DHF-Boot: n/m OK", Exitcode 0 nur wenn alles gruen.
#═════════╤══════╤═══════════════════════════════════════════════════════════╤══════
# Datum   │ Ver. │ Aenderung                                                 │ Wer
#─────────┼──────┼───────────────────────────────────────────────────────────┼──────
# 26-09-26│ 1.00 │ Erster Wurf                                               │ Cld
#═════════╧══════╧═══════════════════════════════════════════════════════════╧══════
set -u
KEEP=0; VERBOSE=0
for a in "$@"; do case "$a" in -k) KEEP=1;; -v) VERBOSE=1;; *) echo "unbekannte Option $a" >&2; exit 2;; esac; done

HERE=$(cd "$(dirname "$0")" && pwd)
DHF=$(cd "$HERE/.." && pwd)
FORGE=$(cd "$DHF/../.." && pwd)
EMU=$FORGE/Q9-Flux/Q9-Flux-68k
ROM=$FORGE/Q9-Images/rom_images/Claude_cb030_DHF_BIOS.BIN
BAUM=$FORGE/Q9-Images/dhf_root/boot
IMG=$FORGE/Q9-Images/cf_images/OS9SYS_Claude.hda
USER_=${Q9_LOGIN_USER:-super}
TMO=${Q9_TIMEOUT:-120}

die() { echo "run_dhfboot: $*" >&2; exit 2; }
[ -n "${Q9_LOGIN_PASS:-}" ] || die "Q9_LOGIN_PASS nicht gesetzt (Passwort fuer $USER_)"
for f in "$ROM" "$BAUM/OS9Boot" "$IMG" "$EMU/build/macos/q9.exe"; do [ -e "$f" ] || die "fehlt: $f"; done
command -v expect >/dev/null || die "expect fehlt"

W=$(mktemp -d "${TMPDIR:-/tmp}/dhfboot.XXXXXX")
cleanup() { [ $KEEP = 1 ] && echo "Arbeitsverzeichnis: $W" || { chmod -R u+rwx "$W" 2>/dev/null; rm -rf "$W"; }; }
trap cleanup EXIT

cp -cRp "$BAUM" "$W/boot" 2>/dev/null || cp -Rp "$BAUM" "$W/boot" || die "Klon Bootbaum"
cp -c "$IMG" "$W/cf.hda" 2>/dev/null || cp "$IMG" "$W/cf.hda" || die "Klon CF-Image"
rm -f "$W/boot/dhfboot.txt"

cat >"$W/boot.q9" <<EOF
[board]
name = DHF-Boottest
rom  = $ROM
net  = nat
[dhf0]
hostpath = $W/boot
readonly = no
EOF
cat >"$W/cf.q9" <<EOF
[board]
name = DHF-ROM Rueckfall CF
rom  = $ROM
net  = nat
[c0]
type = rbf
bus = onboard
unit = master
base = 0xFFFFE000
start_sector = 0
length_sectors = 0x800000
descriptor_lsn = 0
image = $W/cf.hda
EOF

# $1 = Config, $2 = Log, weitere = Befehle nach dem Login
emu_run() {
    local cfg=$1 log=$2; shift 2
    {
        echo "log_user $VERBOSE"
        echo "log_file -a $log"
        echo "set timeout $TMO"
        echo 'set prompt {# $}'
        echo "cd \"$EMU\""
        echo "spawn ./build/macos/q9.exe \"$cfg\""
        # Musterlisten mehrzeilig (einzeilig = ein einziges Glob, s. run_dhfregr.sh)
        echo 'expect {'
        echo '    "devices online" {}'
        echo '    timeout { exit 3 }'
        echo '}'
        echo 'send "\r"'
        echo 'set ok 0'
        echo 'for {set i 0} {$i < 10 && !$ok} {incr i} {'
        echo '    expect {'
        echo "        \"User name?:\" { send \"$USER_\\r\"; exp_continue }"
        echo '        -re {Password[^\r\n]*:} { send "$env(Q9_LOGIN_PASS)\r"; exp_continue }'
        echo '        -re $prompt { set ok 1 }'
        echo '        timeout { send "\r" }'
        echo '    }'
        echo '}'
        echo 'if {!$ok} { exit 4 }'
        echo 'proc run {cmd} {'
        echo '    global prompt'
        echo '    send_log "\nRUNNER $cmd\n"'
        echo '    send "$cmd\r"'
        echo '    expect {'
        echo '        -re $prompt {}'
        echo '        timeout { send_log "\nRUNNER: TIMEOUT bei $cmd\n"; exit 5 }'
        echo '    }'
        echo '}'
        for c in "$@"; do printf 'run {%s}\n' "$c"; done
        echo 'send_log "\nRUNNER ENDE\n"'
        echo 'exit 0'
    } >"$W/run.exp"
    ( cd "$EMU" && expect "$W/run.exp" ) </dev/null
    local rc=$?
    pkill -f "q9.exe $cfg" 2>/dev/null
    return $rc
}

T=0; F=0
check() {   # $1 Name, $2 Bedingung (bash)
    T=$((T+1))
    if eval "$2"; then printf "   ok    %s\n" "$1"; else F=$((F+1)); printf "   FEHLER %s\n" "$1"; fi
}

echo "── Lauf 1: Boot von DHF ($W/boot)"
L1=$W/boot.log
emu_run "$W/boot.q9" "$L1" \
    "mdir dhfmgr dhfdrv dd" \
    "devs" \
    "pd" \
    "dir /dd/SYS" \
    "list /dd/SYS/motd" \
    "echo dhfboot >/dd/dhfboot.txt" \
    "list /dd/dhfboot.txt" \
    "dir -e /dd/CMDS/dir"
rc=$?
check "Emulator/Login (expect rc=$rc)"          "[ $rc = 0 ]"
check "ROM-Booter bootet von DHF"               "grep -q 'boot from DHF' '$L1'"
check "kein CF-Boot"                            "! grep -q 'trying to boot from CompactFlash' '$L1'"
check "gueltige Bootdatei"                      "grep -q 'A valid OS-9 bootfile was found' '$L1'"
check "Module dhfmgr/dhfdrv/dd geladen"         "[ \$(grep -A6 'RUNNER mdir' '$L1' | grep -o 'dhfmgr\|dhfdrv\|dd' | sort -u | wc -l) -ge 3 ]"
check "devs zeigt dd mit dhfmgr"                "grep -A40 'RUNNER devs' '$L1' | grep -q 'dd.*dhfdrv.*dhfmgr\|dd.*dhfmgr'"
check "pd = /dd..."                             "grep -A3 'RUNNER pd' '$L1' | grep -q '^/dd'"
check "dir /dd/SYS zeigt startup"               "grep -A12 'RUNNER dir /dd/SYS' '$L1' | grep -q startup"
check "Schreiben auf /dd landet auf dem Host"   "[ \"\$(tr '\r' '\n' <'$W/boot/dhfboot.txt' 2>/dev/null)\" = dhfboot ]"
check "keine Fehlermeldungen"                   "! grep -A2 'RUNNER' '$L1' | grep -qi 'error #\|can.t'"

echo "── Lauf 2: gleicher ROM ohne [dhf0] -> CF"
L2=$W/cf.log
emu_run "$W/cf.q9" "$L2" "pd"
rc=$?
check "Emulator/Login (expect rc=$rc)"          "[ $rc = 0 ]"
check "Rueckfall auf CompactFlash"              "grep -q 'trying to boot from CompactFlash' '$L2'"
check "gueltige Bootdatei von CF"               "grep -q 'A valid OS-9 bootfile was found' '$L2'"

echo "DHF-Boot: $((T-F))/$T OK"
[ $F = 0 ] || { echo "Logs: $L1 $L2"; KEEP=1; exit 1; }
exit 0
