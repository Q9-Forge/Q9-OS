# Live aus dem RAM extrahierte Systemmodule (OS-9000/x86 v4.9)

**Lückenlose Herkunftskette (Quell-Zip → Disk-Image → RAM-Adresse →
SHA-256 je Datei) steht in [`PROVENANCE.md`](PROVENANCE.md).** Dieses
README beschreibt nur die Methode, nicht die exakten Hashes/Adressen.

Diese 8 Dateien sind die **tatsächlich laufenden** Kopien von
`kernel`/`ioman`/`rbf`/`ssm`/`scf`/`pcf`/`cdfm`/`pipeman`, direkt aus dem
physischen RAM eines gebooteten `os9000-xibase.img`-Systems extrahiert —
NICHT aus einer Datei kopiert. Im Gegensatz dazu: [`../vendor/`](../vendor/)
enthält dieselben Modulnamen, aber aus der separaten `mw86.tar` (Eval-CD).
**Alle 8 Module unterscheiden sich zwischen `vendor/` und `vendor-live/`**
(unterschiedliche Dateigrößen) — das laufende System bootet einen anderen,
vermutlich neueren Build, der direkt im `sysboot`-Bootfile eingebettet ist
und dort nicht als Einzeldatei zugänglich ist (`sysboot` selbst ist ein
komprimiertes Container-Format, siehe [`../docs/FINDINGS.md`](../docs/FINDINGS.md),
Fund 5 "Bonus-Fund").

## Methode

OS-9000s eigener `ident`-Befehl unterstützt `-m` (Modul im Speicher) UND
`-o` (Speicheradresse anzeigen) — zusammen liefert `ident -m -o <name>`
exakt die physische RAM-Adresse und Größe eines geladenen Moduls. Da die
CPU im laufenden System einen flachen 32-Bit-Protected-Mode-Adressraum
nutzt (Segment-Basis 0, per `info registers` bestätigt), entspricht diese
"Offset" genannte Adresse direkt einer physischen Adresse — genau das, was
QEMUs Monitor-Befehl `pmemsave <addr> <size> <file>` erwartet.

```
(OS9_w0)[/dd/>] ident -m -o kernel
Offset:      $21E400      #2221056
Header for:  kernel
Module size: $12C90       #76944
...
```

```bash
# im QEMU-Monitor (unix socket, per `nc` oder `qemu.py` erreichbar):
pmemsave 0x21e400 76944 "/tmp/kernel_live.bin"
```

Jede extrahierte Datei wurde gegen ihren eigenen Modul-Header verifiziert:
Sync-Byte `0x4AFC`/`0xFC4A` vorhanden, Größenfeld im Header (Offset `0x04`,
4-Byte-LE) stimmt exakt mit der tatsächlichen Dateigröße überein,
Namensstring an dem im Header referenzierten Offset stimmt mit dem
erwarteten Modulnamen überein.

## Adressen (dieses konkrete Boot, `os9000-xibase.img` v4.9 — können sich
   bei einem Neustart/anderen Boot-Konfiguration ändern)

| Modul | Adresse | Größe | Type-Byte |
|---|---|---|---|
| `kernel` | `0x21E400` | 76.944 | `0x0C` (Systm) |
| `ioman` | `0x23347C` | 17.480 | `0x0C` (Systm) |
| `rbf` | `0x24D858` | 38.672 | `0x0D` (Fmgr) |
| `ssm` | `0x2378C4` | 4.744 | `0x0C` (Systm) |
| `scf` | `0x23B768` | 16.952 | `0x0D` (Fmgr) |
| `pcf` | `0x256F68` | 35.920 | `0x0D` (Fmgr) |
| `cdfm` | `0x2F9218` | 16.312 | `0x0D` (Fmgr) |
| `pipeman` | `0x238CFC` | 10.680 | `0x0D` (Fmgr) |

`sbf` war im Test-Boot nicht verlinkt/geladen (`ident -m -o sbf` → "Module
not found") — kein Live-Export möglich, `../vendor/sbf` (aus `mw86.tar`)
bleibt die einzige verfügbare Kopie.

## Reproduzierbarkeit

Diese Module sind jetzt saubere, eigenständige, Ghidra-taugliche
Binärdateien — für eine Disassemblierung (analog zum bisherigen
68k-Vorgehen) sind das die richtigen Ausgangsdateien, nicht die
`vendor/`-Kopien aus `mw86.tar`.

**Erstellt**: 2026-08-13
