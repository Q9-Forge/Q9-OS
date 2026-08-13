# Thema 00: Kernel-Modul-Aufbau und Header

Bevor der Kernel irgendetwas initialisiert, muss ihn zuerst jemand als
gültiges OS-9-Modul erkennen und an seinen Einsprungpunkt springen. Beide
Header-Formate sind **offiziell dokumentiert** (nicht geraten) in den
lokal vorliegenden Technical Manuals:

- 68K: `MWOS/DOC/RadiSys/68k_tech.pdf`, Kapitel 1, Table 1-6/1-7/1-8
- OS-9000: `MWOS/DOC/RadiSys/os9k_tech.pdf`, Kapitel 1, C-Struct `mh_com`

## So sieht der Header beim OS-9/68K-Kernel aus

Alle Werte real aus `vendor/68020/dker030s` (dem Original-Kernel-Binary)
ausgelesen:

| Offset | Feld | Breite | Wert | Bedeutung |
|---|---|---|---|---|
| `0x00` | `M$ID` | 2 Byte | `0x4AFC` | Sync-Bytes, damit findet man das Modul im Speicher |
| `0x02` | `M$SysRev` | 2 Byte | `1` | Format-Revision |
| `0x04` | `M$Size` | 4 Byte | `28.476` | Gesamtgröße des Moduls in Byte |
| `0x08` | `M$Owner` | 4 Byte | `0` | Owner-ID |
| `0x0C` | `M$Name` | 4 Byte | `0x6F30` | Offset zum Namens-String, relativ zum Modulanfang |
| `0x10` | `M$Accs` | 2 Byte | `0x0555` | Zugriffsrechte (r-x r-x r-x) |
| `0x12` | `M$Type` | 1 Byte | `0x0C` | Modultyp: `Systm` (12) = Systemmodul |
| `0x13` | `M$Lang` | 1 Byte | `0x01` | Sprache: `Objct` (1) = Maschinencode |
| `0x14` | `M$Attr` | 1 Byte | `0xA0` | Attribute: system-state + reentrant |
| `0x15` | `M$Revs` | 1 Byte | `0` | Revisionsstufe |
| `0x16` | `M$Edit` | 2 Byte | `375` | Build-/Edition-Zähler |
| `0x18` | `M$Usage` | 4 Byte | `0` | Offset Kommentar-String (unbenutzt) |
| `0x1C` | `M$Symbol` | 4 Byte | `0` | Offset Symboltabelle (reserviert) |
| `0x20` | `M$Ident` | 2 Byte | `0` | Ident-Code (unbenutzt) |
| `0x22`–`0x27` | *reserviert* | 6 Byte | `0` | — |
| `0x28` | `M$HdExt` | 4 Byte | `0` | Offset Header-Erweiterung |
| `0x2C` | `M$HdExtSz` | 2 Byte | `0` | Größe der Header-Erweiterung |
| `0x2E` | `M$Parity` | 2 Byte | `0x1D2D` | Prüfsumme — **Standard-Header endet hier, bei Byte 46** |
| `0x30` | `M$Exec` | 4 Byte | `0x54` | **Einsprungpunkt** |
| `0x34` | `M$Excpt` | 4 Byte | `0` | Trap-Einsprung für unbehandelte User-Traps (hier: keiner) |
| `0x38`+ | *(kein offizielles Feld für Systemmodule)* | — | — | ab hier nur noch Microware-interne Konvention, siehe Thema-Text unten |
| `0x54` | *(Programmcode)* | — | `BRA.W` | Sprung über den ID-String, landet bei `0x67A0` — der echten Init-Funktion |

## So sieht der Header beim OS-9000/x86-Kernel aus

Alle Werte real aus `modules/os9000-x86/vendor-live/kernel` (der live
laufenden Kopie) ausgelesen:

| Offset | Feld | Breite | Wert | Bedeutung |
|---|---|---|---|---|
| `0x00` | `m_sync` | 2 Byte | `0x4AFC` | dasselbe Sync-Feld wie beim 68K |
| `0x02` | `m_sysrev` | 2 Byte | `2` | Format-Revision |
| `0x04` | `m_size` | 4 Byte | `76.944` | Gesamtgröße des Moduls in Byte |
| `0x08` | `m_owner` | 4 Byte | `0` | Owner-ID |
| `0x0C` | `m_name` | 4 Byte | `0x58` | Offset zum Namens-String — **derselbe Feldtyp wie beim 68K** |
| `0x10` | `m_access` | 2 Byte | `0x0555` | Zugriffsrechte — identischer Wert wie beim 68K |
| `0x12` | `m_tylan` | 2 Byte | `0x0C01` | Type+Lang **in einem Feld** (Type=`0x0C`, Lang=`0x01` — dieselben Werte wie beim 68K, s. Kasten unten) |
| `0x14` | `m_attrev` | 2 Byte | `0xA000` | Attr+Revs **in einem Feld** (Attr=`0xA0`, Revs=`0` — dieselben Werte wie beim 68K) |
| `0x16` | `m_edit` | 2 Byte | `205` | Build-/Edition-Zähler |
| `0x18` | `m_needs` | 4 Byte | `0` | Hardware-Anforderungsflags |
| `0x1C` | `m_share` | 4 Byte | `0` | Offset Shared-Data |
| `0x20` | `m_symbol` | 4 Byte | `0` | Offset Symboltabelle |
| `0x24` | `m_exec` | 4 Byte | `0xA4` | **Einsprungpunkt** |
| `0x28` | `m_excpt` | 4 Byte | `0` | Trap-Einsprung (hier: keiner) |
| `0x2C` | `m_data` | 4 Byte | `7.008` | Größe des Datenbereichs |
| `0x30` | `m_stack` | 4 Byte | `16.384` | Stackgröße (16 KB) |
| `0x34` | `m_idata` | 4 Byte | `68.952` | Offset initialisierte Daten |
| `0x38` | `m_idref` | 4 Byte | `75.968` | Offset Datenreferenzlisten |
| `0x3C`–`0x4B` | `m_init`/`m_term`/`m_dbias`/`m_cbias` | je 4 Byte | `0` | ungenutzt |
| `0x4C` | `m_ident` | 2 Byte | `0` | ungenutzt |
| `0x4E`–`0x55` | *reserviert* | 8 Byte | `0` | — |
| `0x56` | `m_parity` | 2 Byte | `0x4E0D` | Prüfsumme — **Header endet hier, bei Byte 88** |
| `0x58` | Name | — | `"kernel\0"` | direkt nach dem Header |
| `0xA4` | *(Programmcode)* | — | `JMP` | Sprung über den Copyright-String, landet bei `0x21E4C0` — der echten Init-Funktion |

## Der wichtigste Unterschied: kein verschobenes Feld, sondern ein längerer Header

Bis Byte `0x17` sind beide Header **praktisch identisch** — gleiche
Felder, gleiche Werte, nur Type/Lang und Attr/Revs sind bei x86 zu je
einem 16-Bit-Wert zusammengelegt (Kasten unten). **Danach trennen sich die
Wege**, aus einem einfachen Grund: Der 68K-Header ist nach 46 Byte fertig
(`M$Parity` bei `0x2E`) — alles danach (`M$Exec` bei `0x30` usw.) ist eine
**optionale** Erweiterung, die es nur bei bestimmten Modultypen gibt. Der
OS-9000-Header dagegen ist **eine einzige, 88 Byte lange Struktur**, die
mehrere Felder, die beim 68K nur optional waren (Einsprungpunkt, Trap-
Einsprung, Datengröße, Stackgröße, ...), fest eingebaut hat — **jedes**
OS-9000-Modul hat sie, unabhängig vom Typ. Deswegen ergibt ein reiner
"Feld X bei Offset Y" Vergleich ab `0x18` keinen Sinn mehr — es ist kein
verschobenes Feld, sondern ein bewusst vereinheitlichter, längerer Header.

**Was inhaltlich trotzdem entspricht:** `M$Exec`(68K)/`m_exec`(x86) sind
derselbe Einsprungpunkt-Mechanismus, nur an unterschiedlicher Position.

### Kasten: warum Type/Lang und Attr/Revs "vertauscht" aussehen

68K hat `M$Type`+`M$Lang` (und `M$Attr`+`M$Revs`) als **zwei unabhängige
Einzelbytes** — bei Einzelbytes gibt es keine Byte-Reihenfolge-Frage. Der
C-Neuschrieb bei OS-9000 hat je zwei dieser Bytes zu **einem** 16-Bit-Feld
zusammengelegt (`m_tylan`, `m_attrev`). Ein 16-Bit-Wert wird aber — genau
wie die Modulgröße oder das Sync-Byte — je nach Prozessor in
unterschiedlicher Byte-Reihenfolge gespeichert (Little-Endian bei x86,
Big-Endian beim 68K). Deswegen liegt bei x86 das niederwertige Byte
zuerst — es sieht aus wie eine Vertauschung, ist aber derselbe Effekt wie
beim Sync-Byte (`4AFC` ↔ `FC4A`), nur eben erst durch die Feld-
Zusammenlegung sichtbar geworden.

## Der Einsprung: derselbe Trick in beiden Kernen

Beide Kernel springen über einen eingebetteten Copyright-/ID-String
hinweg, bevor die echte Init-Funktion beginnt:

**68K** (`M$Exec` = `0x54`, Auszug aus [`asm-68k.r`](asm-68k.r)):
```asm
L000054:
	dc.w	$6000                        * BRA.W
	dc.w	Q9_kernel_init_67a0-*        * Displacement -> Ziel 0x67a0
```
String, der übersprungen wird: `"68030\0 OS-9/68K Kernel (Dev-Std) V3.2.0\0Copyright (c) 1999 by Microware Systems Corp.\0"`

**x86** (`m_exec` = `0xA4`, Auszug aus [`asm-x86.txt`](asm-x86.txt)):
```asm
0021e4a4: JMP 0x0021e4c0                 ; bytes=5
```
String, der übersprungen wird: `"OS-9000/x86 V4.9\0Copyright (c) 1989-2008 by RadiSys Corporation\0"`

**Bonus-Fund:** Der x86-Copyright-String nennt **RadiSys** (nicht
Microware) und **2008** — ein weiterer Beleg, dass die live gebootete
Kopie ein anderer, jüngerer Build ist als die statische `mw86.tar`-Kopie
in `vendor/` (die von der 1998/99-Eval-CD stammt).

## C-Variante?

Für dieses Thema nicht nötig — der Header ist reine Datenstruktur. Lohnt
sich erst ab Thema 04 (Header-Prüfsumme + Relozierer, echte Kontrolllogik).

## Quellen

- Primärquelle: `MWOS/DOC/RadiSys/68k_tech.pdf` (Table 1-6/1-7/1-8), `MWOS/DOC/RadiSys/os9k_tech.pdf` (`mh_com`-Struct)
- [`../../../modules/os9000-x86/docs/FINDINGS.md`](../../../modules/os9000-x86/docs/FINDINGS.md), Fund 1 (mit Herleitung/Korrekturen)
- [`../../../modules/os9000-x86/docs/KERNEL_INIT.md`](../../../modules/os9000-x86/docs/KERNEL_INIT.md), Fund 1
- [`../../REVERSE_ENGINEERING.md`](../../REVERSE_ENGINEERING.md), Abschnitt "Modul-Header"

**Erstellt**: 2026-08-13
