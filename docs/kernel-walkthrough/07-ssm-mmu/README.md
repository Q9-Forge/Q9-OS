# Thema 07: SSM — wie und wann die MMU initialisiert wird

Andreas' Frage: "Weißt du, wann in OS-9 (oder OS9000) die MMU
initialisiert wird? Oder wird das indirekt über die Systemcalls gemacht?
In den Treibern oder Deskriptoren wird das vermutlich nicht gemacht,
oder?" — Antwort direkt aus dem offiziellen `68k_tech.pdf` (nicht
geraten): **Die MMU wird von einem eigenen, separat geladenen Modul
verwaltet — `SSM`, "System Security Module"** — nicht vom Kernel, nicht
von Treibern/Deskriptoren. Dieses Thema untersucht SSM selbst, mit
echten Binaries auf beiden Seiten.

**Quellenlage:** komplett neues Thema, keine 68K-Vorarbeit vorhanden —
beide Seiten in dieser Runde erstmals disassembliert.

- **68K**: [`../../../modules/ssm/vendor/ssm851`](../../../modules/ssm/vendor/ssm851) (jetzt im Repo, siehe [`PROVENANCE.md`](../../../modules/ssm/vendor/PROVENANCE.md)) — das für Q9/CB030
  (68030) relevante Modul laut Manual (unterstützt MC68851-PMMU sowie
  die in 68020/68030 integrierte MMU).
- **x86**: `modules/os9000-x86/vendor-live/ssm` (live extrahiert, RAM-
  Adresse `0x2378C4`, siehe `../../../modules/os9000-x86/vendor-live/PROVENANCE.md`).

## Header bestätigt: SSM ist ein ganz normales Systemmodul

Beide Header entsprechen exakt dem aus Thema 00 bekannten Schema:

| | 68K `ssm851` | x86 `ssm` (live) |
|---|---|---|
| Sync | `0x4AFC` | `0x4AFC` |
| Type/Lang | `0x0C` (Systm) / `0x01` (Objct) | `0x0C` (Systm) / `0x01` (Objct) |
| Größe | 1.908 Byte | 4.744 Byte (statisch aus `mw86.tar`: 4.672 Byte — dieselbe "live ist ein anderer Build"-Abweichung wie bei allen anderen x86-Modulen dieser Session) |
| Einsprung | `M$Exec` = `0x54` | `m_exec` = `0x98` |

**Keine Überraschung, aber wichtig als Beleg**: SSM ist typisch `Systm`
(`0x0C`), genau wie Kernel/IOMan — kein eigener, spezieller Modultyp.
Andreas' Vermutung war also doppelt richtig: nicht in Treibern/
Deskriptoren (die hätten Type `0x0E`/`0x0F`), sondern in einem
gewöhnlichen Systemmodul.

## 68K: `Q9_ssm851_entry` — Installation prüfen, Speicherblockgröße setzen, echter Syscall

Die 124-Byte-Einsprungfunktion (`asm-68k.txt`) macht im Kern:

1. Prüft ein Kernel-Global-Feld (`(0x3d8,A6)`) — bereits installiert?
   Wenn ja, überspringen (Idempotenz-Schutz).
2. **Schreibt `0x1000` (4096 — die klassische MMU-Seitengröße) in
   `(0x7c,A6)`** — das ist **derselbe Offset**, den unser eigenes
   `src/q9sysglob.h` bereits als `Q9_D_BLKSIZ` ("minimale allozierbare
   Systemblockgröße") führt! Starker, unabhängiger Beleg, dass diese
   Kernel-Global-Konstante tatsächlich mit der MMU-Seitengröße
   zusammenhängt — SSM setzt sie beim Installieren auf die Seitengröße
   der jeweiligen CPU/MMU-Variante.
3. Markiert sich selbst als installiert, hängt eine eigene Callback-
   Routine in eine Systemstruktur ein.
4. **Macht einen echten `TRAP #0`-Aufruf** (`move.w #0xc00,D0w` — Funktions-
   code `0x0C`, Namenszeiger in `A0`) — SSM nutzt also ganz normal den
   Kernel-Syscall-Weg (F$Link-artig), keine interne Abkürzung.

**Woanders im Modul** (`FUN_000006ea`) findet sich echte
MMU-*angrenzende* Registerarbeit: `MOVEC SFC,D0`/`MOVEC D0,SFC` — Lesen
und Setzen des **Source Function Code**-Registers (Werte `1`=User-Data,
`5`=Super-Data beobachtet). Das ist der 68K-Mechanismus, der bestimmt,
welcher Adressraum/welche MMU-Tabelle ein Speicherzugriff nutzt — SSMs
Beitrag zur MMU ist also eher **Funktionscode-basierte Zugriffssteuerung**
als direktes Programmieren der Übersetzungstabellen.

**Wichtig, ehrlich markiert**: `ssm851` selbst enthält **keine einzige
`PMOVE`-Instruktion** (die eigentlichen PMMU-Register `CRP`/`SRP`/`TC`
werden hier nicht direkt geschrieben) — vollständig durchsucht. Entweder
geschieht das echte Tabellen-Setup in einem tieferen, hier nicht gelesenen
Funktionsteil, oder es ist Teil einer anderen, noch nicht identifizierten
Komponente (z. B. dem Boot-ROM selbst, siehe Thema 10 "Boot-Vorkette" —
guter Anknüpfungspunkt für eine spätere Vertiefung).

## x86: `CR3`-Programmierung gefunden — die direkte Entsprechung

Anders als beim 68K wurde bei x86 **echte MMU-Registerarbeit** gefunden:
vier `MOV CR3,EAX`/`MOV CR3,EDX`-Instruktionen (in zwei größeren, noch
nicht vollständig gelesenen Funktionen). `CR3` ist x86s
Seitentabellen-Basisregister — das direkte Gegenstück zu 68Ks `CRP`/`SRP`.

Der gelesene Kontext eines Treffers (`asm-x86.txt`) zeigt ein plausibles
Muster: über das aus Thema 02 bekannte `FS:[0]`-Kernel-Globals-Muster
wird ein Feld gelesen und mit einem Prozess-/Kontext-Zeiger verglichen —
**nur wenn der zu löschende Adressraum gerade der aktive ist**, wird `CR3`
auf `0` gesetzt (Seitentabelle ungültig machen/TLB-Effekt). Das sieht nach
einer **Adressraum-Freigabe-Routine** aus (Prozess terminiert → seine
Seitentabelle wird ungültig gemacht, aber nur falls sie gerade geladen
war), nicht nach der initialen MMU-Aktivierung selbst.

## Vergleichstabelle

| # | Was passiert | 68K (`ssm851`) | x86 (`ssm`) |
|---|---|---|---|
| 1 | Modultyp | `Systm` (`0x0C`) — wie erwartet, kein Sondertyp | `Systm` (`0x0C`) — identisch |
| 2 | Installationsprüfung (Idempotenz) | ja, Kernel-Global-Flag | nicht in dieser Runde gesucht |
| 3 | Speicherblock-/Seitengrößen-Konstante setzen | ja — `0x1000` in `Q9_D_BLKSIZ`-Offset | nicht in dieser Runde gefunden |
| 4 | Direkte MMU-Übersetzungsregister programmieren | **nein** — kein `PMOVE` im Modul gefunden | **ja** — `MOV CR3,...` viermal gefunden |
| 5 | Funktionscode-/Zugriffssteuerung | `MOVEC SFC` (User-/Super-Data-Umschaltung) | nicht in dieser Runde untersucht |
| 6 | Echter Kernel-Syscall genutzt | ja, `TRAP #0` beim Einsprung | nicht verifiziert |

**Einordnung:** Beide Module tragen nachweislich zur MMU-/Speicherschutz-
Verwaltung bei, aber mit unterschiedlichem Schwerpunkt in dem, was diese
Runde tatsächlich gelesen hat — 68K eher Funktionscode-Umschaltung +
Konstanten-Setup, x86 eher direkte Seitentabellen-Verwaltung beim
Adressraum-Wechsel. Das ist **keine vollständige Aussage über das gesamte
MMU-Verhalten** beider Module (siehe "Offene Punkte") — beide Module haben
deutlich mehr ungelesenen Code als gelesenen.

## Wann wird SSM relativ zu Kernel/IOMan geladen?

**Nicht in dieser Runde direkt belegt** (kein Boot-Sequenz-Log
untersucht) — aber aus der Struktur ableitbar: SSM nutzt in seiner
Einsprungfunktion bereits einen echten `TRAP #0`-Syscall (68K) bzw. das
volle Kernel-Globals-Schema über `FS:[0]` (x86) — das setzt voraus, dass
der Kernel selbst (Thema 01) und mindestens das grundlegende Modul-
Link-System bereits laufen. Passt zur bereits aus dem Manual bekannten
Aussage: SSM wird vom **Init-Modul** geladen, also **nach** dem reinen
Kernel-Bootstrap, aber **vor** dem ersten User-Prozess (da User-State-
Speicherschutz ab dem ersten Prozess gelten muss). Eine exakte Verortung
bräuchte Thema 10 (Boot-Vorkette/Init-Modul).

## Offene Punkte

- **68K**: wo die eigentlichen PMMU-Übersetzungstabellen (`CRP`/`SRP`/`TC`)
  gesetzt werden, bleibt ungeklärt — nicht in `ssm851` selbst gefunden.
- **x86**: die beiden CR3-tragenden Funktionen (`FUN_00237a63`,
  `FUN_00238216`) nicht vollständig gelesen, nicht umbenannt — nur der
  unmittelbare Kontext eines Treffers.
- Exakter Boot-Zeitpunkt von SSM relativ zu IOMan nicht direkt belegt,
  nur aus der Struktur plausibilisiert.
- x86-Äquivalent zu 68Ks `TRAP #0`-Nutzung/Idempotenz-Prüfung/
  Blockgrößen-Konstante nicht gesucht.
- Der Fehlercode `0xE7` am Ende von `Q9_ssm851_entry` nicht identifiziert
  (könnte "bereits installiert" oder ein anderer Statuswert sein).

## Für den eigenen Kernel (Ergänzung zu `docs/OWN_KERNEL_INIT_PLAN.md`)

Siehe dortige neue Sektion 6 — kurz zusammengefasst: MMU-/Speicherschutz-
Verwaltung als **eigenständiges, optionales Modul** (nicht im Kernel selbst
verdrahtet) ist eine bewusste, bei beiden Architekturen bestätigte
Design-Entscheidung — passt zur bereits dokumentierten Atomic-/Development-
Kernel-Unterscheidung (Atomic kommt ganz ohne SSM aus). Für den eigenen
Kernel: MMU-Unterstützung als optionales, nachladbares Modul zu behandeln
(nicht Kernel-Pflichtbestandteil) ist eine klare Übernahme-Empfehlung.

## Quellen

- Manual: `MWOS/DOC/RadiSys/68k_tech.pdf`, Kapitel "The Kernel", Abschnitt "SSM"
- 68K-Binary: [`../../../modules/ssm/vendor/ssm851`](../../../modules/ssm/vendor/ssm851) (Provenienz: [`PROVENANCE.md`](../../../modules/ssm/vendor/PROVENANCE.md))
- x86-Binary: [`../../../modules/os9000-x86/vendor-live/ssm`](../../../modules/os9000-x86/vendor-live/ssm), [`../../../modules/os9000-x86/vendor/ssm`](../../../modules/os9000-x86/vendor/ssm)
- [`asm-68k.txt`](asm-68k.txt), [`asm-x86.txt`](asm-x86.txt)
- Ghidra-Projekte: `/Volumes/SSD1TB/projects/Q9-OS-ghidra-ssm851/`, `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-ssm/`
- Skripte: [`../../../modules/ssm/ghidra_scripts/ssm851_AnalyzeAndDump.java`](../../../modules/ssm/ghidra_scripts/ssm851_AnalyzeAndDump.java), [`../../../modules/os9000-x86/ghidra_scripts/ssm_AnalyzeAndDump.java`](../../../modules/os9000-x86/ghidra_scripts/ssm_AnalyzeAndDump.java)

**Erstellt**: 2026-08-14
