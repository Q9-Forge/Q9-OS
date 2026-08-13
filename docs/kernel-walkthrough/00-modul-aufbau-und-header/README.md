# Thema 00: Kernel-Modul-Aufbau und Header

Bevor der Kernel irgendetwas initialisiert, muss ihn zuerst jemand als
gültiges OS-9-Modul erkennen und an seinen Einsprungpunkt springen. Alle
drei Header-Formate sind **offiziell dokumentiert** (nicht geraten) —
inklusive dem Urvater, OS-9/6809 (1980er, Motorola 6809, der eigentliche
Ursprung der ganzen Modul-Architektur):

- **OS-9/6809**: `OS-9 System Programmer's Manual` (1983), Kapitel 4,
  "Module Header Definitions" — plus realer Kernel-Quelltext
  (`krn.asm`, NitrOS-9-Projekt, LGPL/Open-Source-Nachbau, lokal unter
  `#INFO/#Microware/OS-9 6809/nitros09/6809l2/modules/kernel/krn.asm`)
- **OS-9/68K**: `68k_tech.pdf`, Kapitel 1, Table 1-6/1-7/1-8
- **OS-9000/x86**: `os9k_tech.pdf`, Kapitel 1, C-Struct `mh_com`

**Hinweis zur 6809-Spalte:** kein eigenes 6809-Kernel-Binary vorhanden
(anders als bei 68K/x86, wo alle Werte aus echten, live geprüften Kernen
stammen) — die Werte hier kommen aus dem Manual bzw. den Parametern im
echten NitrOS-9-Quelltext, sind also dokumentiert, aber nicht Byte-für-
Byte aus einem gebooteten System verifiziert.

## Alle drei Header im direkten Vergleich

| # | Feld (Konzept) | OS-9/6809 | OS-9/68K (`dker030s`) | OS-9000/x86 (`kernel`, live) | Gleich? |
|---|---|---|---|---|---|
| 1 | Sync-Bytes | `$00` (2B) = `$87CD` | `$00` (2B) = `$4AFC` | `$00` (2B) = `$4AFC` (LE: `FC4A`) | Wert ist **je Prozessorfamilie eigen** (Manual: "processor dependent") — 68K/x86 identischer Wert, nur Byte-Reihenfolge gedreht |
| 2 | Modulgröße | `$02` (2B) | `$04` (4B) = 28.476 | `$04` (4B) = 76.944 | Konzept gleich, 6809 nur 2 Byte (64-KB-Adressraum reicht für 16 Bit) |
| 3 | Name-Offset | `$04` (2B) | `$0C` (4B) = `$6F30` | `$0C` (4B) = `$58` | Konzept identisch bei allen drei; 6809 schmaler aus demselben Adressraum-Grund |
| 4 | Modultyp | oberes Nibble von `$06` = `$C` (Systm) | `$12` (1B) = `$0C` | oberes Byte von `$12` (`$0C01`) = `$0C` | **Wert überall identisch** (`$C`=Systm, seit 1980 bis heute unverändert!), aber **3 verschiedene Kodierungen**: Nibble-Paar → zwei Bytes → ein 16-Bit-Wort |
| 5 | Sprache | unteres Nibble von `$06` = `1` (6809-Code) | `$13` (1B) = `$01` (Objct) | unteres Byte von `$12` (`$0C01`) = `$01` | **Wert überall `1`** = "Maschinencode dieser CPU" — dieselbe 3-Kodierungs-Entwicklung wie Typ |
| 6 | Attribute | oberes Nibble von `$07`, nur Bit 7 = reentrant dokumentiert | `$14` (1B) = `$A0` | oberes Byte von `$14` (`$A000`) = `$A0` | 6809 kennt nur "reentrant"; `system-state`-Bit kommt erst mit 68K dazu — **echte Erweiterung**, nicht nur Umkodierung |
| 7 | Revisionsstufe | unteres Nibble von `$07`, 0–15 | `$15` (1B) = `0`, 0–255 | unteres Byte von `$14`, 0–255 | 6809 auf 4 Bit begrenzt — **echte Kapazitätserweiterung** ab 68K |
| 8 | Header-Prüfsumme | `$08` (1B) | `$2E` (2B) = `$1D2D` | `$56` (2B) = `$4E0D` | Konzept (XOR-Prüfsumme) über die gesamte Session hinweg gleich; Breite wächst mit der Headergröße |
| 9 | Einsprungpunkt | `$09` (2B) | `$30` (4B) = `$54` | `$24` (4B) = `$A4` | **Konzept über 3 Prozessorgenerationen identisch**: kurzer Sprung-Stub vor dem echten Code (s. Abschnitt unten) |
| 10 | Datenbereich/Storage-Größe | `$0B` (2B), universell für jeden Modultyp | *(für `Systm` nicht dokumentiert)* | `$2C` (4B) = 7.008 | 6809 hatte es universell, 68K spart es sich für Systemmodule, x86 holt es sich zurück |
| 11 | Standard-Header endet nach | **9 Byte** | **46 Byte** (`$2E`/`$2F`) | **88 Byte** (`$58`) | wächst mit jeder Generation deutlich — 6809→68K ~5×, 68K→x86 ~2× |

**Bemerkenswert:** Zeile 4/5 (Typ-Code `$C`=System, Sprach-Code `1`=
Maschinencode) sind über **drei komplett unabhängige Prozessorarchitekturen
und ~28 Jahre** (6809 1980 → 68K ~1990er → x86 2008) **wortwörtlich
identisch geblieben** — vermutlich der am längsten unverändert
durchgehaltene Teil der gesamten OS-9-Familie.

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

### Kasten: drei Generationen, drei Kodierungen für dasselbe Typ/Sprache-Paar

Type+Language (und Attribut+Revision) durchlaufen über die drei
Architekturen eine klare Entwicklung:

1. **6809** (1980): ein **einziges Byte**, aufgeteilt in zwei 4-Bit-Nibbles
   (Typ oben, Sprache unten) — spart Platz, begrenzt aber auf 16 Werte je
   Feld.
2. **68K**: zwei **unabhängige, volle Bytes** — mehr Werte möglich (bis
   255), aber auch mehr Platzverbrauch. Bei Einzelbytes gibt es keine
   Byte-Reihenfolge-Frage.
3. **OS-9000/x86**: die beiden 68K-Bytes wurden zu **einem 16-Bit-Feld**
   zusammengelegt (`m_tylan`, `m_attrev`). Ein 16-Bit-Wert wird aber —
   genau wie Modulgröße oder Sync-Byte — je nach Prozessor in
   unterschiedlicher Byte-Reihenfolge gespeichert (Little-Endian bei x86,
   Big-Endian beim 68K). Deswegen liegt bei x86 das niederwertige Byte
   zuerst — sieht aus wie eine Vertauschung gegenüber 68K, ist aber
   derselbe Effekt wie beim Sync-Byte (`4AFC` ↔ `FC4A`), nur eben erst
   durch die Feld-Zusammenlegung sichtbar geworden.

## Der Einsprung: derselbe Trick über alle drei Kernel-Generationen

Sowohl 68K als auch x86 springen über einen eingebetteten Copyright-/
ID-String hinweg, bevor die echte Init-Funktion beginnt (6809 vermutlich
ebenso, aber ohne eigenes Kernel-Binary hier nicht am realen Byte-Code
nachvollzogen):

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

- Primärquelle 68K/x86: `MWOS/DOC/RadiSys/68k_tech.pdf` (Table 1-6/1-7/1-8), `MWOS/DOC/RadiSys/os9k_tech.pdf` (`mh_com`-Struct)
- Primärquelle 6809: `#INFO/#Microware/OS-9_6809/OS-9 System Programmers Manual 1983-01.pdf`, Kapitel 4; realer Kernel-Quelltext `#INFO/#Microware/OS-9 6809/nitros09/6809l2/modules/kernel/krn.asm` (NitrOS-9, Open Source)
- [`../../../modules/os9000-x86/docs/FINDINGS.md`](../../../modules/os9000-x86/docs/FINDINGS.md), Fund 1 (mit Herleitung/Korrekturen)
- [`../../../modules/os9000-x86/docs/KERNEL_INIT.md`](../../../modules/os9000-x86/docs/KERNEL_INIT.md), Fund 1
- [`../../REVERSE_ENGINEERING.md`](../../REVERSE_ENGINEERING.md), Abschnitt "Modul-Header"

**Erstellt**: 2026-08-13
