#!/usr/bin/env python3
"""
fixmod.py -- Berechnet die OS-9 Modul-CRC (24-Bit) und trägt sie am Modulende ein.
"""

import sys
import struct

def os9_crc(data):
    crc = 0xFFFFFF
    for b in data:
        crc ^= (b << 16)
        for _ in range(8):
            crc <<= 1
            if crc & 0x1000000:
                crc ^= 0x1800063
    crc &= 0xFFFFFF
    return crc ^ 0xFFFFFF

def fixmod(path):
    with open(path, "rb") as f:
        mod = bytearray(f.read())

    if len(mod) < 16 or mod[0:2] != b"\x4a\xfc":
        print(f"Error: {path} ist kein OS-9 Modul (Sync-Bytes $4AFC fehlen)")
        sys.exit(1)

    mod_size = struct.unpack_from(">I", mod, 4)[0]
    if len(mod) < mod_size:
        print(f"Error: Datei ist kleiner ({len(mod)}) als im Header angegeben ({mod_size})")
        sys.exit(1)

    crc = os9_crc(mod[:mod_size - 3])
    mod[mod_size - 3] = (crc >> 16) & 0xFF
    mod[mod_size - 2] = (crc >> 8) & 0xFF
    mod[mod_size - 1] = crc & 0xFF

    with open(path, "wb") as f:
        f.write(mod)

    print(f"[OK] {path}: CRC auf ${crc:06X} aktualisiert (Modulgröße {mod_size} Bytes).")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <module_file>")
        sys.exit(1)
    fixmod(sys.argv[1])
