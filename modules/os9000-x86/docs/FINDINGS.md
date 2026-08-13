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

## Fund 4: Image erfolgreich gebootet, live geprüft — RBF ist tatsächlich der aktive File Manager, "XD00BT" ist nur der x86-BIOS-Boot-Wrapper

QEMU per Homebrew installiert (`brew install qemu`, 11.0.3), Image direkt
mit der aus `OS9000.bat` übernommenen Konfiguration gebootet:

```bash
qemu-system-i386 -boot c -m 64 -drive file=os9000-xibase.img,format=raw,if=ide \
  -net nic -net user -display none -vga std \
  -monitor unix:/tmp/qemu-mon.sock,server,nowait \
  -qmp unix:/tmp/qemu-qmp.sock,server,nowait
```

`-display none` allein reicht NICHT (OS-9 schreibt nach dem IPL direkt in
den VGA-Textmodus, nicht auf die serielle Schnittstelle — `-nographic`
zeigt nur die frühen BIOS/IPL-Zeilen). Funktionierender Weg: `-vga std`
mitlaufen lassen und über den QEMU-Monitor `screendump <datei>.ppm`
periodisch Bildschirmfotos ziehen (`sips -s format png` zur Konvertierung).

**Tastatureingabe:** Die legacy-HMP-`sendkey`-Schnittstelle sendet
Leertaste (`spc`) bei dieser QEMU-Version zuverlässig NICHT an den Gast
(andere Tasten funktionieren) — Ursache nicht geklärt, evtl. QEMU-11-
Regression. Workaround: **QMP** (`-qmp unix:...`, JSON-Protokoll) mit
`input-send-event`/`qcode: spc` funktioniert einwandfrei. Kleines Hilfs-
skript geschrieben (nicht Teil des Commits, Scratchpad), das Text in
QMP-Tastendruck-Sequenzen umsetzt.

**Boot erfolgreich:** `OS-9/x86 U4.9  PC-AT Compatible 80386`, direkt am
System-Root-Terminal eingeloggt (kein Passwort-Prompt am primären
Konsolen-Fenster, anders als bei einem `telnet`-Zugang laut Installations-
PDF). Prompt `(OS9_w0)[/dd/>]`.

**`devs`-Ausgabe (Gerätetabelle) zeigt entscheidend:**

| Device | Driver | File Mgr |
|---|---|---|
| `hc1` | `rb1003` | **`rbf`** |
| `r0` | `ram` | **`rbf`** |
| `h0` | `rb1003` | **`rbf`** |
| `d0` | `rb765` | **`rbf`** |
| `term`/`t1`/`mterm1-3`/`nil` | `sc8042m`/`sc16550`/`null` | `scf` |
| `ip0`/`tcp0`/`udp0`/`raw0`/`route0` | `spip` u. a. | `spf` |

**RBF ist also der aktive File Manager für alle Festplatten-/Disketten-
Geräte** — genau wie beim OS-9/68K-System. `chd`/`dir` auf `/dd` verhalten
sich vollständig wie klassisches RBF (hierarchische Verzeichnisse,
`Directory of . <Zeit>`-Kopfzeile identisch zum 68k-Format). Screenshot:
[`../boot-evidence/devs_full_table.png`](../boot-evidence/devs_full_table.png).

**Live-`ident -m`-Abfrage auf die tatsächlich laufenden Module** (direkt
aus dem Speicher, nicht aus einer Datei) bestätigt Fund 1 vollständig und
unabhängig:

| Modul | Ty/La (live) | Modulgröße (live) | Modulgröße (`../vendor/`-Datei) |
|---|---|---|---|
| `kernel` | `$0C01` = Type `0x0C` (Systm), Lang `0x01` | **76.944 Byte** | 62.352 Byte |
| `ioman` | `$0C01` (Systm) | — (Kopf abgeschnitten im Screenshot) | 15.504 Byte |
| `rbf` | `$0D01` = Type **`0x0D` (Fmgr)**, Lang `0x01` | **38.672 Byte** | 32.432 Byte |

`ident` beschreibt `rbf` textuell exakt als **"80386 File Mngr, Object
Code, Sharable, System State Process"** — die Type-Byte-Bedeutung `0x0D` =
File Manager ist damit nicht nur aus rohen Bytes abgeleitet, sondern vom
laufenden System selbst bestätigt. Screenshots:
[`../boot-evidence/ident_kernel_live.png`](../boot-evidence/ident_kernel_live.png),
[`../boot-evidence/ident_ioman_rbf_live.png`](../boot-evidence/ident_ioman_rbf_live.png).

**Wichtige Diskrepanz:** Die live laufenden Module sind alle GRÖSSER als
die aus `mw86.tar` extrahierten (`kernel` 76.944 vs. 62.352 Byte, `rbf`
38.672 vs. 32.432 Byte) — das gebootete System nutzt also einen anderen
(vermutlich neueren) Modul-Build, direkt in die `sysboot`-Datei des
Disk-Images eingebettet, nicht die separat auf der Eval-CD mitgelieferten
`mw86.tar`-Dateien. Die `../vendor/`-Kopien bleiben trotzdem nützlich (sie
sind eigenständige, per Ghidra disassemblierbare Einzeldateien mit
bekannter Herkunft) — für einen Vergleich mit dem TATSÄCHLICH laufenden
Code müssten die Module aus dem Disk-Image selbst extrahiert werden
(`kernel`/`ioman`/`rbf` liegen NICHT als Einzeldateien in
`/dd/CMDS/BOOTOBJS/` — dieser Ordner enthält nur Zusatztreiber/-deskriptoren,
kein `kernel`/`ioman`/`rbf`; die Kernmodule stecken im `sysboot`-Bootfile
selbst, `ident -m` liest sie nur aus dem laufenden Speicher, nicht von
Platte).

**Auflösung der ursprünglichen Frage:** Die Boot-Sektor-Signatur
`"XD00BT"` (Fund 3) ist mit hoher Sicherheit NUR ein x86-BIOS-kompatibler
Bootstrap-Wrapper (nötig, weil x86-BIOS beim Booten reinen Realmode-Code
bei Sektor 0 erwartet — eine Notwendigkeit, die auf 68k-Hardware ohne
BIOS-Konvention nicht besteht) — **nicht** ein Ersatz für RBF als
Dateisystem. Funktional läuft RBF unverändert als File Manager für alle
Block-Geräte, live bestätigt. Andreas' Erinnerung an "kein RBF mehr" lässt
sich damit nicht am File-Manager selbst festmachen — am ehesten erklärbar
dadurch, dass ein roher Byte-Blick auf Sektor 0 (wie in Fund 3) tatsächlich
kein klassisches RBF-LSN0 zeigt, auch wenn RBF governance-seitig
weiterhin aktiv ist.

**Nicht mehr offen (Runde 4 abgeschlossen), verbleibt spekulativ:** ob die
XD00BT-Struktur an einer Stelle einen Zeiger auf ein "echtes" RBF-LSN0
(mit z. B. `DD.TOT` etc.) enthält oder ob RBF x86-seitig direkt mit einem
angepassten Layout arbeitet, das den BIOS-Bootcode einfach als ersten paar
Bytes VOR den eigentlichen RBF-Feldern platziert (ähnlich wie ein
Boot-Sektor + Partitions-Offset bei FAT) — dafür wäre ein Byte-für-Byte-
Vergleich der LSN0-Struktur mit dem Technical-Manual-Layout nötig, nicht
mehr getan (Priorität sank, nachdem die Kernfrage "welcher File Manager
läuft tatsächlich" live beantwortet war).

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

1. **Erledigt:** `en_OS9_x86_Install.pdf` gelesen — reine Windows-
   Installationsanleitung (QEMU-Batch-Dateien, TAP-Adapter-Setup), keine
   Erklärung zum Boot-Volume-Format. Bestätigt aber Login (`super`/`user`)
   und Systemversion (OS-9/x86 U4.9, 80386).
2. **Erledigt (Fund 4):** Image tatsächlich gebootet (QEMU per Homebrew),
   live per `devs`/`dir`/`ident -m` geprüft — RBF ist der aktive File
   Manager für alle Block-Geräte, `"XD00BT"` ist nur ein x86-BIOS-Boot-
   Wrapper, kein Ersatz-Dateisystem. Kernfrage damit beantwortet.
3. Offen (nachrangig, s. Fund 4 Ende): ob die XD00BT-Struktur ein
   klassisches RBF-LSN0 an anderer Stelle referenziert oder RBF x86-seitig
   mit angepasstem Layout arbeitet — Byte-für-Byte-Vergleich mit dem
   Technical Manual (Kapitel 6) nicht mehr gemacht, da nachrangig.
4. Ghidra-Disassemblierung von `kernel`/`ioman`/`rbf` (x86-Target,
   analog zum bisherigen 68k-Vorgehen) — mit Andreas' Vermutung im
   Hinterkopf, dass der C-kompilierte Code sich besser dekompilieren
   lassen könnte als der handoptimierte 68k-Assembler. **Wichtig:** dafür
   eher die live aus dem Speicher extrahierten, größeren Module verwenden
   (76.944/38.672 Byte, s. Fund 4) statt der `../vendor/`-Dateien aus
   `mw86.tar` — die laufen tatsächlich auf diesem System, die
   `mw86.tar`-Kopien sind ein anderer (älterer) Build. Live-Extraktion aus
   dem laufenden Speicher wäre der nächste technische Schritt (z. B. über
   `ident -m -o` für den Speicher-Offset, dann per QEMU-Monitor `memsave`).
5. Restliche Kopf-Felder (`0x14`–`0x58`) Byte für Byte zuordnen.

**Erstellt**: 2026-08-13
