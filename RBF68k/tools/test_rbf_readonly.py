#!/usr/bin/env python3
"""
test_rbf_readonly.py -- Host-Side Unit-Test für RBF68k Filesystem-Logik
Testet LSN0-Parsing, Directory-Suche, Segment-Mapping und Datei-Lesen
anhand des realen Test-Images q9data-rbf.img.
"""

import os
import struct
import sys

IMAGE_PATH = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-Images/cf_images/q9data-rbf.img"

def test_rbf():
    print(f"=== RBF68k Read-Only Test gegen {IMAGE_PATH} ===")
    assert os.path.exists(IMAGE_PATH), f"Image nicht gefunden: {IMAGE_PATH}"

    with open(IMAGE_PATH, "rb") as f:
        # 1. Sektor 0 lesen und parsen
        f.seek(0)
        sct0 = f.read(512)
        tot_sectors = int.from_bytes(sct0[0:3], "big")
        map_bytes = int.from_bytes(sct0[4:6], "big")
        bit_ratio = int.from_bytes(sct0[6:8], "big")
        root_dir_lsn = int.from_bytes(sct0[8:11], "big")
        sync_code = sct0[96:100]
        map_lsn = int.from_bytes(sct0[100:104], "big")
        lsn_size = int.from_bytes(sct0[104:106], "big") or 512

        print(f"[OK] Sektor 0 gelesen:")
        print(f"     DD_TOT:    {tot_sectors} Sektoren")
        print(f"     DD_DIR:    LSN {root_dir_lsn}")
        print(f"     DD_MapLSN: LSN {map_lsn}")
        print(f"     DD_LSNSize:{lsn_size} Bytes")
        print(f"     DD_Sync:   {sync_code}")

        assert tot_sectors == 4096, f"Falsche Sektoranzahl: {tot_sectors}"
        assert root_dir_lsn == 2, f"Falsche Root-Dir-LSN: {root_dir_lsn}"
        assert sync_code == b"Cruz", f"Falscher Sync-Code: {sync_code}"

        # 2. Root-Verzeichnis FD lesen
        f.seek(root_dir_lsn * lsn_size)
        root_fd = f.read(lsn_size)
        fd_att = root_fd[0]
        fd_siz = int.from_bytes(root_fd[9:13], "big")
        is_dir = bool(fd_att & 0x80)

        print(f"[OK] Root Directory FD gelesen:")
        print(f"     FD_ATT:    0x{fd_att:02x} (Directory={is_dir})")
        print(f"     FD_SIZ:    {fd_siz} Bytes")

        assert is_dir, "Root Directory hat kein Directory-Attribut!"
        assert fd_siz == 96, f"Root Dir Größe erwartet 96, ist {fd_siz}"

        # Segmente lesen
        seg0_lsn = int.from_bytes(root_fd[16:19], "big")
        seg0_cnt = int.from_bytes(root_fd[19:21], "big")
        print(f"     Segment 0: LSN {seg0_lsn}, {seg0_cnt} Sektoren")
        assert seg0_lsn == 3, f"Root Dir Segment 0 LSN erwartet 3, ist {seg0_lsn}"

        # 3. Root-Verzeichnis Daten lesen und nach 'README' suchen
        f.seek(seg0_lsn * lsn_size)
        dir_data = f.read(fd_siz)
        readme_fd_lsn = None

        print(f"[OK] Verzeichniseinträge durchsuchen:")
        for off in range(0, len(dir_data), 32):
            ent = dir_data[off:off+32]
            name_chars = []
            for b in ent[:28]:
                if b == 0: break
                name_chars.append(chr(b & 0x7F))
                if b & 0x80: break
            name = "".join(name_chars)
            fd_lsn = int.from_bytes(ent[28:32], "big")
            print(f"     - '{name}' -> FD LSN {fd_lsn}")
            if name == "README":
                readme_fd_lsn = fd_lsn

        assert readme_fd_lsn == 64, f"README FD LSN erwartet 64, ist {readme_fd_lsn}"

        # 4. README FD lesen und Segment-Mapping prüfen
        f.seek(readme_fd_lsn * lsn_size)
        file_fd = f.read(lsn_size)
        file_att = file_fd[0]
        file_siz = int.from_bytes(file_fd[9:13], "big")
        file_seg_lsn = int.from_bytes(file_fd[16:19], "big")
        file_seg_cnt = int.from_bytes(file_fd[19:21], "big")

        print(f"[OK] README File Descriptor:")
        print(f"     FD_ATT:    0x{file_att:02x}")
        print(f"     FD_SIZ:    {file_siz} Bytes")
        print(f"     Segment 0: LSN {file_seg_lsn}, {file_seg_cnt} Sektoren")

        assert file_siz == 30, f"README Dateigröße erwartet 30, ist {file_siz}"
        assert file_seg_lsn == 65, f"README Segment LSN erwartet 65, ist {file_seg_lsn}"

        # 5. README Daten lesen
        f.seek(file_seg_lsn * lsn_size)
        content = f.read(file_siz)
        print(f"[OK] README Dateiinhalt erfolgreich gelesen ({len(content)} Bytes):")
        print("---")
        print(content.decode("latin1", "replace").strip())
        print("---")

    print("\n>>> ALLE TESTS BESTANDEN: RBF68k Read-Only Logik verifiziert! <<<")

if __name__ == "__main__":
    test_rbf()
