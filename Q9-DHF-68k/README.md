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
| `descriptor/dd_dhf.a` | Systemlaufwerk `dd` für das **Booten von DHF** (Port `$FFFF4000`) |
| `boot/mk_dhfboot.sh` | baut den Bootbaum `Q9-Images/dhf_root/boot` (OS9Boot mit DHF-Modulen) |
| `boot/mk_dhfrom.sh` | baut den ROM `Claude_cb030_DHF_BIOS.BIN` (Booter `ROM_CBOOT/io_dhf.c` im Q9-Port) |
| `test/run_dhfboot.sh` | Boottest: von DHF booten, einloggen, arbeiten; Rückfall auf CF |
| `test/dhfregr_68k.a` | Regressionstest im Gast (Modul `dhfregr`, 125 Prüfungen) |
| `test/dhflock_68k.a`, `test/dhflockp_68k.a` | Sperrtest über zwei Prozesse (vom Runner benutzt) |
| `test/run_dhfregr.sh` | kompletter Lauf auf dem Mac (baut, testet auf Image-Kopie, prüft Host-Seite) |
| `Q9-Flux/Q9-Flux-68k/src/devices/dhf/` | Gerät im Emulator (`dhf_emu_device.c`, `dhf_host_fs.c`, …) |
| `docs/PROTOCOL.md` | Protokoll und nachgebildete RBF-Semantik |
| `docs/DHF_ERRNO_MAP.md` | Fehlercodes |
| `STATUS.md` | ausführliches Protokoll aller Befunde |

Benutzen: `Q9-Images/emu_config/dhf_claude.q9` starten; `/SYS/startup` im Claude-Image hängt
`d0` und `d1` an. Testen:
`Q9_LOGIN_PASS=… test/run_dhfregr.sh [-k] [-i]`.
Host-Verzeichnis und Schreibschutz: am einfachsten per `[dhf0]`/`[dhf1]` in der `.q9`-Datei
(`hostpath =`, `readonly = yes|no`, Vorrang vor dem Deskriptor); sonst Basispfad und Flags-Wort
(Bit 0 = nur lesbar) in der `DevCon`-Tabelle des Deskriptors.

## Von DHF booten

`Q9-Images/emu_config/dhf_boot.q9` bootet OS-9 ganz ohne CompactFlash aus dem Host-Verzeichnis
`Q9-Images/dhf_root/boot`:

1. Der ROM `Claude_cb030_DHF_BIOS.BIN` ist der Claude-ROM mit einem zusätzlichen Booter
   „boot from DHF“ (Q9-Port `ROM_CBOOT/io_dhf.c`, 540 Byte; die ROM-Module sind unverändert,
   es bleiben 400 Byte frei). Er spricht das DHF-Fenster direkt an und lädt `OS9Boot` per
   INIT/OPEN/GETSTT/READ – im ROM sind dafür **keine** DHF-Module nötig.
2. Er bootet nur, wenn die `.q9` `[dhf0] hostpath` setzt; sonst meldet das Gerät E$NotRdy und
   sysboot versucht wie bisher CompactFlash (derselbe ROM läuft also auch mit alten Configs).
3. Das `OS9Boot` im Bootbaum enthält `dhfmgr`, `dhfdrv` und `dd` aus `descriptor/dd_dhf.a`
   (statt des cfide-`dd`); `init` nennt `/dd` als Systemlaufwerk, also ist `/dd` wieder das
   Host-Verzeichnis. `SYS/startup` hängt nur noch `dd r0` an (keine CF).

Neu bauen: `boot/mk_dhfrom.sh` (ROM, Wine-Toolchain des SDK) und `boot/mk_dhfboot.sh [-n]`
(Bootbaum als APFS-Klon von `cf_images/OS9SYS`, OS9Boot, startup). Testen:
`Q9_LOGIN_PASS=… test/run_dhfboot.sh [-k] [-v]`.

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
