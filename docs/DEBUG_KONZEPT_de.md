# Debug-Konzept für den Q9-Kernel

Stand: 2026-10-06. Abgestimmt mit Andreas; noch nicht umgesetzt.
Ergänzt am selben Tag: Zeit bei Eintritt und Ende (2.2), Ringpuffer als
allgemeiner Baustein (2.8).

Ziel: Fehler im Kernel und im Zusammenspiel mit fremden Modulen (IOMan,
RBF, SCF, csl, mshell …) **messen** statt erraten. Die Sitzungen bis
Fortsetzung 111 haben gezeigt, was fehlte: Fast alle Funde kamen über
Ausgaben, die jedes Mal neu in eine Wegwerfkopie des Kernels eingebaut
werden mussten (Syscall-Folge, Register am Ein- und Ausgang, Werte beim
Stacküberlauf). Dieses Konzept macht daraus feste, abschaltbare Werkzeuge.

## 1. Kernelvarianten

| Variante | Inhalt |
|---|---|
| **Developer** | alle Debug-Maßnahmen dieses Dokuments, Testcode (TestProcA & Co.), Diagnosen |
| **Atom** | nur der Kernel selbst; kein Debug-Code, keine Testprozesse, keine Laufzeitkosten |

Ein einziger Schalter in `build.sh` (heute schon
`Q9K_KERNEL_VARIANT=development|atomic`) steuert beide Sprachen:

- **C:** `-dQ9K_DEBUG` (zusätzlich zum bestehenden
  `-dQ9K_KERNEL_DEVELOPMENT`), im Code `#ifdef Q9K_DEBUG`.
- **Assembler:** `r68` kennt kein `#ifdef`, aber `ifne`/`ifeq` auf
  Symbole. `build.sh` setzt das Symbol von außen (`r68 -a=Q9K_DEBUG`, wie
  heute schon `-a=_UCC`); im Code `ifdef Q9K_DEBUG` bzw. `ifne Q9K_DEBUG`
  … `endc`.

Unter den Schalter wandert auch, was heute fest im Kernel steckt:
Testprozesse und Marker, RaceRing, Rahmenvalidierung im Timer-IRQ,
Speicherspur, die verstreuten `*-Trace`-Zellen.

## 2. Syscall-Trace

### 2.1 Was aufgezeichnet wird

| Ereignis | Wann | Inhalt |
|---|---|---|
| **Eintritt** | Dispatcher, vor dem Handler | Callcode, Prozess, Benutzer, Tiefe, Parameter (je Syscall, s. 2.3) |
| **Rückkehr** | Dispatcher, nach dem Handler | Ergebnis (Carry, Fehlercode), Ausgabewerte |
| **Intern-Eintritt / -Rückkehr** | Makro `Q9K_TRACE_FN` in Kernfunktionen | Funktions-ID, bis zu zwei Werte |

**Tiefe:** Verschachtelte Aufrufe (IOMans eigene Syscalls innerhalb von
`F$Load`, das Nachladen in `F$Fork`, interne Funktionen) bekommen die
Tiefe mit und werden in der Anzeige eingerückt:

```
P07 alice  F$Fork   "echo" mem=0 par=26
P07 alice    > Q9K_ProcFork          name="echo"
P07 alice    > Q9K_ModDirLinkByName  -> nicht gefunden
P07 alice    F$Load "echo" mode=Exec
P07 alice      I$Open "echo" mode=Exec            -> p5 fd=dd:$0142
P07 alice      F$SRqMem 3216                      -> ok
P07 alice      I$Close p5                         -> ok
P07 alice    F$Load                               -> ok mod=$71090
P07 alice  F$Fork                                 -> ok pid=8
```

Der Konfigurations-Syscall selbst (2.5) wird nie aufgezeichnet.

### 2.2 Kompaktes Speicherformat

Im Kernel wird **binär und knapp** gespeichert; erst das Tool expandiert
und formatiert. Sätze variabler Länge, big-endian, 2-Byte-ausgerichtet:

| Offset | Größe | Feld |
|---|---|---|
| +0 | 1 | Satztyp: 1 Eintritt, 2 Rückkehr, 3 intern-Eintritt, 4 intern-Rückkehr, 5 verlorene Sätze, 6 Prozess-Info, 7 Zeitbasis |
| +1 | 1 | Satzlänge in Byte (einschließlich Kopf) |
| +2 | 1 | Callcode bzw. Funktions-ID |
| +3 | 1 | Bits 0–3 Tiefe, Bit 4 Carry (Rückkehr), Bit 5 Satz gekürzt |
| +4 | 2 | Prozess-ID |
| +6 | 4 | Tick-Zähler seit dem Boot (voll, 32 Bit) |
| +10 | 2 | Feinzeit innerhalb des Ticks (Zählerstand des Hardware-Timers) |
| +12 | n | Nutzdaten nach Satztyp und Syscall-Beschreibung (s. 2.3) |

**Zeit bei Eintritt und Ende:** Jeder Eintritts- und jeder Rückkehrsatz
trägt den vollen Tick-Zähler und die Feinzeit. Bei 100 Ticks pro Sekunde
reicht der 32-Bit-Zähler für über 497 Tage; die Feinzeit (Restzählerstand
des Timers) löst innerhalb eines Ticks auf. Daraus zeigt das Tool die
**Uhrzeit** von Eintritt und Ende und die **Dauer** jedes Aufrufs:

```
10:42:17.315  P07  F$Fork "echo" mem=0 par=26
10:42:17.341  P07  F$Fork -> ok pid=8                    (26,1 ms)
```

Für die Uhrzeit schreibt der Kernel beim Einschalten des Trace (und bei
`F$STime`) einen **Zeitbasis-Satz** (Typ 7): Datum/Uhrzeit von `F$Time`
plus Tick-Zähler und Feinzeit im selben Moment. Das Tool rechnet jede
andere Tick-Angabe relativ dazu um; die einzelnen Sätze bleiben dadurch
kompakt.

- **Benutzer** (Gruppe.Benutzer) steht nicht in jedem Satz. Ein Satz Typ 6
  wird geschrieben, wenn ein Prozess zum ersten Mal auftaucht und bei
  `F$Fork`; das Tool merkt sich die Zuordnung.
- **Namen** (Pfade, Module) werden mit bis zu 15 Zeichen kopiert; der
  Zeiger allein wäre später wertlos. Abgeschnitten: Bit 5.
- **Ringpuffer:** der Trace benutzt den allgemeinen Ringpuffer-Baustein
  aus Abschnitt 2.8; feste Größe im Developer-Kernel (Vorschlag 64 KByte),
  Modus *überschreiben* (immer die neuesten Sätze) oder *anhalten wenn
  voll* (Beginn erhalten). Gehen Sätze verloren, wird ein Typ-5-Satz mit
  der Anzahl eingefügt.

### 2.3 Syscall-Beschreibungen

Eine Tabelle legt je Callcode fest, welche Register Ein- und Ausgaben
sind und wie sie gelesen werden (Zahl, Pfadnummer, Name, Größe, Modus,
Zeiger). Daraus folgt, was in den Satz geschrieben wird. Die Tabelle
existiert zweimal aus **einer** Quelle: kompakt im Kernel (was speichern)
und ausführlich im Tool (wie anzeigen, inklusive Fehlernamen wie
`E$MNF`). Quelle wird eine Textdatei im Repo, aus der beide erzeugt werden.

### 2.4 Filter

Alle Filter wirken vor dem Speichern (der Puffer bleibt für das
Wesentliche frei) und lassen sich kombinieren:

| Filter | Form |
|---|---|
| **Syscalls** | Bitmaske über alle **256** Callcodes, jeder einzeln an/aus |
| **Interne Funktionen** | eigene Bitmaske über die Funktions-IDs |
| **Prozess** | Prozess-ID oder 0 = alle |
| **Benutzer** | Gruppe.Benutzer oder 0 = alle |
| **Pfad** | prozesslokale Pfadnummer |
| **Datei** | Datei-ID (s. u.) |
| **Nur Fehler** | nur Rückkehr mit gesetztem Carry (samt zugehörigem Eintritt) |
| **Detailstufe** | 1: Code/Prozess/Ergebnis, 2: + Parameter, 3: + interne Funktionen |

**Datei-ID:** Gerät + Sektornummer des Dateideskriptors (FD-LSN) — die
OS-9-Entsprechung einer Inode, eindeutig und kompakt. Gespeichert und
gefiltert wird nur diese Zahl. Den Namen braucht nur das Tool:
`trace filter -f=/dd/SYS/startup` öffnet die Datei, fragt die FD-Nummer per
`I$GetStt` ab und setzt damit den Filter. Für die Anzeige verknüpft das
Tool die `I$Open`-Sätze (Name → Pfad → Datei-ID). Pfade ohne Dateisystem
(Konsole, native Pfade) haben keine Datei-ID; für sie gilt der Pfadfilter.

### 2.5 Konfigurations-Syscall `F$Q9Dbg`

Ein einziger neuer Callcode mit Unterfunktionen, Vorschlag **`$7F`**.
Microware belegt `F$` bis `$70` (`F$HLProto`; `$68`–`$6F` reserviert),
`I$` beginnt bei `$80`; `$71`–`$7F` sind in MWOS nirgends vergeben. `$7F`
liegt am weitesten von künftigen Microware-Codes entfernt.

| `d0.w` | Unterfunktion | Eingabe |
|---|---|---|
| 0 | Version/Fähigkeiten abfragen | – (Ausgabe: Version, Puffergröße, Satzzähler) |
| 1 | Trace an/aus | `d1` = 0/1 |
| 2 | Syscall-Maske setzen | `a0` = 32 Byte Maske |
| 3 | Syscall-Maske lesen | `a0` = Ziel, 32 Byte |
| 4 | Filter setzen | `d1` = Filterart, `d2` = Wert |
| 5 | Detailstufe/Modus setzen | `d1` = Stufe, `d2` = Pufferausgabe/Konsole/beides, Überschreiben/Anhalten |
| 6 | Ringpuffer lesen | `d3` = Puffernummer (Trace = 0), `d1` = ab Satznummer, `d2` = max. Byte, `a0` = Ziel (Ausgabe: gelesene Byte, nächste Satznummer, verlorene Sätze) |
| 7 | Ringpuffer leeren | `d3` = Puffernummer |
| 8 | Interne-Funktionen-Maske setzen | `a0` = Maske |

Im **Atom-Kernel** ist der Callcode nicht registriert (`E$UnkSvc`).

**Makros:** ein Assembler-Include (`q9dbg.d`, etwa `Q9DBG TRACE_ON` statt
von Hand `d0` zu setzen und `OS9 F$Q9Dbg`) und ein C-Header (`q9dbg.h`)
mit kleinen Funktionen (`q9dbg_on()`, `q9dbg_mask_set(...)`,
`q9dbg_filter(Q9DBG_PID, 7)` …).

### 2.6 Ausgabe

- **Ringpuffer** (Standard): kompakt, ändert das Zeitverhalten kaum.
- **Konsole** (zuschaltbar, auch beides): eine Kurzzeile je Satz direkt auf
  die DUART — sofort sichtbar beim Debuggen, mischt sich aber mit der
  Programmausgabe und verlangsamt.
- **Auslesen** über `F$Q9Dbg` Unterfunktion 6. Später optional ein Gerät
  `/trace` darauf aufbauend (`list /trace`), sobald IOMan sicher läuft.
  Der Emulator kann den Puffer zusätzlich direkt aus dem Speicher lesen —
  auch nach einem Absturz, wenn kein OS-9-Programm mehr laufen kann.

### 2.7 Werkzeuge

> **Stand 2026-10-06 (Fortsetzung 121):** umgesetzt sind das OS-9-Kommando
> `trace [on|off|clear]` (Assembler, `Q9-KERNEL/68k/src/kernel/trace.a`) und
> der Host-Dekoder `tools/q9trace_decode.py` fuer Q9-Flux-Dumps.

- **`trace`** (OS-9-Programm): `trace on|off`, `trace mask +I$Open -F$SRqMem`,
  `trace filter -p=7 -u=0.5 -f=/dd/SYS/startup -e`, `trace show`,
  `trace clear`. Läuft auch aus einer `startup`-Datei.
- **Host-Tool** (Python, im Repo): dekodiert den Puffer aus einem
  Emulator-Dump oder einer gespeicherten Datei mit derselben
  Beschreibungstabelle.

### 2.8 Ringpuffer als allgemeiner Baustein

Der Trace-Puffer ist nur der erste von mehreren Ringpuffern. Später sollen
weitere dazukommen und auch als **interne Pipes** dienen, etwa ein
**System-Ereignisprotokoll** (Systemmeldungen, Treiberfehler,
Speicherengpässe), das ein Programm liest und weiterverarbeitet. Damit das
später ohne Umbau geht, wird der Ringpuffer **von Anfang an als
eigenständiger Baustein** gebaut und der Trace als dessen erste Instanz:

| Eigenschaft | Bedeutung |
|---|---|
| Kennung | Nummer und Name (z. B. `0 = trace`, später `syslog`) |
| Größe | in Byte, beim Anlegen festgelegt |
| Satzorientiert | Sätze variabler Länge mit Längenfeld, nie halbe Sätze lesen |
| Modus | überschreiben (neueste behalten) oder anhalten wenn voll (älteste behalten) |
| Zähler | geschriebene Sätze, verlorene Sätze, Lesestand |

**Jetzt (Paket 2) nur einfach:** ein einziger, im Kernel fest angelegter
Puffer für den Trace — aber schon mit diesem Beschreibungsblock, sodass
Lesen/Leeren/Modus über die Puffernummer laufen (`F$Q9Dbg` Unterfunktionen
6/7 bekommen dafür `d3` = Puffernummer, Trace = 0).

**Später** (bewusst zurückgestellt):
- Ringpuffer zur Laufzeit anlegen und freigeben (eigener Syscall oder
  Unterfunktionen), mehrere Leser mit eigenem Lesestand.
- Anlegen schon beim Booten über einen Eintrag im `init`-Modul (Liste:
  Name, Größe, Modus), z. B. für das System-Ereignisprotokoll.
- Lesen und Schreiben aus normalen Programmen als **interne Pipe** — über
  einen kleinen Gerätetreiber/File-Manager, sodass `list /syslog` oder ein
  Pipe-Aufruf funktioniert, sobald IOMan sicher läuft.
- Wartende Leser (blockierendes Lesen bis neue Sätze da sind).

## 3. Register-Erhaltungsprüfung

Nur Developer-Kernel. Bei jedem Syscall merkt sich der Dispatcher die
Register, die der Aufrufer unverändert zurückbekommen muss (`d2`–`d7`,
`a2`–`a6`, abzüglich der laut Beschreibungstabelle als Ausgabe
deklarierten), und vergleicht sie bei der Rückkehr. Abweichung → eigener
Trace-Satz und Konsolenmeldung, z. B. `F$Fork hat d7 veraendert:
$5D2BA -> $2022`.

Der Anlass: Der `d7`-Fehler im Dispatcher (Fortsetzung 111) hat jeden
Syscall jedes Programms betroffen und blieb lange unentdeckt. Diese
Prüfung hätte ihn beim ersten Syscall gemeldet.

Ergänzend: `a6` vor C-Aufrufen prüfen (zeigt `a6` auf
`Q9K_CRuntimeData`?) — der Fehler in `F$Wait` (Fortsetzung 111) wäre
damit ebenfalls sofort aufgefallen.

## 4. Weitere Prüfungen im Developer-Kernel

- **Stack-Wächter je Prozess:** Kanarienwerte am Stackende, Höchststand
  der Stacknutzung, Meldung bei Überschreitung.
- **Invarianten** (`Q9K_ASSERT`): Warteschlangen, Pfad-Pool,
  Link-Zähler, Moduldirectory.
- **Fehlerbericht mit Symbolen:** bei einer Exception im Dump
  „in `Q9K_WaitQInsert`+`$18`“ statt nur einer Adresse (Symbole aus der
  Linkmap, s. 5).
- **`make check`:** alle Regressionsskripte und Hosttests in einem Aufruf.

## 5. Debugger im Emulator (Q9-Flux)

Eigenes Paket, nach dem Trace. Stand der Planung: 2026-10-06
(Fortsetzung 121), Messwerte in `docs/OWN_KERNEL_STATUS.md`.

### 5.1 Ziele

1. Jede Adresse in Spur, Dump und Debugger **symbolisch** anzeigen
   (`Q9K_IRQDispatch+$E6`, `ioman+$1560`) statt roh.
2. Haltepunkte, Einzelschritt, Register und Speicher im laufenden Emulator.
3. **Quelltext-Debugging:** Adresse <-> `datei.c:Zeile`, Haltepunkt auf eine
   Zeile, Einzelschritt pro Zeile.
4. OS-9-Sichten: Prozesse, Moduldirectory, Pfade, Trace-Puffer.

### 5.2 Symbole

- **Kernel:** `l68 -s=<datei>` liefert `Name COD Offset` fuer alle globalen
  Symbole (auch Assembler-Labels). Laufzeitadresse = Ladeadresse des Kernels
  (heute $7100, aus der Bootliste) + Offset. Bereits erprobt
  (Absturzanalysen in Fortsetzung 113-120). Luecke: `static`-Funktionen
  fehlen in der Karte und werden dem vorangehenden Symbol zugeschlagen.
- **Eigene Module** (forkchild, trace, spaeter QCC-Programme): ebenfalls
  Linkkarte beim Bauen mitschreiben.
- **Microware-Module** (ioman, scf, rbf, sysgo ...): MWOS enthaelt
  `*.stb`/`*.map` (z. B. `OS9/68000/CMDS/BOOTOBJS/STB/`), selbst als
  OS-9-Modul verpackt (`4AFC`-Kopf); Format noch zu entschluesseln. Ohne
  Symbole bleibt `modul+offset` -- auch das ist schon viel wert.
- **Laufzeit-Zuordnung:** Q9-Flux liest das Moduldirectory des Kernels
  (`Q9K_MODDIR_HEAD_ADDR` $1238, Eintraege mit Kopfadresse/Groesse/Name --
  der Dump zeigt das heute schon) und legt die Symbole jedes Moduls an seine
  tatsaechliche Ladeadresse. Geladene Module (F$Load) werden so automatisch
  erfasst.
- **Format im Emulator:** eine einfache Textdatei je Modul
  (`modulname offset name`), vom Build erzeugt. Fuer GDB zusaetzlich als
  ELF-Symboldatei exportierbar.

### 5.3 Quelltext-Debugging (gemessen)

- `xcc -g` reicht `--add_debug_info` an den Compiler und `-g` an `be68k`
  und laesst **beide Optimierer weg** (`iopt`, `opt68k`). Der Code wird
  dadurch groesser und anders (q9kernel_date.c: $610 -> $8B0 Byte,
  Funktionen verschoben). Das ist gewollt -- im unoptimierten Code
  entspricht jede Zeile einem Codestueck, Einzelschritt und Variablen sind
  nachvollziehbar (uebliche Praxis bei Debug-Builds).
- Die Zeileninformation steht in einem eigenen **Debug-Abschnitt** der
  ROF-Objektdatei (`rdump` zeigt nur seine Groesse, `Debug: $AB8`). Format
  noch zu entschluesseln; Hinweise: Microwares Quelltext-Debugger
  (`mwsrcdbg.dll`, `hawkdbg.dll` in MWOS/DOS/BIN).
- **Ganzer Kernel mit `-g` geht nicht:** ~40 % mehr Code, 27 `bsr`-Sprunge
  (68000: nur +-32 KB) erreichen ihr Ziel nicht mehr. Deshalb **pro Datei**:
  Bauschalter `Q9K_SRCDEBUG="q9kernel_date.c ..."` uebersetzt nur diese
  Dateien unoptimiert mit `-g` (umgesetzt, build.sh). Mit einer kleinen
  Datei baut und bootet der Kernel bis zum Login; schon zwei Dateien
  (date+sched) sprengen wieder zwei Sprunge -- der Kernel sitzt an der
  Grenze.
- **Strukturelle Loesung der Reichweite:** ein Fernaufruf-Makro statt
  `bsr` fuer Aufrufe aus q9kernel_entry.a in C, PC-relativ ueber eine
  Label-Differenz (`lea *(pc),a0 / adda.l #Ziel-*,a0 / jsr (a0)`), die der
  Linker als Konstante aufloest -- falls l68 das kann (zu pruefen). Das
  beseitigt die 32-KB-Grenze allgemein, nicht nur fuer Debug-Builds.
- **QCC:** eigener Weg mit frei waehlbarem Format -- `LINE datei zeile` im
  IR, das Backend setzt Labels und schreibt eine Zeilentabelle neben das
  Modul. Gehoert ins QCC-Repo.
- Der Praeprozessor muss dafuer nichts in den Quelltext einsetzen: er reicht
  nur Zeilenmarken (`# 123 "datei.c"`) an den Compiler weiter; `__LINE__`/
  `__FILE__` im Code wuerden Laufzeitdaten erzeugen und helfen dem Debugger
  nicht.

### 5.4 Debugger-Kern

- Erst ein **Monitor im Emulator** (Konsolenbefehle oder TCP-Port):
  Haltepunkt (PC), Watchpoint (Schreibzugriff, gibt es schon als
  `Q9_WATCH_ADDR`), Einzelschritt, Register, Speicher -- alles symbolisch.
  Q9-Flux hat die Haken bereits (Instruktionshook, Freeze nach PC/SP,
  Instruktionsspur).
- Danach ein **GDB-Remote-Stub** (`m68k-elf-gdb`, VS Code), mit der
  ELF-Symboldatei aus 5.2.

### 5.5 Reihenfolge

1. Symbolische Adressen im Q9-Flux-Dump und in der Spur (Kernel-Linkkarte
   + Moduldirectory) -- billig, sofort nuetzlich.
2. Fernaufruf-Makro pruefen (l68-Label-Differenz); bei Erfolg `bsr` aus
   q9kernel_entry.a nach C darauf umstellen -- loest die 32-KB-Grenze.
3. ROF-Debug-Abschnitt (`xcc -g`) entschluesseln -> Zeilentabelle.
4. Monitor mit Haltepunkten/Einzelschritt, dann GDB-Stub.
5. `.stb`-Format der Microware-Module.
6. QCC-Zeilentabellen (QCC-Repo).

## 6. Arbeitspakete

1. Bauschalter für C und Assembler; vorhandenen Debug- und Testcode
   darunter verschieben; Atom-Kernel muss bauen und booten.
2. Trace-Kern: Satzformat, Ringpuffer, Filter, `F$Q9Dbg`, Makros/Header,
   Hosttests für Filter und Satzaufbau.
3. Register-Erhaltungsprüfung.
4. Beschreibungstabelle für die wichtigsten Syscalls, Programm `trace`,
   Host-Dekoder.
5. Emulator-Debugger (GDB-Stub + Symbole).

## 7. Offene Punkte

- Puffergröße und Ort (fester Bereich im Developer-Kernel oder per
  `F$SRqMem` beim Einschalten).
- Feinzeit: welches Timer-Register des Boards den Restzählerstand liefert
  und in welcher Einheit (für die Umrechnung im Tool).
- Wie die Funktions-IDs für interne Aufrufe vergeben werden (Tabelle im
  Repo, damit Kernel und Tool übereinstimmen).
- Ermittlung der FD-Nummer: genaue `I$GetStt`-Unterfunktion bei RBF
  prüfen, bevor das Tool sie benutzt.
- Timing: Konsolenausgabe verändert das Zeitverhalten deutlich; für
  zeitkritische Fehler nur den Ringpuffer verwenden.
