# Q9-OS

## Projektübergreifender Kontext

Der gemeinsame projektübergreifende Kontext und die verbindlichen Namen
stehen in [Q9-Forge/AI_CONTEXT.md](../AI_CONTEXT.md).

Ursprünglich als reiner Port von echtem, proprietärem Microware OS-9/68K
für die Q9-Plattform gestartet ([Q9-Flux](https://github.com/Q9-Forge/Q9-Flux)
emuliert die Zielhardware). Nach einer längeren Planungs-/Recherchephase
(vergleichende Analyse von OS-9/68K und OS-9000/x86) ist diese
Beschränkung aufgehoben (2026-08-14) — Ziel ist jetzt ein **eigener
Kernel**, mit **verbindlicher Kompatibilität zu echten OS-9/68K- und
OS-9000-Modulen** (bestehende Treiber/File-Manager müssen weiterhin
laufen), aber offen für zusätzliche, eigene Modularten. Erster grober
Anforderungsentwurf (intern dokumentiert), dazu die konkrete
Modul-/Syscall-Stückliste:
[`docs/OWN_KERNEL_MODULES_OVERVIEW.md`](docs/OWN_KERNEL_MODULES_OVERVIEW.md).
Frühere, unabhängige Vorarbeit dazu: [Q9RESUME-Kernel](https://github.com/foellmy51/Q9RESUME-Kernel)
(archiviert).

## Kernel (68k)

Der aktuelle Schwerpunkt ist der eigenständig in C und Assembler
geschriebene 68k-Kernel unter
[`Q9-KERNEL/68k/src/kernel/`](Q9-KERNEL/68k/src/kernel/) (Einstiegspunkt
`q9kernel_entry.a`, Bau- und Testskript `build.sh`). Laufender
Entwicklungsstand, Meilensteine und offene Punkte:
[`docs/OWN_KERNEL_STATUS.md`](docs/OWN_KERNEL_STATUS.md).

## Werkzeuge

- **`Q9-KERNEL/common/src/mbr.c`** — MBR-/FAT-/RBF-Partitionsinspektor,
  reines ANSI-C (läuft sowohl nativ auf dem Host als auch als
  OS-9-Kommando `/dd/CMDS/mbr`). Erkennt MBR-, FAT- und RBF-Partitionen,
  dient als Eingabe für die PCF-/RBF-Descriptor-Erzeugung. Aus `Q9-Flux`
  extrahiert (2026-07-31, volle Historie erhalten).
  - `Q9-KERNEL/68k/src/mbr` — vorkompiliertes OS-9-Kommando
  - `Q9-KERNEL/68k/src/mbr.r` — OS-9-Relocatable-Binary
- **`Q9-KERNEL/68k/src/q9sysglob.a`** / **`Q9-KERNEL/common/src/q9sysglob.h`**
  — eigene Definition des Kernel-System-Global-Bereichs und der
  Exception-Sprungtabelle (`Q9_D_*`/`Q9_T_*`). Kein Abdruck der
  proprietären Microware-Quelle — eigene Namen/Beschreibungen,
  Offsets/Größen als verifizierte oder aus dem öffentlichen Technical
  Manual bekannte Fakten übernommen, unbestätigte Felder klar als
  `PLATZHALTER` markiert.
- **`Q9-KERNEL/68k/src/q9moduleheader.a`** / **`Q9-KERNEL/common/src/q9moduleheader.h`**
  — eigene, architekturübergreifende Definition des OS-9-Modul-Headers:
  OS-9/6809 (1980), OS-9/68K (klassisch) und OS-9000 (universell,
  x86/PowerPC/ARM/MIPS/SPARC/...) als drei getrennte Layouts
  (`Q9_MH6809_*`/`Q9_MH68K_*`/`Q9_MH9K_*`) plus gemeinsame, über alle
  drei Generationen identische Typ-/Sprach-Codes (`Q9_MT_*`/`Q9_ML_*`).
  Ziel: Grundlage für ein künftiges `ident`-artiges Werkzeug, das Module
  jeder OS-9-Generation am Sync-Wort erkennt und beschreiben kann. Aus
  den drei offiziellen Technical Manuals sowie eigener, unabhängiger
  Analyse rekonstruiert, kein Abdruck der proprietären `module.h`/
  `oskdefs.d`.
- **`tools/annotate_trace.py`** — ordnet Instruktionsspuren aus dem
  Emulator automatisch Modul/Symbol zu und prüft die Call/Return-Bilanz.
- **`tools/mkbootfile.sh`** — baut eine Bootdatei aus dem eigenen Kernel
  plus den unveränderten Original-Systemmodulen für ein Testabbild.

## Referenzmaterial

- [`Scheduler.md`](Scheduler.md) — Notizen zu Kernel-Aufbau und
  -Arbeitsweise (Kernel-Typen, Speicherallokatoren, Scheduler-Konzept),
  aus dem öffentlichen Technical Manual sowie eigenen Entwurfsideen.

Vertiefendes Analysematerial zu den originalen Systemmodulen (Aufbau,
Vergleich verschiedener OS-9-Generationen, Werkzeugnotizen) wird
projektintern gepflegt und ist nicht Teil dieses öffentlichen Repos.

## Build

```sh
cd Q9-KERNEL/68k/src/kernel && ./build.sh
```
