#!/bin/bash
# login_shell_test.sh -- Regressionstest des ECHTEN Starts (A1 im Arbeitsplan):
#
#   sysgo -> SYS/startup (mshell) -> tsmon -> login -> mshell -> echo/pd/dir/date
#
# Baut den Kernel (Variante atomic oder development, jeweils mit
# Q9K_BOOT=sysgo), setzt ein frisches Testabbild zusammen, bootet es im
# Emulator, meldet sich als "q9test" (ohne Passwort) an und prueft die
# Ausgabe von vier Befehlen. Zusaetzlich muss der Q9-Flux-Dump "Vektor=0"
# melden (keine Exception).
#
# Aufruf: tools/login_shell_test.sh [atomic|development]
#         (Standard: development)
# Umgebung (optional): FLUX, REFBOOT, OS9, MWOS, Q9_TEST_TIMEOUT,
#         Q9_KEEP_TEST_WORK=1 (Arbeitsverzeichnis behalten).
# Ausgabe: je Pruefung "ok"/"FAIL"; Exit 0 nur, wenn alles gruen ist.
#
# Der Emulator laeuft mit einem Wegwerfabbild; Master-Abbilder werden nie
# beruehrt. Der Verzeichnisname enthaelt nur Ziffern/Kleinbuchstaben: der
# Emulator gibt den Abbildpfad aus, und Grossbuchstaben darin loesten bei
# frueheren Skripten Marken zu frueh aus.

set -u
VARIANT=${1:-development}
case "$VARIANT" in atomic|development) ;; *) echo "FEHLER: Variante muss atomic oder development sein" >&2; exit 2 ;; esac

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
for c in echo dir date setime; do [ -f "$CMDSDIR/$c" ] || die "Befehl fehlt: $CMDSDIR/$c"; done
command -v expect >/dev/null 2>&1 || die "expect fehlt"

tag=$(echo "$VARIANT" | cut -c1-3)
WORK="${TMPDIR:-/tmp}/q9_login_${tag}_$$"
rm -rf "$WORK"; mkdir -p "$WORK"
if [ "${Q9_KEEP_TEST_WORK:-0}" = 1 ]; then echo "KEEP_WORK=$WORK"; else trap 'rm -rf "$WORK"' EXIT; fi

# 1. Kernel bauen (echter Start: Q9K_BOOT=sysgo).
Q9K_KERNEL_VARIANT=$VARIANT Q9K_BOOT=sysgo "$KDIR/build.sh" "$WORK/kb" > "$WORK/build.log" 2>&1
brc=$?
if [ $brc -ne 0 ] || [ ! -f "$WORK/kb/q9kernel" ] || grep -qE 'operand size|Errors: 0*[1-9]|FEHLER' "$WORK/build.log"; then
    tail -12 "$WORK/build.log" | sed 's/^/        /'
    die "[$VARIANT] Kernelbau fehlgeschlagen"
fi
ksize=$(wc -c < "$WORK/kb/q9kernel" | tr -d ' ')

# 2. Abbild: Bootdatei (Referenz mit unserem Kernel), dann Dateisystem.
IMG="$WORK/img.hda"
"$OS9" format -q -k -nQ9TEST -bs512 -l32768 -c32 "$IMG" > "$WORK/format.log" 2>&1 || die "[$VARIANT] os9 format"
# C4 (mkbootfile haengt Q9_BOOT_MODULES nur mit --disk an): os9p2 ist eine Erweiterung aus der Liste M$Extens ("OS9P2 syscache ssm fpu OS9P3") und kommt als zusaetzliches Bootmodul ins Abbild
Q9_BOOT_MODULES="$WORK/kb/os9p2" Q9K_BUILD_DIR="$WORK/kb" "$HERE/tools/mkbootfile.sh" --disk "$REFBOOT" "$IMG" > "$WORK/mkboot.log" 2>&1 \
    || { tail -5 "$WORK/mkboot.log"; die "[$VARIANT] mkbootfile.sh"; }
"$OS9" makdir "$IMG,CMDS" >/dev/null 2>&1 || die "[$VARIANT] makdir CMDS"
"$OS9" makdir "$IMG,SYS" >/dev/null 2>&1 || die "[$VARIANT] makdir SYS"
for c in echo dir date setime; do
    "$OS9" copy "$CMDSDIR/$c" "$IMG,CMDS/$c" >/dev/null 2>&1 || die "[$VARIANT] copy $c"
    "$OS9" attr -q -e -pe "$IMG,CMDS/$c" >/dev/null 2>&1 || true
done
# icptsleep: eigenes Testprogramm aus dem Kernelbau (A9: Signal steht an, wenn F$Sleep gerufen wird).
"$OS9" copy "$WORK/kb/icptsleep" "$IMG,CMDS/icptsleep" >/dev/null 2>&1 || die "[$VARIANT] copy icptsleep"
"$OS9" attr -q -e -pe "$IMG,CMDS/icptsleep" >/dev/null 2>&1 || true
"$OS9" copy "$WORK/kb/icptrte" "$IMG,CMDS/icptrte" >/dev/null 2>&1 || die "[$VARIANT] copy icptrte"
"$OS9" attr -q -e -pe "$IMG,CMDS/icptrte" >/dev/null 2>&1 || true
# A13: Skript fuer "mshell <loop" (Umleitung der Eingabe aus einer Datei)
printf 'echo skript eins\recho skript zwei\r' > "$WORK/loop"
"$OS9" copy "$WORK/loop" "$IMG,loop" >/dev/null 2>&1 || die "[$VARIANT] copy loop"
for c in icptwait waitkid trapfail tlinkerr dbgchild dbgtest semchild semtest alarmtest p2test; do
    "$OS9" copy "$WORK/kb/$c" "$IMG,CMDS/$c" >/dev/null 2>&1 || die "[$VARIANT] copy $c"
    "$OS9" attr -q -e -pe "$IMG,CMDS/$c" >/dev/null 2>&1 || true
done
# Textdateien mit CR-Zeilenende (OS-9). Passwort leer: Benutzer q9test.
printf '* Q9 regress startup\rchx /dd/CMDS\rchd /dd\rsetime 26/10/15 12:00:00\r' > "$WORK/startup"
printf 'q9test,,0.0,128,/dd/CMDS,/dd,mshell -l\r' > "$WORK/password"
printf 'Q9 regression image\r' > "$WORK/motd"
for f in startup password motd; do
    "$OS9" copy "$WORK/$f" "$IMG,SYS/$f" >/dev/null 2>&1 || die "[$VARIANT] copy SYS/$f"
done

# 3. Emulator-Lauf.
dump="$FLUX/local_images/q9dbg_dump.txt"
rm -f "$dump"
( cd "$FLUX" && Q9_TEST_TIMEOUT="${Q9_TEST_TIMEOUT:-90}" \
    expect -f "$HERE/tools/run_login_test.exp" "$FLUX" "$IMG" "$WORK/console.log" \
    > "$WORK/expect.out" 2>&1 )
erc=$?
if [ $erc -ne 0 ]; then
    grep -a "LOGIN-TEST" "$WORK/expect.out" "$WORK/console.log" 2>/dev/null | head -3 | sed 's/^[^:]*://'
    echo "        letzte Konsolenzeilen:"; tr -d '\r' < "$WORK/console.log" | grep -av '^\s*$' | tail -8 | cut -c1-120 | sed 's/^/        /'
    die "[$VARIANT] Boot/Login/Befehle ($erc)"
fi

# 4. Ausgabe pruefen.
python3 - "$WORK/console.log" "$VARIANT" "$ksize" <<'PY' || exit 1
import re, sys
log, variant, ksize = sys.argv[1], sys.argv[2], sys.argv[3]
raw = open(log, 'rb').read().decode('latin1').replace('\r', '')
NOISE = re.compile(r'^\s*(\]F\$|M [APF] r=|\[q9dbg\]|\$ ?$)')
def block(n):
    m = re.search(r'\nzq9b%d\n(.*?)\nzq9e%d' % (n, n), raw, re.S)
    if not m:
        return None
    # Trace-Zeilen des Developer-Kernels (]F$SRqMem, M A ...) und Eingabeaufforderungen
    # stehen zwischen den Zeilen; Terminalecho der eingetippten Zeile ("$ echo ...") auch.
    return '\n'.join(l for l in m.group(1).split('\n') if not NOISE.match(l) and not l.lstrip().startswith('$ '))
bad = []
def check(name, cond, detail=''):
    print(('ok    ' if cond else 'FAIL  ') + '[%s] %s%s' % (variant, name, (' -- ' + detail) if (detail and not cond) else ''))
    if not cond: bad.append(name)
check('Kernelbau (%s Byte), Boot, tsmon, login' % ksize, 'logged on' in raw)
b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13 = block(1), block(2), block(3), block(4), block(5), block(6), block(7), block(8), block(9), block(10), block(11), block(12), block(13)
def rawblock(n):
    # Ungefilterter Text zwischen den Marken. block() wirft Zeilen weg, die mit "$ " beginnen (Terminalecho); die Kindshell von
    # "mshell <loop" schreibt aber selbst "$ " vor jede Ausgabe (Eingabe aus einer Datei, kein Echo).
    m = re.search(r'\nzq9b%d\n(.*?)\nzq9e%d' % (n, n), raw, re.S)
    return m.group(1) if m else None
r8 = rawblock(8)
check('echo: Ausgabe "q9 regress ok"', b1 is not None and any(l.strip() == 'q9 regress ok' for l in b1.split('\n')), repr(b1))
check('pd: gibt das Datenverzeichnis /dd aus', b2 is not None and any(l.strip() == '/dd' for l in b2.split('\n')), repr(b2))
names = set(re.findall(r'[A-Za-z0-9_.]+', b3 or ''))
check('dir /dd/SYS: motd, password, startup', {'motd', 'password', 'startup'} <= names, repr(b3))
check('date: Jahreszahl und Uhrzeit', b4 is not None and re.search(r'20\d\d', b4) and re.search(r'\d+:\d\d', b4), repr(b4))
check('icptsleep: anstehendes Signal bei F$Sleep wird zugestellt (A9)', b5 is not None and 'intercept ok' in b5 and 'sleep done' in b5, repr(b5))
check('icptrte: zweites Signal bei F$RTE wird zugestellt (A10)', b6 is not None and all(w in b6 for w in ('h32', 'h33', 'main done')), repr(b6))
check('icptwait: F$Wait mit anstehendem Signal (A11)', b7 is not None and all(w in b7 for w in ('w intercept', 'wait signal ok', 'child reaped')), repr(b7))
check('mshell <loop: Skript aus umgeleiteter Eingabe (A13)', r8 is not None and 'skript eins' in r8 and 'skript zwei' in r8, repr(r8))
check('tlinkerr: F$TLink-Fehlerpfade (B2): M$Init-Fehler zweimal, Speichermangel, Rueckbau', b9 is not None and all(w in b9 for w in ('tlink init err=A5', 'tlink init again err=A5', 'tlink mem err=CF', 'tlinkerr done 00')), repr(b9))
check('dbgtest: F$DFork/F$DExec/F$DExit live (C2): Einzelschritt, Haltepunkte, Fehlerfaelle',
      b10 is not None and all(w in b10 for w in (
          'dfork err=00', 'step1 executed=02', 'step1 remaining=00', 'step1 d3=01', 'step1 pc+=04', 'step2 d3=02', 'step2 pc+=06',
          'run carry=01', 'dexit err=00', 'dexit again err=E0', 'bp executed=04', 'bp remaining=00', 'bp d3=03', 'bp pc+=08',
          'bp2 executed=05', 'bp2 remaining=63', 'bp2 pc+=0A', 'bad pid err=E0', 'dexit2 err=00', 'dbgtest done 00')), repr(b10))
sem_lines = [l.strip() for l in (b11 or '').split('\n')]
sem_ok = b11 is not None and all(w in sem_lines for w in (
    'reserve z=01', 'child A blocking', 'child B blocking', 'queue length=02', 'bad op err=D0', 'bad ptr err=D2', 'v err=00',
    'child A acquired', 'child B acquired', 'wait1 status=00', 'wait2 status=00', 'queue length end=00', 'semtest done 00'))
sem_order = sem_ok and sem_lines.index('child A blocking') < sem_lines.index('child B blocking') < sem_lines.index('v err=00') \
    < sem_lines.index('child A acquired') < sem_lines.index('child B acquired')
check('semtest: F$Sema P/V live mit zwei wartenden Kindern, FIFO-Weckung (C3)', sem_ok and sem_order, repr(b11))
def alarm_val(name):
    m = re.search(r'^' + re.escape(name) + r'=([0-9A-F]{2})$', b12 or '', re.M)
    return m.group(1) if m else None
alarm_expect = (('set elapsed', ('02', '03')), ('set sig', ('28',)), ('set256 elapsed', ('02', '03')), ('cycle elapsed', ('03', '04')),
                ('cycle count', ('03',)), ('cycle stopped', ('01',)), ('reset elapsed', ('01', '02')), ('reset sig', ('29',)),
                ('delete all stays', ('01',)), ('delete bad err', ('00',)), ('atjul elapsed', ('02', '03', '04')),
                ('atdate elapsed', ('02', '03', '04')), ('set zero err', ('D2',)), ('bad func err', ('D0',)), ('reset bad err', ('D2',)))
alarm_bad = [n for n, ok in alarm_expect if alarm_val(n) not in ok]
if not re.search(r'^alarmtest done 00$', b12 or '', re.M):
    alarm_bad.append('alarmtest done 00')
check('alarmtest: F$Alarm-Matrix live (Set, 256stel, Cycle, Delete, Reset, AtJul, AtDate, Fehler) (C3)', b12 is not None and not alarm_bad,
      'abweichend: %s in %r' % (', '.join('%s=%s' % (n, alarm_val(n)) for n in alarm_bad), b12))
if variant == 'development':
    ext_ok = all(re.search(r'^!%s (?i:%s)\s*$' % (c, n), raw, re.M) for c, n in (('X', 'os9p2'), ('x', 'syscache'), ('x', 'ssm'), ('x', 'fpu')))
    check('Erweiterungs-Scan (C4): os9p2 aufgerufen, syscache/ssm/fpu bewusst uebersprungen, OS9P3 nicht vorhanden und ohne Wirkung', ext_ok,
          '; '.join(l for l in raw.split('\n') if l.startswith('!X') or l.startswith('!x'))[:200])
p2_lines = [l.strip() for l in (b13 or '').split('\n')]
check('p2test: Erweiterungsmodul os9p2 beim Boot ausgefuehrt, Dienst $70 und sein statischer Speicher (C4)',
      b13 is not None and all(w in p2_lines for w in ('p2 svc=42', 'p2 svc again=42', 'p2 unknown err=D0', 'p2test done 00')), repr(b13))
regchk = [l.strip() for l in raw.split('\n') if l.startswith('!R ')]
check('keine Register-Erhaltungsmeldung (!R, Arbeitsplan A5)', not regchk, '; '.join(regchk[:3]))
body = '\n'.join(x for x in (b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13) if x)
errs = [e for e in ('Error #', "can't", 'Unhandled', 'syntax error', 'E$') if e in body]
check('keine Fehlermeldung in den Befehlsausgaben', not errs, ' '.join(errs))
sys.exit(1 if bad else 0)
PY
prc=$?
# 5. Q9-Flux-Dump: keine Exception.
if [ -f "$dump" ]; then
    if grep -q 'Vektor=0 (Fmt' "$dump"; then echo "ok    [$VARIANT] Debug-Dump: Vektor=0 (keine Exception)"
    else echo "FAIL  [$VARIANT] Debug-Dump meldet eine Exception: $(grep -m1 'Vektor=' "$dump")"; prc=1; fi
else
    echo "FAIL  [$VARIANT] kein Debug-Dump geschrieben"; prc=1
fi
[ $prc -eq 0 ] && echo "ALLE TESTS OK (login_shell_test.sh $VARIANT)"
exit $prc
