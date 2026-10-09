#!/usr/bin/env python3
"""q9trace_decode.py -- dekodiert den Syscall-Trace-Ringpuffer des Q9-Kernels
aus einem Q9-Flux-Dump (local_images/q9dbg_dump.txt).

Der Dump enthaelt den Puffer roh ("--- Q9-Trace-Puffer addr=... ---" und
Zeilen "T <offset> <hex>"). Gelesen wird ab dem Leseoffset "used" Byte weit,
ringfoermig -- genau der Inhalt, den F$Q9Dbg Unterfunktion 6 liefern wuerde.
Satzformat s. Q9-KERNEL/68k/src/kernel/q9trace.h (12-Byte-Kopf: Typ, Laenge,
Callcode, Flags[Tiefe 0-3, Carry 4, gekuerzt 5], PID.w, Tick.l, Feinzeit.w).

Eingabe: Textdump aus Q9-Flux-Logs (siehe oben) ODER roher Binaer-Dump (.bin):
ein linearer Strom aus Saetzen, beginnend am aeltesten Satz (so liefert ihn
F$Q9Dbg Unterfunktion 6). Die Art wird an Endung/Inhalt erkannt, --bin und
--text erzwingen sie.

Ausgabe je Satz: Zeitstempel (Tick.Feinzeit), PID, Einrueckung nach Tiefe
(2 Leerzeichen je Stufe), Richtung, Callcode-Name, bei Rueckkehr Carry-Status.

Aufruf: q9trace_decode.py <dump> [--bin|--text] [--pid ID] [--call NAME]
                          [--min-depth N] [--last N]
"""
import argparse
import re
import sys

# Kurzname je Callcode (OS-9/68K funcs.a; nur die gaengigen).
NAMES = {
    0x00: 'F$Link', 0x01: 'F$Load', 0x02: 'F$UnLink', 0x03: 'F$Fork', 0x04: 'F$Wait',
    0x05: 'F$Chain', 0x06: 'F$Exit', 0x07: 'F$Mem', 0x08: 'F$Send', 0x09: 'F$Icpt',
    0x0A: 'F$Sleep', 0x0B: 'F$SSpd', 0x0C: 'F$ID', 0x0D: 'F$SPrior', 0x0E: 'F$STrap',
    0x0F: 'F$PErr', 0x10: 'F$PrsNam', 0x11: 'F$CmpNam', 0x12: 'F$SchBit', 0x13: 'F$AllBit',
    0x14: 'F$DelBit', 0x15: 'F$Time', 0x16: 'F$STime', 0x17: 'F$CRC', 0x18: 'F$GPrDsc',
    0x19: 'F$GBlkMp', 0x1A: 'F$GModDr', 0x1B: 'F$CpyMem', 0x1C: 'F$SUser', 0x1D: 'F$UnLoad',
    0x1E: 'F$RTE', 0x1F: 'F$GPrDBT', 0x20: 'F$Julian', 0x21: 'F$TLink', 0x22: 'F$DFork',
    0x23: 'F$DExec', 0x24: 'F$DExit', 0x25: 'F$DatMod', 0x26: 'F$SetCRC', 0x27: 'F$SetSys',
    0x28: 'F$SRqMem', 0x29: 'F$SRtMem', 0x2A: 'F$IRQ', 0x2B: 'F$IOQu', 0x2C: 'F$AProc',
    0x2D: 'F$NProc', 0x2E: 'F$VModul', 0x2F: 'F$FindPD', 0x30: 'F$AllPD', 0x31: 'F$RetPD',
    0x32: 'F$SSvc', 0x33: 'F$IODel', 0x37: 'F$GProcP', 0x38: 'F$Move', 0x3A: 'F$Permit',
    0x54: 'F$Gregor', 0x56: 'F$Alarm', 0x57: 'F$SigMask', 0x58: 'F$ChkMem', 0x5A: 'F$CCtl',
    0x5C: 'F$SRqCMem', 0x5E: 'F$Panic', 0x7F: 'F$Q9Dbg',
    0x80: 'I$Attach', 0x81: 'I$Detach', 0x82: 'I$Dup', 0x83: 'I$Create', 0x84: 'I$Open',
    0x85: 'I$MakDir', 0x86: 'I$ChgDir', 0x87: 'I$Delete', 0x88: 'I$Seek', 0x89: 'I$Read',
    0x8A: 'I$Write', 0x8B: 'I$ReadLn', 0x8C: 'I$WritLn', 0x8D: 'I$GetStt', 0x8E: 'I$SetStt',
    0x8F: 'I$Close',
}
TYPES = {1: '->', 2: '<-', 3: '+>', 4: '<+', 5: '!!', 6: 'PI', 7: 'TB'}


RECORD_NAMES = {1: 'Eintritt', 2: 'Rueckkehr', 3: 'intern-Eintritt',
                4: 'intern-Rueckkehr', 5: 'verloren', 6: 'Prozess-Info', 7: 'Zeitbasis'}
HDR_LEN = 12
TRACE_HDR_RE = re.compile(
    r'--- Q9-Trace-Puffer addr=(\w+) size=(\w+) wr=(\w+) rd=(\w+) used=(\w+) '
    r'written=(\w+) lost=(\w+) enabled=(\w+)')


def load(path):
    """Textdump -> (Kopfdaten, Pufferbytes) oder (None, b'')."""
    hdr = None
    data = bytearray()
    with open(path, errors='replace') as f:
        for line in f:
            m = TRACE_HDR_RE.match(line)
            if m:
                hdr = {k: int(v, 16) for k, v in zip(
                    ('addr', 'size', 'wr', 'rd', 'used', 'written', 'lost', 'enabled'),
                    m.groups())}
                continue
            if hdr and line.startswith('T '):
                parts = line.split()
                if len(parts) >= 3:
                    data += bytes.fromhex(parts[2])
    return hdr, bytes(data)


def ring_stream(hdr, data):
    """Linearisiert den Ringpuffer ab Leseoffset, 'used' Byte weit."""
    size, rd, used = hdr['size'], hdr['rd'], hdr['used']
    if size <= 0 or not data:
        return b''
    return bytes(data[(rd + i) % len(data)] for i in range(min(used, size)))


def is_text_dump(path):
    with open(path, 'rb') as f:
        return b'--- Q9-Trace-Puffer' in f.read()


def records(stream):
    """Zerlegt einen linearen Satzstrom. Liefert Tupel (Zustand, Offset, Bytes);
    Zustand 'ok', 'kaputt' (Laenge < Kopf, Abbruch) oder 'abgeschnitten'
    (Satz reicht ueber das Stromende, Abbruch)."""
    off = 0
    n = len(stream)
    while off < n:
        if off + HDR_LEN > n:
            yield ('abgeschnitten', off, stream[off:])
            return
        rlen = stream[off + 1]
        if rlen < HDR_LEN:
            yield ('kaputt', off, stream[off:off + HDR_LEN])
            return
        if off + rlen > n:
            yield ('abgeschnitten', off, stream[off:])
            return
        yield ('ok', off, stream[off:off + rlen])
        off += rlen


def parse(rec):
    """Satzbytes -> dict mit den Kopffeldern und den Nutzdaten."""
    flags = rec[3]
    return {
        'type': rec[0], 'len': rec[1], 'code': rec[2], 'flags': flags,
        'depth': flags & 0x0F, 'carry': bool(flags & 0x10), 'trunc': bool(flags & 0x20),
        'pid': (rec[4] << 8) | rec[5],
        'tick': int.from_bytes(rec[6:10], 'big'),
        'fine': (rec[10] << 8) | rec[11],
        'payload': bytes(rec[HDR_LEN:rec[1]]),
    }


def call_name(r):
    if r['type'] in (3, 4):
        return 'FN$%02X' % r['code']
    return NAMES.get(r['code'], '$%02X' % r['code'])


def _norm(name):
    return name.strip().lower().replace('$', '')


def call_matches(r, wanted):
    """--call: Name ohne Beachtung von Gross-/Kleinschreibung und '$'
    ('I$Open', 'iopen', 'i$open'), oder Zahl ('0x84', '$84')."""
    if r['type'] not in (1, 2, 3, 4):
        return False
    w = wanted.strip()
    try:
        num = int(w[1:], 16) if w.startswith('$') else int(w, 0)
        return r['code'] == num
    except ValueError:
        pass
    return _norm(call_name(r)) == _norm(w)


def select(recs, pid=None, call=None, min_depth=None, last=None):
    """Filtert dekodierte Saetze. Satztyp 5/7 (Verlust/Zeitbasis) bleiben immer
    sichtbar; Typ 6 (Prozess-Info) unterliegt nur dem PID-Filter."""
    out = []
    for r in recs:
        t = r['type']
        if t in (5, 7):
            out.append(r)
            continue
        if pid is not None and r['pid'] != pid:
            continue
        if t != 6:
            if call is not None and not call_matches(r, call):
                continue
            if min_depth is not None and r['depth'] < min_depth:
                continue
        elif call is not None or min_depth is not None:
            continue
        out.append(r)
    if last is not None and last >= 0:
        out = out[-last:] if last else []
    return out


def format_record(r):
    t = r['type']
    stamp = '%8d.%04x' % (r['tick'], r['fine'])
    if t == 5:
        n = int.from_bytes(r['payload'][:4], 'big')
        return '%s  !! %d Saetze verloren' % (stamp, n)
    if t == 7:
        return '%s  TB Zeitbasis raw=%s' % (stamp, r['payload'][:4].hex())
    if t == 6:
        u = int.from_bytes(r['payload'][:2], 'big')
        return '%s  pid %2d  PI Benutzer %d.%d' % (stamp, r['pid'], u >> 8, u & 0xFF)
    line = '%s  pid %2d  %s%s %s' % (stamp, r['pid'], '  ' * r['depth'],
                                     TYPES.get(t, '?'), call_name(r))
    if t in (2, 4):
        line += ' CARRY' if r['carry'] else ' ok'
    if r['trunc']:
        line += ' (gekuerzt)'
    return line


def decode_stream(stream):
    """Satzstrom -> (Satzliste, Warnzeilen)."""
    recs, warn = [], []
    for state, off, raw in records(stream):
        if state == 'ok':
            recs.append(parse(raw))
        elif state == 'kaputt':
            warn.append('  ?? unlesbarer Satz bei %d: %s' % (off, raw.hex()))
        else:
            warn.append('  ?? abgeschnittener Satz bei %d: %s' % (off, raw.hex()))
    return recs, warn


def main(argv=None):
    ap = argparse.ArgumentParser(description='Dekodiert den Q9-Syscall-Trace.')
    ap.add_argument('dump', help='Q9-Flux-Textdump oder rohe .bin-Datei')
    kind = ap.add_mutually_exclusive_group()
    kind.add_argument('--bin', action='store_true', help='als rohen Binaerstrom lesen')
    kind.add_argument('--text', action='store_true', help='als Textdump lesen')
    ap.add_argument('--pid', type=int, help='nur diese Prozess-ID')
    ap.add_argument('--call', help='nur diesen Callcode (Name oder Zahl)')
    ap.add_argument('--min-depth', type=int, help='nur Saetze ab dieser Tiefe')
    ap.add_argument('--last', type=int, help='nur die letzten N Zeilen (nach Filter)')
    args = ap.parse_args(argv)

    as_bin = args.bin or (not args.text and (args.dump.lower().endswith('.bin')
                                             or not is_text_dump(args.dump)))
    if as_bin:
        with open(args.dump, 'rb') as f:
            stream = f.read()
        print('Binaerdump, %d Byte' % len(stream))
    else:
        hdr, data = load(args.dump)
        if not hdr:
            print('kein Trace-Puffer im Dump (Developer-Kernel? Trace eingeschaltet?)')
            return 1
        print('Puffer %(addr)08x, %(size)d Byte, belegt %(used)d, geschrieben %(written)d, '
              'verloren %(lost)d, an=%(enabled)d' % hdr)
        stream = ring_stream(hdr, data)
    recs, warn = decode_stream(stream)
    sel = select(recs, args.pid, args.call, args.min_depth, args.last)
    lines = [format_record(r) for r in sel] + warn
    if lines:
        print('\n'.join(lines))
    return 0


if __name__ == '__main__':
    sys.exit(main())
