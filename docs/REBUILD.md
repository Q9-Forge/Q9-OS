# Byte-exakter Nachbau — Werkzeugkette und Vorgehen

Ziel: `dker030s` als eigene `.r`-Assembler-Quelle nachbauen, mit den
**echten Microware-Werkzeugen** (`r68`/`l68`) assemblieren/linken und
das Ergebnis Byte für Byte gegen `vendor/68020/dker030s` diffen.
Vorgehen **in Adressreihenfolge**, damit früh und in kleinen Schritten
klar wird, ob unser Verständnis stimmt — nicht erst am Ende der
gesamten Datei.

## Werkzeugkette

- **Assembler/Linker**: `r68.exe`/`l68.exe` aus `MWOS/DOS/BIN/` (DOS-
  Binaries), ausgeführt über Wine — **kein natives Mac-Tool**, siehe
  unten. Es gibt keine eigene Kernel-Assembler-Quelle in diesem
  Projektbaum (nur das fertige Binärmodul) — das rechtfertigt zusätzlich
  den disassemblierungsbasierten Ansatz.
- **Wine-Aufruf**:
  ```bash
  export WINEPREFIX="$HOME/.local/wineprefix-os9"
  export WINEDEBUG=-all
  WINE="$HOME/.local/wine-stable/Wine Stable.app/Contents/Resources/wine/bin/wine"
  arch -x86_64 "$WINE" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\r68.exe" -m3 -o=<out>.r68o "M:\\...\\quelle.r"
  arch -x86_64 "$WINE" "Z:\\Volumes\\SSD1TB\\projects\\MWOS\\DOS\\BIN\\l68.exe" -t=os9_68k -n=<modulname> -gu=0.0 -o=<out> <out>.r68o
  ```
  `M:` ist im Wineprefix bereits auf `/Volumes/SSD1TB/projects`
  gemappt (`dosdevices/m: -> /Volumes/SSD1TB/projects`).
- **Hinweis**: Unter `MWOS/tools/macos/bin/r68`/`l68` liegen nur
  Bash-Wrapper, die intern genau dasselbe tun (Wine + `os9make.exe`) —
  keine echten Mac-Programme, kein DOS-zu-Mac-Compiler.
- `-m3` = Ziel-CPU 68030 (passend zu `dker030s`/CB030).

## `psect`-Direktive (r68) — korrekte Syntax

Quelle: offizielles Microware-Handbuch *"OS-9 Assembler-Linker 1991.pdf"*
(reine Werkzeug-Dokumentation, kein Kernel-Quellcode) plus reale
CB030-Makefiles (`SYSMODS/tkcb030.make`, `INIT/init.make` — Build-
Skripte, keine Kernel-Implementierung) als Aufruf-Referenz.

```
psect name,typelang,attrev,edition,stacksize,entrypt,trapent
```

**7 Parameter**, nicht weniger (früherer Fehlversuch mit 5 Parametern
scheiterte mit "comma expected"). Wichtig:

- `typelang` = **ein gepacktes Wort** `(Type<<8)|Language`, nicht zwei
  getrennte Werte. Für unseren Kernel: `Type=0x0C` (Systm) `<<8 |
  Language=0x01` (Objct) `= 0x0C01 = 3073`.
- `attrev` = ebenfalls **ein gepacktes Wort** `(Attr<<8)|Revision`. Für
  uns: `Attr=0xA0 <<8 | Rev=0 = 0xA000 = 40960`.
- `trapent` ist nur für den Modultyp **Trap Library** (Type `0x0B`)
  relevant — unser Kernel ist **Systm** (`0x0C`), bestätigt direkt aus
  dem Original-Byte, **kein** Trap-Library-Modul (siehe Diskussion in
  der Sitzung: der Kernel *implementiert* den `TRAP #0`-Mechanismus,
  ist aber selbst kein Modul dieses speziellen Typs). `trapent=0`.

## Bestätigtes Standard-Header-Layout (0x00–0x2F)

Per Testaufbau (`build/test_header.r`) Wort für Wort gegen das Original
verifiziert:

| Offset | Feld | Bemerkung |
|---|---|---|
| `0x00` | `M$ID` | `0x4AFC` |
| `0x02` | (unbekannt) | immer `0x0001` beobachtet |
| `0x04` | (unbekannt) | immer `0x0000` beobachtet |
| `0x06` | `M$Size` (Wort) | Modulgröße — bei `dker030s` passt sie als reines Wort (`0x6F3C` < 64K) |
| `0x08` | `M$Owner` | über `-gu=<group>.<user>` gesteuert; `-gu=0.0` ergibt `0x0000`, passend zum Original |
| `0x0E` | `M$Name` (Offset) | relativer Zeiger auf den Namensstring im Modul, von `r68`/`l68` automatisch aus dem `nam`-Namen platziert |
| `0x10` | Sync-Marker | immer `0x0555` beobachtet |
| `0x12` | `typelang` | `(Type<<8)\|Lang)`, bei uns `0x0C01` |
| `0x14` | `attrev` | `(Attr<<8)\|Rev)`, bei uns `0xA000` |
| `0x16` | `M$Edit` (Edition) | im Original `0x0177` (375) — muss beim echten Nachbau exakt gesetzt werden |
| `0x28` | Parity | XOR aller 24 Header-Worte ergibt `0xFFFF`; wird von `l68` automatisch korrekt berechnet |

Diese Zuordnung ist jetzt **werkzeugseitig bestätigt** (nicht nur aus
der Disassemblierung geraten) — `psect` + `r68` + `l68` erzeugen exakt
dieses Layout, wenn die Parameter stimmen.

## Kernel-spezifische Header-Erweiterung (0x30–0x53) — gelöst

`l68`s automatische Header-Synthese (egal mit welcher `psect`/Flag-
Kombination, auch `-i` probiert) erzeugt für diesen 36-Byte
"M$Exec-Erweiterungsblock"-Typ **nie** das richtige Byte-Muster — weder
ohne `-i` (12 Byte, falscher Inhalt) noch mit `-i` (32 Byte, inkl.
unerwünschtem doppeltem eingebettetem Namensstring). Vermutlich kennt
`l68`s Synthese-Logik diese spezielle Erweiterungsform schlicht nicht,
für keine der ausprobierten `psect`-Parameterkombinationen.

**Lösung: `l68 -r` (Rohbinär-Ausgabemodus).** Dieser Modus überspringt
die automatische Header-Synthese vollständig — auch für den
Standardteil (0x00–0x2F), der vorher noch korrekt automatisch erzeugt
wurde. Im Gegenzug bekommen wir volle Byte-Kontrolle: der komplette
Header (0x00–0x53, beide Teile) wird jetzt in `kernel.r` selbst als
`dc.b`-Datenkonstanten geschrieben, 1:1 aus dem Original übernommen
(reine Strukturfakten: Größe, Typ/Sprache, Parität, Einsprungoffset —
keine geschützten Werksausdrücke).

`psect` bleibt syntaktisch weiterhin Pflicht (Name/Typ/Sprache/
Attribute/Edition/Einsprungpunkt-Label für Assembler/Linker,
Relocation-Buchführung), hat im Raw-Modus aber **keinen Einfluss mehr
auf die tatsächlichen Header-Bytes im Output** — die `typelang`/
`attrev`/`edition`-Werte in der `psect`-Zeile sind nur noch
Dokumentation/Konsistenz zu den parallel von Hand geschriebenen
`dc.b`-Werten.

Build-Befehle (Wrapper-Skripte `r68`/`l68` in `~/.local/bin`,
kapseln den Wine-Aufruf):

```bash
export PATH="$HOME/.local/bin:$PATH"
cd src/kernel
r68 -m3 -b -o=kernel.r68o kernel.r
l68 -t=os9_68k -n=kernel -gu=0.0 -r -o=kernel.out -s=kernel.symmap kernel.r68o
```

`-b` = kurze Branches automatisch aufweiten statt Fehler (r68);
`-r` = Rohbinär statt Modul-Header-Synthese (l68); `-s=` = Symbolkarte
für die Drift-Analyse (siehe unten).

## Ergebnis: Byte-exakter Nachbau erreicht (2026-08-01)

`kernel.out` = 28476 Bytes (`0x6F3C`), **0 abweichende Bytes** gegen
`vendor/68020/dker030s`. Drei Klassen von Restproblemen mussten dafür
noch gelöst werden, nachdem Header + Rohbytes standen:

1. **68020-Voll- vs. Brief-Erweiterungswort bei indizierter/Speicher-
   indirekter Adressierung.** `r68` wählt bei einfacher
   `disp(An,Xn)`-Syntax immer das kürzeste Brief-Format (8-Bit-
   Displacement), auch wenn das Original ein 68020-Vollformat mit
   16-/32-Bit-Displacement nutzt. Fix laut Handbuch (*OS-9
   Assembler-Linker 1991*, Kapitel 1, Adressierungsmodi-Tabelle):
   äußere Klammern erzwingen Vollformat mit 32-Bit-Displacement als
   Default (`(disp,An,Xn.s*S)`), `(disp).w` davon erzwingt 16-Bit statt
   32-Bit. Betraf 3 Stellen im Kernel (ein `bset.b`, zwei `lea` mit
   Speicher-indirekter Adressierung).
2. **`r68` optimiert Branches mit explizitem `.w`-Suffix trotzdem auf
   Kurzform**, sobald das Ziel in ein vorzeichenbehaftetes Byte passt —
   unabhängig vom expliziten Größensuffix (per Testfall verifiziert,
   nicht im Handbuch dokumentiert). Das Original nutzt an mehreren
   Stellen aber echt die Wortform, vermutlich weil der ursprüngliche
   Compiler keine Branch-Größenoptimierung durchführt. Fix: Opcode-Wort
   direkt aus den Ghidra-Rohbytes als `dc.w` übernehmen, gefolgt von
   einem separat berechneten `dc.w ZIEL-*` als Displacement — das
   Sternchen (`*`) steht dabei für die Adresse des Displacement-Worts
   selbst, genau wie bei der normalen 68k-PC-relativ-Konvention. Betraf
   5 Branch-Stellen.
3. **Ghidra-Fehldisassemblierung von Datenbereichen als Scheincode**
   (`ori.b`/`andi.b`/`cmpi.b`/`subi.b`/`btst.b` mit Immediate).
   Erkennbar daran, dass das vermeintliche Byte-Immediate ein von Null
   verschiedenes oberes Wort-Byte hat — bei einer echten `.b`-
   Instruktion ist das laut ISA immer 0, unser Assembler erzeugt dort
   also korrekterweise 0, während das Original den rohen Datenbyte-Wert
   an der Stelle hat. Betraf 25 Einzelbytes an über 20 Adressen, per
   `FORCE_RAW_BYTES` im Konverter (`tools/ghidra_to_r68.py`) als
   Rohbytes statt Instruktion ausgegeben.

**Diagnosemethode für (1) und (2):** `l68 -s=kernel.symmap` erzeugt
eine Symbolkarte mit der tatsächlichen Linkadresse jedes Labels. Da
jedes Label `LxxxxxX` per Namenskonvention die *Original*-Adresse
codiert, zeigt ein Vergleich Name-Adresse vs. tatsächliche Adresse
genau, ab welchem Punkt wie viele Bytes Drift entstanden sind — die
Stelle, an der sich das Delta ändert, ist die (oder eine von wenigen)
fehlerhafte Instruktion. Für (3) reichte am Ende ein einfacher
Volltext-Bytevergleich der fertigen, größenkorrekten Datei gegen das
Original.

## Vorgehen: Adressreihenfolge, kleine verifizierbare Schritte

Nicht die ganze Datei auf einmal — nach Adresse aufsteigend in kleinen
Häppchen bauen, nach jedem Häppchen assemblieren/linken/diffen.

**Quelle:** `src/kernel/kernel.r` (neu angelegt, erster Abschnitt).

### Stand: Standard-Header + Einsprung + ID-String — bestätigt

Mit den echten Kernel-Werten (`typelang=3073`, `attrev=40960`,
`edition=375`) aufgebaut:

- **`0x00`–`0x2F` (Standard-Header):** Layout vollständig bestätigt
  (siehe Tabelle oben), inkl. `M$Edit=0x0177`.
- **`0001 09be`-Präfix + kompletter ID-String** (`"68030\0 OS-9/68K
  Kernel (Dev-Std) V3.2.0\0Copyright (c) 1999 by Microware Systems
  Corp.\0"`): **byte-für-byte identisch mit dem Original**, per Diff
  bestätigt (nur um die noch offene Erweiterungsblock-Länge
  verschoben, Inhalt exakt gleich). `dc.b` mit **doppelten**
  Anführungszeichen für Strings (`fcc` ist bei diesem Assembler kein
  gültiges Mnemonic; einfache Anführungszeichen ergeben "value out of
  range").

### Offen: `M$Exec`-Erweiterungsblock (`0x30`–`0x53`)

Noch **nicht** reproduziert. `l68` erzeugt automatisch einen
Erweiterungsblock zwischen Standard-Header und Einsprungpunkt (worauf
`M$Exec` zeigt), aber:

- **Ohne** `-i`: 12 Byte Erweiterung (`0x30`–`0x3B`), Muster
  `00000050 00000050 00000000` (zwei identische Werte = vermutlich
  `M$Exec`/`M$Excpt`-Duplikat als "kein Exception-Handler gesetzt"-
  Konvention) — Original hat aber **36 Byte** (`0x30`–`0x53`) mit
  andersartigem Muster (`00000054 00000000 ... b0bdb0bd 00000001
  00000001 ...`).
- **Mit** `-i` ("initialisierter statischer Speicher für OS9/68k-
  Systemmodul"): Erweiterung wächst auf 32 Byte, aber mit einem
  unerwünschten **doppelten eingebetteten `"kernel\0\0"`-String**
  darin — passt ebenfalls nicht zum Original.
- **Nächster Ansatz:** weitere `l68`-Flags durchprobieren (`-M=<n>`
  Zusatzspeicher, `-S` sticky, evtl. Kombination mit `-i`), oder klären
  ob `b0bd b0bd` und `00000001 00000001` even über normale
  `psect`/`l68`-Mechanismen entstehen können, oder ob sie doch von Hand
  per `dc.l` in einem Bereich gesetzt werden müssen, den `l68`
  freilässt (z. B. über explizite Vsect-Deklaration statt automatischer
  Erweiterung).
