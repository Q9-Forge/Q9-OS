# Q9-OS

## Projektübergreifender Kontext

Der gemeinsame projektübergreifende Kontext und die verbindlichen Namen
stehen in [Q9-Forge/AI_CONTEXT.md](../AI_CONTEXT.md).

Port von echtem, proprietärem Microware OS-9/68K für die Q9-Plattform
([Q9-Flux](https://github.com/Q9-Forge/Q9-Flux) emuliert die Zielhardware).
Kein Neubau eines eigenen Betriebssystems — dafür siehe
[Q9RESUME-Kernel](https://github.com/foellmy51/Q9RESUME-Kernel) (archivierter,
unabhängiger früher Versuch).

## Werkzeuge

- **`src/mbr.c`** — MBR-/FAT-/RBF-Partitionsinspektor, reines ANSI-C (läuft
  sowohl nativ auf dem Host als auch als OS-9-Kommando `/dd/CMDS/mbr`).
  Erkennt MBR-, FAT- und RBF-Partitionen, dient als Eingabe für die
  PCF-/RBF-Descriptor-Erzeugung. Aus `Q9-Flux` extrahiert (2026-07-31,
  volle Historie erhalten).
  - `src/mbr` — vorkompiliertes OS-9-Kommando
  - `src/mbr.r` — OS-9-Relocatable-Binary
- **`src/q9sysglob.a`** / **`src/q9sysglob.h`** — eigene Definition des
  Kernel-System-Global-Bereichs und der Exception-Sprungtabelle
  (`Q9_D_*`/`Q9_T_*`), per Disassemblierung des Original-Kernels
  rekonstruiert. Kein Abdruck der proprietären Microware-Quelle — eigene
  Namen/Beschreibungen, Offsets/Größen als verifizierte oder aus dem
  Handbuch bekannte Fakten übernommen, unbestätigte Felder klar als
  `PLATZHALTER` markiert. Siehe
  [`docs/REVERSE_ENGINEERING.md`](docs/REVERSE_ENGINEERING.md) für den
  Hintergrund.

Weitere Tools folgen, sobald sie feststehen — bis dahin bleibt die Struktur
flach (`src/`, kein Modulverzeichnis pro Tool), siehe `Q9-Forge`s
Namenskonvention: ein Modul = flach, mehrere Module = Unterverzeichnisse.

## Referenzmaterial

- [`vendor/`](vendor/README.md) — Original-Kernelmodule (`aker*`/`dker*`)
  aus dem Microware/RadiSys OS-9/68K SDK, unverändert, als Referenz für
  den eigenen Kernel-Nachbau.
- [`docs/KERNEL.md`](docs/KERNEL.md) — Notizen zu Kernel-Aufbau/-Arbeitsweise
  (Kernel-Typen, Speicherallokatoren, Init-Modul, Prozesserzeugung,
  Exception-Verarbeitung), aus dem offiziellen Technical Manual extrahiert.
- [`docs/REVERSE_ENGINEERING.md`](docs/REVERSE_ENGINEERING.md) —
  Arbeitsstand der Kernel-Disassemblierung (Ziel: byte-exakter Nachbau als
  Assembly-Quellcode), Modul-Header-Layout, bisherige Funde.

## Weitere OS-9-Systemmodule (`modules/`)

Seit 2026-08-12: jetzt, wo mehrere Systemmodule untersucht werden
(Kernel bereits weit fortgeschritten, IOMan begonnen, SysCache/SSM als
Adressbereich bekannt), bekommt jedes ein eigenes Unterverzeichnis unter
[`modules/`](modules/) — Original-Binary, Disassemblierungs-Notizen und
Ghidra-Skripte gebündelt. Das Kernel-Modul selbst bleibt vorerst an seinem
bestehenden Platz (`vendor/`, `src/kernel/`, `docs/REVERSE_ENGINEERING.md`)
statt rückwirkend nach `modules/kernel/` verschoben zu werden — vermeidet,
bestehende Tool-Pfade (`tools/ghidra_to_r68.py`, `src/Makefile`) anzufassen.

- [`modules/SYSCALL_MODULE_MAP.md`](modules/SYSCALL_MODULE_MAP.md) —
  vollständige Zuordnung aller ~97 OS-9-Systemaufrufe zu ihrem jeweiligen
  Modul (Kernel/IOMan/SysCache/SSM), per Adressvergleich aus der
  Kernel-Disassemblierung und einer Live-Vermessung im Q9-Flux-Emulator
  gewonnen.
- [`modules/ioman/`](modules/ioman/) — IOMan-Disassemblierung (4 Runden):
  Grundgerüst, gemeinsamer Kernel-Trampolin (F$Link/F$Send/F$GProcP/
  F$SRqMem), Treiber-Dispatch, und der OS-9-"Dreiklang"
  Descriptor→Treiber→File-Manager vollständig nachvollzogen.
- [`modules/c0-descriptor/`](modules/c0-descriptor/) — Geräte-Descriptor
  `c0` (CF-Master), vollständig analysiert (148 Byte): bestätigt die
  Modul-Typ-Codes (Descrptr=`0x0F`/Driver=`0x0E`/Fmgr=`0x0D`) und die
  Namensfelder, die `I$Attach` ausliest.
- [`modules/cfide-driver/`](modules/cfide-driver/) — CompactFlash-Treiber
  `cfide`: klassische 6-Slot-Einsprungtabelle (Init/Read/Write/GetStat/
  SetStat/Term) empirisch bestätigt.
- [`modules/rbf-filemanager/`](modules/rbf-filemanager/) — RBF-File-Manager:
  13-Slot-Tabelle bestätigt (passend zu den 13 `I$`-Callcodes `I$Create`–
  `I$Close`); referenziert selbst ebenfalls den Kernel-Trampolin
  (15 Fundstellen) — relativiert die Annahme "File-Manager ohne
  Kernel-Calls" teilweise.
- [`modules/os9000-x86/`](modules/os9000-x86/) — Systemmodule von
  **OS-9000 v4.9** (Microwares späterer, portabler C-Neuschrieb,
  x86-Ziel): Kernel/IOMan/RBF/SSM/SCF/PCF/CDFM/PipeMan als reale Binaries
  extrahiert, Modul-Header-Format mit dem 68k-Original verglichen
  (Type/Lang-Byte-Offsets architekturübergreifend identisch, Namensfeld-
  Format anders). Image tatsächlich unter QEMU gebootet und live geprüft:
  **RBF ist weiterhin der aktive File Manager**, die zunächst rätselhafte
  Boot-Sektor-Signatur `"XD00BT"` ist nur ein x86-BIOS-Bootstrap-Wrapper,
  kein Ersatz-Dateisystem.
- [`modules/os9000-x86-6.1/`](modules/os9000-x86-6.1/) — dieselbe Prüfung
  für **OS-9000 v6.1** (2018, VirtualBox-Appliance, ~20 Jahre nach v4.9):
  ebenfalls gebootet, RBF weiterhin als File Manager bestätigt, Type/Lang-
  Byte-Konvention ein drittes Mal (nach 68k und v4.9) unabhängig
  verifiziert — architektonisch stabil über zwei Jahrzehnte und zwei
  CPU-Familien.
- `modules/syscache/`, `modules/ssm/` — bisher nur Platzhalterverzeichnisse,
  noch nicht disassembliert.

## Build

```sh
cd src && make
```
