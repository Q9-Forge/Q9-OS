# Q9-DHF-68k – Direct Host Filesystem

Ein OS-9-Laufwerk, das im **Q9-Flux-Emulator** direkt auf ein Verzeichnis des Hosts zugreift.
Im Gast sieht es aus wie ein RBF-Laufwerk; echte Utilities (`dir -e -r`, `attr`, `copy`,
`del`, `deldir -q`, `rename`, `free`, `dsave | mshell`, bash) arbeiten darauf.

## Aktive Teile

| Datei | Rolle |
|---|---|
| `manager/dhfmgr_68k.a` | FileManager (Modul `dhfmgr`) |
| `driver/dhfdrv_68k.a` | Treiber (Modul `dhfdrv`) |
| `descriptor/d0_dhf.a` | Laufwerk `d0` → `Q9-Images/dhf_root/d0`, beschreibbar, Port `$FFFF4000` |
| `descriptor/d1_dhf.a` | Laufwerk `d1` → `Q9-Images/cf_images/OS9SYS`, **nur lesbar**, Port `$FFFF4100` |
| `test/dhfregr_68k.a` | Regressionstest im Gast (Modul `dhfregr`, 118 Prüfungen) |
| `test/run_dhfregr.sh` | kompletter Lauf auf dem Mac (baut, testet auf Image-Kopie, prüft Host-Seite) |
| `Q9-Flux/Q9-Flux-68k/src/devices/dhf/` | Gerät im Emulator (`dhf_emu_device.c`, `dhf_host_fs.c`, …) |
| `docs/PROTOCOL.md` | Protokoll und nachgebildete RBF-Semantik |
| `docs/DHF_ERRNO_MAP.md` | Fehlercodes |
| `STATUS.md` | ausführliches Protokoll aller Befunde |

Benutzen: `Q9-Images/emu_config/dhf_claude.q9` starten; `/SYS/startup` im Claude-Image hängt
`d0` und `d1` an. Testen:
`Q9_LOGIN_PASS=… test/run_dhfregr.sh [-k] [-i]`.
Laufwerk schreibbar/nur lesbar: Flags-Wort in der `DevCon`-Tabelle des Deskriptors (Bit 0).

## Veraltet – nicht mehr gepflegt, nicht darauf aufbauen

Aus der Frühphase (Unix-artiges Design mit einem nie existierenden `_os9_f_viread`), vom
heutigen Protokoll überholt und von keinem aktiven Teil benutzt:

- `manager/dhf_manager*.c`, `manager/dhf_manager.c.old`, `manager/dhf.h`,
  `manager/os9_sysalloc.*`, `manager/path_desc.h`
- `driver/dhfdrv-68k.c`, `driver/dhfdrv-68k.h`
- `descriptor/dhf_descriptor.*`
- `host_simulator/`, `examples/`, `include/`
- `test/dhftest_bisect*`, `test/dhftest_rbf*`, `test/dhftest_srqmem*` (Bisektions-Hilfen)
- Schwesterprojekt `Q9-OS/Q9-DHFDRV-68k` (eigene, ältere Protokollkopie mit alten Fehlercodes)
- TCP-Backend `Q9-Flux-68k/src/devices/dhf/dhf_socket_client.c` (nicht auf Protokollstand)

Löschen wäre möglich, ist aber bewusst nicht geschehen.
