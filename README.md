# Q9-OS

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
  Arbeitsstand der Disassemblierung (Ziel: byte-exakter Nachbau als
  Assembly-Quellcode), Modul-Header-Layout, bisherige Funde.

## Build

```sh
cd src && make
```
