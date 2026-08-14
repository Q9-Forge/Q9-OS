# Q9-OS

## Projektübergreifender Kontext

Der gemeinsame projektübergreifende Kontext und die verbindlichen Namen
stehen in [Q9-Forge/AI_CONTEXT.md](../AI_CONTEXT.md).

Ursprünglich als reiner Port von echtem, proprietärem Microware OS-9/68K
für die Q9-Plattform gestartet ([Q9-Flux](https://github.com/Q9-Forge/Q9-Flux)
emuliert die Zielhardware). Nach einer längeren Planungs-/Recherchephase
(siehe [`docs/kernel-walkthrough/`](docs/kernel-walkthrough/), der
vergleichenden Analyse von OS-9/68K und OS-9000/x86) ist diese
Beschränkung aufgehoben (2026-08-14) — Ziel ist jetzt ein **eigener
Kernel**, mit **verbindlicher Kompatibilität zu echten OS-9/68K- und
OS-9000-Modulen** (bestehende Treiber/File-Manager müssen weiterhin
laufen), aber offen für zusätzliche, eigene Modularten. Erster grober
Anforderungsentwurf dafür: [`docs/OWN_KERNEL_INIT_PLAN.md`](docs/OWN_KERNEL_INIT_PLAN.md).
Frühere, unabhängige Vorarbeit dazu: [Q9RESUME-Kernel](https://github.com/foellmy51/Q9RESUME-Kernel)
(archiviert).

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
- **`src/q9moduleheader.a`** / **`src/q9moduleheader.h`** — eigene,
  architekturübergreifende Definition des OS-9-Modul-Headers: OS-9/6809
  (1980), OS-9/68K (klassisch) und OS-9000 (universell, x86/PowerPC/ARM/
  MIPS/SPARC/...) als drei getrennte Layouts (`Q9_MH6809_*`/`Q9_MH68K_*`/
  `Q9_MH9K_*`) plus gemeinsame, über alle drei Generationen identische
  Typ-/Sprach-Codes (`Q9_MT_*`/`Q9_ML_*`). Ziel: Grundlage für ein
  künftiges `ident`-artiges Werkzeug, das Module jeder OS-9-Generation
  am Sync-Wort erkennt und beschreiben kann. Aus den drei offiziellen
  Technical Manuals sowie eigener Disassemblierung rekonstruiert, kein
  Abdruck der proprietären `module.h`/`oskdefs.d`. Siehe
  [`docs/kernel-walkthrough/00-modul-aufbau-und-header/`](docs/kernel-walkthrough/00-modul-aufbau-und-header/)
  für den Hintergrund und die Herleitung jedes einzelnen Felds.

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

**Provenienz-Konvention:** Jedes Verzeichnis mit aus einer externen Quelle
übernommenen Binärdateien (Archiv-Entpackung, Live-RAM-Extraktion, ...)
bekommt eine eigene `PROVENANCE.md` direkt daneben — Quelldatei(en) mit
SHA-256, ggf. Zwischenschritt (Disk-Image, Boot-Session, Speicheradresse)
und SHA-256 je extrahierter Einzeldatei. Beispiel:
[`modules/os9000-x86/vendor/PROVENANCE.md`](modules/os9000-x86/vendor/PROVENANCE.md)
(statischer Tar-Export) und
[`modules/os9000-x86/vendor-live/PROVENANCE.md`](modules/os9000-x86/vendor-live/PROVENANCE.md)
(Live-RAM-Extraktion) — damit bleibt bei jedem Fund nachvollziehbar, woher
er stammt, auch wenn Quelldateien später umbenannt/verschoben/erneut
gebootet werden.

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
  kein Ersatz-Dateisystem. **RBFs On-Disk-Format hat sich für x86 aber
  grundlegend geändert** (4-Byte-Little-Endian-Felder, LSN0 bei Block 1,
  64-Byte-Verzeichniseinträge statt der 68k-Konvention) — Toolshed kann
  es deshalb nicht lesen; von Hand aus Rohbytes dekodiert und dabei
  `sysboot` erfolgreich extrahiert (982 KB, stellte sich als eigenes,
  vermutlich komprimiertes Container-Format `"OS9Z"` heraus, keine
  einfache Modul-Verkettung). Alle 8 aktuell geladenen Module zusätzlich
  live per `ident -m -o` + QEMU `pmemsave` direkt aus dem laufenden RAM
  extrahiert (siehe [`modules/os9000-x86/vendor-live/`](modules/os9000-x86/vendor-live/README.md))
  — unterscheiden sich alle in der Größe vom statischen `mw86.tar`-Build.
  Kernel-Init-Sequenz per Ghidra disassembliert und mit dem 68K-Kernel
  verglichen (siehe [`modules/os9000-x86/docs/KERNEL_INIT.md`](modules/os9000-x86/docs/KERNEL_INIT.md)):
  Entry-Point empirisch gefunden, **70 % Code-Abdeckung/229 Funktionen
  komplett automatisch** durch Ghidra (deutlich mehr als beim
  handoptimierten 68K-Kernel), dieselbe grobe Boot-Reihenfolge
  (Speicher/Arena → Exception-Dispatch-Tabelle → Prozess-Freilisten →
  erster Ausführungskontext von Hand konstruiert), koppelt Hardware/
  Systemlogik aber über Funktionszeiger in eine externe, zielspezifische
  Beschreibungsstruktur statt sie wie beim 68K fest im Modul zu verdrahten
  — bestätigt die dokumentierte "Low-Level System"-Portierungsgrenze
  erstmals im Code selbst.
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
