# IOMan-Disassemblierung — Arbeitsstand (Runde 1)

Ziel: wie beim Kernel-Modul (siehe `../../kernel/docs/REVERSE_ENGINEERING.md`
bzw. das historische `../../../docs/REVERSE_ENGINEERING.md`) — byte-exakte
Rekonstruktion als eigenständig assemblierbaren Quellcode. Konkreter Anlass:
`docs/SYSCALL_MODULE_MAP.md` (Ergebnis dieser Runde) zeigt per Adressvergleich,
welche `I$`/`F$`-Aufrufe außerhalb des Kernel-Moduls landen — der größte Teil
davon in IOMan. Diese Runde verifiziert das gewählte Binary und macht den
ersten Disassemblierungsschritt (Modulkopf, Einsprungpunkt, Grundgerüst);
**kein** vollständiger Nachbau wie beim Kernel (das war ein Mehrsitzungs-
Projekt mit ~2270 Dokumentationszeilen — realistischer nächster Schritt,
nicht in einem Rutsch machbar).

## Untersuchtes Modul

[`../vendor/ioman_DEV`](../vendor/ioman_DEV) — Development-IOMan, aus
`MWOS/OS9/68000/CMDS/BOOTOBJS/ioman_DEV`. Bestätigt als die vom
CB030-Bootfile tatsächlich geladene Variante:
`MWOS/OS9/68030/PORTS/CB030/BOOTFILE/dev.bl` referenziert wörtlich
`MWOS OS9/68000/CMDS/BOOTOBJS/ioman_DEV` (Kommentar dort: "Development
IOMAN"). IOMan ist — anders als der Kernel — **nicht CPU-familienspezifisch**
verzweigt (kein `aker*`/`dker*`-Äquivalent pro CPU-Familie im SDK), passend
dazu, dass IOMan keine MMU-/Exception-Vektor-Details des jeweiligen
Prozessors kennen muss.

**Unabhängige Bestätigung, dass dies exakt das live gebootete Modul ist:**
Datei ist 5660 Byte groß; die Live-Vermessung in
`Q9-Forge/Q9-Flux/docs/OS9_SYSCALL_OWNERSHIP.md` (Abschnitt 1, `mdir -e` auf
einem laufenden Boot-Image) ergab den IOMan-Adressbereich `$00E03C`–`$00F658`
= exakt `0x161C` = **5660 Byte**. Zusätzlich bestätigt `M$Size` im
Modulkopf selbst denselben Wert (`0x0000161C`) — drei unabhängige Quellen
(Dateigröße, Live-Speicherbereich, Modulkopf-Feld) stimmen exakt überein.

## Werkzeug

Ghidra 12.1.2 (Homebrew, `/opt/homebrew/opt/ghidra`), headless über
`analyzeHeadless` (`JAVA_HOME` muss explizit auf
`/opt/homebrew/opt/openjdk@21/...` gesetzt werden — das System-`java` findet
sonst keine JDK, siehe `Get Started`-Hinweis unten). Prozessor
`68000:BE:32:default` (generisches 68000 — IOMan braucht anders als der
Kernel keine 68030-Spezifika), Raw-Binary-Import bei Basisadresse 0.

**Ghidra-Projekt liegt NICHT im Git-Repo** (analog zum Kernel-Projekt) —
lokal unter `/Volumes/SSD1TB/projects/Q9-OS-ghidra-ioman/`. Die verwendeten
Skripte liegen dagegen in [`../ghidra_scripts/`](../ghidra_scripts/)
(kleine Textdateien, unproblematisch fürs Repo).

## Modulkopf

Parity-Check bestanden (XOR aller 24 Words `0x00`–`0x2E` ergibt `0xFFFF`,
wie beim Kernel) — bestätigt die Standard-Header-Grenze `0x00`–`0x2F`.

| Offset | Wert | Interpretation |
|---|---|---|
| `0x00` | `0x4AFC` | M$ID (Sync) |
| `0x02` | `0x0001` | M$SysRev |
| `0x04` | `0x0000161C` (5660) | M$Size — deckt sich mit Dateigröße und Live-Messung |
| `0x08` | `0x00000000` | M$Owner |
| `0x12` | `0x0C01` | M$Type=`0x0C`(12, „Systm") / M$Lang=`0x01`(1, „Objct"/68k) — deckt sich mit dem beim Kernel bestätigten Muster |
| `0x14` | `0xA000` | M$Attr=`0xA0` (system-state + reentrant, nicht sticky) / M$Revs=`0x00` |
| `0x2C` | `0x00001C7D` | vermutlich Parity/CRC-Feld, wie beim Kernel — noch nicht im Detail verifiziert |
| `0x30` | `0x00000098` | M$Exec (Einsprungpunkt-Offset) — analog zum Kernel, wo dieselbe Position dessen `M$Exec` trug |

**Offen/nicht abschließend verifiziert:** die exakten Feldnamen zwischen
`0x0A` und `0x12` (erster Parse-Versuch hatte hier eine Verschiebung, durch
Abgleich mit den beim Kernel bestätigten Typ/Lang/Attr-Werten korrigiert,
aber noch nicht Feld für Feld gegen eine Primärquelle nachgewiesen — bei
Bedarf in einer Folgerunde klären). Eingebetteter Identifikationsstring ab
ca. Offset `0x4C`: `"OS-9/68K IOMan (Dev) V3.1.0\0Copyri..."` (analog zum
Kernel-Idiom, ID-String direkt nach dem Header/Sprung über ihn hinweg).

## Einsprungpunkt und erste Analyse

Disassemblierung ab `M$Exec` = Offset `0x98` ergab eine erste, saubere
234-Byte-Funktion ohne Disassemblierungsfehler — Indiz, dass die
Einsprungpunkt-Interpretation (gleiche Konvention wie beim Kernel) korrekt
ist. Nach Ghidras automatischer Analyse ab diesem Einsprungpunkt:

- **17 Funktionen** gefunden (Startpunkt + automatische Funktionserkennung)
- **14,4 %** des 5660-Byte-Moduls als Code disassembliert (816 Byte)
- **7,0 %** als Daten (394 Byte)
- **78,6 %** noch undefiniert (4450 Byte) — zum Vergleich: beim Kernel lag
  der Ausgangswert nach dem reinen Einsprungpunkt-Schritt bei 55 % Code,
  IOMan braucht hier vermutlich mehrere weitere Runden (Trap-/Dispatch-
  Tabellen-Suche wie beim Kernel, s. dortiger Abschnitt "Trap-/Exception-
  Tabellen-Initialisierung"), um vergleichbar hoch zu kommen.

Funktionsliste (automatisch benannt, `FUN_<offset>`, noch keine inhaltliche
Zuordnung — das ist der nächste Schritt):

```
ioman_entry_98 @ 0x0098  size=234
FUN_00000178   @ 0x0178  size=40
FUN_00000296   @ 0x0296  size=6
FUN_0000029c   @ 0x029c  size=16
FUN_000002ac   @ 0x02ac  size=6
FUN_000002b2   @ 0x02b2  size=6
FUN_000002b8   @ 0x02b8  size=20
FUN_000002cc   @ 0x02cc  size=8
FUN_000002d4   @ 0x02d4  size=18
FUN_000002e6   @ 0x02e6  size=10
FUN_000002f8   @ 0x02f8  size=26
FUN_00000312   @ 0x0312  size=10
FUN_00000d56   @ 0x0d56  size=20
FUN_0000107a   @ 0x107a  size=42
FUN_000010d8   @ 0x10d8  size=32
FUN_000014f8   @ 0x14f8  size=180
FUN_000015ca   @ 0x15ca  size=28
```

Die Adress-Cluster (`0x178`–`0x312` dicht beieinander, dann eine Lücke bis
`0xd56`, dann `0x107a`–`0x15ca`) erinnern an das beim Kernel beobachtete
Muster kleiner, dicht gepackter Hilfsfunktionen nahe dem Einsprungpunkt plus
größerer Funktionen weiter hinten — noch keine belastbare Interpretation,
nur eine Beobachtung für die nächste Runde.

## Bezug zur Syscall-Modul-Zuordnung

Der eigentliche Auslöser dieser Untersuchung — siehe
[`../../SYSCALL_MODULE_MAP.md`](../../SYSCALL_MODULE_MAP.md) — ist bereits
**ohne** weitere Disassemblierung beantwortet: die Live-Adressbereiche aus
`OS9_SYSCALL_OWNERSHIP.md` und die vollständige `D_SysDis`/`D_UsrDis`-Tabelle
aus dem Kernel-Fund wurden per Adressvergleich gekreuzt (reine Arithmetik,
kein Ghidra nötig) und ergeben für **alle** ~90 im Kernel-Dump sichtbaren
Codes eine Modul-Zuordnung. Das beantwortet "wo landet welcher Syscall"
vollständig. Was diese IOMan-Disassemblierung zusätzlich liefern soll (und
in Runde 1 noch nicht liefert): **wie** IOMan diese ~24 Aufrufe intern
organisiert — eigene Datei-Manager-/Treiber-Dispatch-Tabelle, Pfad-
Deskriptor-Handling usw. Das ist eigenständig interessant (Architekturbild),
aber nicht mehr nötig, um die ursprüngliche Frage zu beantworten.

## Nächste Schritte (falls gewünscht)

1. Die 17 gefundenen Funktionen einzeln lesen und benennen (analog zum
   Kernel-Vorgehen), beginnend mit `ioman_entry_98`.
2. Trap-/Dispatch-Tabellen-Suche analog zum Kernel (`FindTrapInit.java`-
   Muster aus `../../kernel`-Skripten als Vorlage) — IOMan braucht vermutlich
   eine interne Verzweigung von "welcher I$-Callcode" zu "welcher
   Datei-Manager/Treiber", analog zur `D_SysDis`/`D_UsrDis`-Zwei-Tabellen-
   Struktur des Kernels.
3. Undefinierte Bereiche (78,6 %) systematisch schließen (Kontrollfluss-
   Verfolgung von den 17 bekannten Funktionen aus, wie beim Kernel).
4. Modulkopf-Feldnamen zwischen `0x0A`–`0x12` gegen eine Primärquelle
   absichern (aktuell nur per Analogieschluss zum Kernel-Kopf bestimmt).

**Erstellt**: 2026-08-12
