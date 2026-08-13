# OS-9000 (x86) — Systemmodule gefunden und ersten Header-Vergleich gemacht

Andreas erinnerte sich, dass OS-9000 (Microwares 1990er/2000er C-Neuschrieb
von OS-9, portabel für 386+/PowerPC/MIPS/SPARC) schon einmal unter QEMU
gebootet wurde und dass dabei aufgefallen war, das Root-Dateisystem sei
**nicht mehr RBF**. Diese Runde: Quelle gefunden, Kernel/IOMan/File-Manager
als reale x86-Binaries extrahiert, Modul-Header mit dem 68k-Format
verglichen, erster Blick auf den Boot-Sektor des Disk-Images.

## Quelle

`/Volumes/SSD1TB/#INFO/#Microware/OS9000/OS9Eval/RESIDENT/mw86.tar` —
Teil der offiziellen OS-9000-Evaluierungs-CD (Microware, ca. 1998/1999,
siehe `HOWTO.TXT` im selben Ordner). Enthält ein komplettes
`CMDS/BOOTOBJS/`-Verzeichnis mit denselben Modulklassen wie beim
OS-9/68K-System (Kernel, IOMan, File-Manager, Treiber, Descriptor-Rohlinge).

Zusätzlich vorhanden, aber noch nicht ausgewertet:
`/Volumes/SSD1TB/#INFO/#Microware/OS9000/OS9000-4.9_on_QEmu-with-XiBase9_en.zip`
— enthält ein fertiges, bootfähiges Disk-Image (`os9000-xibase.img`,
~62,9 MB) samt QEMU-Binaries für Windows und einer Installationsanleitung
(`en_OS9_x86_Install.pdf`, noch nicht gelesen). "XiBase9" ist vermutlich
Microwares eigenes grafisches Frontend/Window-System für OS-9000 (analog zu
`maui_win`/`maui_inp`, die auch im `mw86.tar` liegen), nicht zwingend
namensgebend für ein Dateisystem.

Extrahierte Module liegen in [`../vendor/`](../vendor/): `kernel`, `ioman`,
`rbf`, `ssm`, `pipeman`, `scf`, `cdfm` (CD-ROM-File-Manager), `pcf`,
`sbf`, `null`, `nil`, `scllio`, `cache386`, `vectx86`, `fpu`, `fpuem`.

## Fund 1: Modul-Header-Format strukturell identisch, aber breitere Felder

Sync-Byte ist `fc 4a` statt `4a fc` — **dieselbe** 16-Bit-Konstante
`0x4AFC`, nur wegen x86-Little-Endian byte-vertauscht gespeichert (68k ist
big-endian). Bestätigt per Rohbyte-Vergleich mehrerer Module
nebeneinander:

| Offset | Feld | Kernel-Wert | Vergleich zu 68k |
|---|---|---|---|
| `0x00` | M$ID | `0x4AFC` (LE) | identisch |
| `0x04` | M$Size (4 Byte) | `0xF390` = 62352, deckt sich exakt mit Dateigröße | identisch (auch 68k: 4 Byte an `0x04`) |
| `0x08` | M$Owner | `0` | identisch |
| `0x0C` | Name-Offset | `0x58` (4 Byte, LE) | **68k hatte hier nur 2 Byte** — echte Formatabweichung, kein reiner Endianness-Effekt |
| `0x12` | Language-Byte | `0x01` (Objct) | **identisch — selbes Byte-Offset wie 68k, architekturübergreifend bestätigt** |
| `0x13` | Type-Byte | `0x0C` (Systm) bei kernel/ioman/ssm, `0x0D` (Fmgr) bei rbf/scf/pcf/cdfm/pipeman | **identisch — selbes Byte-Offset wie 68k**, deckt sich exakt mit den in `../SYSCALL_MODULE_MAP.md`/`../c0-descriptor/docs/FINDINGS.md` empirisch bestätigten Typ-Codes |
| ab `0x58` | Modulname, NUL-terminiert (`"kernel\0"`) | — | **68k nutzte High-Bit-Terminierung** (letztes Zeichen mit gesetztem Bit 7) statt NUL — echte Formatabweichung |

**Einordnung:** Die grundlegende Modul-Klassifizierung (Type/Lang-Bytes an
denselben absoluten Offsets `0x12`/`0x13`, dieselben Typ-Codes
Systm=`0x0C`/Fmgr=`0x0D`) ist über zwei Prozessorarchitekturen und
~10 Jahre Zeitunterschied hinweg **stabil geblieben** — das ist ein
starkes Signal, dass dieser Teil des Modulkopfs ein bewusst langfristig
stabiler Vertrag war. Die Namensfeld-Breite (2→4 Byte) und die
Terminierungskonvention (High-Bit→NUL) haben sich dagegen geändert —
plausibel, weil OS-9000 laut Porting-Guide-Dokumentation komplett in C neu
geschrieben wurde, nicht binärkompatibel zum 68k-Original sein musste.

**Noch nicht geklärt:** die übrigen Kopf-Felder zwischen `0x14` und `0x58`
(Edition, Attr/Revs, Usage, Symbol-Tabelle o.ä. beim 68k) — nur teilweise
mit Werten belegt, noch nicht einzeln zugeordnet. Auch die CRC-Konvention
am Modulende ist noch nicht verifiziert.

## Fund 2: Kernel deutlich größer, keine automatische Rückschlüsse auf "besseren Code"

| Modul | Größe 68k (`dker030s`) | Größe x86 (`kernel`) | Faktor |
|---|---|---|---|
| Kernel | 28.476 Byte | 62.352 Byte | ~2,2× |
| IOMan | 5.660 Byte | 15.504 Byte | ~2,7× |
| RBF | 9.638 Byte | 32.432 Byte | ~3,4× |

Deutlich größer — plausibel durch C-Compiler-Overhead (kein
handoptimierter Assembler mehr) UND durch echten Funktionszuwachs
(OS-9000 hat u.a. deutlich mehr F$/I$-Funktionalität, Multiprozessor-
Vorbereitung, POSIX-nähere Semantik laut Technical Manual). Ob der
tatsächliche COMPILIERTE x86-Code für einen Ghidra-Decompiler-Durchlauf
wirklich "besser" (lesbarer) wird als der handgeschriebene 68k-Assembler,
ist noch nicht getestet — das war Andreas' Vermutung, aber noch keine
Ghidra-Disassemblierung dieser x86-Module durchgeführt (nur Rohbyte-
Vergleich des Headers bisher).

## Fund 3: Boot-Sektor der Partition zeigt eine untypische Signatur — Frage nach dem Dateisystem noch NICHT abschließend geklärt

MBR des Disk-Images (`os9000-xibase.img`) ist Standard-x86 (Signatur
`55 AA`), eine aktive Partition, **Partitionstyp `0x09`** — vermutlich
Microwares eigener registrierter OS-9000-Partitionstyp (passt zur
Beschreibung "Create OS9000 type partition" in `HOWTO.TXT`), Start bei
LBA 17.

Der erste Sektor DIESER Partition (also das, was klassisch bei RBF die
Identifikationsstruktur LSN0 wäre) beginnt aber **nicht** mit den
klassischen RBF-LSN0-Feldern (die fangen mit einer 3-Byte-Sektorenzahl an,
kein Programmcode), sondern mit:

```
eb 0a 58 44 30 30 42 54 00 00 00 00 bc 00 10 6a ...
```

= x86-Opcode `jmp short +0x0a`, gefolgt von der ASCII-Signatur
**`"XD00BT"`** — strukturell ähnlich dem FAT-Boot-Sektor-Muster (Jump +
OEM-Kennung am Sektoranfang), aber mit einer Kennung, die weder "RBF" noch
"FAT"/"MSDOS" ist. Eine Volltextsuche im Bereich um den Partitionsanfang
fand außerdem `",BFOS9Sys"` (Zeichenkette, evtl. abgeschnitten/verschoben,
möglicherweise "RBFOS9Sys" mit einem nicht-druckbaren ersten Byte) sowie
an anderer Stelle im Image die Zeichenkette `"must be RBF devices"`
(vermutlich aus einem Utility wie `format`/`fdisk`) — das deutet darauf
hin, dass RBF als Konzept im System durchaus noch existiert/unterstützt
wird, aber **nicht zwingend**, dass DIESES konkrete Boot-Volume klassisch
RBF-formatiert ist.

**Bewertung:** Stützt Andreas' Erinnerung ("kein RBF mehr") in dem Sinne,
dass der Boot-Sektor eindeutig NICHT dem klassischen RBF-LSN0-Layout
entspricht — aber noch nicht geklärt, was `"XD00BT"` tatsächlich
bezeichnet (ein neuer, eigener Dateisystemtyp? Ein reiner x86-Bootloader-
Wrapper VOR einem intern weiterhin RBF-artigen Format? Eine
XiBase9-spezifische Boot-Variante?). Das wäre der nächste sinnvolle
Schritt: die LSN0-Struktur nach `"XD00BT"` Byte für Byte gegen die
RBF-Feldbeschreibung aus dem Technical Manual (Kapitel 6, "Disk File
Organization") vergleichen, um zu sehen, ob es sich um eine Variante von
RBF mit vorangestelltem x86-Bootloader handelt oder um ein wirklich neues
Format.

## Werkzeuge / Wiederholbarkeit

```bash
# Tar mit den Systemmodulen entpacken:
tar -xf "/Volumes/SSD1TB/#INFO/#Microware/OS9000/OS9Eval/RESIDENT/mw86.tar"
# -> CMDS/BOOTOBJS/{kernel,ioman,rbf,ssm,pipeman,scf,cdfm,pcf,...}

# Disk-Image aus dem QEMU-Zip extrahieren:
unzip "/Volumes/SSD1TB/#INFO/#Microware/OS9000/OS9000-4.9_on_QEmu-with-XiBase9_en.zip" \
  "*/os9000-xibase.img"

# Partitionstabelle/Boot-Sektor ansehen:
xxd -s 446 -l 66 os9000-xibase.img      # MBR-Partitionstabelle
xxd -s 8704 -l 256 os9000-xibase.img    # LSN0 der ersten Partition (LBA 17)
```

Es gibt bereits fertig extrahierte Verzeichnisse aus einer früheren Sitzung
unter `/Volumes/SSD1TB/#INFO/#Microware/OS9000/{OS9000,OS9000SM,OS9Eval}/`
— enthalten größtenteils `CMDS`/`SYS`/`ASSETS` eines laufenden Systems,
aber (geprüft) **keine** `BOOTOBJS`-Systemmodule direkt — die kamen aus dem
separaten `RESIDENT/mw86.tar`.

## Nächste Schritte

1. `en_OS9_x86_Install.pdf` lesen (liegt im QEMU-Zip) — vermutlich
   Klartext-Erklärung des Boot-Volume-Formats und was "XiBase9" ist.
2. LSN0-Struktur nach `"XD00BT"` Feld für Feld auswerten, gegen RBF-
   Referenz aus dem Technical Manual vergleichen (Fund 3 oben).
3. Versuchen, das Image tatsächlich unter QEMU/UTM zu booten (das war
   Andreas' ursprüngliche Erinnerung) — dann live per `dir`/`iniz`/`devs`
   nachsehen, welcher Filemanager tatsächlich an `/dd` hängt, statt nur
   aus Bytes zu schließen.
4. Ghidra-Disassemblierung von `kernel`/`ioman`/`rbf` (x86-Target,
   analog zum bisherigen 68k-Vorgehen) — mit Andreas' Vermutung im
   Hinterkopf, dass der C-kompilierte Code sich besser dekompilieren
   lassen könnte als der handoptimierte 68k-Assembler.
5. Restliche Kopf-Felder (`0x14`–`0x58`) Byte für Byte zuordnen.

**Erstellt**: 2026-08-13
