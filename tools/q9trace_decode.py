#!/usr/bin/env python3
"""q9trace_decode.py -- dekodiert den Syscall-Trace-Ringpuffer des Q9-Kernels
aus einem Q9-Flux-Dump (local_images/q9dbg_dump.txt).

Der Dump enthaelt den Puffer roh ("--- Q9-Trace-Puffer addr=... ---" und
Zeilen "T <offset> <hex>"). Gelesen wird ab dem Leseoffset "used" Byte weit,
ringfoermig -- genau der Inhalt, den F$Q9Dbg Unterfunktion 6 liefern wuerde.
Satzformat s. Q9-KERNEL/68k/src/kernel/q9trace.h (12-Byte-Kopf: Typ, Laenge,
Callcode, Flags[Tiefe 0-3, Carry 4, gekuerzt 5], PID.w, Tick.l, Feinzeit.w).

Ab Detailstufe 2 (`trace on` setzt sie) tragen Eintritts- und Rueckkehrsaetze
d0,d1,d2,a0,a1 des Aufrufers (Eingaben bzw. Ausgaben), Eintritte mit
Namensparameter zusaetzlich den Namen (q9kernel_dbg.c, Q9DBG_PAY_REGS).

Aufruf: q9trace_decode.py <dump> [Optionen]
  --last N       nur die letzten N Zeilen
  --pid P        nur Prozess P
  --code +F$Open -F$ID ...   nur diese (+) bzw. ohne diese (-) Aufrufe;
                 ohne Vorzeichen = nur diese
  --err          nur Aufrufe, die mit Carry (Fehler) enden
  --name TEXT    nur Aufrufe, deren Namensparameter TEXT enthaelt
  --regs         immer alle fuenf Register ausgeben (Rohansicht)
  --events       Eintritt und Rueckkehr getrennt statt als Aufrufzeile zeigen
"""
import re
import sys

FUNCS = {
    0x00: 'F$Link',
    0x01: 'F$Load',
    0x02: 'F$UnLink',
    0x03: 'F$Fork',
    0x04: 'F$Wait',
    0x05: 'F$Chain',
    0x06: 'F$Exit',
    0x07: 'F$Mem',
    0x08: 'F$Send',
    0x09: 'F$Icpt',
    0x0A: 'F$Sleep',
    0x0B: 'F$SSpd',
    0x0C: 'F$ID',
    0x0D: 'F$SPrior',
    0x0E: 'F$STrap',
    0x0F: 'F$PErr',
    0x10: 'F$PrsNam',
    0x11: 'F$CmpNam',
    0x12: 'F$SchBit',
    0x13: 'F$AllBit',
    0x14: 'F$DelBit',
    0x15: 'F$Time',
    0x16: 'F$STime',
    0x17: 'F$CRC',
    0x18: 'F$GPrDsc',
    0x19: 'F$GBlkMp',
    0x1A: 'F$GModDr',
    0x1B: 'F$CpyMem',
    0x1C: 'F$SUser',
    0x1D: 'F$UnLoad',
    0x1E: 'F$RTE',
    0x1F: 'F$GPrDBT',
    0x20: 'F$Julian',
    0x21: 'F$TLink',
    0x22: 'F$DFork',
    0x23: 'F$DExec',
    0x24: 'F$DExit',
    0x25: 'F$DatMod',
    0x26: 'F$SetCRC',
    0x27: 'F$SetSys',
    0x28: 'F$SRqMem',
    0x29: 'F$SRtMem',
    0x2A: 'F$IRQ',
    0x2B: 'F$IOQu',
    0x2C: 'F$AProc',
    0x2D: 'F$NProc',
    0x2E: 'F$VModul',
    0x2F: 'F$FindPD',
    0x30: 'F$AllPD',
    0x31: 'F$RetPD',
    0x32: 'F$SSvc',
    0x33: 'F$IODel',
    0x37: 'F$GProcP',
    0x38: 'F$Move',
    0x39: 'F$AllRAM',
    0x3A: 'F$Permit',
    0x3B: 'F$Protect',
    0x3C: 'F$SetImg',
    0x3D: 'F$FreeLB',
    0x3E: 'F$FreeHB',
    0x3F: 'F$AllTsk',
    0x40: 'F$DelTsk',
    0x41: 'F$SetTsk',
    0x42: 'F$ResTsk',
    0x43: 'F$RelTsk',
    0x44: 'F$DATLog',
    0x45: 'F$DATTmp',
    0x46: 'F$LDAXY',
    0x47: 'F$LDAXYP',
    0x48: 'F$LDDDXY',
    0x49: 'F$LDABX',
    0x4A: 'F$STABX',
    0x4B: 'F$AllPrc',
    0x4C: 'F$DelPrc',
    0x4D: 'F$ELink',
    0x4E: 'F$FModul',
    0x4F: 'F$MapBlk',
    0x50: 'F$ClrBlk',
    0x51: 'F$DelRAM',
    0x52: 'F$SysDbg',
    0x53: 'F$Event',
    0x54: 'F$Gregor',
    0x55: 'F$SysID',
    0x56: 'F$Alarm',
    0x57: 'F$SigMask',
    0x58: 'F$ChkMem',
    0x59: 'F$UAcct',
    0x5A: 'F$CCtl',
    0x5B: 'F$GSPUMp',
    0x5C: 'F$SRqCMem',
    0x5D: 'F$POSK',
    0x5E: 'F$Panic',
    0x5F: 'F$MBuf',
    0x60: 'F$Trans',
    0x61: 'F$FIRQ',
    0x62: 'F$Sema',
    0x63: 'F$SigReset',
    0x64: 'F$DAttach',
    0x65: 'F$Flash',
    0x66: 'F$PwrMan',
    0x67: 'F$Crypt',
    0x70: 'F$HLProto',
    0x80: 'I$Attach',
    0x81: 'I$Detach',
    0x82: 'I$Dup',
    0x83: 'I$Create',
    0x84: 'I$Open',
    0x85: 'I$MakDir',
    0x86: 'I$ChgDir',
    0x87: 'I$Delete',
    0x88: 'I$Seek',
    0x89: 'I$Read',
    0x8A: 'I$Write',
    0x8B: 'I$ReadLn',
    0x8C: 'I$WritLn',
    0x8D: 'I$GetStt',
    0x8E: 'I$SetStt',
    0x8F: 'I$Close',
    0x92: 'I$SGetSt',
    0x7F: 'F$Q9Dbg',
}
ERRORS = {
    0xC8: 'E$PTHFUL',
    0xC9: 'E$BPNUM',
    0xCA: 'E$POLL',
    0xCB: 'E$BMODE',
    0xCC: 'E$DEVOVF',
    0xCD: 'E$BMID',
    0xCE: 'E$DIRFUL',
    0xCF: 'E$MEMFUL',
    0xD0: 'E$UNKSVC',
    0xD1: 'E$MODBSY',
    0xD2: 'E$BPADDR',
    0xD3: 'E$EOF',
    0xD4: 'E$VCTBSY',
    0xD5: 'E$NES',
    0xD6: 'E$FNA',
    0xD7: 'E$BPNAM',
    0xD8: 'E$PNNF',
    0xD9: 'E$SLF',
    0xDA: 'E$CEF',
    0xDB: 'E$IBA',
    0xDC: 'E$HANGUP',
    0xDD: 'E$MNF',
    0xDE: 'E$NOCLK',
    0xDF: 'E$DELSP',
    0xE0: 'E$IPRCID',
    0xE1: 'E$PARAM',
    0xE2: 'E$NOCHLD',
    0xE3: 'E$ITRAP',
    0xE4: 'E$PRCABT',
    0xE5: 'E$PRCFUL',
    0xE6: 'E$IFORKP',
    0xE7: 'E$KWNMOD',
    0xE8: 'E$BMCRC',
    0xE9: 'E$SIGNAL',
    0xEA: 'E$NEMOD',
    0xEB: 'E$BNAM',
    0xEC: 'E$BMHP',
    0xED: 'E$NORAM',
    0xEE: 'E$DNE',
    0xEF: 'E$NOTASK',
    0xF0: 'E$UNIT',
    0xF1: 'E$SECT',
    0xF2: 'E$WP',
    0xF3: 'E$CRC',
    0xF4: 'E$READ',
    0xF5: 'E$WRITE',
    0xF6: 'E$NOTRDY',
    0xF7: 'E$SEEK',
    0xF8: 'E$FULL',
    0xF9: 'E$BTYP',
    0xFA: 'E$DEVBSY',
    0xFB: 'E$DIDC',
    0xFC: 'E$LOCK',
    0xFD: 'E$SHARE',
    0xFE: 'E$DEADLK',
    0xFF: 'E$FORMAT'
}

FUNCS[0x7F] = 'F$Q9Dbg'
FUNC_BY_NAME = {v.lower(): k for k, v in FUNCS.items()}
TYPES = {1: '->', 2: '<-', 3: '+>', 4: '<+', 5: '!!', 6: 'PI', 7: 'TB'}

# Registerbedeutung je Aufruf: (Eingaben, Ausgaben) als {Register: Beschriftung}.
# Nur eingetragen, was in OS-9/68K sicher so belegt ist; alles andere erscheint
# (wenn ungleich 0) roh. Name-Aufrufe zeigen den Namen aus dem Satz.
CALLS = {
    0x00: ({'d0': 'typlang'}, {'d0': 'typlang', 'd1': 'attrrev', 'a0': 'nameptr', 'a1': 'entry'}), # F$Link
    0x01: ({'d0': 'modus', 'd1': 'farbe'}, {'d0': 'typlang', 'd1': 'attrrev', 'a1': 'entry'}), # F$Load
    0x02: ({'a2': 'modul'}, {}),                                                 # F$UnLink
    0x03: ({'d0': 'typlang', 'd1': 'extra', 'd2': 'pargroesse', 'a0': 'modptr', 'a1': 'parptr'}, {'d0': 'pid', 'a0': 'nameptr'}), # F$Fork; d3 nicht im Trace-Satz
    0x04: ({}, {'d0': 'pid', 'd1': 'status'}),                                   # F$Wait
    0x06: ({'d1': 'status'}, {}),                                                # F$Exit
    0x07: ({'d0': 'groesse'}, {'d0': 'groesse', 'a1': 'ende'}),                  # F$Mem
    0x08: ({'d0': 'pid', 'd1': 'signal'}, {}),                                   # F$Send
    0x0A: ({'d0': 'ticks'}, {'d0': 'rest'}),                                     # F$Sleep
    0x0C: ({}, {'d0': 'pid', 'd1': 'gruppe.benutzer', 'd2': 'prio'}),            # F$ID
    0x0D: ({'d0': 'pid', 'd1': 'prio'}, {}),                                     # F$SPrior
    0x28: ({'d0': 'groesse'}, {'d0': 'groesse', 'a2': 'adresse'}),               # F$SRqMem
    0x29: ({'d0': 'groesse', 'a2': 'adresse'}, {}),                              # F$SRtMem
    0x2E: ({'a1': 'modul'}, {}),                                                 # F$VModul
    0x80: ({'d0': 'modus'}, {}),                                                 # I$Attach (a2 wird nicht geloggt)
    0x82: ({'d0': 'pfad'}, {'d0': 'pfad'}),                                      # I$Dup
    0x83: ({'d0': 'modus', 'd1': 'attr', 'd2': 'groesse'}, {'d0': 'pfad'}),     # I$Create
    0x84: ({'d0': 'modus'}, {'d0': 'pfad'}),                                     # I$Open
    0x85: ({'d0': 'modus', 'd1': 'zugriff', 'd2': 'groesse'}, {}),               # I$MakDir
    0x86: ({'d0': 'modus'}, {}),                                                # I$ChgDir
    0x87: ({'d0': 'modus'}, {}),                                                # I$Delete
    0x88: ({'d0': 'pfad', 'd1': 'pos'}, {}),                                     # I$Seek
    0x89: ({'d0': 'pfad', 'd1': 'anzahl', 'a0': 'puffer'}, {'d1': 'gelesen'}),   # I$Read
    0x8A: ({'d0': 'pfad', 'd1': 'anzahl', 'a0': 'puffer'}, {'d1': 'geschr'}),    # I$Write
    0x8B: ({'d0': 'pfad', 'd1': 'max', 'a0': 'puffer'}, {'d1': 'gelesen'}),      # I$ReadLn
    0x8C: ({'d0': 'pfad', 'd1': 'max', 'a0': 'puffer'}, {'d1': 'geschr'}),       # I$WritLn
    0x8D: ({'d0': 'pfad', 'd1': 'funktion'}, {}),                                # I$GetStt
    0x8E: ({'d0': 'pfad', 'd1': 'funktion'}, {}),                                # I$SetStt
    0x8F: ({'d0': 'pfad'}, {}),                                                  # I$Close
}
REGS = ('d0', 'd1', 'd2', 'a0', 'a1')
MODULE_NAME_CALLS = {0x00, 0x01, 0x02, 0x03, 0x05, 0x21, 0x22, 0x4D}
PATH_NAME_CALLS = {0x83, 0x84, 0x85, 0x86, 0x87}

def name_label(code):
    if code in MODULE_NAME_CALLS:
        return 'modul'
    if code == 0x80:
        return 'geraet'
    if code in PATH_NAME_CALLS:
        return 'datei' if code in (0x83, 0x84) else 'pfad'
    return 'name'


def load(path):
    hdr = None
    data = bytearray()
    for line in open(path, errors='replace'):
        m = re.match(r'--- Q9-Trace-Puffer addr=(\w+) size=(\w+) wr=(\w+) rd=(\w+) used=(\w+) '
                     r'written=(\w+) lost=(\w+) enabled=(\w+)', line)
        if m:
            hdr = {k: int(v, 16) for k, v in zip(
                ('addr', 'size', 'wr', 'rd', 'used', 'written', 'lost', 'enabled'), m.groups())}
            continue
        if hdr and line.startswith('T '):
            data += bytes.fromhex(line.split()[2])
    return hdr, bytes(data)


def records(hdr, data):
    size, rd, used = hdr['size'], hdr['rd'], hdr['used']
    if size <= 0 or rd >= size or used > size or len(data) < size:
        yield ('kaputt', 0, b'ungueltige Puffergrenzen')
        return
    stream = bytes(data[(rd + i) % size] for i in range(used))
    off = 0
    while off < len(stream):
        if off + 12 > len(stream):
            yield ('kaputt', off, stream[off:])
            return
        rlen = stream[off + 1]
        if rlen < 12 or off + rlen > len(stream):
            yield ('kaputt', off, stream[off:min(off + max(rlen, 12), len(stream))])
            return
        yield ('ok', off, stream[off:off + rlen])
        off += rlen


def parse(rec):
    """Zerlegt einen Satz in ein Dict (Kopf, Register, Name)."""
    r = {
        'type': rec[0], 'len': rec[1], 'code': rec[2], 'flags': rec[3],
        'depth': rec[3] & 0x0F, 'carry': bool(rec[3] & 0x10), 'trunc': bool(rec[3] & 0x20),
        'pid': (rec[4] << 8) | rec[5], 'tick': int.from_bytes(rec[6:10], 'big'),
        'regs': None, 'name': None,
    }
    if r['type'] == 5 and r['len'] >= 16:
        r['lost'] = int.from_bytes(rec[12:16], 'big')
    if r['type'] == 6 and r['len'] >= 14:
        r['user'] = int.from_bytes(rec[12:14], 'big')
    if r['type'] == 7 and r['len'] >= 16:
        r['time_raw'] = int.from_bytes(rec[12:16], 'big')
    if r['type'] in (1, 2) and r['len'] >= 32:
        vals = [int.from_bytes(rec[12 + 4 * i:16 + 4 * i], 'big') for i in range(5)]
        r['regs'] = dict(zip(REGS, vals))
        if r['len'] > 32:
            n = rec[32]
            r['name'] = rec[33:33 + n].decode('latin1')
    return r


def errname(code):
    code &= 0xFFFF
    return ERRORS.get(code, '#%03d' % code)


# Beschriftungen, deren Wert nur ein Wort/Byte des Registers ist, und solche,
# die dezimal gezeigt werden.
MASKS = {'typlang': 0xFFFF, 'modus': 0xFF, 'attr': 0xFF, 'zugriff': 0xFF, 'pfad': 0xFFFF, 'signal': 0xFFFF, 'status': 0xFFFF,
         'prio': 0xFFFF, 'pfade': 0xFFFF, 'pid': 0xFFFF, 'attrrev': 0xFFFF}
DECIMAL = {'pid', 'pfad', 'pfade', 'ticks', 'groesse', 'anzahl', 'gelesen', 'geschr', 'signal',
           'status', 'prio', 'extra', 'pargroesse', 'farbe', 'rest', 'max', 'zugriff'}


def fmt_regs(r, labels, raw):
    """Registerteil einer Zeile: benannte Register mit Beschriftung, die uebrigen
    nur wenn ungleich 0 (oder bei --regs alle)."""
    regs = r['regs']
    if regs is None:
        return ''
    parts = []
    for reg in REGS:
        v = regs[reg]
        if r.get('name') is not None and reg == 'a0' and not raw:
            continue
        if reg in labels:
            lab = labels[reg]
            v &= MASKS.get(lab, 0xFFFFFFFF)
            if lab == 'gruppe.benutzer':
                value = '%d.%d' % ((v >> 16) & 0xFFFF, v & 0xFFFF)
            else:
                value = ('%d' % v) if lab in DECIMAL else '$%X' % v
            parts.append('%s=%s' % (lab, value))
        elif raw or v != 0:
            parts.append('%s=$%X' % (reg, v))
    return ' '.join(parts)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    args = sys.argv[2:]

    def opt(name, n=1):
        if name in args:
            i = args.index(name)
            return args[i + 1:i + 1 + n]
        return None

    last = opt('--last')
    last = int(last[0]) if last else None
    pidf = opt('--pid')
    pidf = int(pidf[0]) if pidf else None
    namef = opt('--name')
    namef = namef[0] if namef else None
    errf = '--err' in args
    raw = '--regs' in args
    events = '--events' in args
    inc, exc = set(), set()
    if '--code' in args:
        for a in args[args.index('--code') + 1:]:
            if a.startswith('--'):
                break
            sign = a[0] if a[0] in '+-' else '+'
            nm = a.lstrip('+-').lower()
            if nm not in FUNC_BY_NAME:
                print('unbekannter Aufruf: %s' % a)
                return 2
            (exc if sign == '-' else inc).add(FUNC_BY_NAME[nm])

    hdr, data = load(sys.argv[1])
    if not hdr:
        print('kein Trace-Puffer im Dump (Developer-Kernel? Trace eingeschaltet?)')
        return 1
    print('Puffer %(addr)08x, %(size)d Byte, belegt %(used)d, geschrieben %(written)d, '
          'verloren %(lost)d, an=%(enabled)d' % hdr)

    recs = []
    for state, off, rec in records(hdr, data):
        if state != 'ok':
            recs.append({'bad': '  ?? unlesbarer Satz bei %d: %s' % (off, rec.hex())})
            break
        recs.append(parse(rec))

    # Eintritt und Rueckkehr paaren: gleicher Prozess + Tiefe, naechster Rueckkehrsatz
    # (Aufrufe ohne Rueckkehr wie F$Sleep/F$Wait/F$Exit bleiben unverbunden).
    open_calls = {}
    for r in recs:
        if 'bad' in r or r['type'] not in (1, 2):
            continue
        key = (r['pid'], r['depth'], r['code'])
        if r['type'] == 1:
            open_calls[key] = r
            r['ret'] = None
        else:
            e = open_calls.pop(key, None)
            if e is not None:
                e['ret'] = r
                r['entry'] = e

    out = []
    for r in recs:
        if 'bad' in r:
            out.append(r['bad'])
            continue
        if r['type'] == 5:
            out.append('%8d  !! %d Saetze verloren' % (r['tick'], r.get('lost', 0)))
            continue
        if r['type'] == 6:
            user = r.get('user')
            if user is None:
                out.append('%8d  pid %2d  PI Benutzerinfo unvollstaendig' % (r['tick'], r['pid']))
            else:
                out.append('%8d  pid %2d  PI Benutzer %d.%d' %
                           (r['tick'], r['pid'], user >> 8, user & 0xFF))
            continue
        if r['type'] == 7:
            raw_time = r.get('time_raw')
            if raw_time is None:
                out.append('%8d  TB Zeitbasis unvollstaendig' % r['tick'])
            else:
                out.append('%8d  TB F$Time-Rohwert=$%08X' % (r['tick'], raw_time))
            continue
        if pidf is not None and r['pid'] != pidf:
            continue
        if r['type'] in (1, 2):
            if not events and r['type'] == 2 and r.get('entry') is not None:
                continue
            if inc and r['code'] not in inc:
                continue
            if r['code'] in exc:
                continue
            owner = r if r['type'] == 1 else r.get('entry')
            ret = r.get('ret') if r['type'] == 1 else r
            if errf and not (ret is not None and ret['carry']):
                continue
            if namef and not (owner is not None and owner['name'] and namef in owner['name']):
                continue
        elif errf or namef or inc:
            continue
        depth = r['depth']
        name = FUNCS.get(r['code'], '$%02X' % r['code'])
        ind = '  ' * depth
        prefix = '%8d  pid %2d  ' % (r['tick'], r['pid'])
        if r['type'] in (1, 2) and not events:
            line = prefix + ind + name
            ins, outs = CALLS.get(r['code'], ({}, {}))
            args_text = fmt_regs(r, ins, raw)
            named = ('%s="%s"%s' % (name_label(r['code']), r['name'], '...' if r['trunc'] else '')) if r['name'] is not None else ''
            fields = [v for v in (named, args_text) if v]
            line += '(' + ', '.join(fields) + ')'
            ret = r.get('ret')
            if ret is not None:
                if ret['carry']:
                    rv = ('Fehler %s (%d)' % (errname(ret['regs']['d1']), ret['regs']['d1'] & 0xFFFF)) if ret['regs'] else 'Fehler'
                    line += ' -> ' + rv
                else:
                    ret_view = dict(ret)
                    ret_view['name'] = r.get('name')
                    rv = fmt_regs(ret_view, outs, raw)
                    if rv: line += ' -> ' + rv
                line += '  %d Ticks' % ((ret['tick'] - r['tick']) & 0xFFFFFFFF)
        else:
            line = prefix + ind + TYPES.get(r['type'], '?') + ' ' + name
            if r['type'] in (1, 2):
                ins, outs = CALLS.get(r['code'], ({}, {}))
                if r['type'] == 1 and r['name'] is not None:
                    line += ' %s="%s"%s' % (name_label(r['code']), r['name'], '...' if r['trunc'] else '')
                event_view = r
                if r['type'] == 2 and r.get('entry') is not None:
                    event_view = dict(r)
                    event_view['name'] = r['entry'].get('name')
                if r['type'] == 2 and r['carry']:
                    if r['regs'] is not None:
                        line += ' Fehler %s (%d)' % (errname(r['regs']['d1']), r['regs']['d1'] & 0xFFFF)
                    else:
                        line += ' Fehler'
                else:
                    regs_text = fmt_regs(event_view, ins if r['type'] == 1 else outs, raw)
                    if regs_text:
                        line += ' ' + regs_text
        out.append(line)
    if last:
        out = out[-last:]
    print('\n'.join(out))
    return 0


if __name__ == '__main__':
    sys.exit(main())
