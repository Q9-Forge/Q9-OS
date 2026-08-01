# OS-9/68K Kernel — Notizen aus dem offiziellen SDK

Zusammenfassung/Auszug aus dem Microware/RadiSys-Originalmaterial
(`MWOS/DOC/RadiSys/68k_tech.pdf` = "OS-9 for 68K Processors Technical
Manual", Kapitel 2 "The Kernel"; `68k_bls.pdf` = "OS-9 for 68K
Processors BLS Reference") als Referenz für den eigenen Kernel-Nachbau.
Die Original-Binaries liegen in [`../vendor/`](../vendor/README.md).
Kein Ersatz für die Originaldokumente — bei Detailfragen dort
nachschlagen (Volltext-PDF, per `pdftotext` durchsuchbar).

## Rolle des Kernels

Der Kernel ist der Nukleus von OS-9: ROMable, kompaktes Modul, verwaltet
Systemressourcen, Prozessausführung und Exception-/Interrupt-Verarbeitung.
Seine Kernaufgabe ist die Koordination von System Calls. OS-9 kennt zwei
Arten:

- **I/O-Aufrufe** (read/write) — der Kernel übernimmt nur die erste
  Verarbeitungsstufe und reicht dann an **IOMan** weiter, der wiederum
  den passenden File Manager/Device Driver aufruft.
- **System-Function-Aufrufe** (Speicherverwaltung, Systeminitialisierung,
  Prozesserzeugung/-scheduling, Exception-/Interrupt-Verarbeitung) —
  führt der Kernel direkt aus.

Ein System Call löst einen User-Trap zum Kernel aus; der Kernel bestimmt
den Typ und verzweigt entsprechend.

## Kernel-Typen: Atomic vs. Development

Zwei Klassen, binär als eigene Kernel-Module ausgeliefert (siehe
[`vendor/README.md`](../vendor/README.md) für die Dateinamen):

| | Development Kernel (`dker*`) | Atomic Kernel (`aker*`) |
|---|---|---|
| Zielgruppe | Voll ausgestattet, Embedded **und** Multi-User | primär Embedded (auch Multi-User nutzbar) |
| User-State-Debugging (`F$DFork`/`F$DExec`/`F$DExit`) | ja | nein — nur ROM-Debugger |
| Multi-User-Schutz (Nutzerparameter-Validierung etc.) | ja | nein |
| Externe Cache-Hardware | unterstützt | nur On-Chip-Caches |
| MMU-Speicherschutz | ja | nein |
| Modul/User-ID-Zugriffsprüfung beim Linken | ja | nein — jeder darf an jedes Modul linken |

Empfehlung aus dem Handbuch: unter dem Development-Kernel entwickeln
(bessere Fehlersuche), dann für den produktiven/Atomic-Einsatz wechseln,
sobald der Code als korrekt gilt.

## Speicherallokatoren: Standard vs. Buddy

Ebenfalls Teil der Kernel-Variante (`*s`/`*b`-Suffix):

- **Standard-Allocator** (`*s`) — Auflösung 16 Byte (1025 Byte → 1040
  Byte gerundet). Der klassische, von den meisten Systemen genutzte
  Allocator.
- **Buddy-Allocator** (`*b`) — Binary-Buddy-Algorithmus, rundet auf die
  nächste Zweierpotenz (1025 Byte → 2048 Byte). Weniger
  speichereffizient, dafür deterministischer/schneller — typisch für
  Echtzeit-/Atomic-Systeme. Colored-Memory-Listen (ROM oder Init-Modul)
  müssen bei Buddy auf die Blockgröße ausgerichtet sein.

## Init-Modul (Configuration Module)

Nicht-ausführbares Modul vom Typ `Systm` (Code `$0C`), muss beim
Kernelstart im Speicher liegen (üblich: in `OS9Boot` oder ROM). Beginnt
mit dem Standard-Modulheader, danach zusätzliche Felder ab Offset `$30`
(vollständige Tabelle: Kapitel 2, Table 2-4; Beispielcode: Anhang A,
Offset-Namen in `sys.l`):

| Offset | Name | Bedeutung | Default Atomic / Development |
|---|---|---|---|
| `$34` | `M$PollSz` | Einträge in der IRQ-Polling-Tabelle (1 je interruptfähigem Gerät) | 16 / 32 |
| `$36` | `M$DevCnt` | Größe der System-Device-Tabelle (1 je Gerät) | 8 / 32 |
| `$38` | `M$Procs` | Initiale Prozesstabellengröße (bei Atomic fix, bei Development wachsend) | 32 / 64 |
| `$3A` | `M$Paths` | Initiale Path-Tabellengröße (bei Atomic fix, bei Development wachsend) | 32 / 64 |

Nach dem Hardware-Reset führt das Boot-ROM den Kernel aus (aus ROM oder
Disk geladen); der Kernel initialisiert das System (ROM-Module lokalisieren,
Systemstart-Task — üblich `Sysgo` — starten).

## Prozesserzeugung (`F$Fork`)

Vier Schritte:

1. **Modul lokalisieren/laden** — erst im Speicher suchen, sonst von
   Mass-Storage nachladen (Dateiname = Modulname).
2. **Prozessdeskriptor allozieren+initialisieren** — Tabelle mit Status,
   Speicherbelegung, Priorität, I/O-Pfaden; automatisch verwaltet.
3. **Stack-/Datenbereich allozieren** — Größen stehen im Modulheader,
   ein zusammenhängender Speicherbereich wird reserviert.
4. **Prozess initialisieren** — Register auf Daten-/Codeadressen setzen,
   initialisierte Variablen/Pointer aus dem Objektcode in den Datenbereich
   kopieren.

Scheitert ein Schritt, wird die Prozesserzeugung abgebrochen und der
Fehler an den `fork`-Aufrufer zurückgemeldet. Bei Erfolg: neue Prozess-ID
(plus geerbte Group-/User-ID) und Einreihung in die Scheduling-Queue.

**Registerkonvention beim Prozessstart** (Table 2-6, relevant für
Compiler-Backend/Runtime-Startup):

| Register | Inhalt |
|---|---|
| `pc` | Modul-Einstiegspunkt |
| `a3` | Modul-Startadresse |
| `a1` | Top-of-Memory-Pointer |
| `a5`/`a7` | Parameter-Startadresse / Stack-Top |
| `a6` | Datenbereich-Basisadresse (niedrigste Adresse) |
| `a0`, `a2` | undefiniert |
| `sr` | `N000` (N=0 non-MSP, N=1 MSP-Systeme) |

Terminierung (`F$Exit`, Fatal Signal/Error): offene Pfade schließen,
Speicher freigeben, primäres Modul unlinken.

## Exception-/Interrupt-Verarbeitung

Vektortabelle (68020/030/040, Auszug Table 2-9):

- **Vektoren 0, 1 — Reset**: `SSP`-Initialwert (mind. 4K RAM davor/danach
  für System-Globaldaten) + Coldstart-Einstiegspunkt. Von Usercode nicht
  anfassen.
- **Vektoren 2–8, 10–24, 48–63 — Error Exceptions**: i.d.R. fataler
  Programmfehler → Prozess terminiert unbedingt (bei `F$DFork`-Prozessen
  bleiben Ressourcen für Postmortem-Debugging erhalten). Per `F$STrap`
  lässt sich ein User-Handler für die nicht-fatalen Fälle dieser Gruppe
  installieren. FPCP-Vektoren 48–54 nur bei 68020/030 relevant.
- **Vektor 9 — Trace**: Single-Step (Trace-Bit im SR), Basis für
  `F$DFork`/`F$DExec`/`F$DExit`.
- **Vektoren 64–255 — `F$IRQ`**: nutzerdefinierte vektorisierte
  Interrupts.
- Nicht vektorisierte Polling-Interrupts werden intern wie vektorisierte
  behandelt. Level-7-Interrupts sind non-maskable und sollten nur für
  z.B. DRAM-Refresh genutzt werden — dabei **keine** OS-9-Systemcalls/
  -Datenstrukturen anfassen.
- User-Trap 0 (Vektor 32) ist für OS-9-System-Service-Requests reserviert,
  die restlichen 15 User-Traps für Library-Routinen-Links zur Laufzeit.

## Offene Punkte / noch nicht ausgewertet

Nicht in dieser Notiz, aber im Technical Manual vorhanden und bei Bedarf
nachzuschlagen: `OS-9 Memory Map`, `Colored Memory` (Definition List,
System Memory Cache Lists), `Customization Modules` (`Syscache`, `SSM`,
`FPU/FPSP`), `Process Memory Areas`/`Process State`/`Process Scheduling`
(inkl. `D_MinPty`/`D_MaxAge`), vollständige Vektortabelle (Table 2-9),
sowie `Appendix D: OS-9 for 68K System Calls` (alle Syscalls +
Verfügbarkeit je Kernel-Typ).
