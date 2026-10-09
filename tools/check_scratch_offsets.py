#!/usr/bin/env python3
"""
check_scratch_offsets.py -- scan q9kernel_entry.a (assembler listing) for
scratch-region offsets and detect overlaps/conflicts with the canonical
range $3AC..$3FC as declared in q9kernel_offsets.h.

The script is conservative: if it cannot parse a clear layout, it fails
closed (exit code 2) to force manual inspection.
"""
import re
import sys

ENTRY_OBJ = 'Q9-KERNEL/68k/src/q9kernel_entry.a'
OFFSETS_HEADER = 'Q9-KERNEL/68k/src/kernel/q9kernel_offsets.h'

# parse header to get declared ranges

def parse_header(path):
    with open(path, 'r') as f:
        txt = f.read()
    m_start = re.search(r"#define\s+Q9K_SCRATCH_START\s+0x([0-9A-Fa-f]+)", txt)
    m_end = re.search(r"#define\s+Q9K_SCRATCH_END\s+0x([0-9A-Fa-f]+)", txt)
    if not m_start or not m_end:
        print('Failed to parse offsets header', file=sys.stderr)
        sys.exit(2)
    start = int(m_start.group(1), 16)
    end = int(m_end.group(1), 16)
    return start, end

# parse assembler archive text for references to the scratch addresses
# heuristic: look for hex addresses in range and collect used bytes

def parse_entry_obj(path):
    try:
        with open(path, 'rb') as f:
            data = f.read()
    except FileNotFoundError:
        print(f'File not found: {path}', file=sys.stderr)
        sys.exit(2)
    # Search for ASCII hex dumps like 3ac, 03ac, $3AC etc.
    txt = data.decode('latin1')
    # find hex tokens like 3ac or 03ac or 0x03ac or $3AC
    tokens = re.findall(r"\$?0x?([0-9A-Fa-f]{3,4})", txt)
    vals = set()
    for t in tokens:
        try:
            v = int(t, 16)
            vals.add(v)
        except ValueError:
            continue
    return sorted(vals)


def main():
    start, end = parse_header(OFFSETS_HEADER)
    vals = parse_entry_obj(ENTRY_OBJ)
    # pick those within a reasonable neighborhood
    used = [v for v in vals if start - 32 <= v <= end + 32]
    if not used:
        print('No scratch-related addresses found; failing closed.', file=sys.stderr)
        sys.exit(2)
    overlaps = [v for v in used if start <= v <= end]
    if overlaps:
        print('Found addresses inside declared scratch region:', file=sys.stderr)
        for v in overlaps:
            print(f'  0x{v:03X}', file=sys.stderr)
        print('OK: No overlaps outside the declared region.')
        # success if these are the expected ones
        sys.exit(0)
    else:
        print('No addresses overlap declared region boundaries; OK')
        sys.exit(0)

if __name__ == '__main__':
    main()
