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

## Fund 1: Modul-Header-Format vollständig anhand der offiziellen Technical Manuals geklärt (nicht geraten)

Andreas' berechtigter Einwand beim Aufbau des Kernel-Walkthroughs: "ist der
Header nicht in der Doku definiert... das können wir doch nachschlagen ...
denke da müssen wir nicht raten." Stimmt — beide Header-Formate sind
offiziell dokumentiert:

- **68K**: `MWOS/DOC/RadiSys/68k_tech.pdf`, Kapitel 1, Table 1-6/1-7/1-8
  ("Module Header Fields" / "Module Header Standard Fields" / "Additional
  Header Fields for Individual Modules").
- **OS-9000**: `MWOS/DOC/RadiSys/os9k_tech.pdf`, Kapitel 1, "Module Header
  Definitions" — vollständige C-Struct-Deklaration `mh_com` aus `module.h`.

Sync-Byte ist `fc 4a` statt `4a fc` — **dieselbe** 16-Bit-Konstante
`0x4AFC`, nur wegen x86-Little-Endian byte-vertauscht gespeichert (68k ist
big-endian), exakt wie für `M$ID`/`m_sync` dokumentiert.

### Die Type/Lang-Byte-"Vertauschung" — real, aber vollständig durch Endianness erklärt

Erster empirischer Blick sah aus wie eine Vertauschung: 68K hat `0x0C`
(Type) bei Offset `0x12` und `0x01` (Lang) bei `0x13`; x86 hat `0x01`
(Lang) bei `0x12` und `0x0C` (Type) bei `0x13`. **Das ist kein Zufall und
keine willkürliche Neuordnung** — beide Manuals erklären es:

- **68K, Table 1-7**: `$12 M$Type` und `$13 M$Lang` sind zwei **getrennte,
  unabhängige 1-Byte-Felder**. Da beides Einzelbytes sind, gibt es hier
  keinerlei Endianness-Frage — Type steht einfach immer vor Lang.
- **OS-9000, `mh_com`-Struct**: die beiden Felder wurden im C-Neuschrieb zu
  **einem einzigen `u_int16 m_tylan`** zusammengelegt ("Contains the module
  type (first/high byte) and language (second/low byte)", konzeptionell
  also `m_tylan = (Type<<8) | Lang`, z. B. `0x0C01`). Ein `u_int16` ist
  aber eine waschechte Mehrbyte-Ganzzahl — und die wird, exakt wie
  `m_sync`/`m_size`, in der **nativen Endianness des Zielsystems**
  gespeichert. Little-Endian legt das niederwertige Byte (Lang) an die
  niedrigere Adresse (`0x12`), das höherwertige Byte (Type) an die höhere
  (`0x13`) — **genau umgekehrt zur 68K-Reihenfolge**, obwohl der
  *konzeptionelle* Wert (`0x0C01`) identisch ist.

Verifiziert per direktem Rohbyte-Vergleich (`vendor/68020/dker030s`,
`modules/os9000-x86/vendor-live/kernel` **und** `vendor-live/ioman` als
Gegenprobe — alle drei konsistent):

| | Byte @ `0x12` | Byte @ `0x13` | `m_tylan` als 16-Bit-Wert |
|---|---|---|---|
| 68K (`dker030s`) | `0x0C` = M$Type (Systm) | `0x01` = M$Lang (Objct) | *(zwei getrennte Felder, kein Integer)* |
| x86 (`kernel`, `ioman`) | `0x01` (LSB) | `0x0C` (MSB) | `0x0C01` LE-dekodiert = Type `0x0C`, Lang `0x01` — **identisch zum 68K-Wert** |

**Dasselbe Muster wiederholt sich bei `M$Attr`/`M$Revs` → `m_attrev`:** 68K
hat sie als zwei getrennte Bytes (`$14 M$Attr`, `$15 M$Revs`), OS-9000
legt sie zu einem `u_int16 m_attrev` zusammen (LE gespeichert: `0x14`=Revs
(LSB), `0x15`=Attr (MSB) — Attr-Wert `0xA0` beim Kernel identisch zum
68K-Wert `0xA0` "system-state + reentrant"). Konsistentes Muster: der
C-Neuschrieb hat mehrere eng verwandte 68K-Bytepaare zu einzelnen
16-Bit-Feldern zusammengelegt, wodurch sie erst durch die x86-Endianness
scheinbar "vertauscht" wirken.

### Vollständiger, offiziell dokumentierter OS-9000-Header (`kernel`, alle Werte real ausgelesen)

Damit ist der bisher als "noch nicht geklärt" markierte Bereich `0x14`–`0x58`
**vollständig aufgelöst** — keine der Zahlen unten ist geraten, alle Feldnamen
stammen direkt aus `mh_com` (`os9k_tech.pdf`):

| Offset | Feld (`mh_com`) | Wert (`kernel`, live) | Bedeutung |
|---|---|---|---|
| `0x00` | `m_sync` | `0x4AFC` | Sync, identisch zu 68K (endian-gespiegelt) |
| `0x02` | `m_sysrev` | `0x0002` | Format-Revision |
| `0x04` | `m_size` | `76944` | = exakte Dateigröße |
| `0x08` | `m_owner` | `0` | kein Owner |
| `0x0C` | `m_name` | `0x58` | Name-Offset (4 Byte breit, s. u.) |
| `0x10` | `m_access` | `0x0555` | r-x r-x r-x (owner/group/world), keine Schreibrechte |
| `0x12` | `m_tylan` | `0x0C01` (LE) | Type=`0x0C` Systm, Lang=`0x01` Objct — s. o. |
| `0x14` | `m_attrev` | `0xA000` (LE) | Attr=`0xA0` (system-state+reentrant), Revs=`0` |
| `0x16` | `m_edit` | `0x00CD` (205) | Edition/Build-Zähler |
| `0x18` | `m_needs` | `0` | keine Hardware-Anforderungsflags |
| `0x1C` | `m_share` | `0` | kein Shared-Data-Offset |
| `0x20` | `m_symbol` | `0` | keine Symboltabelle (Release-Build) |
| `0x24` | `m_exec` | `0xA4` | **Einsprungpunkt** — bestätigt den empirischen Fund aus `KERNEL_INIT.md` Fund 1 exakt |
| `0x28` | `m_excpt` | `0` | kein Default-User-Trap-Handler |
| `0x2C` | `m_data` | `0x1B60` (7008) | Datenbereichsgröße (Analogon zu 68Ks `M$Mem`) |
| `0x30` | `m_stack` | `0x4000` (16384) | Stackgröße — 16 KB, plausibel für den Kernel-Init-Stack |
| `0x34` | `m_idata` | `0x10D58` (68952) | Offset initialisierte Daten |
| `0x38` | `m_idref` | `0x128C0` (75968) | Offset Datenreferenzlisten, nahe Modulende |
| `0x3C`–`0x4B` | `m_init`/`m_term`/`m_dbias`/`m_cbias` | alle `0` | ungenutzt |
| `0x4C` | `m_ident` | `0` | ungenutzt |
| `0x4E`–`0x55` | `m_spare[8]` | `0` | reserviert |
| `0x56` | `m_parity` | `0x4E0D` | Header-Prüfsumme |
| `0x58` | Name | `"kernel\0"` | direkt nach dem 88 Byte (`0x58`) langen Standard-Header — passt exakt zu `m_name=0x58` |

**Korrektur einer eigenen früheren Fehlinterpretation** (`KERNEL_INIT.md`
Fund 1 hatte Feld `0x20` versuchsweise als "candidate M$Excpt" bezeichnet —
das ist laut `mh_com` tatsächlich `m_symbol`; der echte `m_excpt` liegt bei
`0x28`, Wert `0`, siehe Tabelle oben. `KERNEL_INIT.md` wurde entsprechend
korrigiert.

**Für den 68K-Kernel ebenfalls geklärt** (per `68k_tech.pdf`, Table 1-8):
Feld `0x34` (bisher in `docs/REVERSE_ENGINEERING.md` als "unbekannt, immer
0 beobachtet" notiert) ist **`M$Excpt`** — Wert `0`, dieselbe Bedeutung wie
x86s `m_excpt`. Table 1-8 bestätigt außerdem, dass `M$Exec`+`M$Excpt` die
**einzigen** offiziell für System-Module (`Systm`) definierten
Erweiterungsfelder sind (`M$Mem`/`M$Stack`/etc. sind laut Manual nur für
Program-/Trap-Handler-/Device-Driver-Module dokumentiert) — der Rest von
68Ks Erweiterungsbereich (`0x38`–`0x53`, inkl. der `0xB0BD`-Magic-Konstante)
ist also **kein** offiziell dokumentiertes Feld, sondern Microware-interne
Konvention/Padding, für System-Module ungenutzt.

**Namensfeld-Breite (`M$Name`: 2 Byte bei 68K vs. 4 Byte bei x86) und
Terminierungskonvention (68K High-Bit vs. x86 NUL) bleiben echte
Formatabweichungen**, nicht durch Endianness erklärbar — beides sind
bewusste Entscheidungen im C-Neuschrieb, keine Fehlinterpretation unsererseits.

CRC-Konvention am Modulende weiterhin nicht verifiziert (kein `mh_com`-Feld
dafür, das liegt hinter dem eigentlichen Modulkörper).

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

## Fund 5: Toolshed kann OS-9000/x86-RBF-Images NICHT direkt lesen — das On-Disk-Format hat sich geändert (nicht nur der Modul-Header)

Andreas fragte, ob Toolshed (`os9`-Kommando, NitrOS-9-Projekt, bereits für
die 68k-Images im Einsatz) das x86-Image "zerlegen" kann. Kurzantwort:
**nein, jedenfalls nicht ohne Weiteres** — aber der Versuch hat die
RBF-On-Disk-Struktur für OS-9000/x86 vollständig geklärt.

**Test 1** (`os9 dir` direkt auf `os9000-xibase.img`): schlägt fehl
(`error 1`, unbekannter Fehler — Toolshed erkennt das MBR-gewrappte Image
gar nicht als Datenträger).

**Test 2** (MBR-Partition per `dd` herausgeschnitten, `skip=17`
Sektoren): Toolshed öffnet die Datei jetzt, aber `error 216`
("pathname not found") — Root-Verzeichnis nicht auffindbar. Blindes
Offset-Sondieren (verschiedene `skip`-Werte, `-nb400`-Vermutung aus
`bootgen`, Byte-Muster-Suche nach plausiblen `DD.TOT`-Werten im ersten MB)
brachte **keinen** Treffer — bestätigt später: der Bereich bei
Partitionsanfang ist tatsächlich nur x86-BIOS-Bootcode (`"XD00BT"`), keine
verschleierte RBF-LSN0.

**Test 3, entscheidend:** eine frisch **vom laufenden OS-9000-System
selbst** formatierte Diskette (`format /d0`, per QMP-Tastatursimulation
im gebooteten Gast) ebenfalls mit Toolshed geöffnet — **auch das
schlägt fehl** (`error 211`, "input past end-of-file", nach
`Warning: T0S is zero`). Das ist der Beweis: es liegt **nicht** an einem
MBR-/Bootloader-Wrapper, sondern am RBF-Format selbst — OS-9000/x86 nutzt
ein grundlegend anderes On-Disk-Layout als das 68k-RBF, das Toolshed kennt.

### Manuell dekodiert (reines Python, kein Toolshed nötig)

Rohbyte-Analyse der guest-formatierten Diskette (1,44 MB, 2879 Blöcke à
512 Byte, Name `"SYSBOOT"`) ergab ein vollständig konsistentes,
neues LSN0-Format — jedes Feld stimmt exakt mit den vom `format`-Dialog
selbst angezeigten Parametern überein:

| Klassisches 68k-RBF | OS-9000/x86-RBF (empirisch ermittelt) |
|---|---|
| LSN0 bei physischem Block 0 | **LSN0 bei physischem Block 1** (Block 0 reserviert/leer — Platz für einen Bootsektor, selbst auf einer reinen Datendiskette) |
| Felder 2–3 Byte, Big-Endian | **Alle Felder 4 Byte, Little-Endian** |
| Verzeichniseinträge 32 Byte, Name High-Bit-terminiert | **Verzeichniseinträge 64 Byte, Name NUL-terminiert**, FD-Zeiger als 4-Byte-LE am Ende des Eintrags |
| `DD.DIR` (Root-Verzeichnis-LSN) 3 Byte @ Offset 8 | `DD.DIR`-Äquivalent 4 Byte @ Offset `0x28` (LSN2 im Test) |

Erfolgreich von Hand nachvollzogen: LSN0 → Root-FD (LSN2) → Root-
Verzeichnisdaten (LSN3, Einträge `".."`/`"."`/`"sysboot"`) → `sysboot`s
eigener FD (LSN960, direkt hinter der vom `format`-Dialog genannten
Bitmap-Adresse `959`) → Segmentliste (Daten ab LSN961, Länge 1918 Blöcke)
→ **`sysboot` erfolgreich rein aus den Rohbytes extrahiert, 982.016 Byte,
ohne jedes Toolshed/Ghidra-Werkzeug, nur Python** (Details/Skript-Logik
siehe Abschnitt "Werkzeuge" unten).

### Bonus-Fund: `sysboot` selbst ist ein komprimiertes Container-Format, keine einfache Modul-Verkettung

Die extrahierten 982.016 Byte beginnen mit einem eigenen Header —
ASCII **`"OS9Z"`** gefolgt von der Konstante `0x12345678` (klassischer
Byte-Order-Erkennungswert) und weiteren Feldern — **nicht** mit dem
bekannten Modul-Sync `0x4AFC`. Eine Suche nach den Modul-Sync-Byte-Paaren
(`4a fc` bzw. `fc 4a`, je nach Byte-Reihenfolge) im Rest der Datei findet
nur ~10–11 Treffer auf 982 KB — das deckt sich mit der **zufällig zu
erwartenden Trefferzahl** (≈15 bei rein zufälligen Bytes dieser Länge),
nicht mit gehäuften echten Modul-Kopfzeilen. Schlussfolgerung: `sysboot`
ist **komprimiert** (der Name `"OS9Z"` passt dazu — vermutlich "Zipped"
o. ä.), keine bloße Aneinanderreihung von `kernel`/`ioman`/`rbf` &Co. Das
Kompressionsverfahren ist nicht identifiziert — für eine weitere Zerlegung
in die Einzelmodule bräuchte es zuerst dessen Header-Format (Segment-
Tabelle, Kompressionsalgorithmus) verstanden.

## Fund 6: Alle 8 live laufenden Systemmodule vollständig aus dem RAM extrahiert

Fund 4 hatte bereits `kernel`/`ioman`/`rbf` teilweise live per `ident -m`
geprüft (nur Type/Lang-Byte und Größe abgelesen, keine Datei extrahiert).
Diese Runde: das systematisch für ALLE aktuell geladenen Module gemacht,
und zwar als tatsächliche Binärdateien, nicht nur als Metadaten-Ablesung.

**Methode:** `ident -m -o <name>` liefert zusätzlich zu Type/Lang/Größe auch
den `Offset` — die physische RAM-Adresse des Moduls. Da die CPU im
32-Bit-Protected-Mode mit Segment-Basis 0 läuft (bestätigt per
`info registers`), ist dieser Offset direkt eine physische Adresse, die
sich unverändert an QEMUs Monitor-Befehl `pmemsave <addr> <size> <file>`
übergeben lässt (Anführungszeichen um den Dateinamen nötig, sonst
Parser-Fehler bei `/`).

Für alle 9 mit `devs`/Fund 4 bekannten Kernmodule abgefragt — `sbf` war zum
Testzeitpunkt nicht verlinkt (`E_MNF`, "Module not found"), die übrigen 8
erfolgreich extrahiert und verifiziert (Sync-Byte `0xFC4A` korrekt,
Größenfeld im Header entspricht exakt der Dateigröße, Namensstring stimmt):

| Modul | Adresse | Größe live | Größe `../vendor/` (`mw86.tar`) |
|---|---|---|---|
| `kernel` | `0x21E400` | 76.944 | 62.352 |
| `ioman` | `0x23347C` | 17.480 | 15.504 |
| `rbf` | `0x24D858` | 38.672 | 32.432 |
| `ssm` | `0x2378C4` | 4.744 | 4.672 |
| `scf` | `0x23B768` | 16.952 | 15.472 |
| `pcf` | `0x256F68` | 35.920 | 38.896 (live hier KLEINER als vendor) |
| `cdfm` | `0x2F9218` | 16.312 | 12.696 |
| `pipeman` | `0x238CFC` | 10.680 | 10.400 |

**Alle 8 Module unterscheiden sich in der Größe vom `mw86.tar`-Referenzbuild**
— bestätigt endgültig, was Fund 4 schon für `kernel`/`rbf` andeutete: das
gebootete System nutzt durchgehend einen anderen (vermutlich neueren) Build,
der komplett im komprimierten `sysboot`-Container (Fund 5, Bonus-Fund)
eingebettet ist. Bemerkenswert: `pcf` ist als einziges Modul live KLEINER
als sein `vendor/`-Gegenstück — kein simples "neuer = immer größer"-Muster,
sondern echte Bau-/Konfigurationsunterschiede pro Modul.

Dateien liegen jetzt unter
[`vendor-live/`](../vendor-live/README.md) — eigenständige, Ghidra-taugliche
Binärdateien, für eine Disassemblierung die richtigen Ausgangsdateien
(nicht `vendor/`), weil sie den tatsächlich laufenden Code enthalten. Die
lückenlose Herkunftskette (Quell-Zip → welche Disk-Image-Kopie →
QEMU-Boot → RAM-Adresse → SHA-256 je Datei) steht in
[`vendor-live/PROVENANCE.md`](../vendor-live/PROVENANCE.md); die statischen
`vendor/`-Dateien aus `mw86.tar` haben ihr eigenes
[`vendor/PROVENANCE.md`](../vendor/PROVENANCE.md).

## Fund 7: Kernel-Init-Sequenz per Ghidra disassembliert und mit dem 68K-Kernel verglichen

Vollständiger Bericht: [`KERNEL_INIT.md`](KERNEL_INIT.md). Kurzfassung:
Entry-Point empirisch gefunden (Header-Feld `0x24`, x86-Analogon von
`M$Exec`, zeigt auf einen `JMP`-Trampolin), Ghidra-Autoanalyse erreicht
**70 % Code-Abdeckung / 229 Funktionen komplett automatisch** (deutlich
besser als beim handoptimierten 68K-Kernel — bestätigt Andreas'
Vermutung, dass C-kompilierter Code sich leichter automatisch erschließen
lässt). Die Init-Kette (Entry-Funktion → `FUN_0022246c` → `FUN_0021eb26`)
zeigt dieselbe grobe Boot-Reihenfolge wie der 68K-Kernel (Speicher/Arena
einrichten, Exception-Dispatch-Tabelle aus einer kompakten Quelltabelle
aufbauen, Prozess-/Deskriptor-Freilisten vorbereiten, ersten
Ausführungskontext von Hand konstruieren) — koppelt Hardware- und
Systemlogik aber konsequent über Funktionszeiger/virtuelle Aufrufe in eine
externe, zielspezifische Beschreibungsstruktur, statt wie beim 68K direkt
im Modul zu verdrahten (VBR, feste Sprungadressen). Bestätigt damit die
im Technical Manual dokumentierte "Low-Level System"-Portierungsgrenze
erstmals im disassemblierten Code selbst, nicht nur in der Dokumentation.
Nebenfund: die 68K-"`0xB0BD`"-Struktursignatur (siehe
`../../docs/REVERSE_ENGINEERING.md`) taucht unverändert auch im x86-Kernel
auf, direkt hinter dem Entry-Trampolin. Auch gefunden: eine x86-Modul-
Relozierungsfunktion (`FUN_0022246c`) — ein echter Architekturunterschied,
da klassische 68K-Module bewusst positionsunabhängig sind und keine
Relozierung benötigen.

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
3. **Geklärt (Fund 5):** die XD00BT-Struktur referenziert kein klassisches
   RBF-LSN0 an anderer Stelle — RBF hat auf x86 tatsächlich ein eigenes,
   grundlegend anderes On-Disk-Format (4-Byte-LE-Felder, LSN0 bei Block 1,
   64-Byte-Verzeichniseinträge). Vollständig dokumentiert und von Hand
   dekodiert, Toolshed kann dieses Format nicht lesen (baut auf 68k-RBF
   auf). `sysboot` erfolgreich rein aus Rohbytes extrahiert — stellte sich
   als eigenes, vermutlich komprimiertes Container-Format heraus (`"OS9Z"`
   Header), nicht als einfache Modul-Verkettung.
4. Offen: `sysboot`s Kompressionsformat identifizieren, um die
   gebündelten Module (kernel/ioman/rbf/...) direkt aus der Boot-Datei zu
   extrahieren — durch Fund 6 aber nicht mehr blockierend: der
   Live-Extraktions-Weg liefert bereits alle laufenden Module als saubere
   Einzeldateien.
5. **Erledigt (Fund 6):** alle 8 aktuell geladenen Module
   (`kernel`/`ioman`/`rbf`/`ssm`/`scf`/`pcf`/`cdfm`/`pipeman`) live per
   `ident -m -o` + `pmemsave` aus dem laufenden Speicher extrahiert, unter
   [`vendor-live/`](../vendor-live/README.md) abgelegt. Alle unterscheiden
   sich in der Größe von den `mw86.tar`-Kopien in `vendor/`.
6. Ghidra-Disassemblierung von `kernel`/`ioman`/`rbf` (x86-Target,
   analog zum bisherigen 68k-Vorgehen) — mit Andreas' Vermutung im
   Hinterkopf, dass der C-kompilierte Code sich besser dekompilieren
   lassen könnte als der handoptimierte 68k-Assembler. Dafür jetzt
   `vendor-live/` als Quelle verwenden, nicht `vendor/`.
7. Restliche Kopf-Felder (`0x14`–`0x58`) Byte für Byte zuordnen.

**Erstellt**: 2026-08-13
