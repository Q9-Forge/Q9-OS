#!/usr/bin/env python3
"""
mk_pipeboot.py -- Werkzeug zum Einbinden von pipeman.mod in eine OS-9 Bootdatei (OS9Boot).
Ersetzt das Standard-pipeman-Modul durch das neu gebaute native Q9-PIPE-Modul.
"""

import os
import struct
import sys

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

def build_pipeboot(src_boot, pipe_mod_path, out_boot):
    print(f"=== Erstelle Test-Bootdatei mit eigenem Q9-PIPE (pipeman) ===")
    print(f"Quelldatei:  {src_boot}")
    print(f"PIPE Modul:  {pipe_mod_path}")
    print(f"Zieldatei:   {out_boot}")

    with open(src_boot, "rb") as f:
        boot_data = f.read()

    with open(pipe_mod_path, "rb") as f:
        new_pipe = f.read()

    modules, trailing = parse_bootfile(boot_data)
    print(f"Gefundene Module in {os.path.basename(src_boot)}: {len(modules)}")

    replaced = False
    new_modules = []
    for name, mdata in modules:
        if name.lower() == "pipeman":
            print(f" -> Ersetze Modul '{name}' ({len(mdata)} Bytes) durch neues Q9-PIPE ({len(new_pipe)} Bytes)")
            new_modules.append((name, new_pipe))
            replaced = True
        else:
            new_modules.append((name, mdata))

    if not replaced:
        print(" -> 'pipeman' nicht gefunden, hänge Q9-PIPE am Ende der Modulliste an.")
        new_modules.append(("pipeman", new_pipe))

    # Bootdatei zusammensetzen
    out_data = bytearray()
    for name, mdata in new_modules:
        out_data.extend(mdata)
    out_data.extend(trailing)

    with open(out_boot, "wb") as f:
        f.write(out_data)

    print(f"[OK] Neue Bootdatei geschrieben: {out_boot} ({len(out_data)} Bytes, Delta: {len(out_data) - len(boot_data)} Bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        # Default-Pfade für Q9
        src = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-SDK/SRC/SYSTEM/Q9-OS/RBF68k/tools/OS9Boot_rbf68k_test"
        mod = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-SDK/SRC/SYSTEM/Q9-OS/Q9-Manager/Q9-PIPE/src/pipeman.mod"
        out = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-SDK/SRC/SYSTEM/Q9-OS/Q9-Manager/Q9-PIPE/tools/OS9Boot_pipe_test"
    else:
        src, mod, out = sys.argv[1:4]

    build_pipeboot(src, mod, out)
