# Thema 03: der Dreiklang im Detail — Descriptor, Driver, File-Manager

Thema 02 hatte den **Link**-Mechanismus geklärt (IOMan linkt Descriptor→
Driver→File-Manager mit den Filtern `0xF00`/`0xE00`/`0xD00`, identisch bei
68K und x86). Dieses Thema geht einen Schritt weiter: was steckt in den
drei Modulen selbst, und wie findet ein tatsächlicher `I$Read`/`I$Write`
seinen Weg zum passenden Code? Dabei konnte **die in Thema 02 offen
gelassene Frage geklärt werden** (s. u.).

**Quellenlage:** Die 68K-Seite ist reine Zusammenfassung bereits
vorhandener Vorarbeit (`modules/c0-descriptor/`, `modules/cfide-driver/`,
`modules/rbf-filemanager/`, alle fertig) — nichts davon wurde neu
disassembliert. Die x86-Seite (`modules/os9000-x86/vendor-live/rbf`) wurde
in dieser Runde **erstmals** untersucht, gezielt auf die
Dispatch-Tabellen-Struktur hin (nicht das ganze 38.672-Byte-Modul).

## Label-Konvention

- **68K**: Namen aus den bestehenden `FINDINGS.md`-Dateien.
- **x86**: Präfix `Q9X_`, neu vergeben (Ghidra-Projekt
  `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-rbf/`).

## Die drei Modultypen im Überblick

| Modultyp | 68K-Beispiel | x86-Beispiel | `M$Type` | Rolle |
|---|---|---|---|---|
| Descriptor | `c0` (148 Byte, reines Datenmodul) | *(nicht Teil dieser Runde — kein x86-Descriptor extrahiert)* | `0x0F` | Beschreibt EIN konkretes Gerät: welcher Treiber, welcher File-Manager, Hardware-Parameter |
| Driver | `cfide` (1.462 Byte) | *(nicht Teil dieser Runde)* | `0x0E` | Spricht die Hardware direkt an, klassische 6-Slot-Tabelle (Init/Read/Write/GetStat/SetStat/Term) |
| File-Manager | `rbf` (9.638 Byte) | `rbf` (38.672 Byte, `vendor-live/`) | `0x0D` | Setzt die 13 (68K) bzw. 16 (x86) `I$`-Dateisystem-Operationen um |

Der Fokus dieser Runde lag bewusst auf dem **File-Manager**, weil dort die
in Thema 02 offene Frage (der eigentliche Dispatch-Sprung) am ehesten zu
beantworten war — das 68K-Vorbild hatte schon gezeigt, dass genau dort die
entscheidende Tabelle sitzt (s. u.). Treiber/Descriptor auf x86-Seite
wurden in dieser Runde nicht angefasst.

## Die zentrale Erkenntnis: Thema 02s offene Frage geklärt — aber mit einem echten Architekturunterschied

**68K**: Die 13-Slot-Callcode-Tabelle (`I$Create`…`I$Close`, Callcodes
`0x83`–`0x8F`) sitzt **an der Adresse, auf die `M$Exec` zeigt** (Datei-
Offset `0x30` im Header = Tabellenbasis `0xA6`). IOMans `FUN_000014f8`
liest `M$Exec`, indiziert mit `(Callcode−0x83)×2` hinein, springt direkt.

**x86**: `m_exec` (Header-Feld `0x24`) zeigt **nicht** auf die Tabelle,
sondern auf trivialen Code (`Q9X_rbf_entry_trampolin` → `Q9X_rbf_entry_stub`:
nur `XOR EAX,EAX; RET` — ein Platzhalter/No-Op-Init). Die echte 16-Slot-
Tabelle liegt stattdessen im **`m_idata`-Bereich** (Header-Feld `0x34`,
"offset to initialized data"): bei Datei-Offset `0x9118` steht ein
3-Word-Kopf `[0, 0x588(=m_data), 0x10(=16=Slot-Anzahl)]`, direkt gefolgt
von der eigentlichen 16-Eintrags-Tabelle bei `0x9124` (Details/Rohbytes:
[`asm-x86.txt`](asm-x86.txt)).

**Beleg, dass es sich wirklich um eine Dispatch-Tabelle handelt** (nicht
Zufall): von den 16 Tabellenwerten zeigten **7 bereits vor dem gezielten
Nachschauen auf einen von Ghidras Auto-Analyse gefundenen Funktionsanfang**
— die restlichen **9 lagen an Adressen, die Ghidra bis dahin überhaupt
nicht als Code kannte** ("NO FUNCTION CONTAINS THIS"). Gezielte
Disassemblierung genau dieser 9 Adressen ergab bei **allen neun** einen
sauberen, gültigen x86-Funktionsprolog (`PUSH EBP; MOV EBP,ESP; ...`) —
das sind also echte Funktionen, die ausschließlich über diese Tabelle
erreichbar sind und die eine reine Kontrollfluss-Analyse nie gefunden
hätte. Genau das Muster, das man von einer Sprungtabelle erwartet.

**Einordnung des Unterschieds:** Beim 68K (handgeschriebener Assembler)
legt der Programmierer die Tabelle bewusst an die durch `M$Exec`
bezeichnete Stelle. Beim x86-C-Compiler landet ein statisch initialisiertes
Funktionszeiger-Array (z. B. `static void (*dispatch[])() = {...}` im
Quelltext) automatisch im `initialized data`-Segment, das der Linker über
`m_idata` referenziert — `m_exec` bleibt für den *tatsächlichen*
Modul-Einsprung reserviert (hier: ein Minimal-Init, der praktisch nichts
tut). **Kein Bruch im Konzept, nur eine andere Werkzeugkette**, die das
Ergebnis an eine andere, dafür aber offiziell dokumentierte Header-Stelle
(`m_idata`) verschiebt.

## Reihenfolge/Anzahl der Slots: 13 (68K) vs. 16 (x86)

Die ersten 13 x86-Slots passen positionell exakt zur bewährten 68K-
Reihenfolge (Create/Open/MakDir/ChgDir/Delete/Seek/Read/Write/ReadLn/
WritLn/GetStt/SetStt/Close) — das ist eine **Positions-Hypothese nach
Analogie**, nicht einzeln gegen echte `I$`-Aufrufe verifiziert (dafür
müsste man IOMans Aufrufer-Code bis zu dieser Tabelle zurückverfolgen,
in dieser Runde nicht mehr geleistet). **Slots 13–15 sind neu** und nicht
identifiziert — passend zu der bereits an anderer Stelle dokumentierten
Beobachtung, dass OS-9000 "deutlich mehr `F$`/`I$`-Funktionalität" als
68K-OS-9 hat (`modules/os9000-x86/docs/FINDINGS.md`, Fund 2). Ehrlich
offen gelassen statt geraten, wofür sie stehen.

## Vergleichstabelle: der volle Aufrufweg

| # | Schritt | 68K | x86 |
|---|---|---|---|
| 1 | Prozess ruft z. B. `I$Read` (`TRAP #0`/x86-Äquivalent) | Kernel-Dispatcher `Q9_disp_488` | (nicht Teil dieser Runde — Thema 01 hat den x86-Kernel-Bootstrap geklärt, der laufende Syscall-Pfad selbst ist ein möglicher künftiger Thema-Kandidat) |
| 2 | Zieladresse in IOMan | `../SYSCALL_MODULE_MAP.md` | Thema 02 |
| 3 | IOMans gemeinsamer Callcode-Dispatcher | `FUN_000014f8` (bestätigt) | **`Q9X_ioman_callcode_dispatch`** — in einer Folgerunde gefunden (2026-08-14), Details: [`../02-io-manager-syscall-dispatch/README.md`](../02-io-manager-syscall-dispatch/README.md), Abschnitt "Geklärt" |
| 4 | Callcode-Index in File-Manager-Tabelle | RBFs 13-Slot-Tabelle **bei `M$Exec`** | RBFs 16-Slot-Tabelle **bei `m_idata+0xC`**, per Attach in den Geräte-Eintrag gecacht |
| 5 | Sprung direkt in File-Manager-Code | bestätigt, kein Rücksprung über IOMan | `Q9X_ioman_dispatch_invoke` — kein normaler `CALL`, sondern derselbe RET-basierte Sprung-Trampolin wie beim Kernel-Bootstrap (Thema 01) |
| 6 | File-Manager → Treiber für eigentlichen Blockzugriff | vermutet (RBF→cfide, Slots 1/2), nicht einzeln verifiziert | nicht untersucht |

## C-Variante

Nicht versucht — die entscheidenden x86-Funktionen in dieser Runde
(Trampolin, Stub, Checksumme) sind zu kurz/trivial, als dass eine
Dekompilierung zusätzlichen Wert brächte; die eigentlichen `I$`-Handler
wurden bewusst nur bis zum Funktionsprolog gelesen (Bestätigung der
Tabellen-Hypothese war das Ziel, nicht die einzelnen Handler-Implementierungen).

## Offene Punkte

- ~~Der genaue IOMan-seitige Code, der `m_idata+0xC` liest und indiziert~~
  **Geklärt (2026-08-14, Folgerunde):** `Q9X_ioman_callcode_dispatch` +
  `Q9X_ioman_dispatch_invoke`, siehe
  [`../02-io-manager-syscall-dispatch/README.md`](../02-io-manager-syscall-dispatch/README.md),
  Abschnitt "Geklärt".
- Bedeutung der x86-Slots 13–15 unbekannt.
- Descriptor/Driver auf x86-Seite nicht untersucht (kein Descriptor
  extrahiert, `cfide`-Äquivalent nicht identifiziert/analysiert).
- Der letzte Schritt (File-Manager → Treiber für den eigentlichen
  Hardwarezugriff) ist bei beiden Architekturen nicht bis ins Detail
  nachverfolgt.

## Quellen

- 68K: [`../../../modules/c0-descriptor/docs/FINDINGS.md`](../../../modules/c0-descriptor/docs/FINDINGS.md), [`../../../modules/cfide-driver/docs/FINDINGS.md`](../../../modules/cfide-driver/docs/FINDINGS.md), [`../../../modules/rbf-filemanager/docs/FINDINGS.md`](../../../modules/rbf-filemanager/docs/FINDINGS.md) — Rohbyte-Auszüge in [`68k-structures.txt`](68k-structures.txt)
- x86: [`asm-x86.txt`](asm-x86.txt), Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-rbf/`
- Provenienz x86-`rbf`-Binary: [`../../../modules/os9000-x86/vendor-live/PROVENANCE.md`](../../../modules/os9000-x86/vendor-live/PROVENANCE.md)
- Vorherige offene Frage: [`../02-io-manager-syscall-dispatch/README.md`](../02-io-manager-syscall-dispatch/README.md), Abschnitt "Offene Frage"

**Erstellt**: 2026-08-14
