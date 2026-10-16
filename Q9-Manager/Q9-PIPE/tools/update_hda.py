#!/usr/bin/env python3
"""
update_hda.py -- Schreibt OS9Boot_pipe_test in OS9SYS.hda anhand der FD-Segmentliste.
"""
import hashlib
import os
import sys

BOOT_FILE = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-SDK/SRC/SYSTEM/Q9-OS/Q9-Manager/Q9-PIPE/tools/OS9Boot_pipe_test"
HDA_FILE = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-SDK/SRC/SYSTEM/Q9-Flux/Q9-Flux-68k/local_images/work/rbf68k-session/OS9SYS.hda"
FD_LSN = 563328

def update_hda(boot_path=BOOT_FILE, hda_path=HDA_FILE):
    assert os.path.exists(boot_path), f"Boot-Datei nicht gefunden: {boot_path}"
    assert os.path.exists(hda_path), f"HDA-Datei nicht gefunden: {hda_path}"

    with open(boot_path, "rb") as f:
        boot_data = f.read()
    boot_hash = hashlib.sha256(boot_data).hexdigest()

    with open(hda_path, "r+b") as f:
        # Sektor 0 lesen
        f.seek(0)
        sct0 = f.read(512)
        tot_sectors = int.from_bytes(sct0[0:3], "big")
        map_bytes = int.from_bytes(sct0[4:6], "big")
        bit_ratio = int.from_bytes(sct0[6:8], "big")
        map_lsn = int.from_bytes(sct0[100:104], "big")

        # FD lesen
        f.seek(FD_LSN * 512)
        fd = bytearray(f.read(512))

        segs = []
        seg_count = 0
        for i in range(48):
            entry = fd[16 + i*5 : 16 + (i+1)*5]
            lsn = int.from_bytes(entry[0:3], "big")
            cnt = int.from_bytes(entry[3:5], "big")
            if cnt > 0:
                segs.append((lsn, cnt))
                seg_count = i + 1

        capacity = sum(cnt * 512 for _, cnt in segs)
        print(f"Bestehende Kapazität: {len(segs)} Segmente, {capacity} Bytes (Bootgröße: {len(boot_data)} Bytes)")

        assert len(boot_data) <= capacity, f"Boot-Datei ({len(boot_data)} Bytes) überschreitet Kapazität ({capacity} Bytes)"

        # Boot-Daten in die Segmente schreiben
        written = 0
        remaining = len(boot_data)
        for lsn, cnt in segs:
            seg_bytes = cnt * 512
            chunk_size = min(remaining, seg_bytes)
            f.seek(lsn * 512)
            if chunk_size > 0:
                f.write(boot_data[written : written + chunk_size])
                written += chunk_size
                remaining -= chunk_size
            if chunk_size < seg_bytes:
                f.write(b"\x00" * (seg_bytes - chunk_size))

        assert remaining == 0, f"Bootdatei passte nicht in Segmente! Rest: {remaining}"

        # FD aktualisieren
        fd[9:13] = len(boot_data).to_bytes(4, "big")
        f.seek(FD_LSN * 512)
        f.write(fd)

    # Verifikation durch Rücklesen
    with open(hda_path, "rb") as f:
        f.seek(FD_LSN * 512)
        fd = f.read(512)
        size = int.from_bytes(fd[9:13], "big")
        read_back = bytearray()
        for lsn, cnt in segs:
            chunk_size = min(size - len(read_back), cnt * 512)
            f.seek(lsn * 512)
            read_back.extend(f.read(chunk_size))
            if len(read_back) == size:
                break

    read_hash = hashlib.sha256(read_back).hexdigest()
    assert read_hash == boot_hash, f"Verifikation fehlgeschlagen: {read_hash} != {boot_hash}"
    print(f"[OK] OS9Boot ({len(boot_data)} Bytes, SHA256 {boot_hash[:16]}...) erfolgreich in OS9SYS.hda aktualisiert und verifiziert!")

if __name__ == "__main__":
    b_path = sys.argv[1] if len(sys.argv) > 1 else BOOT_FILE
    h_path = sys.argv[2] if len(sys.argv) > 2 else HDA_FILE
    update_hda(b_path, h_path)
