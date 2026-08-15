# Thema 06: Exception-/Trap-Handler — was innerhalb der Dispatch-Tabelle passiert

Thema 01 hatte gezeigt: beide Kernel bauen ihre Exception-Dispatch-Tabelle
beim Boot aus einer kompakten Quelltabelle auf. Dieses Thema fragt: was
steckt **hinter** den Tabelleneinträgen — was tun die Handler wirklich?

**Quellenlage:** 68K ist reine Zusammenfassung der umfangreichsten
Einzelrecherche aus `docs/REVERSE_ENGINEERING.md` (alle 8 Handler dort
bereits vollständig gelesen, vor dieser Session). x86 ist neue,
gezielte Untersuchung — mit einem ehrlichen Ergebnis: der Mechanismus,
der die Tabelle **befüllt**, ist jetzt bestätigt (deckt sich exakt mit
Thema 01), aber die einzelnen **Handler-Körper** selbst (inkl. dem
wichtigsten, dem Syscall-Dispatcher) konnten in dieser Runde **nicht**
lokalisiert werden — Details und Begründung unten, nicht geraten.

## 68K: Überblick über alle 8 Handler (aus `docs/REVERSE_ENGINEERING.md`, nicht neu hergeleitet)

| Handler | Deckt ab | Kernidee |
|---|---|---|
| **`Q9_disp_488`** | `TRAP #0` — **jeder** `F$`/`I$`-Syscall | Liest die Funktionsnummer direkt aus dem Code nach der `TRAP`-Instruktion, wählt eine von zwei Syscall-Tabellen (`(0x3a4,A6)`/`(0x3a8,A6)`, verschachtelt vs. User-Aufruf), springt per Dispatch-Trampolin in den Handler, prüft danach einen Stack-Kanarienvogel ("Jimi") und einen Preemption-Punkt vor der Rückkehr. **Der wichtigste Handler.** |
| `Q9_disp_180` | Autovektor-IRQs (Level 1–7) + alle User-Defined Vectors (199 von 256 Einträgen) | Verkettete Liste von Interrupt-Handler-Deskriptoren pro Vektor, Carry-Flag-Konvention "nicht meiner, weiter" für mehrere Geräte an einer IRQ-Leitung. |
| `Q9_disp_452`/`0x472` | Spurious/Uninitialized Interrupt | Knapper Zähler+Log-Pfad. Nebenfund: der periodische Uhr-Tick-Handler (Systemzeit, Zeitscheiben-Ablauf-Erkennung des Schedulers) registriert sich hier als IRQ-Hook. |
| `Q9_disp_5d0` | `TRAP #1`–`#15` | Prozesseigene, installierbare Trap-Handler (z. B. für Sprach-Laufzeiten/Debugger) — läuft im User-Kontext des Prozesses, nicht im Kernel. |
| `Q9_disp_888` | Bus-/Address-Error | Dünner Wrapper, extrahiert 68030-Fault-Zusatzfelder, fällt direkt durch in `Q9_disp_8d0`. |
| **`Q9_disp_8d0`** | Illegal Instr., Zero Div, CHK, TRAPV, Priv. Violation, Line-A/F, FPU, MMU-Fehler | Größter Sammel-Handler: FPU-Exception-Vorverarbeitung, Software-Breakpoints (Illegal-Instruction-Trick), generisches pro-Prozess-Vektor-Handler-System, Signal-Zustellung an den Prozess als Fallback. |
| `Q9_disp_ba4` | Trace | Minimal — Trace-Bit im nächsten Statuswort löschen. |

**Gemeinsame Infrastruktur**: FPU-Save/Restore-Paar (`0xfe0`/`0x1034`,
Lazy-Context-Switch), Signal-/Breakpoint-Pending-Verwaltung (`0xbc0`),
Fallback-Terminierung ohne Handler (`0xfc4`), Panik-/Diagnose-Ausgabe
(`0x7f6`/`0x850`/`0x84a`/`0x868`).

## x86: der Dispatch-Table-Builder — bestätigt, deckt sich mit Thema 01

`FUN_00221540` (240 Byte, noch nicht mit `Q9X_`-Namen versehen — siehe
"Offene Punkte") ist exakt die Funktion, die Thema 01 als
"`Q9X_dispatch_table_build`" bereits beschrieben hatte. Vollständig
gelesen, bestätigt Thema 01 Wort für Wort:

- Läuft `count`-mal (Parameter in `EAX`) über eine **kompakte
  14-Byte-Quelltabelle** (`ECX`, `ADD ECX,0xe` pro Iteration).
- Jeder Quelleintrag: Vektornummer (Word @ `ECX`), gefolgt von einem
  4-Byte-Feld (@`ECX+2`), einem 2-Byte-Feld (@`ECX+6`... via `EDI=ECX+0xa`
  gelesen) und einem weiteren 2-Byte-Feld (@`ECX+0xc`).
- Schreibt in **zwei mögliche Ziel-16-Byte-Slot-Tabellen** gleichzeitig
  (Basisadressen aus Kernel-Globals `+0x8fc`/`+0x8f8`), gesteuert über
  zwei Flag-Bits im `AX`-Parameter (`AND DI,0x2` / `AND AX,0x1`) — passt
  zu Thema 01s zwei Aufrufen mit Flag-Werten `1` und `2`.
- Bricht früh ab (`JNC`), wenn die Vektornummer eine in Kernel-Globals
  hinterlegte Obergrenze (`[EBX+0x30]`) erreicht/überschreitet.

**Aufrufkontext**: beide Aufrufe liegen innerhalb `Q9X_kernel_globals_init`
(Adressbereich `0x21eb26`–`0x21f0a3`, aus Thema 01 bekannt) — die
Quelltabellen-Adressen werden dabei als **Kernel-Globals-relative**
Offsets berechnet (`LEA EAX,[EBX+0x12c4]` bzw. `[EBX+0x16e0]`, wobei
`EBX` = Kernel-Globals-Basis), **nicht** als Offsets im statischen
Moduldateiabbild.

## Warum die einzelnen Handler-Körper (inkl. Syscall-Dispatcher) in dieser Runde nicht gefunden wurden

Das ist der zentrale Unterschied zu Thema 03 (RBF-Dispatch-Tabelle): dort
lag die Tabelle **statisch im Moduldateiabbild** (`m_idata`-Bereich),
ließ sich also direkt aus der Binärdatei auslesen und die Zieladressen
verifizieren. Hier liegt die **Quelltabelle** dagegen in **zur Laufzeit
allozierten Kernel-Globals** (`EBX+0x12c4`/`EBX+0x16e0`) — das ist
**kein Bereich, der in der statischen `vendor-live/kernel`-Datei
existiert** (Kernel-Globals werden erst beim Booten angelegt). Ein
direkter Blick in die Rohdatei an diesen Offsets liefert deshalb keine
Tabelle, sondern zufälligen Code aus einem völlig anderen Modulbereich
(verifiziert und verworfen).

Was stattdessen gefunden wurde: an mehreren Stellen **kurz vor** den
beiden `FUN_00221540`-Aufrufen installiert `Q9X_kernel_globals_init`
über das klassische x86-PIC-Idiom (`CALL $+5` / `POP EDI` / `LEA
EAX,[EDI+Offset]` — PC-relative Konstantenberechnung ohne Relozierungs-
Eintrag) mehrere feste Modul-Adressen in einzelne Kernel-Globals-Felder
(`+0xa0c`, `+0xa14`, `+0xa28`, `+0xa2c`, `+0xa38`, `+0xa30`, `+0xa34`,
`+0xa50`, `+0xa98`, ...). Eine dieser Adressen (`current+0x26b6` an einer
Stelle) errechnet sich exakt zu `0x22151C` — das ist **derselbe
`LAB_0022151c`-Sentinel-Wert**, den Thema 01 bereits als Marker für
"leerer Deskriptor-Slot" identifiziert hatte. Das bestätigt: diese
Adress-Installationen sind größtenteils bereits aus Thema 01 bekannte
Sentinel-/Marker-Werte, nicht neue Exception-Handler-Körper — die
eigentliche Quelltabelle bei `+0x12c4`/`+0x16e0` wird vermutlich erst
**aus** diesen (und weiteren, hier nicht verfolgten) Einzelwerten
zur Laufzeit zusammengesetzt, bevor `FUN_00221540` sie liest.

**Zusätzlicher Befund**: die komplette Kernel-Moduldatei enthält laut
Volltextsuche in `modules/os9000-x86/disasm/kernel_full_disasm.txt`
**keine einzige `INT`- oder `IRET`-Instruktion** (nur ein `SIDT`, reines
Auslesen der aktuellen IDT-Basisadresse, kein `LIDT`) — passt zu Thema
01s bereits dokumentiertem Befund, dass die echte Hardware-IDT
vermutlich außerhalb dieses Moduls in der Low-Level-System-Schicht
gesetzt wird. Das bedeutet aber auch: **der konkrete x86-Auslöse-
Mechanismus für einen Syscall** (klassisches `INT`, `SYSENTER`, ein
Call-Gate, oder etwas anderes) ließ sich aus diesem Modul allein nicht
bestimmen.

## Vergleichstabelle

| # | Was passiert | 68K | x86 |
|---|---|---|---|
| 1 | Dispatch-Tabelle aus kompakter Quelle beim Boot befüllen | in `Q9_kernel_init_67a0` (Thema 01) | `FUN_00221540`, 2× aus `Q9X_kernel_globals_init` (Thema 01) — **Mechanismus bestätigt, identisch im Aufbau** |
| 2 | Ort der Quelltabelle | statisch im Modul (Offset `0x3802`) | **zur Laufzeit in Kernel-Globals** (`+0x12c4`/`+0x16e0`) — echter Architekturunterschied, erklärt warum x86 hier schwerer statisch zu untersuchen ist |
| 3 | Zentraler Syscall-Dispatcher | `Q9_disp_488`, vollständig gelesen | **nicht lokalisiert** in dieser Runde |
| 4 | Auslöse-Mechanismus für einen Syscall | `TRAP #0` (68K-Instruktion, eindeutig) | **nicht bestimmt** — kein `INT`/`IRET` im Modul gefunden |
| 5 | Sonstige Exception-Handler (Bus Error, Illegal Instr., FPU, ...) | alle 7 übrigen Handler vollständig gelesen | nicht lokalisiert |
| 6 | Handler-Adressen ins System einbinden | über die kompakte Quelltabelle bei Boot | zusätzlich(!) über einzelne PC-relative Konstanten-Installationen in Kernel-Globals (neu gefunden, aber größtenteils bereits bekannte Sentinel-Werte wie `LAB_0022151c`) |

**Nachtrag (Thema 09):** der konkrete Auslöse-Mechanismus wurde
inzwischen doch gefunden — nicht im Kernel-Modul selbst, sondern auf der
**aufrufenden** Seite in einem Treiber-Modul (`scllio`, x86-Gegenstück zu
`cfide`): ein kleiner Trampolin führt einen echten `INT 0xFF` aus, mit
einem Zeiger auf einen Callcode-Parameterblock in `ECX`. Details, Code
und Einordnung siehe [Thema 09](../09-treiber-hardware/). Der Ziel-
Handler im Kernel (die `IDT`-Vektor-0xFF-Routine) bleibt weiterhin nicht
lokalisiert — nur der Auslöser ist jetzt bekannt.

## Offene Punkte

- **Der x86-Syscall-Dispatcher selbst wurde nicht gefunden** — das war
  das Hauptziel dieser Runde. Nächster sinnvoller Ansatz: Live-Speicher-
  Inspektion (wie schon für die Modul-Extraktion selbst verwendet, siehe
  `modules/os9000-x86/vendor-live/PROVENANCE.md`) — die Kernel-Globals-
  Bereiche `+0x12c4`/`+0x16e0` in einem laufenden Gast per `pmemsave`
  auslesen, dann die dort tatsächlich stehenden Werte gegen die Funktions-
  liste abgleichen. Reine statische Analyse der Moduldatei reicht dafür
  nicht aus.
- **`FUN_00221540` selbst noch nicht mit `Q9X_`-Namen versehen** (im
  Ghidra-Projekt) — sollte bei einer Vertiefungsrunde nachgeholt werden.
- Kein konkreter x86-Auslöse-Mechanismus für Syscalls identifiziert
  (`INT`, `SYSENTER`, Call-Gate?) — reine Vermutung wäre hier
  unseriös, deshalb offen gelassen.
- Die neun PC-relativ berechneten Adressen (`+0xa0c` bis `+0xa98`) sind
  nur teilweise identifiziert (eine davon = `LAB_0022151c`) — die
  übrigen acht nicht einzeln aufgelöst.

## Für den eigenen Kernel (Ergänzung zu `docs/OWN_KERNEL_INIT_PLAN.md`)

Siehe dortige neue Sektion 2c — kurz zusammengefasst: das Prinzip
"kompakte Quelltabelle → beim Boot expandierte Dispatch-Tabelle" bleibt
eine klare Übernahme-Empfehlung (jetzt zweifach bestätigt: die
Tabellen-**Struktur** stimmt exakt zwischen 68K und x86 überein, auch
wenn die Handler-Körper selbst x86-seitig nicht verifiziert werden
konnten). Neu und wichtig für den eigenen Entwurf: **wo die Quelltabelle
liegt ist eine echte Design-Entscheidung** — statisch im Modul (68K-Stil,
leichter offline zu prüfen/debuggen) oder zur Laufzeit zusammengesetzt
(x86-Stil, flexibler aber schwerer statisch nachvollziehbar). Für einen
eigenen Kernel spricht die leichtere Nachvollziehbarkeit für die
68K-Variante (statische Tabelle im Modul).

## Quellen

- 68K: [`../../REVERSE_ENGINEERING.md`](../../REVERSE_ENGINEERING.md), Abschnitte "Fund: `Q9_disp_488`", "Fund: `Q9_disp_180`", "Fund: `Q9_disp_452`/`0x472`", "Fund: `Q9_disp_5d0`", "Fund: `Q9_disp_8d0`", "Fund: `Q9_disp_888`/`Q9_disp_ba4`"
- x86: [`asm-x86.txt`](asm-x86.txt), `modules/os9000-x86/disasm/kernel_full_disasm.txt`, Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-kernel/`
- Vorherige Themen: [Thema 01](../01-kernel-bootstrap/) (Dispatch-Tabellen-Setup, `Q9X_dispatch_table_build`)

**Erstellt**: 2026-08-14
