> **VERALTET (26.09.2026):** Dieses Schwesterprojekt ist vom aktiven DHF in
> `Q9-OS/Q9-DHF-68k` überholt (eigene, ältere Protokollkopie mit alten Fehlercodes; kein
> FileManager). Nicht mehr gepflegt – s. `Q9-OS/Q9-DHF-68k/README.md`.

# Q9-DHFDRV-68k

OS-9 (Q9) Treiber und emuliertes Hardware-Gegenstück für den **DHF Filemanager** (Direct Host Filesystem).

## Architektur

Das System nutzt ein **Zero-Copy Pointer-Modell**:
- Das Userprogramm stellt die Speicherpuffer für Pfadnamen und Daten bereit.
- Der 68k-Treiber kopiert keine Puffer in die Shared Area, sondern übergibt die **68k-Speicheradressen als Zeiger in Registern**:
  - `A0` = 32-Bit 68k-Adresse des Pfad-/Dateinamens (Null-terminierter String).
  - `A1` = 32-Bit 68k-Adresse des Datenpuffers (User-Speicher).
  - `D0` = Pfadnummer / Deskriptor / Datei-Handle.
  - `D1` = Byteanzahl / Maximallänge / Dateimodus / Seek-Offset.
  - `D2` = Zugriffsmodus / Attribute / Flags.
- Der **Hardware-Simulator** greift direkt auf den emulierten Speicherbereich (`emu_memory` Array) des Emulators (Q9-Flux) an den Adressen `A0` und `A1` zu.

## Shared Command Area (`struct dhf_shared`)

Die Shared Command Area ist kompakt (28 Bytes, Big-Endian):

```c
struct dhf_shared {
    uint8_t  version;  /* Protokollversion (1) */
    uint8_t  command;  /* 0 = Idle, 1..N = Kommandos, 255 = Return */
    uint8_t  status;   /* Rückgabestatus (0 = OK, OS-9 Fehlercode) */
    uint8_t  flags;    /* Reserviert */
    uint32_t seq;      /* Sequenzzähler (BE) */

    uint32_t a0;       /* 68k-Zeiger: Pfad- / Dateiname */
    uint32_t a1;       /* 68k-Zeiger: Datenpuffer */
    uint32_t d0;       /* Handle / Pfadnummer */
    uint32_t d1;       /* Bytezahl / Modus / Offset */
    uint32_t d2;       /* Attribute / Flags / Whence */
};
```

Standard-Adresse im 68k-Adressraum: `$FFFFD000` (über Deskriptor / Init konfigurierbar).

## Komponenten

1. **`driver/` (68k OS-9 Treiber)**
   - `dhfdrv.c`: Implementiert alle `dhfdrv_*`-Funktionen (`init`, `open`, `create`, `close`, `read`, `write`, `seek`, `readln`, `writeln`, `getstat`, `setstat`, `mkdir`, `chdir`, `rmdir`, `unlink`, `rename`, `opendir`, `readdir`).
   - Füllt `a0`, `a1`, `d0`, `d1`, `d2` und setzt `command`.
   - Wartet auf Quittierung (`command == 0`) und setzt ggf. `errno`.

2. **`emulator/` (Simulierter Hardware-Treiber für Q9-Flux)**
   - `dhf_host_fs.c` / `.h`: Host-Dateisystem-Anbindung mit striktem Pfad-Confinement (`basepath`) gegen Symlink-/Traversal-Ausbrüche. Verwaltet Host-Deskriptoren.
   - `dhf_emu_device.c` / `.h`: Simulierter Hardware-Baustein.
     - Löst 68k-Adressen direkt im Emulator-RAM-Array (`emu_memory[a0]`, `emu_memory[a1]`) auf.
     - Führt Host-I/O aus und setzt Status und Rückgabelängen.
     - Unterstützt 8/16/32-Bit MMIO-Zugriffe (kompatibel mit Q9-Flux `devreg`).

3. **`socket/` (Remote-Host Weiterleitung)**
   - Ermöglicht es dem Hardware-Simulator, Dateibefehle transparent über TCP an einen externen Host (z. B. Linux-Server, Mac, NAS) weiterzuleiten.
   - `dhf_socket_client.c`: TCP-Client im Simulator.
   - `dhf_socket_server.c`: Server-Logik.
   - `dhf_net_daemon.c`: Standalone-CLI-Daemon (`build/bin/dhf-net-daemon`).

4. **`tests/` (Unit- & Integrationstests)**
   - `test_dhfdrv.c`: End-to-End-Test (Treiber -> Shared Memory -> Hardware-Simulator -> Host-Dateisystem).
   - `test_dhf_socket.c`: End-to-End-Test der Socket-Weiterleitung über TCP.

## Kompilierung und Tests

```sh
# Alles bauen und Tests ausführen:
make test

# Standalone Remote-Daemon starten:
./build/bin/dhf-net-daemon --port 9988 --dir /pfad/zum/verzeichnis
```
