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
(kleine Textdateien, unproblematisch fürs Repo). Rohe Disassemblierungs-
Listings liegen in [`../disasm/`](../disasm/): `round2_all_functions.txt`
(alle 17 Runde-1-Funktionen vollständig), `round3_syscall_targets.txt`
(alle 24 IOMan-Syscall-Zieladressen direkt angesprungen) und
`round4_all_functions.txt` (alle inzwischen ~32 bekannten Funktionen
vollständig, inkl. `I$Attach`/`FUN_000014f8`).

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

## Fund (Runde 2): IOMan ruft Kernel-Primitive über dieselbe Trampolin-Tabelle wie der Kernel selbst auf

Alle 17 Funktionen vollständig disassembliert (Ghidra-Textlisting,
`DumpAllFunctions.java`). Wichtigster Fund: **vier** der Funktionen
(`FUN_00000d56`, ein Zweig von `FUN_0000107a`, `FUN_000010d8`,
`FUN_000015ca`) folgen exakt demselben Muster:

```
move.l   A3,-(SP)
movea.l  (0x3a4,A6),A3      ; A3 = D_SysDis (dieselbe Systemglobal-Adresse
                             ;   wie beim Kernel-Fund "Q9_disp_488")
pea      (0xc,PC)            ; Rücksprungadresse pushen
move.l   (OFFSET,A3),-(SP)   ; Primärarray-Slot als "Wert" pushen
movea.l  (0x400+OFFSET,A3),A3 ; Sekundärarray-Slot laden
rts                          ; "kehrt zurück" in den Sekundärarray-Wert
```

Das ist **byte-genau dasselbe PEA+RTS-Trampolin**, das im Kernel-Fund
"`0x3140` — Cache-Flush + Sprung in Syscall-Tabellen-Slot 90" beschrieben
wurde. `OFFSET/4` ist der Syscall-Tabellenslot (identisch zur Nummerierung
in `../SYSCALL_MODULE_MAP.md`):

| Funktion | Offset | Slot | Syscall lt. Tabelle | Interpretation |
|---|---|---|---|---|
| `FUN_00000d56` | `0x00` | `0x00` | F$Link | IOMan bindet ein Modul — vermutlich für den eigenen Treiber-/Dateimanager-Nachladeweg |
| `FUN_0000107a` (unterer Zweig, `0x1090`ff.) | `0x20` | `0x08` | F$Send | Signal an einen Prozess senden — passt zu "I/O-Operation fertig, wartenden Prozess aufwecken" |
| `FUN_000010d8` | `0xdc` | `0x37` | F$GProcP | aktuellen Prozess-Deskriptor-Zeiger holen |
| `FUN_000015ca` | `0xa0` | `0x28` | F$SRqMem | Speicher anfordern |

**Bedeutung:** IOMan hat keine eigene Kopie dieser Primitive — es nutzt
denselben, vom Kernel bei Boot befüllten Trampolin-Mechanismus direkt,
genau wie es der Kernel intern für sich selbst tut. Bestätigt die im
Kernel-Fund "`0x1390`" bereits geäußerte Vermutung, dieser
Dispatch-Mechanismus sei "aufrufbar sowohl über den öffentlichen
`F$`-Callpfad als auch direkt für interne, performancekritische
Kernel-zu-Kernel-Aufrufe" — jetzt am Beispiel eines ANDEREN Moduls
(IOMan, nicht Kernel) bestätigt, also offenbar ein modulübergreifender
Mechanismus, keine kernel-interne Abkürzung.

## Fund (Runde 2): Einsprungpunkt ist die Init-Routine, mit Hex-Diagnose-Ausgabe bei Fehlern

`ioman_entry_98` (234 Byte) ruft zweimal `FUN_000015ca` (= `F$SRqMem`, s.o.)
auf — jeweils nach einer `mulu.w`/Größenberechnung (Anzahl × Elementgröße),
mit `bcs.w 0x246` (Carry = Fehler) direkt danach. Das ist klassisches
Init-Zeit-Verhalten: **zwei Tabellen anfordern** (vermutlich Geräte-/
Pfaddeskriptor-Tabellen, analog zum Kernel-Fund der Deskriptor-Tabellen),
mit Fehlerpfad bei Speichermangel.

**Fehlerpfad (`0x246`–`0x272`) nutzt eine eigene kleine Diagnose-Ausgabe-
Infrastruktur**, strukturell dem bereits beim Kernel gefundenen
Panik-Reporter (`0x7f6`) ähnlich, aber offenbar IOMan-eigen:

- `FUN_000002b8`/`FUN_000002b2`/`FUN_000002ac`: klassischer **Hex-Ziffern-
  Formatierer** — `FUN_000002b8` wandelt die unteren 4 Bit von `D0` in ein
  ASCII-Hex-Zeichen um (`andi #0xf`, `+7` falls `>9` für `A`-`F`, `+0x30`),
  `FUN_000002b2`/`FUN_000002ac` rotieren `D0` um 4 bzw. 8 Bit, rufen den
  Formatierer rekursiv auf und rotieren zurück — damit wird ein 32-Bit-Wert
  Nibble für Nibble ausgegeben (8 Hex-Ziffern).
- `FUN_0000029c`: **String-Ausgabe-Schleife** — liest Bytes ab einem
  Zeiger, bis `0x00`, ruft für jedes Byte eine Ausgaberoutine über
  `(A1+8)` (Vtable-Zeiger) auf.
- `FUN_00000296`: lädt `A1` aus `(0x64,A6)` — **neu gefundenes
  System-Global-Feld**, ein Zeiger auf eine Ausgabe-Vtable (analog zu
  `D_Proc`/`D_ExcJmp` an anderen Offsets, aber bisher nicht im
  Kernel-Fund dokumentiert; ein Kandidat für eine Folgerunde am Kernel
  selbst).
- `FUN_000002d4`/`FUN_000002e6`/`FUN_000002f8`/`FUN_00000312`: verketten
  diese Bausteine zu "gib String X aus, gib Hex-Wert Y aus" — der
  Fehlerpfad in `ioman_entry_98` (`0x24a`–`0x272`) gibt einen String
  (Zeiger bei PC-relativ `0x119`, Inhalt noch nicht gelesen) gefolgt von
  zwei Hex-Werten (`D0`, `D1` aus dem fehlgeschlagenen `F$SRqMem`-Aufruf)
  aus — plausibel eine Meldung wie *"IOMan: not enough memory, wanted
  &lt;D0&gt; got &lt;D1&gt;"* (Wortlaut spekulativ, Zahlen-Interpretation
  gesichert).

## Fund (Runde 3): IOMan dispatcht die eigentliche I/O-Arbeit an separate Treiber-Module — NICHT über Kernel-Calls

Andreas' Einwand (Buch über OS-9: ein beschriebener File-Manager enthält
**keine** Kernel-Calls) war der Anstoß, die tatsächlichen Zieladressen aller
24 IOMan-zugeordneten Syscalls direkt anzuspringen (`ExploreSyscallTargets.java`,
Offsets aus `../SYSCALL_MODULE_MAP.md` in modulrelative Adressen umgerechnet,
Basis = live gemessener IOMan-Ladeadresse `$00E03C`).

**Ergebnis: Es gibt tatsächlich zwei GETRENNTE Mechanismen in IOMan, die
beide vorkommen, aber unterschiedliche Zwecke haben:**

1. **Kernel-Trampolin** (`(0x3a4,A6)`/`(0x3a8,A6)`, s. Runde 2) — IOMan nutzt
   ihn für **eigene interne Buchhaltung**: Speicher anfordern (`F$SRqMem`,
   in `I$Read`s Präambel z. B. Tabellenslot 88 mit `D1=2`), Signale senden
   (`F$Send`), Modul-Linking (`F$Link`, in `I$Attach` zweimal für
   Geräte-Deskriptor bzw. -Treiber). Das sind Dienste, die IOMan als
   dauerhaft residente Systemkomponente selbst braucht.

2. **Direkter Sprung in ein separates Treiber-Modul** — gefunden in
   `I$Detach` (Offset `0xe5e`), am Ende der Aufräumlogik:

   ```
   movea.l (0x8,A2),A1        ; A1 = Geraete-Tabelleneintrag(?)
   movea.l (0x0,A2),A2
   exg     A0,A2               ; A0 = Zeiger auf den GERAETETREIBER (Modulbasis)
   moveq   0xa,D2
   add.l   (0x30,A0),D2        ; D2 = 10 + Langwort bei Treiber-Offset 0x30
                                ;   (die treibereigene Einsprungpunkt-Tabelle,
                                ;   OS-9-Standard: P$xxx-Offsets nahe Treiberstart)
   move.w  (0x0,A0,D2*0x1),D2w ; D2 = Wort-Offset aus dieser Tabelle
   ...
   jmp     (0x0,A0,D2w*0x1)    ; Sprung DIREKT in den Treiber -- kein
                                ;   RTS-Rueckweg wie beim Kernel-Trampolin!
   ```

   Das ist **kein** Aufruf über die Kernel-Tabelle bei `0x3a4`/`0x3a8` —
   `A0` zeigt auf eine **eigenständige Moduladresse außerhalb von IOMan**
   (Treiber-Modul, per Geräte-Deskriptor-Kette aufgelöst, nicht Teil der
   5660-Byte-`ioman_DEV`-Datei). Der `jmp` (nicht `jsr`) plus die
   PC-relativ am Aufrufer vorbereitete Rücksprungadresse
   (`pea (0xa,PC)` + `move.l SP,(0x140,A4)` unmittelbar davor) entspricht
   exakt dem klassischen OS-9-Treiber-Aufrufschema aus dem Technical
   Manual: Treiber-Modul hat eine kleine Tabelle aus P-relativen Offsets
   zu seinen Standard-Einsprungpunkten (Init/Read/Write/GetStat/SetStat/
   Term) nahe Modulanfang; der Aufrufer berechnet den Offset und springt
   direkt hinein, der Treiber kehrt selbst zurück.

**Einordnung, deckt sich mit dem Buch:** Sobald man EINMAL in den Treiber
(bzw. vermutlich analog: File-Manager) gesprungen ist, läuft dessen eigener
Code — der hat mit IOMans Kernel-Trampolin nichts mehr zu tun, exakt wie im
Buch beschrieben ("keine Kernel-Calls" IM File-Manager/Treiber selbst).
IOMans Kernel-Calls (Mechanismus 1) sind ausschließlich IOMans EIGENE,
system-globale Buchhaltung — nicht Teil der eigentlichen I/O-Ausführung.

**Für `I$Read`/`I$Write`/`I$GetStt`/`I$SetStt` selbst noch nicht
abschließend geklärt**, ob sie denselben Treiber-Sprung-Mechanismus direkt
enthalten oder über den gemeinsamen Kernel-Tabellenslot 88 (`(0x160,A3)`/
`(0x560,A3)`, mit `D1`-Subop-Code: `I$Read`→`D1=2`, `I$Write`→ vermutlich
anderer Wert, noch nicht gelesen) **indirekt** dorthin gelangen — die
Zieladresse von Slot 88 selbst ist per reiner Disassemblierung nicht
sichtbar (dieselbe Einschränkung wie beim Kernel bei Slot 64/88/89/90,
"bleibt ohne Laufzeit-Speicherinspektion unbekannt", s. Kernel-Fund
"Update — 0x4078 gelesen"). Naheliegende Hypothese: Slot 88 ist ein
**gemeinsamer generischer Pfad-Operations-Dispatcher**, der intern denselben
Treiber-Sprung wie `I$Detach` macht, mit `D1` als Sub-Operation (Read/
Write/GetStat/SetStat/...) — noch nicht verifiziert.

**Für `../SYSCALL_MODULE_MAP.md` bedeutet das:** die dortige Modul-Spalte
("IOMan") ist weiterhin korrekt (die Zieladresse liegt tatsächlich in
IOMans 5660-Byte-Modul) — aber bei mehreren dieser Einträge (mindestens
`I$Detach`, vermutlich auch `I$Read`/`I$Write`/`I$Open` u. a.) ist das nur
IOMans **Dispatch-Stub**, nicht der Ort, an dem die eigentliche Datei-/
Geräte-Arbeit passiert — die liegt in einem separaten, hier noch nicht
identifizierten Treiber-/File-Manager-Modul.

## Fund (Runde 4): `I$Attach` bestätigt den OS-9-"Dreiklang" — drei F$Link-Aufrufe für Descriptor/Driver/File-Manager

`I$Attach` (jetzt vollständig gelesen, 588 Byte) ruft **dreimal** den
Kernel-Trampolin `FUN_00000d56` (= `F$Link`, Runde 2) auf, mit
unterschiedlichem Typ-Filter in `D0w` und unterschiedlicher Namensquelle:

1. `D0w = 0xf00`, Name = der geparste Gerätename aus dem Pfad (führendes
   `/` wird übersprungen) → linkt die **Geräte-Descriptor**-Modul.
2. `D0w = 0xe00`, Name = String bei Offset `0x3a` **innerhalb** des gerade
   gelinkten Descriptor-Moduls → linkt ein zweites Modul, dessen Name der
   Descriptor selbst nennt.
3. `D0w = 0xd00`, Name = String bei Offset `0x38` des Descriptors → linkt
   ein drittes Modul.

Das ist der aus der Fachliteratur bekannte OS-9-**"Dreiklang"**
File-Manager/Treiber/Descriptor (vgl. bereits vorhandene Notiz in
`Q9-Flux/docs/MODULES.md`, "OS-9-Dreiklang File-Manager/Treiber/Descriptor")
— live im Disassemblat bestätigt: der Descriptor referenziert **zwei**
weitere Module (Treiber und File-Manager) über eigene Namensfelder,
`I$Attach` linkt alle drei nacheinander. **Welcher der beiden Filter (`0xe`
vs. `0xd`) genau "Treiber" bzw. "File-Manager" bedeutet, ist noch nicht
sicher** — die Typ-Filter-Byte-Bedeutung (0xf/0xe/0xd) ist aus dem Code
selbst plausibel als Modultyp-Filter für `F$Link` erkennbar, aber noch
nicht gegen eine Primärquelle abgeglichen (anders als die bereits
verifizierten `M$Type`-Werte 1/12 aus dem Kopf-Fund).

Nach erfolgreichem Dreifach-Link: Prüfung, ob das Gerät **schon** über
einen anderen Pfad attached ist (Vergleich von Treiber- und
Descriptor-Zeiger gegen bestehende Gerätetabellen-Einträge, Link-Count-
Erhöhung statt Neuanlage falls Treffer — klassische OS-9-Attach-Semantik).
Falls neu: Speicher für einen Gerätetabellen-Eintrag anfordern
(`FUN_000015ca` = `F$SRqMem`, Runde 2), dann **derselbe PEA+RTS-Sprung
in den Treiber wie bei `I$Detach`** (Runde 3), hier aber ohne den `+0xa`-
Versatz — ruft also einen **anderen** Tabellen-Slot des Treibers auf:
vermutlich **Init** (Slot 0) statt **Term** (Slot `0xa`). Damit ist die
klassische OS-9-Treiber-Tabelle (Init an fixer Position, weitere
Standard-Einsprungpunkte an festen Folge-Offsets) empirisch mit zwei
Slots belegt (0 = Init, `0xa` = Term); die dazwischenliegenden Slots
(Read/Write/GetStat/SetStat, vermutlich) noch nicht einzeln bestätigt.

Fehlerpfad ruft bei Bedarf `I$Detach` selbst auf (`bsr 0xe5e`) — sauberes
Rückabwickeln eines teilweise erfolgreichen Attach.

## Fund (Runde 4): `FUN_000014f8` ist der generische I$-Dispatcher (nicht "Wait/Event")

Vollständig gelesen. Aufgerufen von `I$Create(sys)` direkt per `bsr`
(nicht über die Kernel-Trampolin-Tabelle) — Korrektur der Runde-2-Vermutung
("vermutlich Wait/Event-Routine"): es ist der **gemeinsame Dispatcher, der
einen I$-Callcode in den passenden Slot der Treiber-Tabelle übersetzt und
dorthin springt** — genau der in Runde 3 vermutete "Slot-88-artige
gemeinsame Pfad-Operations-Dispatcher", nur als direkt aufgerufene
IOMan-Funktion gefunden statt über die Kernel-Tabelle:

- **Pfad-Deskriptor-Sperre** (kritischer Abschnitt, `ori #0x700,SR`):
  prüft `(0x8,A1)` (Pfad-Deskriptor-Feld, „wer hat den Pfad gerade
  belegt"); ist es `0`, wird die aktuelle Prozess-ID (`(0x0,A4)`, `A4` =
  `D_Proc`) dort UND in `(0x1c,A1)` eingetragen — klassischer
  Belegen-oder-warten-Mechanismus für Mehrprozess-Zugriff auf denselben
  Pfad.
- **Ist der Pfad schon belegt** (Zweig `0x15a6`): Interrupts sofort wieder
  freigeben, `F$GProcP` (`FUN_000010d8`) holen, einen Schwellenwert
  `(0x26,A4)` prüfen und — falls im gültigen Bereich (`< 0x20`) — über
  `F$Send` (`FUN_0000107a`) signalisieren/wecken; sonst zurück zum
  Sperren-Versuch (`bra 0x1500`). Klassisches Warteschlangen-/Retry-Muster
  für "Pfad ist gerade in Benutzung".
- **Callcode → Treiber-Slot:** `D1 = (A5+0x3c) − 0x83`, verdoppelt als
  Wort-Index. `0x83` ist exakt der Callcode von `I$Create` — d. h. die
  Tabelle ist **relativ zu `I$Create` als Slot 0** organisiert:
  `I$Create`→0, `I$Open`→1, `I$MakDir`→2, `I$ChgDir`→3, `I$Delete`→4,
  `I$Seek`→5, `I$Read`→6, `I$Write`→7, `I$ReadLn`→8, `I$WritLn`→9,
  `I$GetStt`→0xa, `I$SetStt`→0xb, `I$Close`→0xc (Codes 0x83–0x8f
  durchgehend, passt exakt zur Callcode-Reihenfolge aus
  `../SYSCALL_MODULE_MAP.md`).
- **Treiber-Auflösung:** `A0 = *(*(A1+4)+0xc) + 0x30` — `(A1+4)` = der in
  `I$Attach` gefundene Gerätetabellen-Eintrag, `+0xc` daraus vermutlich der
  Treiber-Modul-Zeiger, `+0x30` dessen Offset-Tabellen-Basis (dieselbe
  Konvention wie bei `I$Attach`/`I$Detach`). `A0 += Tabelle[Slot]` (Slot
  wie oben berechnet), dann derselbe PEA+RTS-Sprung wie in `I$Attach`/
  `I$Detach` direkt in den Treiber.

**Damit ist die zentrale offene Frage aus Runde 3 beantwortet:** `I$Read`/
`I$Write`/`I$Open`/... (die "einfachen" `I$`-Aufrufe, s.
`../SYSCALL_MODULE_MAP.md`) laufen alle über GENAU DIESEN gemeinsamen
Dispatcher (`FUN_000014f8`) in den jeweiligen Treiber-Slot — nur
`I$Attach`/`I$Detach` (die den Treiber/File-Manager selbst erst
auflösen bzw. abbauen müssen) haben eigene, längere Sonderbehandlung.

## Nächste Schritte (falls gewünscht)

1. **Erledigt (Runde 2):** alle 17 gefundenen Funktionen gelesen; vier
   davon als Kernel-Trampolin-Aufrufe identifiziert (F$Link/F$Send/
   F$GProcP/F$SRqMem), der Einsprungpunkt als Init-Routine mit
   Fehlerdiagnose-Ausgabe verstanden.
2. **Erledigt (Runde 3):** alle 24 IOMan-zugeordneten Syscall-Zieladressen
   direkt angesprungen und disassembliert (`ExploreSyscallTargets.java`).
   Zentraler Fund: `I$Detach` springt am Ende per `jmp` direkt in ein
   separates Treiber-Modul (klassisches OS-9-P-relatives Einsprung-
   Offset-Schema) — NICHT über den Kernel-Trampolin. Bestätigt, dass
   Kernel-Calls (Mechanismus 1) und Treiber-/File-Manager-Dispatch
   (Mechanismus 2) getrennte, beide vorkommende Dinge sind.
3. **Erledigt (Runde 4):** `I$Attach` vollständig gelesen — bestätigt den
   OS-9-"Dreiklang" (drei `F$Link`-Aufrufe: Descriptor, dann zwei weitere
   Module aus dessen eigenen Namensfeldern) sowie den Init/Term-Treiber-
   Sprung. `FUN_000014f8` vollständig gelesen und als der gemeinsame
   Callcode→Treiber-Slot-Dispatcher identifiziert (beantwortet die
   Slot-88-Frage aus Runde 3 funktional, s. Fund-Abschnitt).
4. Offen: welcher der drei Typ-Filter (`0xf`/`0xe`/`0xd`) genau
   "Descriptor"/"Treiber"/"File-Manager" bedeutet, gegen eine Primärquelle
   absichern (aktuell nur aus der Aufrufreihenfolge plausibel).
5. Treiber-Tabellen-Slots zwischen Init (Slot 0) und Term (Slot `0xa`)
   einzeln bestätigen (Read/Write/GetStat/SetStat vermutet, nicht
   verifiziert) — bräuchte entweder ein echtes Treiber-Modul zum
   Gegenlesen oder Laufzeit-Speicherinspektion wie beim Kernel.
6. Neues System-Global-Feld `(0x64,A6)` (Ausgabe-Vtable-Zeiger) gegen den
   Kernel gegenprüfen — taucht dort vermutlich auch auf, war im
   bisherigen Kernel-Fund aber nicht dokumentiert.
7. Ein echtes Treiber- oder File-Manager-Modul aus dem MWOS-SDK
   identifizieren und mit demselben Verfahren disassemblieren — würde die
   noch offenen Tabellen-Slots und die Typ-Filter-Frage (Punkt 4) direkt
   beantworten.
8. Undefinierte Bereiche weiterhin systematisch schließen (Kontrollfluss-
   Verfolgung von den jetzt ~32 bekannten Funktionen aus, wie beim Kernel).
9. Modulkopf-Feldnamen zwischen `0x0A`–`0x12` gegen eine Primärquelle
   absichern (aktuell nur per Analogieschluss zum Kernel-Kopf bestimmt).

**Erstellt**: 2026-08-12
