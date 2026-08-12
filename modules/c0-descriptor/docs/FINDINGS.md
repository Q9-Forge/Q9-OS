# Geräte-Descriptor `c0` — vollständig analysiert (148 Byte)

Ziel: die in `../ioman/docs/REVERSE_ENGINEERING.md` (Runde 4) noch offene
Frage klären, welcher der drei `F$Link`-Typ-Filter (`0xf`/`0xe`/`0xd`) in
`I$Attach` genau Descriptor/Treiber/File-Manager bedeutet, und was an den
Offsets `0x38`/`0x3a` des Descriptors tatsächlich steht.

**Quelle:** `MWOS/OS9/68030/PORTS/CB030/CMDS/BOOTOBJS/c0` — der Onboard-
CompactFlash-Descriptor (Master), aus dem CB030-Bootfile `dev.bl`
(`LOCAL CMDS/BOOTOBJS/c0`). 148 Byte, komplett per Hexdump gelesen (kein
Ghidra nötig, reines Datenmodul ohne Programmcode).

Da 148 Byte klein genug für eine vollständige Lektüre sind: die komplette
Datei ist unten als Hexdump dokumentiert (Referenz für Folgearbeit).

## Ergebnis: Typ-Codes und Namensfelder vollständig bestätigt

| Offset | Wert | Bedeutung |
|---|---|---|
| `0x00` | `0x4AFC` | M$ID |
| `0x12` | `0x0F00` | **M$Type = `0x0F` (15) = Descrptr**, M$Lang = `0x00` |
| `0x38` | `0x007C` | Offset-Zeiger auf den Namen des **File-Managers** — zeigt auf String `"RBF"` bei Offset `0x7C` |
| `0x3A` | `0x0084` | Offset-Zeiger auf den Namen des **Treibers** — zeigt auf String `"cfide"` bei Offset `0x84` |

**Damit sind die drei `I$Attach`-Typ-Filter aus `../ioman/docs/REVERSE_ENGINEERING.md`
(Runde 4) exakt geklärt** — bestätigt durch Cross-Check gegen die
tatsächlichen Modul-Header von `cfide` und `rbf` (deren `M$Type`-Bytes
direkt gelesen, kein Ghidra nötig):

| Filter in `I$Attach` | Namensquelle | Ziel-Modul | M$Type des Ziels | Bedeutung |
|---|---|---|---|---|
| `0xF00` (1. Link) | Pfadname selbst | `c0` (dieser Descriptor) | `0x0F` | **Descrptr** |
| `0xE00` (2. Link) | `(0x3A,A2)` → `"cfide"` | `cfide` | `0x0E` | **Driver** |
| `0xD00` (3. Link) | `(0x38,A2)` → `"RBF"` | `rbf` | `0x0D` | **Fmgr** (File Manager) |

Reihenfolge exakt umgekehrt zur Nennung im Descriptor (Treiber-Offset
`0x3A` kommt vor Dateimanager-Offset `0x38` im Speicher, wird aber als
ZWEITER Link aufgerufen, nicht als erster) — kein Widerspruch, nur
Lese-Reihenfolge im Code war `0x3A` vor `0x38`.

## Vollständiger Hexdump (Referenz)

```
0000: 4a fc 00 01 00 00 00 94 00 00 00 00 00 00 00 8c
0010: 05 55 0f 00 80 00 00 1a 00 00 00 00 00 00 00 00
0020: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 3f 55
0030: ff ff e0 00 00 00 00 b7 00 7c 00 84 00 80 00 00
0040: 00 00 00 00 00 00 00 34 01 00 02 80 00 00 00 00
0050: 00 01 00 00 00 00 00 01 03 00 00 00 02 00 00 09
0060: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0070: 00 00 00 00 00 00 ff ff 00 00 00 00 52 42 46 00
0080: 00 00 00 00 63 66 69 64 65 00 4e 71 63 30 00 00
0090: 00 4b 3b 88
```

**Weitere, noch nicht interpretierte Felder** (Kandidaten für eine
Folgerunde, falls relevant): `0x20` = `0x3F55` (unbekannt), `0x32` =
`0xE000_0000` (evtl. Basisadresse des CF-Interfaces? passt zur Größenordnung,
nicht verifiziert), `0x3C` = `0x0080` (zeigt auf Nullbytes bei `0x80` —
Zweck unklar), Bereich `0x44`–`0x5F` (diverse kleine Werte, vermutlich
Laufwerksparameter wie Zylinder/Sektoren, nicht verifiziert), `0x8A`–`0x8D`
= `"Nqc0"` bzw. `4e 71 63 30` (unklar — `0x4E71` ist zufällig der 68k-NOP-
Opcode, vermutlich Zufall in einem Datenfeld, nicht weiter untersucht).

**Erstellt**: 2026-08-12
