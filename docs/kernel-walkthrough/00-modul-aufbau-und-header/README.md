# Thema 00: Kernel-Modul-Aufbau und Header

Bevor der Kernel irgendetwas initialisiert, muss ihn zuerst jemand als
gültiges OS-9-Modul erkennen und an seinen Einsprungpunkt springen. Dieses
Thema zeigt, wie dieser Kopfbereich bei beiden Architekturen aussieht —
strukturell fast identisch, mit einem überraschend direkten Parallel-Fund
beim Einsprungmechanismus selbst.

**Wichtig:** Nichts hier ist geraten — beide Header-Formate sind offiziell
dokumentiert (68K: `68k_tech.pdf`, Table 1-6/1-7/1-8; OS-9000: `os9k_tech.pdf`,
`mh_com`-C-Struct). Alle Werte unten wurden gegen diese Manuals **und**
direkt gegen die Rohbytes beider Original-Binaries geprüft.

## Header-Layout im Vergleich

| Offset | 68K (`dker030s`) | x86 (`kernel`, live) | Gleich/Anders |
|---|---|---|---|
| `0x00` | M$ID `0x4AFC` | M$ID `0x4AFC` (byte-vertauscht gespeichert, x86 = little-endian) | gleich (Endianness-Effekt) |
| `0x04` | M$Size (4 Byte) = 28.476 | M$Size (4 Byte) = 76.944 | gleiches Feld, andere Größe |
| `0x0C` | Name-Offset, **2 Byte breit** (`0x6F30`) | Name-Offset, **4 Byte breit** (`0x58`) | **echte Formatabweichung** |
| `0x12` | **M$Type**-Byte `0x0C` (Systm) | Lang-Byte `0x01` (LSB von `m_tylan`) | s. u. — wirkt vertauscht, ist aber reine Endianness |
| `0x13` | **M$Lang**-Byte `0x01` (Objct) | Type-Byte `0x0C` (MSB von `m_tylan`) | s. u. |
| `0x30` | **M$Exec = `0x54`** (siehe unten) | — (bei x86 liegt das Feld auf `0x24`, siehe unten) | Feld existiert bei beiden, aber an unterschiedlicher Position |
| `0x24` | — | **`m_exec` = `0xA4`** | offiziell bestätigt, siehe `FINDINGS.md` Fund 1 |
| `0x34` | **M$Excpt = `0`** | `m_excpt` = `0` (bei `0x28`, nicht `0x24`!) | gleiches Feld, unterschiedliche Position |
| `0x40`–`0x43` bzw. `0xB4`–`0xB6` | Magic `0xB0BD` (zweimal) | Magic `0xB0BD` (zweimal) | **identische Konstante, andere Fundstelle** — nicht offiziell dokumentiert, Microware-interne Konvention |
| Modulname | `"kernel"` bei Offset `0x6F30` (**ganz am Ende** des Moduls) | `"kernel"` bei Offset `0x58` (**gleich nach dem Standard-Header**) | echte Layout-Abweichung |

Vollständiger Rohbyte-Auszug: [`asm-68k.r`](asm-68k.r) / [`asm-x86.txt`](asm-x86.txt).
Vollständige, feldweise dokumentierte Tabelle (alle `0x00`–`0x58`) in
[`../../../modules/os9000-x86/docs/FINDINGS.md`](../../../modules/os9000-x86/docs/FINDINGS.md), Fund 1.

### Type/Lang: sieht aus wie vertauscht, ist reine Endianness

68K hat `M$Type`(`0x12`) und `M$Lang`(`0x13`) als **zwei unabhängige
1-Byte-Felder** (Table 1-7) — bei Einzelbytes gibt es keine Endianness.
OS-9000 hat beide im C-Neuschrieb zu **einem** `u_int16 m_tylan`
zusammengelegt (`mh_com`-Struct: "type (first/high byte), language
(second/low byte)", konzeptionell `0x0C01`). Ein `u_int16` wird aber genau
wie `m_sync`/`m_size` in der **nativen Endianness** gespeichert —
Little-Endian legt das niederwertige Byte (Lang) an die niedrigere Adresse.
Der *Wert* ist bei beiden identisch (`Type=0x0C, Lang=0x01`), nur die
physische Byte-Reihenfolge dreht sich um, weil x86 zwei 68K-Einzelbytes zu
einem echten Integer-Feld gemacht hat. Dasselbe Muster wiederholt sich bei
`M$Attr`/`M$Revs` → `m_attrev`.

## 68K: der Einsprung (`asm-68k.r`, `L000054` ff.)

```asm
L000054:
	dc.w	$6000                        * BRA.W-Opcode
	dc.w	Q9_kernel_init_67a0-*        * Displacement 0x674A -> Ziel 0x67a0
```

`M$Exec` (Header-Feld `0x30`) zeigt auf Offset `0x54` — dort liegt aber
**keine** Kernel-Logik, sondern ein einzelner `BRA.W`-Sprung, der über einen
eingebetteten ID-String hinwegspringt (kein Standard-Header-Feld, aber
Microware-Konvention):

```
"68030\0 OS-9/68K Kernel (Dev-Std) V3.2.0\0Copyright (c) 1999 by Microware Systems Corp.\0"
```

(4 Byte davor, `00 01 09 be`, sind laut bisheriger Recherche noch nicht
geklärt.) Erst am Sprungziel `0x67a0` beginnt die echte Init-Funktion
`Q9_kernel_init_67a0` (Thema 01+).

## x86: der Einsprung (`asm-x86.txt`)

```asm
Field@0x24 (candidate M$Exec)  = 0x000000A4
...
=== FUNCTION kernel_init_0021e4c0 @ 0021e4a4 size=5 ===
0021e4a4: JMP 0x0021e4c0                 ; bytes=5
```

Exakt dasselbe Prinzip: das x86-Analogon von `M$Exec` (Header-Feld `0x24`)
zeigt auf Offset `0xA4` — auch dort **kein** Kernel-Code, sondern ein
5-Byte-`JMP`-Trampolin, das über den eingebetteten Copyright-String
hinwegspringt:

```
"OS-9000/x86 V4.9\0Copyright (c) 1989-2008 by RadiSys Corporation\0"
```

Erst am Sprungziel `0x21e4c0` beginnt die echte Init-Funktion
`kernel_init_0021e4c0` (Thema 01+).

**Bonus-Fund:** Der Copyright-String nennt **RadiSys** (nicht Microware)
und **2008** (nicht 1998/99 wie die Eval-CD-Version in `vendor/`) — ein
weiterer, unabhängiger Beleg dafür, dass die live gebootete Kopie ein
**anderer, deutlich jüngerer Build** ist als die statische `mw86.tar`-Kopie
(RadiSys übernahm Microware 2001, der Copyright-Vermerk spiegelt das
direkt wider).

## Gemeinsamkeiten

1. **Beide Architekturen springen über einen eingebetteten Identifikations-/
   Copyright-String hinweg**, bevor die eigentliche Init-Funktion beginnt —
   derselbe Trick, nur beim 68K über `BRA.W` (2 Worte) und bei x86 über
   `JMP rel32` (5 Byte) gelöst.
2. Die **Magic-Konstante `0xB0BD`** taucht bei beiden auf, nur an
   unterschiedlicher relativer Position zum Einsprungpunkt.
3. **Type-/Lang-Klassifizierung** (derselbe 2-Byte-Slot `0x12`-`0x13`,
   dieselben Werte Systm=`0x0C`/Objct=`0x01`) ist über den kompletten
   Architekturwechsel stabil geblieben — auch wenn die x86-Seite die
   beiden 68K-Einzelbytes zu einem Integer-Feld zusammengelegt hat und
   dadurch die physische Byte-Reihenfolge dreht (s. o.).

## Unterschiede

1. **Namensfeld-Position**: 68K legt den Modulnamen ans **Ende** des
   Moduls (`0x6F30` von `0x6F3C` Gesamtgröße — praktisch der letzte
   Bereich), x86 legt ihn **direkt nach dem Standard-Header** (`0x58`).
   Für den eigenen Kernel relevant: eine feste Namensposition
   (x86-Stil) ist einfacher zu parsen als eine größenabhängige
   (68K-Stil, Name-Offset muss man ohnehin aus dem Header lesen, aber
   die Konvention "Name kommt ans Ende" ist zumindest beim Debuggen mit
   einem Hex-Editor unpraktischer).
2. **M$Exec-Feldposition**: `0x30` (68K) vs. `0x24` (x86) — kein
   architekturbedingter Zwang, vermutlich einfach eine andere
   Header-Erweiterung im C-Neuschrieb.

## C-Variante?

Für dieses Thema nicht sinnvoll — der Header ist reine Datenstruktur,
keine Programmlogik. Eine C-`struct`-Definition beider Header-Layouts wäre
höchstens eine Wiederholung der Tabelle oben in anderer Syntax. Der
C-Dekompilierungs-Vergleich lohnt sich erst ab Thema 04 (Header-Prüfsumme +
Relozierer, `FUN_0022246c` — dort gibt es echte Kontroll-/Prüflogik, siehe
[`../../../modules/os9000-x86/docs/KERNEL_INIT.md`](../../../modules/os9000-x86/docs/KERNEL_INIT.md), Fund 4).

## Quellen

- **Offizielle Manuals** (Primärquelle, nicht geraten): `MWOS/DOC/RadiSys/68k_tech.pdf`, Kapitel 1, Table 1-6/1-7/1-8; `MWOS/DOC/RadiSys/os9k_tech.pdf`, Kapitel 1, "Module Header Definitions" (`mh_com`-Struct)
- 68K-Reverse-Engineering: [`../../REVERSE_ENGINEERING.md`](../../REVERSE_ENGINEERING.md), Abschnitt "Modul-Header" (Zeile 25, inkl. Nachtrag zu `M$Excpt`) und "Fund: `0x1424` ... löst das `0xB0BD`-Rätsel" (Zeile 1192)
- x86-Reverse-Engineering: [`../../../modules/os9000-x86/docs/FINDINGS.md`](../../../modules/os9000-x86/docs/FINDINGS.md), Fund 1 (vollständige `mh_com`-Tabelle); [`../../../modules/os9000-x86/docs/KERNEL_INIT.md`](../../../modules/os9000-x86/docs/KERNEL_INIT.md), Fund 1

**Erstellt**: 2026-08-13
