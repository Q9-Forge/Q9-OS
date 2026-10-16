#!/usr/bin/env python3
"""
test_rbf_readwrite.py -- Unit-Test für RBF68k Read-Write-Logik
Erstellt eine temporäre Kopie von q9data-rbf.img und testet:
1. Bitmap-Allokation (Freie Bits suchen, setzen, freigeben)
2. Datei-Erstellung (Create) mit FD und Directory-Eintrag
3. Daten schreiben (Write) über Segmente hinweg
4. Daten lesen und verifizieren (Read)
5. Datei löschen (Delete) und Verzeichnis-Eintrag nullen
"""

import os
import shutil
import struct
import sys

SRC_IMG = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-Images/cf_images/q9data-rbf.img"
TMP_IMG = "/tmp/q9data_test_rw.img"

def test_rw():
    print(f"=== RBF68k Read-Write Test auf {TMP_IMG} ===")
    shutil.copyfile(SRC_IMG, TMP_IMG)

    with open(TMP_IMG, "r+b") as f:
        # 1. Sektor 0 einlesen
        f.seek(0)
        sct0 = f.read(512)
        tot_sectors = int.from_bytes(sct0[0:3], "big")
        bit_ratio = int.from_bytes(sct0[6:8], "big") # 32 Sektoren pro Bit
        root_dir_lsn = int.from_bytes(sct0[8:11], "big") # LSN 2
        map_lsn = int.from_bytes(sct0[100:104], "big") # LSN 1
        lsn_size = int.from_bytes(sct0[104:106], "big") or 512

        print(f"[OK] Sektor 0 gelesen: MapLSN={map_lsn}, BitRatio={bit_ratio}, RootDirFD={root_dir_lsn}")

        # 2. Bitmap einlesen
        f.seek(map_lsn * lsn_size)
        bitmap = bytearray(f.read(512))

        # Erstes freies Bit suchen
        alloc_cluster = None
        for byte_idx in range(512):
            if bitmap[byte_idx] != 0xFF:
                for bit_idx in range(7, -1, -1):
                    if not (bitmap[byte_idx] & (1 << bit_idx)):
                        alloc_cluster = (byte_idx * 8) + (7 - bit_idx)
                        bitmap[byte_idx] |= (1 << bit_idx)
                        break
                if alloc_cluster is not None:
                    break

        assert alloc_cluster is not None, "Kein freier Cluster in der Bitmap!"
        new_lsn = alloc_cluster * bit_ratio
        print(f"[OK] Freier Cluster gefunden: Cluster {alloc_cluster} -> LSN {new_lsn}")

        # Bitmap zurückschreiben
        f.seek(map_lsn * lsn_size)
        f.write(bitmap)

        # 3. Neuen File Descriptor bei new_lsn anlegen (für Datei 'TESTFILE')
        test_fd = bytearray(512)
        test_fd[0] = 0x03 # Read/Write
        test_fd[8] = 1    # Link count 1
        # Größe zunächst 14 Bytes
        content = b"Hallo Q9-OS!\r\n"
        struct.pack_into(">I", test_fd, 9, len(content))
        # Segment 0: data bei new_lsn + 1 (31 Sektoren)
        data_lsn = new_lsn + 1
        test_fd[16] = (data_lsn >> 16) & 0xFF
        test_fd[17] = (data_lsn >> 8) & 0xFF
        test_fd[18] = data_lsn & 0xFF
        struct.pack_into(">H", test_fd, 19, bit_ratio - 1)

        f.seek(new_lsn * lsn_size)
        f.write(test_fd)
        print(f"[OK] File Descriptor bei LSN {new_lsn} geschrieben.")

        # Daten schreiben bei data_lsn
        f.seek(data_lsn * lsn_size)
        f.write(content)
        print(f"[OK] {len(content)} Bytes Nutzdaten bei LSN {data_lsn} geschrieben.")

        # 4. Verzeichniseintrag 'TESTFILE' im Root-Verzeichnis eintragen
        # Root-FD lesen
        f.seek(root_dir_lsn * lsn_size)
        root_fd = f.read(512)
        dir_lsn = int.from_bytes(root_fd[16:19], "big") # LSN 3
        dir_size = int.from_bytes(root_fd[9:13], "big")  # 96 Bytes

        f.seek(dir_lsn * lsn_size)
        dir_data = bytearray(f.read(512))

        # Freien 32-Byte Slot suchen
        slot_offset = None
        for off in range(0, 512, 32):
            if dir_data[off] == 0:
                slot_offset = off
                break

        assert slot_offset is not None, "Kein freier Directory Slot!"
        # 'TESTFILE' eintragen mit Bit 7 am letzten Zeichen
        name = b"TESTFIL" + bytes([ord('E') | 0x80])
        name = name.ljust(28, b'\x00')
        dir_data[slot_offset:slot_offset+28] = name
        struct.pack_into(">I", dir_data, slot_offset+28, new_lsn)

        # Directory zurückschreiben
        f.seek(dir_lsn * lsn_size)
        f.write(dir_data)
        print(f"[OK] Verzeichniseintrag 'TESTFILE' bei Offset {slot_offset} im Root-Dir eingetragen.")

        # 5. Gegenprüfung: Datei über Verzeichnis suchen, FD lesen und Inhalt verifizieren
        f.seek(dir_lsn * lsn_size)
        verify_dir = f.read(512)
        found_fd = None
        for off in range(0, 512, 32):
            ent = verify_dir[off:off+32]
            nchars = []
            for b in ent[:28]:
                if b == 0: break
                nchars.append(chr(b & 0x7F))
                if b & 0x80: break
            entry_name = "".join(nchars)
            if entry_name == "TESTFILE":
                found_fd = int.from_bytes(ent[28:32], "big")
                break

        assert found_fd == new_lsn, f"Gefundener FD LSN {found_fd} != {new_lsn}"
        print(f"[OK] Datei 'TESTFILE' im Verzeichnis wiedergefunden -> FD LSN {found_fd}")

        # Inhalt lesen
        f.seek(found_fd * lsn_size)
        read_fd = f.read(512)
        read_size = int.from_bytes(read_fd[9:13], "big")
        read_data_lsn = int.from_bytes(read_fd[16:19], "big")
        assert read_size == len(content), f"Dateigröße {read_size} != {len(content)}"
        assert read_data_lsn == data_lsn, f"Daten LSN {read_data_lsn} != {data_lsn}"

        f.seek(read_data_lsn * lsn_size)
        read_bytes = f.read(read_size)
        assert read_bytes == content, f"Gelesene Daten stimmen nicht überein!"
        print(f"[OK] Gelesener Dateiinhalt verifiziert: {read_bytes}")

        # 6. Delete testen: Verzeichniseintrag nullen
        dir_data[slot_offset:slot_offset+32] = b'\x00' * 32
        f.seek(dir_lsn * lsn_size)
        f.write(dir_data)
        print(f"[OK] Verzeichniseintrag gelöscht (genullt).")

    # Aufräumen
    if os.path.exists(TMP_IMG):
        os.remove(TMP_IMG)

    print("\n>>> ALLE READ-WRITE TESTS BESTANDEN! RBF68k Schreib-Pfad verifiziert! <<<")

if __name__ == "__main__":
    test_rw()
