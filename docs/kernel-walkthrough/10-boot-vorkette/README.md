# Thema 10: Boot-Vorkette — was VOR dem Kernel-Einsprung passiert

Die gesamte bisherige Serie (Themen 00-09) beginnt immer erst am
Kernel-Modul-Einsprung — `docs/KERNEL.md` setzt dort explizit voraus,
dass "System-Global-Bereich existiert bereits im RAM". WAS diesen Bereich
anlegt, wie der Kernel überhaupt erst ins RAM kommt, und wer die
Reset-Vektortabelle füllt, wurde in dieser Serie noch nie untersucht —
genau diese Lücke schließt dieses Thema.

**Der entscheidende Unterschied zu allen bisherigen Themen**: hier gibt
es keine "vendor"-Moduldatei mit OS-9-Header, sondern eine **echte
Boot-ROM-Binärdatei** — `romimage.dev.running.BIN`, die Q9-Flux (das
Schwesterprojekt, der 68K-Emulator) tatsächlich beim Booten lädt (`q9.exe
--rom romimage.dev.running.BIN --cf OS9SYS.hda`, siehe
`Q9-Flux/docs/OS9SYS_BOOT.md`). Diese Datei ist **proprietär und liegt
NICHT in irgendeinem Repository** (weder Q9-Flux noch Q9-OS) — nur kurze,
benannte Assembler-Exzerpte werden hier dokumentiert, keine Bulk-Dumps
oder die Binärdatei selbst.

## Label-Konvention

- **68K**: `Q9_bootrom_*`, neues Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-bootrom/` (außerhalb beider Repos), Skript `../../../modules/bootrom/ghidra_scripts/BootRomAnalyzeAndDump.java`.
- **x86**: `Q9X_vectx86_*`, neues Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-vectx86/`, Skript `../../../modules/os9000-x86/ghidra_scripts/vectx86_AnalyzeAndDump.java`.

## 68K: der Boot-ROM baut den System-Global-Bereich, bevor der Kernel je läuft

Die Reset-Vektortabelle (Datei-Offset 0) verrät sofort die Ladeadresse:
Vektor 1 (initiale PC) = `0xFE000494` = exakt Datei-Offset `0x494` — die
ROM-Abbildung liegt final bei `0xFE000000`. Gleichzeitig liest die CPU
beim Hardware-Reset **dieselbe** Tabelle an Adresse `0x00000000` (68K-
Standard) — klassischer **Overlay-Bootmechanismus**: das ROM ist beim
Reset zusätzlich an Adresse 0 gespiegelt, bis der Reset-Code selbst die
Spiegelung deaktiviert. Bemerkenswert: Vektoren, die dauerhaft gültig
bleiben müssen (Reset, Bus-/Address-/Illegal-Instruction-Error als
früheste Diagnose, Trace, NMI), zeigen konsequent auf die "hohe",
dauerhafte ROM-Adresse (`0xFE00xxxx`); alle übrigen (Zero Divide, CHK,
TRAPV, Line-A/F, ...) zeigen auf "niedrige" `0x0000xxxx`-Adressen —
plausibel Platzhalter, die der später geladene Kernel (Thema 01) mit
echten RAM-Handlern überschreibt.

Der Reset-Handler selbst (`Q9_bootrom_reset_body`, 756 Byte, siehe
[`asm-68k.txt`](asm-68k.txt)) tut der Reihe nach:

1. **Echten System-Stack setzen**: `MOVEC VBR,SP` gefolgt von `MOVEA.L
   (SP),SP` — liest über die Vector-Base-Register-Adresse den eigenen
   Vektor 0 (`0x00004A00`) aus und setzt ihn als SSP. Ein cleverer Trick:
   die Boot-ROM-eigene Vektortabelle liefert dem Reset-Code seinen
   eigenen Stack.
2. **System-Global-Basisadresse laden** (`A5`, aus einer ROM-Konstante).
3. **Den kompletten System-Global-Bereich nullen** — eine `DBF`-Schleife,
   die in **16-Byte-Schritten** einen ca. 19,5-KByte-Bereich (`A5+0x9E0`
   bis `A5+0x5600`) auf Null setzt.

**Das ist die Antwort auf die Lücke, die diese ganze Serie bisher offen
gelassen hatte**: der System-Global-Bereich, den `docs/KERNEL.md` und
Thema 01 als beim Kernelstart bereits vorhanden voraussetzen, wird **hier
im Boot-ROM angelegt und genullt — nicht vom Kernel selbst**. Und die
16-Byte-Granularität (schon in Thema 01/`OWN_KERNEL_INIT_PLAN.md` Abschnitt
2 als auffällige, unabhängig gefundene Konstante dokumentiert) taucht
jetzt ein drittes Mal auf, diesmal eine Ebene *unter* dem Kernel.

Danach folgen zwei weitere, klar erkennbare Blöcke:

- **RAM-Größentest**: klassisches Schreib-/Lese-Verifizieren mit einem
  Muster und seiner Inversion (`0x5A5A5A5A`/`0xA5A5A5A5`) an mehreren
  Kandidatenadressen — Standardtechnik zur Speichergrößenermittlung ohne
  Konfigurationsdaten.
- **Modulketten-Scanner**: prüft an einer Kandidatenadresse das
  Sync-Wort `$4AFC` (`CMPI.W #0x4AFC,(A0)`) — **exakt dieselbe Konstante
  aus Thema 00**. Bei Erfolg folgt eine **24-Word-XOR-Prüfsumme über die
  Offsets `0x00`-`0x2F`**, die auf `0xFFFF` aufgehen muss — **identisch**
  zu dem bereits in `docs/REVERSE_ENGINEERING.md` dokumentierten
  Modul-Prüfsummenverfahren. Danach wird `A0` um den Wert bei
  Header-Offset `0x30` erhöht, plausibel um zum nächsten verketteten
  Modul im Bootfile zu springen (Init-Modul → Kernel → weitere
  Systemmodule, siehe `OS9Boot.noprot.test` als Beispiel einer solchen
  Kette) — die exakte Feldidentität an `0x30` wurde in dieser Runde nicht
  gegen die Doku verifiziert (könnte `M$Size` oder ein anderes Feld sein).

**Fehlerbehandlung, bemerkenswert simpel**: jeder Fehlerpfad (RAM-Test
fehlgeschlagen, Sync-Wort falsch, Prüfsumme falsch) läuft über eine
Sprungtabelle aus zehn `BRA.W`-Einträgen, die eine passende
Fehlermeldung installieren, dann **alle 16 Register auf den Stack retten,
eine Ausgabe-Routine aufrufen — und komplett zum Reset zurückspringen**
(`BRA.W 0xFE0005A6`). Kein selektives Recovery, kein Zurückfallen auf
ein zweites Bootgerät innerhalb dieser Funktion — ein fehlgeschlagener
Bootversuch bedeutet einen kompletten Neustart der ROM-Bootlogik.

## x86: `vectx86` — bisher unbekanntes Modul, bestätigt den `INT 0xFF`-Fund aus Thema 09 ein zweites Mal

Bei der Suche nach einer x86-Vergleichsseite fiel `modules/os9000-x86/vendor/vectx86`
auf — ein kleines Systm-Typ-Modul (Name `"vectors"`), das in der gesamten
bisherigen Session **nie untersucht** wurde. Es installiert über das
bekannte PIC-Trampolin-Muster (`CALL $+5`/`POP`/`LEA`, s. frühere Themen)
drei feste Modul-Adressen in Kernel-Globals-relative Felder
(`+0xAC0`/`+0xAC4`/`+0xAC8`, plus ein viertes bei `+0xAD0`) — konzeptionell
dieselbe Art Operation wie die in Thema 06 gefundenen PC-relativen
Konstanten-Installationen (dort bei `+0xA0C`..`+0xA98`), nur mit anderen
Offsets und in einem **eigenen, separaten Modul** statt im Kernel selbst.
Naheliegende Deutung (nicht abschließend verifiziert): `vectx86` ist ein
früher Bootstrap-Baustein, der Handler-Adressen bereitstellt, bevor oder
während `Q9X_kernel_globals_init` (Thema 01) läuft — ein bisher fehlendes
Puzzleteil zwischen "Boot-Vorkette" und "Kernel-Bootstrap" auf der
x86-Seite.

**Zweiter, wichtiger Fund**: `vectx86` benutzt für einen abschließenden
Aufruf exakt denselben Syscall-Wrapper-Code (`Callcode 0x1D`, Parameterblock-
Aufbau) und denselben `INT 0xFF`-Trampolin wie `scllio` in Thema 09 —
**byteidentisch** in Struktur und Callcode. Das bestätigt: `INT 0xFF` ist
**kein Einzelfall eines Treibers**, sondern ein modulweit verwendeter,
generischer Kernel-Systemaufruf-Mechanismus — ein zweiter, unabhängiger
Beleg für den in Thema 09 gefundenen x86-Syscall-Auslöser.

## Vergleichstabelle

| # | Frage | 68K | x86 |
|---|---|---|---|
| 1 | Wo entsteht der System-Global-Bereich? | **Im Boot-ROM**, vor jedem Kernel-Code — `DBF`-Schleife nullt ~19,5 KByte in 16-Byte-Schritten | Nicht in dieser Runde lokalisiert (vermutlich `Q9X_kernel_globals_init`, Thema 01 — aber das läuft laut Thema 01 bereits IM Kernel-Modul, nicht in einer separaten Vorkette-Komponente) |
| 2 | Wie wird das Bootgerät/die Bootdatei geprüft? | Sync-Wort (`$4AFC`) + 24-Word-XOR-Prüfsumme, **identisch zum bereits bekannten Modul-Prüfsummenverfahren** | Nicht identifiziert (kein Boot-Loader-artiges Modul mit vergleichbarer Rolle gefunden) |
| 3 | Speichergrößenermittlung | Klassischer Schreib-/Lese-Test mit invertiertem Muster | Nicht untersucht |
| 4 | Fehlerbehandlung bei Bootfehler | Kompletter Warmstart der ROM-Logik (`BRA.W` zum Reset-Einsprung), kein selektives Recovery | Nicht untersucht |
| 5 | Frühe Handler-/Adress-Installation in Kernel-Globals | Nicht Teil des Boot-ROM (passiert laut Thema 06 im Kernel selbst) | **`vectx86`** installiert vier feste Adressen in Kernel-Globals-Felder — eigenständiges Modul, nicht Teil des Kernels (neuer Fund) |
| 6 | Syscall-Auslöser, falls im Vorkette-Code verwendet | Nicht beobachtet (Boot-ROM nutzt keine `TRAP`-Instruktion in den untersuchten Bereichen) | **`INT 0xFF`**, identischer Wrapper-Code wie `scllio` (Thema 09) — zweite unabhängige Bestätigung |

## Für den eigenen Kernel (Ergänzung zu `docs/OWN_KERNEL_INIT_PLAN.md`)

Siehe dortige neue Sektion 2e — kurz zusammengefasst: ein eigener
Q9-Bootlader muss (a) den System-Global-Bereich **vor** dem eigentlichen
Kernel-Einsprung anlegen und nullen (nicht erst im Kernel selbst — das
68K-Vorbild zeigt, dass diese Verantwortung sauber eine Ebene tiefer
liegt), (b) Boot-Kandidaten über Sync-Wort + Prüfsumme validieren (exakt
dasselbe Verfahren wie beim Modul-Linken zur Laufzeit, kein separates
Format nötig), und (c) bei einem der grundlegenden Fehlerfälle (RAM-Test,
Bootfile-Validierung) lieber einfach und robust neu starten statt
komplexe Recovery-Pfade zu bauen — das 68K-Vorbild macht genau das.

## Offene Punkte

- 68K: die genaue Semantik von Header-Offset `0x30` im Modulketten-
  Scanner (Modulgröße? etwas anderes?) wurde nicht gegen die offizielle
  Doku verifiziert.
- 68K: die Bit-4-Prüfung bei `0xFE0005EA` und die daraus resultierende
  `SR`-Anpassung sowie die Adress-Delta-Berechnung um `0xFE0005F4`
  (ergibt `0x02000000` — evtl. eine zweite RAM-Basisadresse dieses
  Boards) wurden nicht abschließend gedeutet.
- 68K: die eigentliche Geräte-/CompactFlash-Suchlogik ("Now trying to
  boot from CompactFlash") wurde in dieser Runde nicht lokalisiert — nur
  die zugehörigen Strings gefunden, nicht der Code, der sie ausgibt.
- x86: kein Boot-ROM-Äquivalent gefunden/gesucht (BIOS/IPL liegt
  vermutlich außerhalb des gesamten `os9000-x86`-Modulfundus, in QEMUs
  eigener Firmware) — `vectx86` ist ein Kernel-naher Bootstrap-Baustein,
  keine Entsprechung zum 68K-ROM.
- x86: die genaue Rolle der vier von `vectx86` installierten Adressen
  (`+0xAC0`/`+0xAC4`/`+0xAC8`/`+0xAD0`) wurde nicht mit Thema 06s
  `+0xA0C`-`+0xA98`-Funden abgeglichen — möglicher Zusammenhang, nicht
  verifiziert.

## Quellen

- 68K: [`asm-68k.txt`](asm-68k.txt), `Q9-Flux/local_images/roms/romimage.dev.running.BIN` (lokal, nicht im Repo), `Q9-Flux/docs/OS9SYS_BOOT.md`, Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-bootrom/`, Skript `../../../modules/bootrom/ghidra_scripts/BootRomAnalyzeAndDump.java`
- x86: [`asm-x86.txt`](asm-x86.txt), `modules/os9000-x86/vendor/vectx86`, Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-vectx86/`, Skript `../../../modules/os9000-x86/ghidra_scripts/vectx86_AnalyzeAndDump.java`
- Vorherige Themen: [Thema 00](../00-modul-aufbau-und-header/) (Sync-Wort, Prüfsumme), [Thema 01](../01-kernel-bootstrap/) (16-Byte-Granularität, Kernel-Globals), [Thema 06](../06-exception-handler/) (Kernel-Globals-Adress-Installationen), [Thema 09](../09-treiber-hardware/) (`INT 0xFF`-Syscall-Mechanismus)

**Erstellt**: 2026-08-15
