# Thema 02: IO-Manager — wie ein Systemaufruf beim richtigen Treiber landet

Dieses Thema setzt dort an, wo Thema 01 aufhörte (Kernel hat die Kontrolle
an den Scheduler/ersten Prozess übergeben) und fragt: wenn ein Prozess
jetzt `I$Read`/`F$Open`/etc. aufruft, wie kommt der Aufruf tatsächlich
beim zuständigen Treiber oder File-Manager an?

**Wichtiger Hinweis zur Quellenlage:** Die 68K-Seite stützt sich auf
bereits vorhandene, gründliche Vorarbeit (`modules/ioman/docs/REVERSE_ENGINEERING.md`,
4 Analyserunden, plus `modules/c0-descriptor/`, `modules/cfide-driver/`,
`modules/rbf-filemanager/`) — hier nur zusammengefasst, nicht neu
hergeleitet. Die x86-Seite (`modules/os9000-x86/vendor-live/ioman`) wurde
in dieser Runde **erstmals** disassembliert — deutlich weniger tief
untersucht als die 68K-Seite (14 von 67 Funktionen benannt/gelesen, nicht
alle 4 68K-Runden-Tiefe erreicht). Wo die x86-Seite eine offene Frage
hinterlässt, steht das explizit da.

## Label-Konvention

- **68K**: Namen aus `modules/ioman/disasm/round4_all_functions.txt` (per
  Syscall-Map bereits benannt, z. B. `ioman_I_Attach_b3a`).
- **x86**: Präfix `Q9X_`, neu vergeben in dieser Runde (Ghidra-Projekt
  `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-ioman/`).

## Die zentrale Erkenntnis: derselbe Dreiklang, dieselben Zahlenwerte

Der wichtigste Fund dieser Runde: **`Q9X_ioman_attach` (x86) nutzt exakt
dieselben drei Typ-Filter-Werte wie `ioman_I_Attach_b3a` (68K)** —
`0xF00`/`0xE00`/`0xD00`, in derselben Reihenfolge:

| Schritt | Wert | Zweck | 68K-Beleg | x86-Beleg |
|---|---|---|---|---|
| 1 | `0xF00` | Geräte-**Descriptor** linken (Name = Pfad selbst) | `ioman_I_Attach_b3a`, s. Quellen unten (Runde 4) | `Q9X_ioman_attach`, [`asm-x86.txt`](asm-x86.txt) Zeile 288 (`0x23418b`) |
| 2 | `0xE00` | **Driver** linken (Name aus Descriptor-Feld) | dito | [`asm-x86.txt`](asm-x86.txt) Zeile 435 (`0x234384`) |
| 3 | `0xD00` | **File-Manager** linken (Name aus Descriptor-Feld) | dito | [`asm-x86.txt`](asm-x86.txt) Zeile 457 (`0x2343d6`) |

Diese Werte sind identisch zu den in `modules/c0-descriptor/docs/FINDINGS.md`
bestätigten `M$Type`-Codes (`0xF`=Descrptr, `0xE`=Driver, `0xD`=Fmgr, oberes
Nibble/Byte des Typ-Felds plus Sprachcode `0x00` im unteren Teil) — **über
zwei komplett unabhängige Prozessorarchitekturen hinweg unverändert**,
genau wie schon die reinen `M$Type`-Werte selbst (Thema 00). Verifiziert
zusätzlich am realen Ziel: `modules/os9000-x86/vendor-live/rbf` hat
`M$Type = 0x0D` (Fmgr) — der dritte Link-Schritt landet also nachweislich
bei einem korrekt typisierten File-Manager-Modul, nicht bei irgendetwas.

## Kernel-Service-Aufruf: eigener Mechanismus, aber strukturell dieselbe Idee

**68K** hat einen einzigen gemeinsamen Kernel-Trampolin
(`(0x3a4,A6)`/`(0x3a8,A6)`, PEA+RTS-Sprung in einen Tabellenslot) für
IOMans eigene Buchhaltung (`F$Link`/`F$Send`/`F$GProcP`/`F$SRqMem`) — der
Kernel-Globals-Basiszeiger steckt fest in **Register A6**.

**x86** macht strukturell dasselbe, aber mit einem anderen
CPU-Mechanismus: mehrere kleine Wrapper-Funktionen (`Q9X_link_module`,
`Q9X_alloc_memory`, `Q9X_devtable_lock_insert`, `Q9X_get_current_proc`)
lesen alle denselben Kernel-Globals-Basiszeiger — aber über das
**FS-Segmentregister**, nicht über ein Adressregister:

```asm
00233535: MOV EDX,dword ptr FS:[0x0]     ; Kernel-Globals-Basis lesen
00233542: MOV ECX,dword ptr [ECX + 0xa8c] ; Funktionszeiger aus Offset 0xA8C
0023354a: CALL ECX                        ; aufrufen
```

(`Q9X_kernel_service_call`, [`asm-x86.txt`](asm-x86.txt) Zeile 9-19 —
dasselbe Muster wiederholt sich in `Q9X_link_module` und `Q9X_alloc_memory`,
jeweils mit demselben Offset `0xA8C`.)

**Einordnung:** `FS:[0]` ist auf x86 klassisch das Segment, über das ein
Betriebssystem prozessorlokale/Kernel-Daten erreichbar macht (bei Windows:
das TIB; hier vermutlich eine analoge OS-9000-eigene Konvention). **68K
reserviert dafür ein Adressregister (A6), x86 reserviert dafür ein
Segmentregister (FS)** — zwei verschiedene CPU-Mechanismen für **dieselbe
architektonische Entscheidung**: ein Prozessor-Feature fest für "Zeiger auf
die Kernel-Globals" zu reservieren, statt ihn bei jedem Aufruf neu zu
übergeben. Ob alle vier x86-Wrapper-Funktionen wirklich denselben
EINEN Offset `0xA8C` nutzen (wie ein gemeinsamer Dispatch-Slot) oder ob
das nur bei den hier geprüften der Fall ist, wurde nicht an allen ~14
gelesenen Funktionen einzeln bestätigt — bei den vier explizit geprüften
(`Q9X_kernel_service_call`, `Q9X_link_module`, `Q9X_alloc_memory`,
teilweise `Q9X_devtable_lock_insert`) war es aber durchgehend derselbe
Wert.

## Struktureller Vergleich: was beide Seiten tun

| # | Was passiert | 68K | x86 |
|---|---|---|---|
| 1 | Einsprung ins Modul | `ioman_entry_98` (`M$Exec`=`0x98`, direkter Code, kein Trampolin) | `Q9X_ioman_entry` (`m_exec`=`0xDA`, **auch direkter Code** — anders als beim Kernel, wo `m_exec` auf einen JMP-Trampolin zeigte) |
| 2 | Init: Tabellen anfordern, Fehlerdiagnose bei Speichermangel | `ioman_entry_98` ruft 2× `F$SRqMem`, eigene Hex-Ausgabe-Routinen bei Fehler | `Q9X_ioman_entry` (784 Byte, nicht im Detail bis zum letzten Byte gelesen — Grobstruktur ähnlich: Feld-Setup, Konstanten wie `0xC00` gesetzt) |
| 3 | Pfad öffnen / Gerät auflösen | `I$Open` → bei Bedarf `I$Attach` | `Q9X_ioman_open` → bei Bedarf `Q9X_ioman_attach` (`asm-x86.txt`, ruft `Q9X_ioman_attach` direkt auf) |
| 4 | **Dreiklang**: Descriptor→Driver→File-Manager linken | `ioman_I_Attach_b3a`: 3× `F$Link` mit Filtern `0xF00`/`0xE00`/`0xD00` | `Q9X_ioman_attach`: 3× `Q9X_link_module` mit **denselben** Filtern `0xF00`/`0xE00`/`0xD00` |
| 5 | Bereits attached? Link-Count statt Neuanlage | `ioman_I_Attach_b3a`, Gerätetabellen-Vergleich | `Q9X_ioman_attach`, Gerätetabellen-Walk (`local_8`-Schleife in `decompiled` — Struktur ähnlich, nicht 1:1 gegengelesen) |
| 6 | Direkter Sprung in den Treiber (Init/Term) | `ioman_I_Detach_e5e`: `jmp (0x0,A0,D2w*1)` — direkter Sprung, kein Rücksprung über IOMan | **x86-Äquivalent in dieser Runde nicht gefunden** — offene Frage, s. u. |
| 7 | Generischer `I$`-Callcode → Treiber-/Fmgr-Tabellenslot | `FUN_000014f8`: `(Callcode−0x83)` als Wort-Index in eine 13-Slot-Tabelle (bestätigt an `rbf`) | **x86-Äquivalent in dieser Runde nicht gefunden** — offene Frage, s. u. |

## Offene Frage: der eigentliche Treiber-Sprung auf x86 — Ziel-Ende geklärt, IOMan-seitiger Aufrufer noch offen

Bei 68K ist der Übergang "IOMan → Treiber-/File-Manager-Code" doppelt
belegt (Slot-basierter `jmp` in `I$Detach`, callcode-indizierter
Dispatcher `FUN_000014f8` für die "einfachen" `I$`-Aufrufe). Bei x86 wurde
in dieser Runde **nur der Attach-Mechanismus** (das Linken der drei
Module) gefunden — der Mechanismus, der NACH einem erfolgreichen Attach
bei einem tatsächlichen `I$Read`/`I$Write`/etc. den Sprung in den
File-Manager/Treiber auslöst, ist unter den 14 bisher benannten
Funktionen nicht dabei. `modules/os9000-x86/vendor-live/ioman` hat noch
**53 unbenannte Funktionen** — eine gute nächste Runde, falls das
gebraucht wird (Kandidaten: die größeren, noch nicht gelesenen Funktionen
zwischen `0x235f68` und `0x2370c8`, siehe `Q9-OS-ghidra-os9000-ioman`-Projekt).
Ehrlich offen gelassen statt geraten.

**Nachtrag (Thema 03, 2026-08-14):** die **Ziel-Seite** dieser Frage ist
jetzt geklärt — RBF hat eine echte 16-Slot-Callcode-Dispatch-Tabelle,
allerdings nicht bei `m_exec` wie beim 68K, sondern im `m_idata`-Bereich
(Details, inklusive Beleg über 9 zuvor unbekannte Funktionsadressen mit
gültigem Prolog: [`../03-dreiklang/README.md`](../03-dreiklang/README.md)).
Der **IOMan-seitige** Aufrufer, der diese Tabelle liest und indiziert,
bleibt weiterhin offen — dieselbe Lücke wie oben beschrieben.

## C-Variante

Nur für die zentralen x86-Funktionen versucht, wo es zum Verständnis
beiträgt (`Q9X_ioman_attach` zeigt die Filter-Werte `0xF00`/`0xE00`/`0xD00`
im Kontext klarer als reines Assembler) — siehe rohe Ghidra-Pseudo-C-Zitate
oben im Text, keine eigene `decompiled-x86.c`-Datei diesmal, da die
zitierten Ausschnitte direkt im Fließtext ausreichen und eine komplette
Datei (wie bei Thema 01) hier weniger zusätzlichen Wert hätte.

## Quellen

- 68K: [`../../../modules/ioman/docs/REVERSE_ENGINEERING.md`](../../../modules/ioman/docs/REVERSE_ENGINEERING.md) (4 Runden), [`../../../modules/c0-descriptor/docs/FINDINGS.md`](../../../modules/c0-descriptor/docs/FINDINGS.md), [`../../../modules/cfide-driver/docs/FINDINGS.md`](../../../modules/cfide-driver/docs/FINDINGS.md), [`../../../modules/rbf-filemanager/docs/FINDINGS.md`](../../../modules/rbf-filemanager/docs/FINDINGS.md)
- x86: [`asm-x86.txt`](asm-x86.txt) (frische Disassemblierung, `Q9X_`-Namen), Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-ioman/`
- Provenienz x86-`ioman`-Binary: [`../../../modules/os9000-x86/vendor-live/PROVENANCE.md`](../../../modules/os9000-x86/vendor-live/PROVENANCE.md)

**Erstellt**: 2026-08-14
