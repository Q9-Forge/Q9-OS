# Provenienz — `vendor-live/` (OS-9000/x86 v4.9, aus laufendem RAM extrahiert)

Diese Dateien sind **nicht** aus einer Datei kopiert, sondern per QEMU-
Monitor `pmemsave` direkt aus dem physischen RAM eines gebooteten Gasts
gezogen (Methode siehe [`README.md`](README.md)). Diese Datei hier
dokumentiert lückenlos: welches Archiv → welches Disk-Image → welcher
QEMU-Boot → welche Speicheradresse → welche extrahierte Datei.

## 1. Ursprungs-Archiv

| Feld | Wert |
|---|---|
| Pfad | `/Volumes/SSD1TB/#INFO/#Microware/OS9000/OS9000-4.9_on_QEmu-with-XiBase9_en.zip` |
| SHA-256 | `9e63cfcdab18a43c841ac475ff63c4f6c0d184f6effa020a40c5962e793fd9be` |
| Inhalt | fertiges bootfähiges Disk-Image `os9000-xibase.img` (~63 MB) + Windows-QEMU-Binaries + `en_OS9_x86_Install.pdf` |

## 2. Disk-Image — zwei Kopien im Umlauf, das ist beabsichtigt/erklärt

Das Zip wurde in zwei getrennten Sitzungen entpackt, daher liegen zwei
Kopien vor. **Wichtig:** das Image ist beim Booten beschreibbar
(`format=raw,if=ide`, kein `,readonly=on`) — OS-9000 schreibt beim Hochfahren
Zustandsdaten (Superblock-Zeitstempel, Prozesstabellen o. ä.) auf die
Platte, daher weicht eine bereits gebootete Kopie zwangsläufig vom
Frisch-entpackt-Zustand ab. Das ist normal und beeinträchtigt die
Live-Extraktion nicht (die kam aus dem RAM, nicht von der Platte).

| Kopie | Pfad | SHA-256 | Zustand |
|---|---|---|---|
| Pristin (nie gebootet) | `/Volumes/SSD1TB/os9000-xibase.img` | `3567173527339bd143a3c17dd6c7df986003faaa435640d17ef75a6ce35a84b1` | unverändert seit Entpacken (2026-08-05) |
| Arbeitskopie (für diese Extraktion gebootet) | `/Volumes/SSD1TB/projects/Q9-OS-os9000-extract/imgcheck/OS9000 on QEmu-with-XiBase9 (en)/os9000-xibase.img` | `067091d427cd232ae443b52ab30458c26d49e2c0bdf76701feca478323772228` | zuletzt geändert 2026-08-13 (dieser Boot) |

Die Adressen/Größen unten stammen aus dem Boot der **Arbeitskopie**.

## 3. QEMU-Boot (Sitzung 2026-08-13)

```bash
qemu-system-i386 -boot c -m 64 \
  -drive file=os9000-xibase.img,format=raw,if=ide,index=0 \
  -net nic -net user -display none -vga std \
  -monitor unix:/tmp/qemu-mon49.sock,server,nowait \
  -qmp unix:/tmp/qemu-qmp49.sock,server,nowait
```
(cwd: `.../Q9-OS-os9000-extract/imgcheck/OS9000 on QEmu-with-XiBase9 (en)/`)

Guest-Version laut Boot-Meldung: `OS-9/x86 U4.9  PC-AT Compatible 80386`.

## 4. Extraktion je Modul

Adresse/Größe per `ident -m -o <name>` im Gast abgelesen, dann
`pmemsave <addr> <size> "<file>"` im QEMU-Monitor. Datum: 2026-08-13.

| Modul | Adresse (RAM) | Größe | SHA-256 der extrahierten Datei |
|---|---|---|---|
| `kernel` | `0x21E400` | 76.944 | `1b4d357cee7939706eda37024b3e5bfd796486e88013622edf3bacb3b09cb62e` |
| `ioman` | `0x23347C` | 17.480 | `7bfb1f805ae8f1df310d83fe983ffbff62481930e6b9562a95fe2f83df5f272d` |
| `rbf` | `0x24D858` | 38.672 | `55dcb0db0d91553c32bcfeb5cc9a8e203df65c4ac382e6fbdd4d8c43190d2db2` |
| `ssm` | `0x2378C4` | 4.744 | `c053509dc5199bdceb3c693212c7e268d2d0ecb80dca59df77c208d2674a92c0` |
| `scf` | `0x23B768` | 16.952 | `de79b063ffe66ca4e0cce022c5da810644c3b604c14f36314a5d3549e01dd50c` |
| `pcf` | `0x256F68` | 35.920 | `58f9ef57ce7ece063d7f173e7e311fa659b248262ace11db958162e35948ec62` |
| `cdfm` | `0x2F9218` | 16.312 | `d5faf7037aadd530b1ae03d75da86732f565e8d2c27fe467353659e5c4ae70ee` |
| `pipeman` | `0x238CFC` | 10.680 | `a94db48a06208e0643bfff4b0b028d525135a9459dc719b9fc37f3c57e502185` |

`sbf` zum Extraktionszeitpunkt nicht verlinkt (`ident -m -o sbf` → E_MNF,
"Module not found") — kein Live-Export vorhanden, nur die `vendor/`-Kopie
(aus `mw86.tar`, anderer Build, siehe [`../vendor/PROVENANCE.md`](../vendor/PROVENANCE.md)).

## Nachvollziehbarkeit / Reproduktion

Bei jedem erneuten Boot der Arbeitskopie können sich die RAM-Adressen aus
Abschnitt 4 verschieben (Speicherlayout hängt vom genauen Boot-Ablauf ab) —
vor einer erneuten Extraktion immer zuerst `ident -m -o <name>` neu
abfragen, nicht blind die obigen Adressen wiederverwenden.

**Erstellt**: 2026-08-13
