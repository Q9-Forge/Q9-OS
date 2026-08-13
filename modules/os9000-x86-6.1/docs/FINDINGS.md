# OS-9000 (x86) Version 6.1 — VirtualBox-Appliance gebootet, Live-Vergleich mit 4.9

Fortsetzung von [`../os9000-x86/docs/FINDINGS.md`](../../os9000-x86/docs/FINDINGS.md)
(OS-9000 v4.9). Andreas hatte zwei weitere `.ova`-Dateien (VirtualBox-
Appliances) gefunden, deutlich neuer als das v4.9-Eval-Image. Diese Runde:
eine davon (`OS-9-6.1-EN_10_20_18.ova`) entpackt und gebootet, um zu
prüfen, ob die Type/Lang-Byte-Konvention und "RBF ist noch aktiv" auch
über eine weitere Zeitspanne hinweg stabil bleiben.

## Quelle

`/Volumes/SSD1TB/#INFO/#Microware/OS9000/OS-9-6.1-EN_10_20_18.ova`
(Duplikat auch unter `OS9000 (1)/`) — eine `.ova`-Datei ist ein
Tar-Archiv nach OVF-Spezifikation (VirtualBox/VMware-Appliance-Format):
eine `.ovf`-XML-Beschreibung (virtuelle Hardware) plus `.vmdk`-Festplatten-
images. Datiert 2018-10-20, laut `.ovf`-Annotation "With US/EN Keyboard
setup".

Entpackt nach `/Volumes/SSD1TB/projects/Q9-OS-os9000-6.1/extracted/`
(außerhalb des Git-Repos, wie die Ghidra-Projekte):
- `OS-9-6.1-EN.ovf` — VM-Konfiguration: 1 vCPU, **1024 MB RAM** (deutlich
  mehr als v4.9s 64 MB), zwei IDE-Festplatten (`disk1` 1 GB "System",
  `disk2` 4 GB, praktisch leer — 68 KB tatsächliche Daten dank
  `streamOptimized`-VMDK-Kompression), Netzwerkkarte Am79C973 (PCnet)
  bzw. E1000, kein PAE/LongMode (bestätigt: reines 32-Bit-System).
- `OS-9-6.1-EN-disk001.vmdk` (127 MB komprimiert, 1 GB virtuell)
- `OS-9-6.1-EN-disk002.vmdk` (68 KB komprimiert, 4 GB virtuell)

Zweite `.ova` (`OS-9-6.1-EN-XiBase9_10_20_18.ova`, vermutlich dieselbe
Version plus XiBase9-Grafik-Frontend wie beim v4.9-Paket) liegt ebenfalls
lokal vor, **noch nicht untersucht**.

## Boot funktioniert direkt mit QEMU, kein VirtualBox nötig

`qemu-img info` liest beide `.vmdk`-Dateien ohne Konvertierung (Format
`vmdk`, `streamOptimized`). Boot-Kommando:

```bash
qemu-system-i386 -boot c -m 1024 \
  -drive file=OS-9-6.1-EN-disk001.vmdk,format=vmdk,if=ide,index=0 \
  -drive file=OS-9-6.1-EN-disk002.vmdk,format=vmdk,if=ide,index=1 \
  -net nic,model=pcnet -net user,hostfwd=tcp::2222-:22 \
  -display none -vga std \
  -monitor unix:/tmp/qemu-mon61.sock,server,nowait \
  -qmp unix:/tmp/qemu-qmp61.sock,server,nowait
```

Bootet sauber durch bis zum Shell-Prompt, Startup-Skript sichtbar (Screenshot
[`boot-evidence/startup_and_login.png`](boot-evidence/startup_and_login.png)):

```
* OS-9 - Version 6.0
iniz r0 hc1 term     ;* initialize devices
alias /h0 /hc1
alias /nd /hc1
assign cd chd
chx /h0/cmds
setenv PATH /r0/CMDS:/h0/CMDS:/h0/MWOS/OS9000/80386/cmds
ipstart
list sys/motd
* * * * *  Welcome to OS-9 Version 6.1  * * * * *
...
setenv TERM vt100
setenv PROMPT "Super:%p"
*sshd &
ex mshell<>>>/term -el
Super:/hc1>
```

Bemerkenswert: `PATH` referenziert `/h0/MWOS/OS9000/80386/cmds` — dieselbe
`MWOS`-Verzeichniskonvention und explizite Architektur-Unterordnerung
(`80386`), die auch das eigene `Q9-OS/vendor/`-Layout für 68k-CPU-Familien
verwendet (`68000/`, `68020/`, ...). Ein weiteres Indiz für Microwares
langfristig konsistente Namenskonventionen über Produktlinien hinweg.

`sshd &` läuft, Port 22 auf Host-Port 2222 weitergeleitet — TCP-Verbindung
öffnet (`nc -z` erfolgreich), aber der SSH-Handshake selbst hängt
("Connection timed out during banner exchange", auch mit rohem `nc` keine
Banner-Antwort innerhalb 5s). Nicht weiter verfolgt — plausibel verwandt
mit dem bereits in `Q9-Flux/.claude/ARBEITSPLAN.md` (Schritt 5.15)
dokumentierten `sptcp`/SPF-Stack-TCP-Hänger-Bug unter emulierten
Bedingungen; hier nicht gezielt untersucht, da die Kernfragen (Type-Byte-
Konvention, RBF-Status) auch über die VGA-Konsole per QMP-Tastatur-
Injektion (Verfahren aus der v4.9-Runde übernommen) beantwortbar waren.

## Live-Befund: `devs` und `ident -m` bestätigen v4.9-Fund erneut, unabhängig

`devs` (Screenshot [`boot-evidence/devs_table.png`](boot-evidence/devs_table.png)):

| Device | Driver | File Mgr |
|---|---|---|
| `hc1` | `rb1003v2` | **`rbf`** |
| `r0` | `ram` | **`rbf`** |
| `term` | `sc8042m` | `scf` |
| `nil` | `null` | `scf` |
| `usb` | `usbman` | `nullfm` |
| `ip0`/`tcp0`/`udp0`/`raw0`/`route0` | `spip` u. a. | `spf` |

**RBF ist auch in Version 6.1 (2018) weiterhin der aktive File Manager**
für die Festplatte — dieselbe Aussage wie bei v4.9 (1998/99), zwei
Jahrzehnte Versionsabstand. Neu gegenüber v4.9: ein `usb`-Gerät
(`usbman`-Treiber) — USB-Unterstützung kam erst mit einer späteren
OS-9000-Version dazu, passt zur Datierung.

**`ident -m` auf die live laufenden Module** (Screenshots
[`boot-evidence/ident_kernel_ioman.png`](boot-evidence/ident_kernel_ioman.png),
[`boot-evidence/ident_rbf.png`](boot-evidence/ident_rbf.png)):

| Modul | Ty/La (live) | Modulgröße (live, v6.1) | zum Vergleich: v4.9 (live) |
|---|---|---|---|
| `kernel` | `$0C01` (Systm) | (Kopf abgeschnitten, Größe nicht erfasst) | 76.944 Byte |
| `ioman` | `$0C01` (Systm) | **17.680 Byte** | ~ (nicht erfasst) |
| `rbf` | **`$0D01`** (Fmgr) | **39.864 Byte** | 38.672 Byte |

**Dritte unabhängige Bestätigung derselben Type/Lang-Byte-Konvention**
(Type `0x0C`=Systm/`0x0D`=Fmgr an Offset `0x13`, Lang `0x01`=Objct an
Offset `0x12`) — jetzt über DREI Zeitstände hinweg live verifiziert:
68k-OS-9/68K (Q9-OS-Hauptrecherche), OS-9000 x86 v4.9 (1998/99), OS-9000
x86 v6.1 (2018). `rbf`-Modulgröße ist mit v4.9 fast identisch (39.864 vs.
38.672 Byte) — plausibel wenig funktionale Änderung am RBF-Kern über
diese Zeitspanne, im Gegensatz zum viel dramatischeren Sprung zwischen
68k-RBF (9.638 Byte) und x86-RBF (~38–39 KB).

## Aufräumen

QEMU-Prozess nach dieser Session-Runde beendet
(`kill` auf die PID aus `nohup ... &`); Ghidra-Projektverzeichnis für
diese Version noch nicht angelegt (keine Disassemblierung in dieser
Runde, nur Live-Beobachtung).

## Nächste Schritte (falls gewünscht)

1. `OS-9-6.1-EN-XiBase9_10_20_18.ova` (das zweite, noch nicht geöffnete
   Archiv) auf Unterschiede zur reinen 6.1-Version prüfen.
2. SSH-Hänger-Bug genauer untersuchen (optional, nicht kernrelevant für
   die Modul-Architektur-Fragen).
3. `kernel`/`ioman`/`rbf` dieser Version aus dem laufenden Speicher
   extrahieren (wie in der v4.9-Runde vorgeschlagen) und mit Ghidra
   disassemblieren — dann ließe sich auch prüfen, ob sich der eigentliche
   RBF-Code zwischen v4.9 und v6.1 spürbar geändert hat, trotz fast
   gleicher Modulgröße.

**Erstellt**: 2026-08-13
