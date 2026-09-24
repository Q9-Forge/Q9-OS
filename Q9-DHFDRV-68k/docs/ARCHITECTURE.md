# Q9-DHF Architektur & Ablauf

## 1. Gesamtablauf eines Dateiaufrufs

```text
[User-Programm / Shell in OS-9]
        │
        ▼ (I$Read / I$Write Syscall via IOMAN)
[DHF Filemanager: Q9-DHF-68k/manager/dhf_manager.c]
        │
        ▼ (dhfdrv_read / write)
[DHF 68k Driver: Q9-DHFDRV-68k/driver/dhfdrv.c]
        │
        │ Schreibt 68k-Zeiger & Register in Shared Area:
        │ A0 = 68k-Adresse des Pfadstrings
        │ A1 = 68k-Adresse des Datenpuffers (User-RAM)
        │ D0 = Handle, D1 = Count, D2 = Flags
        │ Command = DHF_CMD_READ / WRITE
        ▼
[Shared Memory Area: $FFFFD000] (28 Bytes)
        │
        ▼ (MMIO-Write auf Command-Byte triggert Emulator)
[Simulierter Hardware-Treiber: Q9-DHFDRV-68k/emulator/dhf_emu_device.c]
        │
   ┌────┴────────────────────────┐
   ▼                             ▼
[Modus A: Lokales Host-FS]    [Modus B: Remote-Socket]
dhf_host_fs.c                 dhf_socket_client.c
  │                             │ (TCP-Paket)
  │ Direktzugriff auf           ▼
  │ emu_memory[A0/A1]         [dhf-net-daemon auf Remote-Server]
  ▼                             │
Host-Dateisystem              Remote-Dateisystem
(POSIX open/read/write/stat)  (POSIX open/read/write/stat)
```

## 2. Speicherabbildung & Zero-Copy

Im Emulator Q9-Flux ist der gesamte 68k-Adressraum als durchgehendes Array (`uint8_t *ram`) allokiert.

Wenn der 68k-Treiber z. B. `dhfdrv_read(fd, buf, 4096)` aufruft:
1. `buf` ist die 32-Bit 68k-Adresse im User-Adressraum.
2. Der Treiber schreibt `A1 = buf` und `D1 = 4096` in die Shared Area.
3. Der Emulator-Treiber nimmt `A1`, berechnet `&ram[A1]` und liest die Daten vom Host-Dateisystem direkt in diesen Speicherbereich ein.
4. Es existiert keinerlei Zwischenkopieren oder Größenbeschränkung auf 512 Bytes.

## 3. Confinement / Sandbox-Sicherheit

Der Host-Dateisystem-Treiber (`dhf_host_fs.c`) prüft jeden Pfad:
- Alle virtuellen Pfade werden relativ zum konfigurierten `basepath` aufgelöst.
- Auflösung via `realpath()` verhindert `../`-Ausbrüche oder Symlink-Angriffe außerhalb des freigegebenen Verzeichnisses.
