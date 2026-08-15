# Thema 11: Programm-Modul-Laden — was `F$Load` wirklich tut

`docs/KERNEL.md` beschreibt `F$Fork`s ersten Schritt seit langem in Theorie:
"Modul lokalisieren/laden — erst im Speicher suchen, sonst von Mass-Storage
nachladen (Dateiname = Modulname)". Dieses Thema unterlegt das erstmals
mit echtem, disassembliertem Code: `F$Load` selbst — der Systemaufruf, den
`F$Fork`, `F$Link` (bei manchen Codepfaden) und explizite `F$Load`-Aufrufe
aus Programmen heraus alle gemeinsam nutzen.

**Ausgangspunkt**: die physische Zieladresse von `F$Load` ist bereits aus
einer früheren Session-Runde bekannt — `docs/REVERSE_ENGINEERING.md`s
vollständige Syscall-Tabelle listet `0x01` → `F$Load` → `0000e712` (in
beiden Dispatch-Tabellen identisch). Ein Abgleich mit den live vermessenen
Modul-Adressbereichen (`modules/SYSCALL_MODULE_MAP.md`) zeigt: diese
Adresse liegt **im IOMan-Modul**, nicht im Kernel — `F$Load` ist also ein
IOMan-Syscall, keine Kernel-Kernfunktion. Modulinterner Offset:
`0xE712 − 0xE03C` (IOMan-Modulbasis) `= 0x6D6`.

## Label-Konvention

- **68K**: `Q9_ioman_*`, bestehendes Ghidra-Projekt (aus Thema 02)
  `/Volumes/SSD1TB/projects/Q9-OS-ghidra-ioman/`, neue Skripte
  `../../../modules/ioman/ghidra_scripts/DumpFLoad.java` und
  `DumpFLoadCore.java`.
- **x86**: kein neuer Fund in dieser Runde — Querverweis auf den bereits
  in Thema 01/`modules/os9000-x86/docs/KERNEL_INIT.md` dokumentierten
  Modul-Scanner (siehe unten).

## 68K: `F$Load` in drei klaren Schritten

Aus [`asm-68k.txt`](asm-68k.txt), Einsprung bei `Q9_ioman_f_load`
(Offset `0x6D6`):

**1. Parameter validieren, Suchkontext aufbauen** (`0x706`): ein
0x4C-Byte-Rahmen auf dem Stack, Default-Typmaske und Default-Fehlercode
(`0x84`) gesetzt, Namenszeiger abgelegt — Standard-OS-9-Aufbaumuster,
identisch im Prinzip zu anderen Syscalls dieser Session.

**2. Erst im Speicher suchen** (`Q9_ioman_load_search_core` @ `0x124A`):
ruft eine Unterroutine bei `0x135E` auf, die über einen **internen
Syscall-Tabellen-Aufruf** (`(0x3A4,A6)`, Offset `0xC0`/`0x4C0` — passt zur
`F$CmpNam`-Position `0x11` aus `modules/SYSCALL_MODULE_MAP.md`) das
**in-memory Modulverzeichnis** durchsucht. Kopf des Verzeichnisses:
Kernel-Global `(0x48,A6)`. Bei Treffer: Link-Zähler erhöhen, fertig —
**kein Zugriff auf Mass-Storage nötig**.

**3. Namensauflösung mit Pfad-/Suchlisten-Unterscheidung**: bevor
überhaupt ein Gerät angesprochen wird, prüft der Code das **erste Zeichen
des Namens gegen `'/'`** (`CMPI.B #0x2F,(A0)`). Beginnt der Name mit `/`,
wird er als **vollständiger Pfad** behandelt (keine Suchliste). Sonst wird
je nach einem Flag-Bit eine von zwei **Kernel-Global-Suchlisten**
verwendet (`(0x148,A4)` oder `(0x158,A4)`) — die aus der OS-9-Praxis
bekannten "durchsuche `/dd/CMDS` etc. der Reihe nach"-Listen. Fehlt die
Suchliste komplett, folgt sofort Fehlercode `0xD7` ("Modul nicht
gefunden"-artig).

**4. Von Mass-Storage laden** (`BSR.W 0xB3A`): erst danach, wenn die
Speichersuche fehlschlägt, folgt der eigentliche Geräte-/Dateizugriff —
in dieser Runde nicht weiterverfolgt (das wäre die Datei-/RBF-Ebene, s.
Themen 08/09, eine andere Schicht als `F$Load` selbst).

**5. Neues Modul ins Verzeichnis einhängen**: nach erfolgreichem Laden
wird der neue Verzeichniseintrag klassisch als **Linked-List-Kopf**
eingehängt (`(0xA,A0)`-Kette, laut `docs/KERNEL.md` die "Colored-Memory-
Listen"), und der Modulname wird aus dem geladenen Modul (Offset `0x46`
relativ zum Modulanfang, bis zu 128 Byte) in den Verzeichniseintrag
kopiert — damit künftige `F$CmpNam`-Suchen (Schritt 2) das frisch
geladene Modul finden, ohne erneut auf Mass-Storage zuzugreifen.

## x86: kein eigenständiger Fund — Querverweis auf den Boot-Zeit-Modul-Scanner

In dieser Runde wurde **keine neue x86-Disassemblierung** durchgeführt.
Grund: `modules/os9000-x86/docs/KERNEL_INIT.md` dokumentiert bereits (aus
Thema 01) einen **Modul-Scanner** (`FUN_0021eb26`, Teil von
`Q9X_kernel_globals_init`), der beim Boot den Speicher nach Modulen
absucht (Sync-Byte-Muster, Prüfsummen-Check, bei Namenskollision die
höchste Revision behält) — das ist konzeptionell näher an "Module beim
Systemstart finden" als an einem laufzeitfähigen, per Syscall aufrufbaren
`F$Load`-Äquivalent. Ob IOMan/x86 einen eigenen, separaten `F$Load`-
Aufrufpfad hat (analog zum 68K-Befund oben), wurde nicht untersucht — laut
Thema 02 sind ohnehin erst 16 von 67 x86-IOMan-Funktionen benannt, eine
gezielte Suche nach dem `F$Load`-Callcode-Slot in der x86-Dispatch-Tabelle
wäre der nächste Schritt.

## Vergleichstabelle

| # | Frage | 68K | x86 |
|---|---|---|---|
| 1 | Wo liegt `F$Load`? | **IOMan-Modul** (nicht Kernel) — über Adressvergleich bestätigt | Nicht identifiziert |
| 2 | Speicher zuerst durchsuchen? | **Ja** — über internen `F$CmpNam`-artigen Aufruf, In-Memory-Modulverzeichnis ab Kernel-Global `0x48` | Boot-Zeit-Scanner bekannt (Thema 01), laufzeitfähiges Äquivalent nicht gesucht |
| 3 | Pfad- vs. Namenssuche unterschieden? | **Ja** — erstes Zeichen `'/'` entscheidet, sonst eine von zwei Kernel-Global-Suchlisten | Nicht untersucht |
| 4 | Modulverzeichnis-Struktur | Verkettete Liste, Kopf pro Typ, Name in den Eintrag kopiert (bis 128 Byte) | Nicht untersucht |
| 5 | Fehlercode bei "nicht gefunden" | `0xD7` (Suchliste fehlt) bzw. `0x84`/`0xCD`/`0xCB` je nach Fehlerursache | Nicht untersucht |

## Für den eigenen Kernel (Ergänzung zu `docs/OWN_KERNEL_INIT_PLAN.md`)

Siehe dortige neue Sektion 3c — kurz zusammengefasst: das 68K-Muster
"erst In-Memory-Verzeichnis durchsuchen (per Namensvergleich), dann erst
Mass-Storage" ist eine klare Übernahme-Empfehlung — spart bei mehrfach
genutzten Modulen (z. B. Compiler-Läufen, mehrfachem Programmstart)
wiederholte Plattenzugriffe komplett. Die Pfad-vs-Suchlisten-Unterscheidung
(`'/'`-Test) ist ein einfaches, gut verständliches Muster für den eigenen
Namensauflösungs-Algorithmus.

## Offene Punkte

- 68K: der eigentliche Geräte-/Dateizugriff bei `0xB3A` wurde nicht
  disassembliert (andere Schicht, s. o.).
- 68K: die genaue Bedeutung der beiden Kernel-Global-Suchlisten
  (`0x148`/`0x158`, ausgewählt über ein Flag-Bit) — vermutlich
  Execution- vs. Data-Directory-Liste nach OS-9-Konvention, nicht gegen
  die offizielle Doku verifiziert.
- x86: kein eigenständiger `F$Load`-Fund in dieser Runde — offen für eine
  Vertiefungsrunde, die gezielt nach dem Callcode-`0x01`-Slot in der
  x86-IOMan-Dispatch-Tabelle sucht (analog zum 68K-Vorgehen hier).

## Quellen

- 68K: [`asm-68k.txt`](asm-68k.txt), `docs/REVERSE_ENGINEERING.md` (Syscall-Tabelle), `modules/SYSCALL_MODULE_MAP.md` (Modul-Zuordnung), Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-ioman/`, Skripte `../../../modules/ioman/ghidra_scripts/DumpFLoad.java`, `DumpFLoadCore.java`
- x86: `modules/os9000-x86/docs/KERNEL_INIT.md` (Modul-Scanner, aus Thema 01)
- Vorherige Themen: [Thema 00](../00-modul-aufbau-und-header/) (Modulheader-Felder), [Thema 01](../01-kernel-bootstrap/) (Kernel-Globals-Basis, x86-Modul-Scanner), [Thema 02](../02-io-manager-syscall-dispatch/) (Syscall-Tabellen `D_SysDis`/`D_UsrDis`), [Thema 06](../06-exception-handler/) (Preemption-/Signal-Check-Muster)

**Erstellt**: 2026-08-15
