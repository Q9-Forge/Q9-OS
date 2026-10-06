#!/usr/bin/env python3
"""q9trace_decode.py -- dekodiert den Syscall-Trace-Ringpuffer des Q9-Kernels
aus einem Q9-Flux-Dump (local_images/q9dbg_dump.txt).

Der Dump enthaelt den Puffer roh ("--- Q9-Trace-Puffer addr=... ---" und
Zeilen "T <offset> <hex>"). Gelesen wird ab dem Leseoffset "used" Byte weit,
ringfoermig -- genau der Inhalt, den F$Q9Dbg Unterfunktion 6 liefern wuerde.
Satzformat s. Q9-KERNEL/68k/src/kernel/q9trace.h (12-Byte-Kopf: Typ, Laenge,
Callcode, Flags[Tiefe 0-3, Carry 4, gekuerzt 5], PID.w, Tick.l, Feinzeit.w).

Aufruf: q9trace_decode.py <dump> [--last N] [--pid P]
"""
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
    stream = bytes(data[(rd + i) % size] for i in range(used))
    off = 0
    while off + 12 <= len(stream):
        rlen = stream[off + 1]
        if rlen < 12:
            yield ('kaputt', off, stream[off:off + 12])
            return
        yield ('ok', off, stream[off:off + rlen])
        off += rlen


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    last = None
    pidf = None
    args = sys.argv[2:]
    if '--last' in args:
        last = int(args[args.index('--last') + 1])
    if '--pid' in args:
        pidf = int(args[args.index('--pid') + 1])
    hdr, data = load(sys.argv[1])
    if not hdr:
        print('kein Trace-Puffer im Dump (Developer-Kernel? Trace eingeschaltet?)')
        return 1
    print('Puffer %(addr)08x, %(size)d Byte, belegt %(used)d, geschrieben %(written)d, '
          'verloren %(lost)d, an=%(enabled)d' % hdr)
    out = []
    for state, off, rec in records(hdr, data):
        if state != 'ok':
            out.append('  ?? unlesbarer Satz bei %d: %s' % (off, rec.hex()))
            break
        rtype, rlen, code, flags = rec[0], rec[1], rec[2], rec[3]
        pid = (rec[4] << 8) | rec[5]
        tick = int.from_bytes(rec[6:10], 'big')
        if pidf is not None and pid != pidf:
            continue
        if rtype == 5:
            out.append('%8d  !! %d Saetze verloren' % (tick, int.from_bytes(rec[12:16], 'big')))
            continue
        depth = flags & 0x0F
        name = NAMES.get(code, '$%02X' % code)
        err = ' CARRY' if flags & 0x10 else ''
        out.append('%8d  pid %2d  %s%s %s%s' % (tick, pid, '  ' * depth, TYPES.get(rtype, '?'), name, err))
    if last:
        out = out[-last:]
    print('\n'.join(out))
    return 0


if __name__ == '__main__':
    sys.exit(main())
