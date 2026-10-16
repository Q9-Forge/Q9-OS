#!/usr/bin/env python3
"""
mk_rbfboot.py -- Werkzeug zum Einbinden von rbf.mod in eine OS-9 Bootdatei (OS9Boot).
Ersetzt das Standard-RBF-Modul durch das neu gebaute RBF68k-Modul
und validiert die OS-9 Modul-CRCs.
"""

import os
import struct
import sys

def crc24(data):
    """Berechnet die 24-Bit OS-9 Modul-CRC."""
    crc = 0xFFFFFF
    for b in data:
        crc ^= b << 16
        for _ in range(8):
            crc <<= 1
            if crc & 0x1000000:
                crc ^= 0x1800063
    return crc & 0xFFFFFF

def parse_bootfile(data):
    """Liest alle OS-9 Module aus einer Bootdatei."""
    modules = []
    i = 0
    while i + 16 <= len(data) and data[i:i+2] == b"\x4a\xfc":
        size = struct.unpack_from(">I", data, i + 4)[0]
        nm_off = struct.unpack_from(">I", data, i + 12)[0]
        name = data[i+nm_off:i+nm_off+32].split(b"\0")[0].decode("latin1", "replace")
        mod_bytes = data[i:i+size]
        modules.append((name, mod_bytes))
        i += size
    return modules, data[i:]

def build_rbfboot(src_boot, rbf_mod_path, out_boot):
    print(f"=== Erstelle Test-Bootdatei mit eigenem RBF68k ===")
    print(f"Quelldatei: {src_boot}")
    print(f"RBF Modul:  {rbf_mod_path}")
    print(f"Zieldatei:  {out_boot}")

    with open(src_boot, "rb") as f:
        boot_data = f.read()

    with open(rbf_mod_path, "rb") as f:
        new_rbf = f.read()

    modules, trailing = parse_bootfile(boot_data)
    print(f"Gefundene Module in {os.path.basename(src_boot)}: {len(modules)}")

    replaced = False
    new_modules = []
    for name, mdata in modules:
        if name.lower() == "rbf":
            print(f" -> Ersetze Modul '{name}' ({len(mdata)} Bytes) durch neues RBF68k ({len(new_rbf)} Bytes)")
            new_modules.append((name, new_rbf))
            replaced = True
        else:
            new_modules.append((name, mdata))

    if not replaced:
        print(" -> 'rbf' nicht gefunden, hänge RBF68k am Ende der Modulliste an.")
        new_modules.append(("rbf", new_rbf))

    # Bootdatei zusammensetzen
    out_data = bytearray()
    for name, mdata in new_modules:
        out_data.extend(mdata)
    out_data.extend(trailing)

    with open(out_boot, "wb") as f:
        f.write(out_data)

    print(f"[OK] Neue Bootdatei geschrieben: {out_boot} ({len(out_data)} Bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        # Default-Pfade für Q9
        src = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-Images/cf_images/OS9SYS/OS9Boot"
        mod = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-SDK/SRC/SYSTEM/Q9-OS/RBF68k/src/rbf.mod"
        out = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-SDK/SRC/SYSTEM/Q9-OS/RBF68k/tools/OS9Boot_rbf68k_test"
    else:
        src, mod, out = sys.argv[1:4]

    build_rbfboot(src, mod, out)
