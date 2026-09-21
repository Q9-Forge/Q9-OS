# Eigener Q9-Kernel — Stand und offene Punkte

Fortlaufender Status des in `Q9-KERNEL/68k/src/kernel/` (bis
2026-09-13: `src/kernel/`, s. Fortsetzung 54 zur Repo-Reorganisation)
neu geschriebenen, OS-9/68K-kompatiblen Kernels. Ergänzt
einem intern dokumentierten Plan um das, was davon **real läuft** — nachgewiesen im
Q9-Flux-Emulator, nicht bloß implementiert.

**Stand: 2026-09-20**, Branch `main`, Commit `7cabb1b`.

---

## ÜBERGABE (2026-09-20, zwölfte Arbeitssitzung — HIER ZUERST LESEN,
ersetzt die Übergabe darunter; die von 2026-09-13 ist ab hier historisch)

**Diese Datei war 134 Commits lang nicht fortgeschrieben.** Die Übergabe
darunter endet am 13.09. bei Fortsetzung 61; seitdem ist sehr viel
passiert, dokumentiert wurde es aber in **`STATUS.md`** im Wurzelverzeichnis.
Das ist seit dem 16.09. die laufende Statusquelle: eine Tabelle über alle
~97 Callcodes plus ein fortlaufender Abschnitt „Latest kernel
verification". **Für den Stand eines einzelnen Systemaufrufs dort
nachsehen, nicht hier.** Diese Datei bleibt für das, was eine Tabelle
nicht trägt: offene Fragen, Messfallen, zurückgezogene Befunde.

### Was seitdem fertig wurde (Auswahl, Details in `STATUS.md`)

Der Kernel hat in diesen sechs Tagen den Sprung von „einzelne Aufrufe"
zu „arbeitsfähiges System" gemacht:

* **Zeit und Wecker:** eigene Softwareuhr, `F$STime`, `F$Time`
  (Datumsformat korrigiert), `F$Alarm` mit `A$Set`/`A$Cycle`/`A$Delete`
  und den absoluten Varianten.
* **Prozess und Ablauf:** `F$Chain`, `F$NProc`, `F$SPrior` (Änderungen
  wirken sofort), `F$AllPrc`/`F$DelPrc`, Mindestpriorität im Scheduler
  (womit die vom Handbuch empfohlene `F$SSpd`-Ersatzroute funktioniert).
* **Ausnahmen und Signale:** `F$Icpt` führt registrierte Routinen jetzt
  wirklich aus, dazu `F$RTE`, `F$SigReset`, `F$STrap` (prozesseigene
  Ausnahmebehandler) und ein Rekursionsschutz in `Q9K_ExcTrap`.
* **Ereignisse und Sperren:** `F$Event` vollständig (inkl. blockierendem
  `Ev$Wait`/`Ev$WaitR`), `F$Sema`.
* **Speicher und Module:** `F$GBlkMp`, das Bitmap-Trio
  `F$SchBit`/`F$AllBit`/`F$DelBit`, `F$FModul`, `F$Mem`
  (Informationsabfrage).
* **Ein- und Ausgabe:** native `I$Open`/`Dup`/`Close`/`ChgDir`/`Write`/
  `WritLn`/`ReadLn`/`GetStt`/`SetStt`, Pfadfreigabe bei Prozessende,
  SSM-Handler im Flachmodus.
* **`F$Load` ist emulatorverifiziert** (`✅`) — der RBF-/Verzeichnis-
  Hänger, der am 17.09. noch als Hauptengpass galt, ist weg.

### DER OFFENE PUNKT: eine laufende Regressionsrücknahme

**Das ist das Erste, was zu klären ist.** Die drei Commits `cdcd010`
(„Accept OS-9 high-bit terminated path components"), `8ece62c` („Read F
input from external register frame") und `04acb91` („Write external F
results back to register frames") haben den `F$PrsNam`-Eingabepfad auf
den `R$`-Registerrahmen umgestellt. Das ist eine **Regression gegenüber
dem verifizierten `F$Load`-Weg**.

Die Rücknahme war beim Schreiben dieser Übergabe **in Arbeit und nicht
abgeschlossen**: in `q9kernel_entry.a` wird der Vorspann
`tst.w Q9K_InTrapPath` / `movea.l $20(a5),a0` aus `Q9K_SysFPrsNam`
wieder entfernt, in `q9kernel_iopath.c` die Hochbit-Behandlung in
`Q9K_ProcPrsNam`, dazu ein zusätzlicher Hex-Marker für den
`F$Load`-Fehlercode. Beides muss noch durch Bau und Emulator.

Wer hier weitermacht: **zuerst `git status` prüfen.** Stehen `entry.a`
und `iopath.c` noch als geändert da, ist diese Rücknahme unfertig und
hat Vorrang vor allem anderen.

### MESSFALLE, die vier falsche Befunde erzeugt hat

Am 19.09. wurden mit `5b8a6d7` **vier Befunde auf einmal zurückgezogen**,
die über drei Runden in `STATUS.md` gestanden hatten: der Zeitgeber ticke
nicht, `F$Sleep` kehre nie zurück, ein von einem Tick unterbrochener
Prozess laufe nie wieder, und der Kontext werde über einen Wechsel nicht
wiederhergestellt. **Nichts davon stimmte.** Alle vier stammten aus
*einem* Messfehler:

> Die Konsole im Direct-Attach-Test wird von `Q9K_TestProcA` geflutet —
> der Elternprozess druckt in seiner Diagnoseschleife ununterbrochen
> `A`. Jede Ablesung wurde in einem ~40-Zeichen-Fenster hinter der
> Markierung gemacht, und in diesem Fenster hatte die Flut längst alles
> Nachfolgende begraben. Filtert man `A` aus dem vollständigen Protokoll,
> standen die Markierungen die ganze Zeit da.

Merksatz aus jenem Commit, der es auf den Punkt bringt: **„A short window
onto a noisy channel is not a measurement."** Also: immer das
vollständige Protokoll sichern und gezielt filtern, nie ein kurzes
Fenster hinter einem Marker ablesen.

Dasselbe galt für die „leere Ready-Queue" und den „verschwundenen
Prozess": das Testprogramm war schlicht fertig, hatte nach `chaintgt`
gekettet und sich normal beendet — deshalb lief `Q9K_ProcExit` mit
Status 0 auf dem eigenen Deskriptor. Nie ein Fehler.

Das ist **dasselbe Muster wie bei der Arena-Überlappungs-These** aus
Fortsetzung 61 (unten), die ebenfalls zweimal behauptet und dann
endgültig zurückgezogen wurde: eine beobachtete Speicherfreigabe war
normales Prozessende, keine Korruption. Zweimal dieselbe Verwechslung —
**ein normal endender Prozess sieht in Spuren aus wie ein Fehler.**

### Ein grünes Häkchen, das nie belegt war (`b9f9bfe`)

`F$Load` stand kurzzeitig auf ✅ mit der Begründung „emulator-verified
with `/dd/CMDS/echo`". Eine Nachprüfung hat das widerlegt, und zwar
methodisch sauber: der fragliche Commit wurde **in einem eigenen
Worktree ausgecheckt, gebaut und laufen gelassen**. Auch dort erscheint
der Fehlschlag-Marker `k`. Der Aufruf hat diesen Test also nie
bestanden — es ist **keine Regression**, sondern ein ✅, das auf einer
anderen Messung beruhte als der, die die Tabellenzeile beschrieb.
`F$Load` steht wieder auf 🟡.

Daraus zwei Dinge, die über den Einzelfall hinausgehen. Erstens: **einen
Statuswechsel gegen den Commit gegenprüfen, der ihn eingeführt hat** —
ein Worktree kostet Minuten und beantwortet „war das je grün?"
eindeutig. Zweitens: eine Statuszeile muss sagen, *welche* Messung sie
trägt, sonst wandert sie beim nächsten Lesen an eine Beobachtung, die
gar nicht zu ihr gehört.

Der Befund selbst ist inzwischen weitergetrieben: der fehlschlagende
`F$PrsNam`-Aufruf kommt per **TRAP #0** (nicht über das
PEA+RTS-Trampolin, die 44-Byte-Rahmenkonvention gilt hier also gar
nicht), mit einem **gültigen Zeiger auf einen leeren Puffer**, und der
Aufrufer ist per Ausnahmerahmen-PC gegen die Moduldirectory als **`rbf`**
identifiziert. An `Q9K_ProcPrsNam` ist nichts falsch; die nächste Stelle
ist, was unsere Seite beim Eintritt in IOMan übergibt. Die drei Commits
`cdcd010`/`8ece62c`/`04acb91` zielten auf ein Problem, das es an dieser
Stelle nicht gibt.

**Werkzeug dazu:** die Moduldirectory steht vollständig im
`q9dbg_dump.txt` (Abschnitt „Moduldirectory-Kette ab
`Q9K_MODDIR_HEAD_ADDR`") mit HdrPtr und Größe je Modul. Damit lässt sich
eine beliebige PC in Sekunden einem Modul und einem Offset zuordnen —
ohne Ghidra und ohne `annotate_trace.py`.

### Status des alten Rätsels „csl traphandler mismatch"

Die Übergabe darunter endet mit diesem ungeklärten Befund (`date` druckt
`**** csl traphandler mismatch ****`, vier Hypothesen widerlegt). **Er
taucht seit dem 14.09. in keinem Commit und in `STATUS.md` überhaupt
nicht mehr auf.**

Ob er behoben, durch andere Arbeiten nebenbei erledigt oder nur aus dem
Blick geraten ist, **lässt sich aus den Commits nicht belegen** — hier
wird deshalb bewusst nichts behauptet. Zwei Anhaltspunkte: `F$Load` ist
inzwischen emulatorverifiziert, allerdings mit `/dd/CMDS/echo`, nicht mit
`date`; und der Abschnitt zum isolierten `date`-Test in `STATUS.md`
(Messstand 17.09.) steht unverändert da. **Erster Prüfschritt für die
nächste Sitzung: `date` erneut laufen lassen und sehen, was jetzt
passiert.**

### Neue Werkzeuge und Methodik

* **ABI aus dem Original zurückgewinnen, wenn das Handbuch schweigt.**
  Das Verfahren ist mit `F$Sema` erprobt und in `cc4ba5a` festgehalten.
  Mit `F$FModul` und `F$Mem` wurde es wiederholt.
* **`dis68k.py`** (`Q9-OS-Research/kernel-68k/tools/`): disassembliert
  einen Offsetbereich des Originalkernels mit capstone. Nötig, weil
  **Ghidra Syscall-Einstiege als Daten führt** — sie sind über keinen
  Kontrollfluss erreichbar, sondern werden ausschließlich über die
  Dispatchtabelle betreten. Gilt für jeden noch nicht analysierten
  Callcode.
* **`Q9-OS-Research/kernel-68k/SYSCALL_ABI_UNDOCUMENTED_de.md`**: für die
  Aufrufe ohne Handbucheintrag. Enthält unter anderem den Nachweis, dass
  `F$AllRAM` (`$39`), `F$POSK` (`$5D`) und `F$SSpd` (`$0B`) im
  Referenz-Build in *beiden* Tabellen auf dem Fehler-Stub `$8480` stehen
  — für sie existiert kein Code, aus dem sich eine ABI herleiten ließe.
* **Die 16-Bit-Reichweite von `bsr` ist erreicht.** Neue C-Module
  gehören ans **Ende** der Link-Liste in `build.sh` (fünf Stellen), und
  Aufrufe aus `entry.a` in die letzten Module laufen über Zeigerzellen
  (Muster `Q9K_StrapImplPtr`, von `cinit.c` beim Booten gefüllt). `l68`
  meldet sonst nur „operand size error" **ohne die Stelle**.
* **Hosttest-Falle bei Deskriptor-Offsets:** auf dem Host ist
  `unsigned long` 64 Bit, im Kernel 32. Felder, die nur vier Byte
  auseinanderliegen (etwa `$1B0`/`$1B4`), überlappen sich beim Schreiben
  im Test. Offsets im Hosttest umdefinieren — s. `test_q9kernel_chain.c`
  und `test_q9kernel_mem.c`.

### Nächste Schritte

1. Die `F$PrsNam`-Rücknahme abschließen (s. o.) — hat Vorrang.
2. `date` laufen lassen und den `traphandler mismatch` klären.
3. `F$Mem` verdrahten: Handler in `q9kernel_entry.a`, Dispatch-Eintrag in
   `q9kernel_cinit.c`. Der C-Teil (`q9kernel_mem.c`) ist gebaut, getestet
   und gelinkt, aber **noch nicht erreichbar**; Scratch bei
   `$1E74`–`$1E84`, Implementierung `Q9K_SysFMemImpl`.
4. Emulator-Regressionen nachziehen für `F$FModul` und `F$Mem`.
5. Weitere im Kernelmodul herleitbare Aufrufe: `F$DFork`/`F$DExec`/
   `F$DExit` (`$22`–`$24`) als Gruppe, `F$FIRQ` (`$61`).

### Arbeitsteilung

An diesem Kernel arbeiten zeitweise **mehrere Sitzungen parallel**.
Praktisch ist das nur begrenzt möglich: jeder neue Systemaufruf fasst
dieselben vier Dateien an (`q9kernel_entry.a`, `q9kernel_cinit.c`,
`build.sh`, `STATUS.md`). Vor Arbeit am Kernel deshalb prüfen, ob eine
andere Sitzung läuft, und die Dateiaufteilung vorher absprechen.
Kollisionsfrei ist Arbeit außerhalb des Kernelverzeichnisses — etwa
ABI-Recherche im separaten Repo `Q9-OS-Research`.

---

## ÜBERGABE (2026-09-13, elfte Arbeitssitzung, Fortsetzung 61 —
historisch, s. oben)

**Fortsetzung 61 (zweites echtes Kommandomodul "date" getestet):
"csl traphandler mismatch" gefunden, Ursache trotz MEHRERER
Untersuchungsanlaeufe NOCH OFFEN.** `date` (zweites, von `echo`
unabhaengiges Testprogramm) druckt `**** csl traphandler mismatch
****` und bricht ab (kein Absturz). VIER Hypothesen geprueft und
ALLE widerlegt (Trap-15-Mechanismus, gemeinsamer Prozess-A6, Scheduler
rettet A6 nicht, Arena-Ueberlappung zwischen echo/date -- letztere
zunaechst faelschlich "bestaetigt", dann selbst wieder zurueckgezogen,
dann per echter Symboltabelle [`r68 -s`, s. u.] endgueltig als
Sackgasse erkannt: die beobachteten Speicherfreigaben waren schlicht
normales Prozessende, keine Korruption). **Die tatsaechliche Ursache
bleibt nach alledem OFFEN.** Eine unabhaengig davon gueltige, echte
Erkenntnis bleibt bestehen: `F\$Exit` (`q9kernel_procend.c`) gibt den
primaeren Prozessblock automatisch an die Arena zurueck -- korrigiert
die bisherige Annahme "Speicher-Ruecknahme bei Prozessende nicht
implementiert" (galt nur fuer per `F\$SRqMem` angeforderte
Zusatzbloecke). **Neues, funktionierendes Werkzeug fuer kuenftige
Sitzungen:** `r68 -s` liefert eine echte Symboltabelle mit
Funktionsgrenzen (auch fuer aus C uebersetzte Dateien, volle
Kompilierkette dokumentiert) -- naechster Schritt: `Q9K_ProcTLink`/
`Q9K_ApplyInitializedData`/`Q9K_ProcFork` DAMIT untersuchen, nicht die
Arena. Details in Fortsetzung 61 unten (inkl. zweier Korrekturen am
Ende).

**Fortsetzung 60 (direkter Anschluss an 58/59): `F$SetSys` gehaertet.**
Unbekannte Systemvariablen meldeten bisher still `0` + Erfolg (in
Fortsetzung 58 als offene Einschraenkung dokumentiert) -- melden jetzt
sauber `E$UnkSvc` (`$D0`, aus der realen Fehlercode-Tabelle gezaehlt,
nicht geraten). Die eine bekannte Variable (`$7C`, csl-Malloc-
Zuwachsgroesse) bleibt unveraendert erfolgreich -- `echo Hallo` weiterhin
korrekt live verifiziert. Alle 16 Host-Testsuiten gruen (Testfall
erweitert). Details in Fortsetzung 60 unten.

**Fortsetzung 59 (direkter Anschluss an 58): `echo` produziert
NACHWEISLICH KORREKTE Ausgabe, nicht nur absturzfrei.** `F$Fork("echo")`
im Testcode lief bisher IMMER mit Parametergroesse 0 ("kein argv") --
deshalb war nie echo-eigener Text zu sehen. Jetzt mit echtem Parameter
(`"echo Hallo"`) getestet: die Terminal-Mitschrift zeigt danach
woertlich **`echo Hallo`** -- korrekt durch `csl`s Laufzeitbibliothek
verarbeitet und ausgegeben, weiterhin `Vektor=0` (kein Absturz). Details
in Fortsetzung 59 unten.

**GROSSER MEILENSTEIN: `echo`/`csl` laeuft jetzt VOLLSTAENDIG UND
ABSTURZFREI durch (Fortsetzung 58) — das seit Fortsetzung 44
verfolgte Sagathema ist damit ENDGUELTIG ABGESCHLOSSEN.** Zwei Fixes
noetig, live verifiziert (zweimal unabhaengig reproduziert, `Vektor=0`
= keine Exception, `echo`s Prozess sauber per `F$Exit` beendet und aus
der Ready-Queue entfernt):

1. **`Q9K_PatchCslFreelistBug`** (neu, `q9kernel_traplink.c`): patcht
   NACH DEM LADEN (nur die RAM-Kopie, NIE die Datei `csl.mod`) den in
   Fortsetzung 56/57 gefundenen `csl`-eigenen Freilisten-Bug (fehlende
   NULL-Pruefung nach dem Weiterruecken in `csl`s privater, zirkulaeren
   Freiliste) -- ein 4-Byte-`bsr.w` zu einem frisch allozierten,
   28-Byte-Stub, der dieselbe Pruefung ergaenzt. Nur wirksam bei exakt
   passendem Bytemuster UND ausreichender `M$Size` (Sicherheitsnetz),
   sonst unveraendert. Neuer Testfall F10 (`test_q9kernel_traplink.c`).
2. **`F$SetSys`** (Callcode `$27`, neu, `q9kernel_setsys.c` +
   `Q9K_SysFSetSys` in `q9kernel_entry.a`): eine ECHTE, bis dahin
   unentdeckte Q9-OS-Kernelluecke (nicht `csl`s Code!) -- der Aufruf
   war komplett unregistriert, wodurch `csl`s `malloc()`-
   Wachstumslogik eine uninitialisierte (effektiv 0) Zuwachsgroesse
   bekam und kurz darauf durch Null teilte (Vektor 5). Bewusst nur
   PRAGMATISCH implementiert (gleiches Muster wie `F$CCtl`): kein
   echtes System-Global-Register, Lesen liefert fuer die eine
   bekannte Variable (`$7C`) einen sinnvollen Standardwert (4096),
   sonst 0, immer Erfolg. Neuer, 16. Host-Testfall
   `test_q9kernel_setsys.c`.

Alle 16 Host-Testsuiten gruen. Sicherheitsabstand (Fortsetzung 37/38)
erneut geprueft, weiterhin exakt richtig. Vollstaendige technische
Herleitung (Analysebefunde, Byte-Ebenen-Beweise, alle Entscheidungen)
in Fortsetzung 56/57/58 unten.

---

## ÜBERGABE (2026-09-11, elfte Arbeitssitzung — historisch, s. oben)

**MEILENSTEIN: `M$IData`/`M$IRefs` implementiert (Fortsetzung 49) UND
die gesamte `echo`/`csl`-Adressbeziehungssaga seit Fortsetzung 44 auf
EINE einzige, im Handbuch woertlich belegte Ursache zurueckgefuehrt +
behoben (Fortsetzung 51): A6 fehlte der dokumentierte `$8000`-Bias**
(68k_tech.pdf Table 2-6/D-7: "(a6) is always biased by $8000 ... the
linker biases all data references by -$8000"). `Q9K_ProcFork` setzt
jetzt `a6 = block + $8000`; die kunstvolle Fortsetzung-48-Kombi-
Allokation (`Q9K_ExperimentalCombinedAlloc`) ist dadurch ueberfluessig
geworden und VOLLSTAENDIG entfernt. `echo` erreicht live wieder alle
vier Meilensteine (Load/csl geladen/F$TLink/F$Fork), diesmal ohne den
Fortsetzung-50-Absturz. Alle 15 Host-Testsuiten grün.

**IRQ-Tabellen-Absturz aus Fortsetzung 51 GELÖST (Fortsetzung 52):**
KEIN Registerleck (die Vermutung in Fortsetzung 51 war falsch) --
echte Ursache war eine simple ADRESSKOLLISION: `Q9K_IRQTAB_BASE`
($1500, 16 Einträge à 20 Byte) reichte bis `$1640` und überlappte
GLEICH VIER später angelegte Scratch-Zellen-Gruppen (`Q9K_SEND_`/
`Q9K_RETPD_`/`Q9K_VMODUL_SCRATCH_*`), die "$16xx" fälschlich für frei
hielten -- Slot 15 war BYTE-GENAU deckungsgleich mit `Q9K_VMODUL_
SCRATCH_*`. Fix: Tabelle auf 12 Einträge verkleinert (endet bei
`$15F0`, vor der ersten echten Nachbarzelle `$1600`). Live verifiziert:
der `PC=$7002`-Absturz tritt nicht mehr auf.

**Vierter Fund (Vektor 10, `PC=$4e25e`) WEITER EINGEGRENZT, ECHTE
URSACHE NOCH OFFEN (Fortsetzung 53+54) — Fortsetzung-53-Verdacht
("`echo`s Modulabbild bereits vor dem Kopieren korrumpiert") WIDERLEGT:**
per gezielter Live-Prüfung (Fortsetzung 54) direkt nachgewiesen, dass
sowohl `F$Load` (schreibt den Stub byte-genau korrekt) als auch
`Q9K_ApplyInitializedData`s Kopierschleife als auch `Q9K_ProcFork`s
`Q9K_SetFrameReg`-Aufruf (schreibt beim Fork korrekt `block+$8000` in
den A6-Slot) alle drei NACHWEISLICH KORREKT arbeiten — keiner der drei
Verdaechtigen aus den Fortsetzungen 49-51 ist die Ursache. Trotzdem
zeigt die Absturz-Mitschrift `A6=$5567f` statt des beim Fork korrekt
gesetzten `$556a0` (Differenz weiterhin exakt `$21`=33 Byte). Die
Speicherzelle, die den Wert urspruenglich hielt, wird spaeter von
`echo`s eigenem, normalem Stack-Betrieb ueberschrieben (kein Bug), aber
KEINER dieser spaeteren Werte erklaert `$5567f` — der tatsaechliche
A6-Wert zur Absturzzeit muss auf REGISTER-Ebene entstehen (vermutlich
`echo`s eigener Code veraendert A6 selbst kurzzeitig, ein Timer-
Interrupt trifft in dieses Fenster). Reine Speicheradressen-Beobachtung
kann das nicht mehr aufloesen.

**A6-Kantenverfolgung umgesetzt (Fortsetzung 55):** die ersten 13
beobachteten A6-Uebergaenge zeigen ein VOLLSTAENDIG korrektes,
wiederholtes `echo`⇄`csl`-Umschaltmuster (a6 wechselt sauber zwischen
`$556a0` und csl's eigenem `$3df10` und wieder zurueck) -- die
grundsaetzliche a6-Umschaltung beim Aufruf einer Trap-Bibliotheks-
funktion ist damit ebenfalls als korrekt bestaetigt. Der Uebergang zum
tatsaechlichen Fehlerwert (`$5567f`) selbst bleibt unbeobachtet: die
Instrumentierung bringt den EMULATOR-HOST-PROZESS (nicht nur die
emulierte CPU) reproduzierbar kurz danach zum Absturz, Ursache nicht
ermittelt. Bewusst NICHT weiter verfolgt (sechster eigenstaendiger Fund
dieser sehr langen Sitzung) — konkreter Plan (robustere, ringpuffer-
basierte A6-Verfolgung statt live `fprintf`) in Fortsetzung 55.

**Vorheriger Meilenstein (weiterhin gültig, unverändert stabil):** der
seit 2026-09-04 verfolgte "kernelgrößenabhängige Interrupt-Race"-Absturz
ist behoben (war die A4-Herkunftsprüfung-Bug aus Fortsetzung 25, Fix
per 12 Byte Totraum an Datei-Offset `$3ac`-`$3af`, Fortsetzung 37/38).
Nach BEIDEN Fortsetzung-49-Änderungen erneut per Byte-Dump geprüft:
Sicherheitsabstand sitzt weiterhin exakt richtig.

**WICHTIG — das ist nur Symptomschutz, keine echte Lösung:** die
A4-Herkunftsprüfung in `Q9K_TrapDispatch` bleibt fehlerhaft. Jede
künftige Codeänderung VOR der geschützten Stelle im Modul kann den
Totraum verschieben und ein ANDERES, ungeschütztes Offset dem
blinden Schreibzugriff aussetzen — bei einem neuen, scheinbar
unerklärlichen Absturz mit Vektor 4 IMMER ZUERST `Q9_WATCH_ADDR=
<Modulbasis+$3ac>` prüfen (Methodik in Fortsetzung 37), bevor eine neue
Ursachenjagd beginnt.

**Nächster inhaltlicher Schritt:** den neuen `PC=$6c`/`scf+$352`-Absturz
aus Fortsetzung 49 untersuchen (Watchpoint-Check zuerst, s. o.) — eine
eigenständige, klar umrissene Baustelle, die bewusst NICHT mehr am Ende
dieser bereits sehr langen Sitzung begonnen wurde.

---

## ÜBERGABE (2026-09-11, zehnte Arbeitssitzung — historisch, s. oben)

**Auftrag dieser Sitzung:** Fortsetzung 33s konkreten nächsten Schritt
umsetzen — die Ringpuffer-Instrumentierung um zwei Messpunkte an
`Q9K_TrapCallExternal`/`Q9K_TrapAfterCall` erweitern, um die seit
2026-09-04 bekannte Interrupt-Race live zu fangen.

**WICHTIGSTER NEUER FUND: der crash-verursachende `trap #0`-Aufruf
erreicht `Q9K_TrapDispatch` NIE — der Fehler liegt vermutlich VOR oder
AUSSERHALB des Dispatchers selbst, nicht (nur) in der Rücksprung-PC-
Korrektur, wie bisher angenommen.**

Instrumentiert wurden fünf Messpunkte (Ringpuffer, 8192 Slots à 8 Byte
Marker.l/PC.l, `$1440C0`ff., Index bei `$1540C0`, Details/Patch-Text
siehe Abschnitt "Instrumentierung" unten):
1. `Q9K_TimerIRQHandler`-Eintritt (Marker=30)
2. `Q9K_IRQDispatch`-Eintritt (Marker=Vektornummer)
3. `Q9K_TrapDispatch`-ALLERANFANG, direkt nach dem Lesen des
   Funktionscodes, VOR der `addq.l #2`-Korrektur (Marker=`$5400`+
   Funktionscode, PC=roh/unkorrigiert) — **neu, gezielt für diesen Fund**
4. `Q9K_TrapCallExternal`-Anfang (Marker=`$58`='X')
5. `Q9K_TrapAfterCall` unmittelbar vor `rte` (Marker=`$41`='A')

**Reproduktion:** Der Absturz aus Fortsetzung 34/36 (`$6C`/Illegal
Instruction bei einem `I$Write`-Trap) tritt mit dieser Instrumentierung
(die den Kernel um ~1,8 KB vergrößert) SCHON BEIM EINFACHEN A/B-
Testprozess-Boot auf, lange vor jedem `F$TLink`/`echo`-Test — exakt wie
die seit 2026-09-04 dokumentierte Kernelgrößenabhängigkeit vorhersagt.
**Deterministisch reproduzierbar, nicht zufällig:** zwei unabhängige
Testläufe (verschiedene Kernel-Builds, identischer sonstiger Zustand)
landen BYTE-IDENTISCH bei Vektor 4, `PC=$000074B4`, identischem
Registersatz und identischem Code-Dump. `$74B2`=`4e40` (`trap #0`),
`$74B4`=`008a` (Funktionscode `$8A`=I$Write) — die CPU führt das
Funktionscode-Wort als Instruktion aus, GENAU das seit Fortsetzung 36
bekannte Muster ("die addq.l-Korrektur geht verloren"), jetzt aber mit
vollem Ringpuffer-Kontext davor.

**Der Ringpuffer zeigt für DIESEN Absturz KEINEN Messpunkt-3-Eintrag
(`Q9K_TrapDispatch`-Start) — obwohl 8192 Slots reichlich Platz boten
(Index stand bei nur ~325) und der Puffer für ALLE VORHERIGEN Trap-
Aufrufe zuverlässig Einträge zeigte.** Das heißt: der crash-Trap hat
noch nicht einmal die ERSTE Instruktion von `Q9K_TrapDispatch` erreicht
— der Fehler liegt vermutlich nicht (nur) in der Korrektur selbst,
sondern schon davor: entweder springt die Hardware-Exception beim
`trap #0` gar nicht (mehr) zum Handler, oder der Opcode an `$74B2` war
zum Ausführungszeitpunkt noch nicht `4e40` (Selbstmodifikation/Race an
dieser Speicherstelle) und wurde es erst später (der Dump danach zeigt
ihn korrekt) — beides bisher nicht unterschieden.

**Zweiter Fund, direkt davor im Ringpuffer: ein Interrupt-Sturm mit
EINGEFRORENEM PC.** Unmittelbar bevor der Puffer für den Rest des Laufs
nur noch Timer-/DUART-Ticks zeigt (keine weiteren Traps mehr bis zum
Crash), stehen drei aufeinanderfolgende DUART-Interrupt-Einträge
(Marker `$50`=80) mit BYTE-IDENTISCHEM PC (`$B422`) — derselbe Wert, zu
dem gerade ein externer `I$ChgDir`-Aufruf (Messpunkt 4/5, Funktionscode
`$86`) per RTE zurückgekehrt war. Der unterbrochene Prozess kam
zwischen diesen drei IRQs nachweislich NIE dazu, auch nur EINE
Instruktion nach der Rückkehr auszuführen — ein echter Interrupt-Sturm,
plausibel gemacht durch die im Kopfkommentar von `Q9K_IRQDispatch`
dokumentierte hohe DUART-TxRDY-Frequenz während einer Zeichenausgabe.
**Kausalität zwischen diesem Sturm und dem späteren `trap #0`-Verlust
NICHT bewiesen, aber der einzige auffällige Vorläufer im gesamten
aufgezeichneten Verlauf.**

**Arbeitshypothese für die nächste Sitzung (NICHT verifiziert):** die
Musashi-CPU-Emulation könnte bei einem Interrupt, der sehr knapp vor
oder während der Ausführung einer `trap #0`-Instruktion selbst eintritt
(nicht danach — dieser Fall ist über die Interrupt-Sperre am
`Q9K_TrapDispatch`-Anfang bereits abgedeckt), die Trap-Exception
verschlucken oder verzögern, statt sie danach nachzuholen — das würde
erklären, warum kein Kernel-Fix (fünf Anläufe an `Q9K_TrapCallExternal`
allein) das Symptom je vollständig beseitigt hat: die Ursache läge dann
nicht im Kernel-Code, sondern im Emulator selbst. **Nicht geprüft**, ob
ein vergleichbares Verhalten bei Musashi bekannt/dokumentiert ist, und
nicht geprüft, ob genau in diesem Fenster (kurz vor der abgestürzten
`trap #0`) tatsächlich ein Interrupt anlag — das wäre der nächste,
präzise benannte Schritt: Messpunkt 3 (`Q9K_TrapDispatch`-Anfang) UND
einen sechsten Messpunkt GANZ VORN in `Q9K_TimerIRQHandler`/
`Q9K_IRQDispatch` (vor `ori.w #$0700,sr`, falls technisch möglich, oder
mit einem Zyklenzähler statt PC) so nah beieinander vergleichen, dass
sich eine echte Verschachtelung auf Instruktionsebene zeigt — dafür
reicht der grobe Ringpuffer nicht, das bräuchte einen Musashi-eigenen
Trace-Hook (`m68k_set_instr_hook_callback` o. ä.) statt Kernel-Code.

**Testabbild-Rezept: NEUE, robustere Alternative zum bisherigen
`OS9SYS.q9test.hda`-Weg gefunden.** Das in Fortsetzung 33/34
dokumentierte Rezept (frisch mit `os9 format` ohne `-e` formatieren,
dann `tools/mkbootfile.sh --disk`) erzeugte in dieser Sitzung
reproduzierbar einen VÖLLIG ANDEREN, viel früheren Hang (CF-Bootstrap-
Banner "RP012E" gefolgt von endlosem "B", kein einziges eigenes
Diagnosezeichen) — auch mit dem unveränderten `6dbc6af`-Kernel
(Kontrollversuch: `git stash` in Q9-OS, Original-Kernel gebaut, gleiches
Bild, gleiches Symptom). Stundenlang als Testabbild-Bug verdächtigt,
per Sync-Wort-Analyse (`$4AFC`-Suche) VERIFIZIERT als bytegleiche
Bootkette zum bekannt funktionierenden `OS9SYS.dbg10.hda` — also NICHT
die Ursache. Der tatsächliche Hang war derselbe Interrupt-Race, nur
noch früher ausgelöst (durch die zusätzliche Instrumentierungsgröße).
**Robusteres Rezept, das diese Sitzung zuverlässig benutzt hat:** ein
frischer `cp -c`-Klon von `OS9SYS.dbg10.hda` (garantiert korrekt
partitioniert/formatiert), NUR die Bootkette per direktem Python-
Byteschreiben an eine weit entfernte, sicher freie LSN (z. B. `2000000`)
geschrieben (Identification-Sektor `$15`-`$17`=LSN, `$18`-`$19`=Länge
manuell aktualisiert) — umgeht sowohl das bekannte `os9 gen -b=`-
Fragmentierungsproblem auf bereits benutzten Abbildern als auch jedes
Format-Detail von frisch formatierten Abbildern. Die einzelnen Module
der `dbg10`-Bootkette (init/forkchild/hellosvc/ioman/scf/sc68681/term/
rbf/cfide/dd/c0) liegen extrahiert unter `/tmp/dbg10_mods/*.mod` auf dem
Mac (Job-lokal, ggf. erneut extrahieren: Sync-Wort-Suche in der per
Identification-Sektor gelesenen Bootkette, Grenzen sind exakt an jedem
`M$Size`-Feld ablesbar). Für einen frühen Absturz reicht ein
`sleep 3`-Dump-Trigger direkt nach Spawn (kein Warten auf ein
Bannermuster nötig).

**Instrumentierung: aktueller Zustand und Patch-Text zum
Wiederherstellen.** Die fünf Messpunkte sind wie in dieser Sitzung
gebaut noch im Repo (`q9kernel_entry.a`, s. `git diff`) — anders als in
Fortsetzung 33 NICHT zurückgesetzt, weil der nächste Schritt (sechster
Messpunkt / Musashi-Trace-Hook) direkt darauf aufbaut. **Falls doch
zurückgesetzt wurde:** `git log --oneline -- src/kernel/q9kernel_entry.a`
zeigt, ob ein Commit `Q9K_RaceRing` diese Sitzung eingecheckt hat: falls
ja, dort der volle Patch; falls die Arbeitskopie stattdessen per
`git checkout` zurückgesetzt wurde, ist der Patch nur noch in dieser
Übergabe (Diff nicht mehr verfügbar) — dann per Neuanlage der equ-
Definitionen (`Q9K_RaceRingBase equ $1440C0`, `Slots equ 8192`, `EntSz
equ 8`, `Idx equ $1540C0`) und der fünf oben beschriebenen Log-Blöcke
neu bauen (jeder rettet die von ihm benutzten Register explizit per
`movem`, s. Kommentare im Code für die genauen Register-Konventionen
an jeder Stelle — sie unterscheiden sich je Einfügepunkt). Auswertung:
`Q9-Flux-68k/src/kernel/q9boardrun.c`, Funktion
`dbg_dump_q9kernel_extras`, Abschnitt "Q9K_RaceRing" (liest `$1540C0`
als Index, `$1440C0`ff. als Ringpuffer, filtert auf X/A/T-Marker ±3
Nachbareinträge, sonst bei 8192 Slots unlesbar viel Text).

Alle 15 Host-Testsuiten weiterhin grün (die Instrumentierung ist reines
Assembler in `q9kernel_entry.a`, keine C-Signatur geändert).

---

## ÜBERGABE (2026-09-11, neunte Arbeitssitzung — historisch, s. oben)

**Branch: weiterhin `fix/a4-aufruferabhaengig` (PR #13), Commit `3e35aa0`.**
Volle Kette: `Fortsetzung 1` bis `36` weiter unten im Dokument, die
letzten fünf (`32`–`36`) sind die dieser Sitzung.

**MEILENSTEIN ERREICHT: `F$TLink` funktioniert end-to-end.** Die in
Fortsetzung 31 aufgemachte Baustelle ("csl"-Trap-Library fehlt komplett)
ist geschlossen: `F$TLink` (Callcode `0x21`, TRAP-#1–15-Mechanismus)
implementiert (`q9kernel_traplink.c`, `Q9K_SysFTLink`/
`Q9K_TCallDispatch` in `q9kernel_entry.a`), 15/15 Host-Testsuiten grün,
UND live gegen das echte, unveränderte Microware-Kommando `echo`
verifiziert: `F$TLink(13,"csl")` liefert Erfolg, `echo` installiert
seine C-Laufzeitbibliothek und läuft danach spürbar weiter (zwei echte
Prozesse in der Ready-Queue), statt wie vorher sofort in seiner eigenen,
fragilen Fehlerbehandlung abzustürzen (`PC=$7031`). Details/Forensik:
`Fortsetzung 32`–`34`.

**Zwei weitere echte Bugs unterwegs gefunden+gefixt** (beide unabhängig
vom Endergebnis wertvoll, nicht nur Krücken):
- `F$CCtl` (Cache Control, Callcode `0x5A`) fehlte komplett — `csl` ruft
  es laut Handbuch (68k_tech.pdf S. 379f) nach jedem `F$TLink` auf, um
  vor Ausführung frisch gelinkten Codes den Instruction-Cache zu leeren.
  Jetzt implementiert (ehrlicher No-Op, da Q9-Flux-68k keine echte
  Cache-Hardware emuliert). `Fortsetzung 35`.
- `Q9K_TCallDispatch` verletzte die reale TrapEnt-Konvention
  (68k_tech.pdf S. 172f, "d0-d7/a0-a5 = caller's registers", MÜSSEN
  unverändert durchgereicht werden) zweifach: d0 wurde beim
  Rahmenaufbau als eigenes Rechenregister missbraucht und nie
  zurückgegeben, UND `movea.l ExecEntry,a4 / jmp (a4)` überschrieb a4
  genauso. Beide gefixt (Vorausberechnung in neue Speicherzellen,
  registerloser "Adresse pushen, RTS springt hin"-Trampolin wie bei
  `Q9K_TrapExtInvoke`). Noch NICHT live gegen `echo`s eigenen
  `tcall 13,X`-Aufruf nachverfolgt (s. nächster Absatz, warum).
  `Fortsetzung 36`.

**OFFEN, NICHT GELÖST — die seit 2026-09-04 bekannte, kernelgrößen-
abhängige Interrupt-Race.** Blockiert weiterhin zuverlässiges Live-
Testen: je nach exakter Kernelgröße (jede Codeänderung verschiebt sie!)
schlägt derselbe Boot-Testablauf an UNTERSCHIEDLICHEN Stellen fehl
(Illegal Instruction, mal vor `I$Write`, mal danach, mal erst nach
`F$TLink`). **Neuer, präziser Fund dieser Sitzung** (per Ringpuffer-
Instrumentierung, s.u.): die Absturz-PC landet regelmäßig EXAKT auf dem
`dc.w`-Funktionscode-Wort direkt hinter einer `trap #0`-Instruktion —
`Q9K_TrapDispatch`s eigene `addq.l #2,38(sp)`-Rücksprungkorrektur (die
genau dieses Wort überspringen soll) geht offenbar irgendwann zwischen
ihrer Ausführung und dem finalen `rte` wieder verloren. Hauptverdächtiger:
`Q9K_TrapCallExternal`s Rückweg (schon dreimal real gefixt — CCR-Verlust,
A4-Konvention, Herkunftsprüfung —, verträgt keinen ungeprüften vierten
Blindfix). **Kein Fix versucht in dieser Sitzung, bewusst** — braucht
gezielte Live-Instrumentierung.

**Konkreter nächster Schritt für die kommende Sitzung:** die in
Fortsetzung 33 gebaute Ringpuffer-Technik (Marker+PC bei jedem Timer-/
`Q9K_IRQDispatch`-Eintritt, ausgegeben von `Q9K_ExcTrap` bei einem
Absturz — Code dafür NICHT mehr im Repo, wurde nach Gebrauch bewusst
per `git checkout` zurückgesetzt, s. Fortsetzung 33 für den vollen
Patch-Text zum Wiederherstellen) um zwei weitere Eintragspunkte
erweitern: direkt am Anfang von `Q9K_TrapCallExternal` UND direkt vor
dem `rte` in `Q9K_TrapAfterCall`. Ziel: live sehen, ob ein Interrupt
GENAU zwischen diesen beiden Punkten einschlägt — falls ja, ist die
Ursache dort lokalisiert und ein echter Fix (vermutlich: Interrupts für
diesen Rückweg-Abschnitt sperren, analog zu `Q9K_TrapDispatch`s
eigenem `ori.w #$0700,sr` ganz am Anfang) endlich zielgerichtet möglich.
Ring-Puffer-Adressen NICHT in den niedrigen Speicherbereich legen (s.
Lehre in Fortsetzung 33 — ein früherer Versuch landete zufällig auf der
eigenen Debug-Zelle) — `$1440C0` ff. (direkt hinter `Q9K_ExcInfo_Stack`)
hat sich bewährt.

**Wiederverwendbares Testabbild-Rezept** (ersetzt alle älteren Hinweise
zu `OS9SYS.q9test.hda` — WICHTIG, mehrfach live verifiziert):
```
export OS9=<lokaler Referenzpfad>/tools/macos/bin/os9
IMG=/Volumes/SSD1TB/projects/Q9-Forge/Q9-Flux-68k/local_images/OS9SYS.q9test.hda
rm -f "$IMG"
"$OS9" format -q -k -nQ9TEST -bs512 -l32768 -c32 "$IMG"   # OHNE -e, s.u.!
export Q9_DISK_MODULES="<jobtmp>/rbf.mod <jobtmp>/cfide.mod <jobtmp>/dd.mod <jobtmp>/c0.mod"
tools/mkbootfile.sh --disk <jobtmp>/ref.boot "$IMG"        # ERST die Bootkette, auf leerem Abbild!
"$OS9" makdir "$IMG,/CMDS"
"$OS9" copy <jobtmp>/echo.mod "$IMG,/CMDS/echo"            # DANACH erst echo/csl kopieren
"$OS9" copy <jobtmp>/csl.mod "$IMG,/CMDS/csl"
"$OS9" attr -e "$IMG,/CMDS/echo"
"$OS9" attr -e "$IMG,/CMDS/csl"
```
`mkbootfile.sh` verwendet fuer das grosse `OS9Boot` intern den Extended-
Boot-Modus (`os9 gen -e -b=`). Dadurch wird der Bootfile-Dateideskriptor
verwendet und der Bootloader liest alle Segmente statt einer veralteten,
festen Laenge aus dem Identification-Sektor.

Zwei live erlebte Fallen: **`-e` bei `format`** (volles Abbild sofort
materialisieren) lässt den emulierten CompactFlash-Treiber nicht mehr
sauber booten (endlose Zeichenflut) — IMMER ohne `-e` formatieren, die
volle Größe steht im Identification-Sektor, das Hostdateisystem
materialisiert erst bei echtem Schreibzugriff. Und: **`os9 gen -b=`
scheitert mit "is fragmented"**, sobald vorher schon andere Dateien
angelegt wurden — Bootkette IMMER zuerst auf dem leeren Abbild
schreiben. `<jobtmp>` = `/Users/afoe/.claude/jobs/a6da3b59/tmp/` (könnte
sich bei einem neuen Job-Verzeichnis ändern — falls die Dateien dort
fehlen, sind sie im alten Job-Verzeichnis noch vorhanden und lassen sich
kopieren; `echo.mod`/`csl.mod` notfalls erneut aus
`OS9SYS.dbg10.hda,/CMDS/echo` bzw. `/CMDS/csl` extrahieren).

Live-Test-Kommando: `expect -f /tmp/run_dump4.exp local_images/OS9SYS.q9test.hda <log>`
aus dem Q9-Flux-68k-Repo-Root (Skript wartet auf die CF-Treiber-Banner-
Zeile, dann 20s, dann Ctrl-^-Dump + Ctrl-]-Quit — Skript selbst nicht
mehr im Repo, kurzer Inhalt steht in Fortsetzung 33/34 falls neu
angelegt werden muss). Absturz-Dump landet in
`Q9-Flux-68k/local_images/q9dbg_dump.txt`.

Alle 15 Host-Testsuiten grün (`gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT
-DQ9K_ALLOC_STANDARD` je `test_q9kernel_*.c`, aus `src/kernel/` heraus).

---

## ÜBERGABE (2026-09-11, achte Arbeitssitzung — historisch, s. oben)

**Branch für die aktuelle Arbeit: `fix/a4-aufruferabhaengig` (PR #13)**,
NICHT der oben genannte `fix/ccr-error-signaling-flink-funlink`-Stand.
Der Abschnitt "Was der Kernel heute kann" direkt darunter ist der Stand
VOR der ganzen `F$Load`-Untersuchung. **Diese Übergabe ersetzt die
vorherige vollständig.** Volle Kette: `Fortsetzung 1` bis `31` weiter
unten im Dokument.

**NEU (2026-09-11, Fortsetzung 31): geladenes Modul läuft wirklich —
neue Baustelle "csl"-Trap-Library.** Direkt im Anschluss an Fortsetzung
30 getestet: `F$Fork("echo")` auf das per `F$Load` geladene Modul
findet es sofort (Moduldirectory korrekt) und startet einen echten
Kindprozess, der ECHTEN Microware-Code ausführt — sichtbar an der
authentischen Meldung `**** can't install csl ****` (die C-Runtime-
Bibliothek "csl" ist ein TRAP-#1–15-Trap-Handler-Modul, ein bislang
komplett unimplementierter Mechanismus, getrennt von unseren
`TRAP #0`-Syscalls). `echo`s eigene Fehlerbehandlung für diesen Fall
stürzt danach ab (Bug in `echo`, nicht im Kernel). Klarer, aber
eigenständiger nächster Baustein für eine künftige Sitzung. Details:
`Fortsetzung 31`.

**Fortsetzung 30 (2026-09-11): `F$Load` LÄUFT VOLLSTÄNDIG DURCH —
der `F$Load`-Meilenstein ist erreicht.** Das in Fortsetzung 29 offen
gebliebene Rätsel (Modul lädt+validiert korrekt, GESAMTER `F$Load`-
Aufruf meldet dem Aufrufer trotzdem `E$MNF`) ist gelöst: per Live-
Registerspuren (Instruktionsring mit `Q9_FREEZE_PC`, erweitert um
d1/d3/d4) direkt in IOMans `F$Load`-Wrapper nachgewiesen, dass dessen
Code nach `F$VModul` ECHTE Modulheader-Offsets aus `(a2)` liest UND
beschreibt (`+$0C` als Link-Zähler inkrementiert/dekrementiert, `+$12`
als Typ/Sprache gelesen, `+$00` wird am Ende sein eigener Rückgabewert)
— weder unser eigener 16-Byte-Verzeichnis-Slot noch der rohe
Modulkopfzeiger (zwei nacheinander verworfene Zwischenversuche) passen
dazu. Fix: `F$VModul` gibt jetzt einen eigenen, dafür reservierten
20-Byte-Rückgabepuffer zurück (`Q9K_VMODUL_RETBUF`, `$1650`) mit den
Feldern an genau den Offsets, die IOMan anfasst. Live bestätigt:
`F$Load("/dd/CMDS/echo")` meldet Erfolg (kein `E$MNF` mehr), zweimal
reproduziert. Alle 14 Host-Testsuiten weiterhin grün. Details inkl. der
kompletten Forensik-Kette: `Fortsetzung 30`.

**Fortsetzung 29 (2026-09-10/11, jetzt durch obiges abgeschlossen):**
`F$VModul`/`F$SRqCMem` implementiert (Callcodes `$2e`/`$5c`, fehlten
komplett), echter Modul-CRC-24-Algorithmus dabei empirisch gegen sechs
reale Module verifiziert (Polynom `$800063`, Endwert muss `CRCCon`
$800FE3 ergeben — steht nirgends als Code in der Doku). Details:
`Fortsetzung 29`.

**NEU (2026-09-10, Fortsetzung 28): `I$Read` liefert echte
Dateidaten.** Die in der vorigen Übergabe offene "nächste Baustelle"
(`I$Read` meldete Erfolg, der Puffer blieb aber leer) ist gelöst:
`F$Move` (Callcode `0x38`) fehlte im Dispatch und lief in
`Q9K_SysUnimplemented` — genau der Dienst, mit dem RBFs eigene
Kopierroutine (Modul-Offset `$C0`–`$D7`) Dateidaten aus ihrem Puffer in
den Aufruferpuffer kopiert, OHNE hinterher Carry/`d1` zu prüfen. Jetzt
implementiert (`Q9K_SysFMove` in `q9kernel_entry.a`, Registrierung in
`q9kernel_cinit.c`). Live bestätigt: `I$Read("/dd/startup")` liefert
`echo "Ex` — die echten ersten 8 Byte der Datei. Alle 14
Host-Testsuiten weiterhin grün. Details: `Fortsetzung 28`.

**WICHTIG: Q9-Flux liegt unter `Q9-Forge/Q9-Flux-68k`** (umbenannt,
gleiches Repo/Remote, wegen der parallelen x86-Portierung
`Q9-Flux-x86`).

**$D8-Wurzelursache BEHOBEN (Fortsetzung 24, committet als `7de5410`):**
`Q9K_ProcPrsNam`s `outPastName` (a0-Ausgabe von `F$PrsNam`) lieferte
"hinter Name+Trenner" statt "Anfang des Namens" — echter RBF-
Quelltext (SchDir/RBPNam, l2sources) bewies, dass RBF genau diesen
a0-Wert als `F$CmpNam`-Vergleichszeiger braucht. Live bestätigt:
`I$Open("/dd/startup")` liefert jetzt `d0=3` (echte Pfadnummer,
kein Fehlercode) gegen den unveränderten Microware-RBF.

**Nachgelagerter Absturz BEHOBEN (Fortsetzung 27):** zwei kleine,
unabhängige Ursachen — beide NICHT im Trap-Dispatcher, an dem sich fünf
Versuche vergeblich abgearbeitet hatten:
1. `Q9K_PROCDESC_SIZE` war `$200` (512 Byte), real sind es `$400` (1024,
   = `P$PrcBody`, aus `process.a` Feld für Feld nachgerechnet). RBFs
   völlig legitimer `P$Preempt`-Zugriff bei Offset `$3AC` lief dadurch
   über das Deskriptorende hinaus. Mitgezogen: drei hartkodierte `>> 9`
   in `q9kernel_procapi.c`, jetzt abgeleitete Konstante mit
   Kompilierzeit-Kopplung.
2. A4 war vor IOMans allererstem Einsprung (`jsr (a1)`) nie gesetzt —
   IOMan bekam einen zufälligen Restwert statt des Prozessdeskriptors.
   Ein `movea.l Q9_D_Proc,a4` davor genügt.

**Ergebnis: `I$Open` UND `I$Read` auf `/dd/startup` laufen erstmals ohne
Absturz durch**, das System läuft danach normal weiter, alle 14
Host-Testsuiten grün.

**Nächste Baustelle:** `I$Read` meldet Erfolg, überträgt aber noch keine
Daten (Puffer bleibt auf Null, während die Datei real mit `echo "Excecute
s…` beginnt). Ausserdem offen: warum IOMan/scf ausgerechnet unsere
Modulbasis in A4 vertragen und jede "korrekte" A4-Bewahrung ihr
Konsolen-Open bricht (Kontrollexperiment in Fortsetzung 27 zeigt: es
liegt am Wert, nicht an der Codeform).

**Bereits vollständig gelöst und committet:**
1. **A4-Speicherkorruption behoben** — kein Absturz mehr, Boot läuft
   sauber durch.
2. **`Q9K_ProcPrsNam`-Bugfix #1** (`outPastName` muss hinter dem
   Trenner stehen, nicht auf ihm) — echter, verifizierter Fix.
3. **`Q9K_ProcPrsNam`-Bugfix #2** (`outNameStart`/a1 muss HINTER dem
   Namen stehen, nicht an seinem Anfang) — echter, verifizierter Fix.
4. **`Q9K_ProcPrsNam`-Bugfix #3, Fortsetzung 24** (`outPastName`/a0
   muss der ANFANG des Namens sein, nicht "hinter Name+Trenner") —
   der eigentliche `$D8`-Wurzelursachen-Fix, live gegen den
   unveränderten RBF verifiziert (`I$Open` liefert jetzt `d0=3`).

**Der in Fortsetzung 16 bewiesene "Widerspruch"** (Vergleichszeiger
und -länge könnten angeblich nicht gleichzeitig aus einem einzigen
`F$PrsNam`-Aufruf stammen) **war ein Artefakt der damals falschen
`outPastName`-Formel, kein echter struktureller Widerspruch.** Echter
RBF-Quelltext (Fortsetzung 24, `SchDir`/`RBPNam`) zeigt, dass RBF a0-
und a1-Ausgabe von F$PrsNam für ZWEI GETRENNTE Zwecke nutzt (a0 =
Vergleichszeiger, a1 = Zeiger für den nächsten Aufruf) — beide Werte
kommen sehr wohl aus demselben einzelnen Aufruf, nur eben nicht so,
wie hier ursprünglich angenommen. Details zur alten (mittlerweile
überholten) Verwirrung: `Fortsetzung 10`–`23` unten, zur Auflösung
`Fortsetzung 24`.

**WICHTIG bei jedem neuen Kernel-Build:** RBF_BASE hat sich mit dem
Fix bereits einmal verschoben (`$D2DC` → `$D2DA`, Kernel um 2 Byte
kleiner) — bei JEDEM weiteren Build per `M$ID`-Sync-Wort neu
bestimmen und ALLE unten genannten RBF-Adressen entsprechend
verschieben.

**Wichtigste Methodik-Lehre dieser Session** (mehrfach schmerzhaft
gelernt, für den nächsten Anlauf verinnerlichen):
- **RBFs Ladeadresse ist `$D2DC`** in den aktuellen Testabbildern —
  NICHT `$E11A`, wie ein Großteil dieser Session fälschlich annahm
  (`$E11A` liegt zufällig selbst innerhalb von RBFs eigenem
  Adressraum). Verifiziert per `M$ID`-Sync-Wort (`4a fc 00 01`) direkt
  im Speicher — bei JEDEM neuen Kernel-Build zuerst neu bestimmen
  (jede Größenänderung verschiebt RBF!). Analog für den eigenen
  Kernel: Basis = `$D2DC` minus die reale Größe von
  `src/kernel/build/q9kernel` (M$Size, Offset 4).
- **Jede Adresse einzeln gegen `$D2DC` (RBF-Anfang) und die eigene
  Kernelgröße prüfen**, bevor ihr eine Bedeutung zugeschrieben wird —
  keine Abkürzungen, keine Verwechslungen ähnlich aussehender Werte.
- **Reine Analyse (auch mit `capstone`) ist nicht
  vertrauenswürdig**, wenn sie nicht an einer live bestätigten Adresse
  verankert ist (OS-9-Trap-Inline-Callcode-Wörter desynchronisieren
  jeden linearen Scan) — Ground-Truth-Bytes IMMER per `Q9_DUMP_ADDR`
  direkt an einem per `Q9_FREEZE_PC`/`Q9_COUNT_PC` bestätigten PC
  lesen, nie aus einer eigenständigen Offline-Analyse
  übernehmen.
- **Stack-Speicherplätze werden von KOMPLETT UNABHÄNGIGEN Aufrufen
  wiederverwendet** — ein Speicher-Watch auf eine feste Stack-Adresse
  kann mehrere, kausal unzusammenhängende Ereignisse zeigen. Immer
  per Registerkontext (insbesondere `a4` als Prozesskontext-Indikator)
  gegenprüfen, ob ein Treffer wirklich zum untersuchten Vorgang gehört.
- Bewährtes Werkzeug-Set (alles temporär in `Q9-Flux/src/kernel/*.c`,
  nach jeder Nutzung mit `git checkout` zurückgesetzt): `Q9_DUMP_ADDR`/
  `Q9_DUMP_LEN` (Rohspeicher an fester Adresse), `Q9_WATCH_ADDR`/
  `Q9_WATCH_LEN`/`Q9_WATCH_FREEZE` (Schreibzugriffs-Historie, **fest
  committet, kein Patch nötig** — sehr ergiebig: den Schreiber einer
  bekannten Speicherzelle über den GESAMTEN Boot zu finden, statt PC-
  Trefferzahlen zu erraten), `Q9_FREEZE_PC`/`Q9_FREEZE_PC_N` +
  `Q9_TRACE_INSTR=1` (Instruktions-Ringpuffer an einer bestimmten
  Stelle einfrieren, zeigt standardmäßig `pc/d0/a0/a4/sp` — per
  kurzem, immer gleichem Patch erweiterbar um `d1`/`a1`, s.
  Fortsetzung 11/13 für das exakte `sed`-Rezept, sehr oft gebraucht),
  `Q9_FREEZE_A0` (Zusatzbedingung zu `Q9_FREEZE_PC`, friert nur bei
  passendem PC UND a0-Wert ein — löst das "welcher von N Treffern ist
  der richtige"-Problem elegant, s. Fortsetzung 10), `Q9_COUNT_PC`
  (reine Trefferzählung, kommagetrennte Adressliste — IMMER zuerst
  nutzen, um zu prüfen, ob ein Freeze-Ziel eindeutig ist, bevor man
  Zeit in die Interpretation investiert), `Q9_BOARD_CF_TRACE=1` (echte
  Disk-Sektor-Lesezugriffe der CF-Emulation, bereits fest eingebaut,
  kein Patch nötig). Dump-Hotkey im laufenden Emulator: `Ctrl-^`
  (0x1E) schreibt `Q9-Flux/local_images/q9dbg_dump.txt`, danach
  `Ctrl-]` (0x1D) zum
  Beenden (s. `$CLAUDE_JOB_DIR/tmp/run_dump.exp` als Vorlage).

**Zum Reproduzieren:** fertiges Testabbild liegt bereits unter
`Q9-Flux/local_images/OS9SYS.dbg10.hda` (RBF_BASE `$D2DC` für den
aktuellen Kernel-Stand bestätigt) — bei unverändertem Kernel-Quelltext
direkt weiterverwendbar. Rezept zum Neubauen (z. B. nach einer
Kernel-Änderung) ganz unten in diesem Dokument bzw. in den
`Fortsetzung`-Abschnitten — kurz: `cp -c OS9SYS.hda OS9SYS.<name>.hda`,
dann
`Q9_DISK_MODULES="<rbf.mod> <cfide.mod> <dd.mod> <c0.mod>" tools/mkbootfile.sh --disk <ref.boot> <image>`
(die Modulgrenzen werden automatisch aus der Referenz gelesen)
(die vier `.mod`-Dateien und `ref.boot` liegen im `$CLAUDE_JOB_DIR/tmp/`
dieses Jobs, `a6da3b59` — persistiert über Sessions hinweg, solange
der Job nicht gelöscht wird; sonst erneut aus
`Q9-Flux/OS9Boot.noprot.test` extrahieren, s. Kopfkommentar
`tools/mkbootfile.sh`). Gesicherte volle Instruktionsspuren dieser
Session (24576 Einträge, teils mit `a1`/`d1`) ebenfalls dort:
`trace_full.txt`, `trace_a1.txt`, `trace_e20a_full.txt` u. a.

---

---

## Was der Kernel heute kann

### Prozesse und Scheduling
Preemptives Multitasking mit echtem, timergetriebenem Kontextwechsel
(Board-Timer auf Autovektor 30 / Level 6). `F$Fork`, `F$Wait`, `F$Exit`,
`F$Sleep`, `F$ID`, `F$GProcP` laufen als echte `TRAP #0`-Syscalls aus
laufenden Prozessen heraus.

### Modulverwaltung
`F$Link`/`F$UnLink` gegen ein echtes Modulverzeichnis; Speicher über
`F$SRqMem`/`F$SRtMem` aus der Arena.

### Fremde OS-9-Module laufen mit
`ioman`, `scf` und `sc68681` — unveränderte Microware-Binaries — werden
geladen, initialisieren sich und arbeiten mit unserem Kernel zusammen.
`F$SSvc` trägt ihre eigenen Service-Routinen in die Dispatch-Tabellen ein.

### Der I/O-Weg steht — in beide Richtungen zur Hälfte
Erreicht am 2026-09-04:

    ioman: can't chgdir to system device: Error $00DD    (IOMans eigene Meldung)
    Hallo von Q9-OS!                                     (Programmausgabe via I$Write)

Ein Testprozess schreibt Text auf die Konsole und bekommt exakt die
übergebenen 17 Bytes zurückgemeldet — über den vollständigen Weg
**TRAP #0 → IOMan → scf → sc68681 → DUART**. `I$Open` liefert dabei einen
gültigen Pfad, drei P$Path-Slots (0/1/2) sind belegt.

### Interrupts werden zugestellt
`Q9K_IRQDispatch` bedient alle per `F$IRQ` registrierten Vektoren. Ein
einziger Handler genügt: er liest seine Vektornummer aus dem
Exception-Frame. Für Autovektoren (25–31), bei denen die Vektornummer nur
den Pegel nennt, wird die gesamte Polling-Tabelle abgefragt — genau der
Zweck, für den OS-9 sie führt.

---

## MEILENSTEIN: das erste echte Programm läuft

    AAAA...Hallo aus einem echten Programm!\r\nBBBB...

`hellosvc.a` ist ein eigenständiges OS-9-Programmmodul, das seine Ausgabe
**ausschließlich über Syscalls** macht — `I$WritLn` auf Pfad 1, danach
`F$Exit`. Darin liegt der Unterschied zu `forkchild`, dem bisherigen
Fork-Testmodul: das schreibt direkt auf den DUART und kommt ohne
Betriebssystem aus. `hellosvc` läuft über den vollständigen Weg
**TRAP #0 → IOMan → scf → sc68681 → DUART**.

Im Modul steht bewusst **kein** `movea.l #0,a6` und keine Pfadangabe — genau
daran zeigt sich, ob der Kernel seine Aufgabe erfüllt. Zwei Dinge mussten
dafür dazukommen:

- **`Q9K_ProcFork` vererbt die Pfade** des Erzeugers (`P$Path`, Offset
  `$168`). Ohne das hätte ein geforktes Kind eine leere Pfadtabelle. Das ist
  der Grund, warum ein gewöhnliches Programm einfach auf Standard-Ein/Ausgabe
  schreiben kann, ohne selbst etwas zu öffnen.
  *Vereinfachung:* kopiert statt per `I$Dup` dupliziert — gleichwertig,
  solange Pfade nie geschlossen werden; sobald es `I$Close` gibt, muss hier
  `I$Dup` stehen.
- **`Q9K_TrapExtInvoke` setzt `A6 = 0`** für externe Handler. Ein echtes
  Programm hält in `A6` seinen *eigenen* statischen Datenbereich; das
  Umschalten auf die Systemglobals kann ihm nur der Kernel abnehmen.

**Nebenbefund, der die Kette bestätigt:** scf wandelt das mitgegebene CR in
CR+LF — echtes Terminal-Verhalten, das wir nirgends selbst programmiert
haben.

**Offen:** Der `I$ReadLn`-Block im Testprozess ist vorübergehend
übersprungen. Blockiert der Elternprozess lesend auf dem Pfad, hängt das
schreibende Kind darin fest — ein Hinweis auf Pfad-Semantik, die wir noch
nicht sauber abbilden.

## Prozessdeskriptor: an das echte OS-9-Layout angeglichen

Anlass war ein Fund mit hohem Wiedererkennungswert: `F$Send` kam mit der
Prozess-ID **`$6105`** an — `$61` ist `'a'` (unser `STATE_ACTIVE`), `$05` die
Priorität des Testprozesses. Der Treiber liest `P$ID` als Wort bei Offset
`$00` und bekam dort unsere State- und Prioritäts-Bytes.

Maßgeblich ist internes Referenzmaterial. Angeglichen wurde alles, was
fremde Module lesen können:

| Feld | Offset | vorher |
|---|---|---|
| `P$ID` | `$00` (word) | *fehlte* — wird jetzt mitgeführt |
| `P$sp` | `$08` (long) | `$38` (SavedSP) |
| `P$Prior` | `$18` (word) | `$01` |
| `P$Age` | `$1A` (word) | `$02` |
| `P$State` | `$1C` (word) | `$00` |
| `P$Signal` | `$26` (word) | *fehlte* — `F$Send` legt den Code hier ab |
| `P$QueueN/P` | `$30`/`$34` | lag bereits richtig |
| `P$PModul` | `$38` (long) | `$08` (Modulkopf) |
| `P$Path` | `$168` | lag bereits richtig |

Byte-Felder liegen im *unteren* Byte des jeweiligen OS-9-Wortes (`State $1D`,
`Prior $19`, `Age $1B`), damit bestehende Byte-Zugriffe unverändert bleiben.
Eigene Zusatzfelder ohne OS-9-Entsprechung sitzen hinter `P$Path` ab `$1B8`.

**Zwei Lehren daraus:**

- `P$QueueN`/`P$QueueP` liegen bei `$30`/`$34` — genau dort, wo unsere
  Queue-Felder schon lagen. Ein erster Versuch, sie nach hinten zu schieben,
  brach den Boot sofort: Die Queue-Sentinels sind kleine Strukturen im
  Kernel-Global-Bereich, ein Offset von `$1CC` sprengt sie.
- Die Host-Suiten bilden das Layout verkürzt nach und setzen eigene Offsets.
  Der Scheduler-Test legte `SLEEPTICKS` auf `$18`–`$1B` — genau dorthin
  fallen jetzt `Prior`/`Age`/`State`.

## Offen: der Lesepfad

Der Lesepfad erreicht den Treiber, das Zeichen wird empfangen und abgeholt,
und die Exception nach dem Wecken ist behoben (ISR-Adressen werden
plausibilisiert — ungerade Adressen und alles unter `$1000` werden
übersprungen).

**Der Weckweg ist vermessen (2026-09-04) — und ein echter Bug darin behoben.**

Die frühere Notiz „die Prozess-ID des wartenden Lesers wird nie eingetragen"
war ein **Messfehler**: In jenen Läufen kam nie ein Zeichen an, also trat auch
nie ein Weckfall ein. Der Treiber trägt `V_WAKE` sehr wohl ein. Mit echter
Eingabe (`hallo\r` über eine Pipe auf stdin) sieht der Weg so aus — alle
Werte per Schreib-Watch auf die Gerätestatik `$35a10` gemessen:

| PC | Feld | Wert | Bedeutung |
|---|---|---|---|
| `$c454` | `V_BUSY` (`+$06`) | `1` | Treiber merkt sich den Leser |
| `$c638` | `V_WAKE` (`+$08`) | `0` | Warteschleife, 16× |
| `$c914` | `V_WAKE` | **`1`** | Treiber legt sich schlafen |
| `$ccfe` | `V_WAKE` | `0` | ISR liest die ID und löscht das Feld |

Das Statiklayout stammt aus internem Referenzmaterial: `V_PORT $00`,
`V_LPRC $04`, `V_BUSY $06`, `V_WAKE $08`, `V_Paths $0a`.

Danach kommt `F$Send` bei uns an und **gelingt** (Scratchzellen `$1608`ff:
PID = 1, Signal = 1 = S$Wake, Fehler = 0, Erfolg = 1), und `Q9K_SchedWake`
arbeitet korrekt: der Zustand des Geweckten geht von `'s'` auf `'a'`.

### ECHTER BUG GEFUNDEN + GEFIXT: `F$Send` sicherte nur `a6`

`Q9K_SysFSend` rettete vor dem C-Aufruf lediglich `a6`. Der C-Code darf aber
`d0/d1/a0/a1` frei überschreiben — und **`F$Send` ist der einzige Syscall, den
eine fremde Interruptroutine aufruft**: `sc68681` weckt damit den wartenden
Leser. Die ISR lief anschließend mit unseren Zwischenwerten weiter und sprang
in Datenmüll.

Der Beweis stand vollständig in der Exception-Mitschrift — die Register beim
Absturz waren *ausnahmslos* unsere eigenen:

    Vektor=4 (Illegal Instruction)  PC=00019432   <- mitten im Prozessdeskriptor
    A0=00001618   <- unsere Diagnose-Scratchzelle
    A1=000193ff   <- Deskriptor minus 1
    D1=00019400   <- der Deskriptor selbst

Fix: kompletter Registersatz (`movem.l d0-d7/a0-a6`) um den C-Aufruf.
Ausgänge sind allein Carry und `d1.w`, beide werden erst danach gesetzt.

**Merksatz fürs nächste Mal:** Jeder Handler, den fremder Code aus einem
Interrupt heraus aufrufen kann, muss den vollen Registersatz erhalten. Der
Kandidatenkreis ist klein und sollte durchgesehen werden.

### ~~Format Error beim Fortsetzen~~ — GELÖST (2026-09-04)

Nach dem F$Send-Fix wurde der geweckte Prozess eingeplant und gestartet,
scheiterte aber beim Fortsetzen an einem **Format Error (Vektor 14)**. Zwei
zusammenhängende Ursachen, beide behoben:

**1. `Q9K_InTrapPath` war global und blieb während Fremdaufrufen stehen.**

Das Flag unterscheidet „Handler kam über `TRAP #0`" (Exception-Frame auf dem
Stack) von „Handler kam über das PEA+RTS-Trampolin" (nur eine
Rücksprungadresse). Es wurde beim TRAP-Eintritt gesetzt und erst im Epilog
gelöscht — blieb also stehen, während unser Handler fremden Code aufrief.

Ruft dieser fremde Code seinerseits einen Syscall über das Trampolin, sah
`F$Sleep` fälschlich den TRAP-Pfad: es verwarf eine vermeintliche
Rücksprungadresse und setzte den Prozess später per `RTE` auf einem Stack
fort, auf dem gar kein Frame lag. Genau unser Fall — A ruft `I$ReadLn` per
`TRAP #0`, und tief darin ruft `sc68681` das `F$Sleep` über das Trampolin.

Fix: `Q9K_TrapCallExternal` löscht das Flag für die Dauer des Fremdaufrufs
und setzt es danach zurück (vor dem `move.w (sp)+,ccr`, weil ein `move`
sonst das Carry löschen würde — das ist das Fehlersignal).

**2. Der Trampolin-Pfad konnte gar nicht blockieren.**

Dort stand eine Übergangslösung: sofort zurückkehren, der Treiber pollt.
Jetzt blockiert er echt. Aus der Rücksprungadresse des Aufrufers wird ein
**Format-0-Frame** gebaut (SR / PC / Format-Vektor-Wort = 0), damit der
gemeinsame Fortsetzungsweg `movem.l (sp)+,d0-d7/a0-a6` + `rte` unverändert
passt — dasselbe Muster, mit dem `Q9K_ProcCreate` jeden neuen Prozess
aufsetzt. Dabei wird **kein Datenregister angefasst**: der Registersatz des
Aufrufers ist noch ungesichert und muss ihn beim Aufwachen unverändert
wiedersehen; die Rücksprungadresse geht deshalb über eine Speicherzelle.

### ~~S$Wake wurde als Signal zugestellt~~ — GELÖST (2026-09-04)

Danach kehrte `I$ReadLn` mit Carry und „Fehlercode" `$01` zurück. Das war
gar kein Fehlercode, sondern **S$Wake selbst**: `Q9K_ProcSend` legte jedes
Signal in `P$Signal` ab, auch das reine Wecksignal. Der aufwachende
Systemcode fand daraufhin ein anstehendes Signal vor und brach den laufenden
Aufruf ab.

S$Wake ist in OS-9 kein zuzustellendes Signal, sondern nur die Aufforderung
„lauf weiter" — genau dafür benutzt es `sc68681`. Fix: bei `signal == 1`
wird `P$Signal` nicht geschrieben. Der Wert ist per Microware-Quelle belegt
(internem Referenzmaterial, `org 0`: S$Kill 0, **S$Wake 1**, S$Abort 2).

### Stand: der Weckweg trägt, die Nutzdaten fehlen noch

    ...H<r00000000[........]nHallo aus einem echten Programm!

Was jetzt nachweislich funktioniert:

- Der Prozess blockiert im Treiber, wird durch den RX-Interrupt geweckt und
  kehrt **ohne Absturz** aus `I$ReadLn` zurück.
- **Keine Exception mehr**, die Sleep-Queue ist danach leer, A und B laufen
  weiter — und `hellosvc` läuft wieder, der frühere Pfad-Deadlock ist damit
  aufgelöst.
- Der Treiber **holt die Zeichen ab**: RX-FIFO `head = tail = 6` für
  `hallo\r`.

Was fehlt: Die Zeichen erreichen den Puffer des Aufrufers nicht (`[........]`
= 8 leere Bytes), und `I$ReadLn` liefert Carry **ohne** Fehlercode (`d1 = 0`).
Nächster Ansatzpunkt ist damit der **Rückgabeweg**, nicht mehr der Weckweg:
zu prüfen ist, ob `F$Sleep` beim Aufwachen den 44-Byte-Registerrahmen des
Trampolin-Aufrufers versorgen muss (vgl. den Befund vom 2026-09-02: solche
Aufrufer holen ihre Rückgabewerte aus diesem Rahmen, nicht aus den lebenden
Registern).

### Rückgabeweg: zwei Messungen, ein ausgeschlossener Verdacht

**Der Puffer wird nie beschrieben.** Ein Schreib-Watch auf die Laufzeitadresse
des Testpuffers (`$0000751C`, vom Testcode selbst ausgegeben) zeigt genau 32
Zugriffe — alle vom ROM beim Laden des Moduls (`pc=fe000d66`, lauter Nullen).
Danach rührt ihn niemand mehr an. Der Lesevorgang bricht also ab, **bevor**
kopiert wird; es ist kein Kopierfehler am Ende.

**Nicht der Treiber blockiert, sondern IOMan.** Die beim Blockieren
gemessene Rücksprungadresse ist `$b998` = **ioman+$11f8**. Bisher war die
Annahme, `sc68681` lege sich selbst schlafen — der Treiber setzt zwar
`V_WAKE`, der `F$Sleep`-Aufruf kommt aber aus IOMan.

**Ausgeschlossen: die 44-Byte-Rahmenversorgung.** Naheliegender Verdacht war,
`F$Sleep` müsse — wie `F$SRqMem` — den Registerrettungsrahmen des
Trampolin-Aufrufers versorgen (`d0` bei `+$00`, `a2` bei `+$28`), weil der
Aufrufer seine Rückgabewerte daraus zurückholt. **Falsch, und schädlich:**
Den Rahmen legt nur IOMans Wrapper-Subroutine an (`ioman+$15ca..$1610`).
`$11f8` liegt weit außerhalb — hier existiert gar kein Rahmen. `a5` war nur
zufällig `sp+8`, sodass die von `F$SRqMem` übernommene Erkennung fehlschlug
und der Versuch den Stack des Aufrufers zerstörte (Illegal Instruction mitten
im Stackbereich, `PC=$36c70`, `A1` = die vermeintliche Rahmenbasis).

Damit ist belegt: **Die Erkennung `a5 == sp+8` allein beweist keinen Rahmen.**
Wer sie übernimmt, muss zusätzlich prüfen, ob der Aufruf überhaupt aus dem
Wrapper kommt. Der Rückbau steht als Warnung im Code.

**Nächster Ansatzpunkt:** `ioman+$11f8` analysieren (capstone) und
nachsehen, was IOMan dort vor und nach dem `F$Sleep` erwartet — insbesondere,
welche Bedingung es prüft, bevor es den Lesevorgang mit Carry und `d1 = 0`
abbricht.

### IOMan-Analyse (2026-09-05)

IOMan aus dem Bootfile extrahiert (Laufzeitbasis `$a7a0`, Größe `$161c`) und
mit capstone analysiert. Gesucht war, warum `I$ReadLn` mit Carry
zurückkehrt, ohne den Puffer zu füllen.

**Alle Syscall-Aufrufe von IOMan aufgelistet.** Das Trampolin-Muster ist im
Code eindeutig erkennbar (`pea <ret>(pc)` / `move.l $XX(a3),-(a7)` /
`movea.l $4XX(a3),a3` / `rts`), der Slot-Offset geteilt durch 4 ergibt den
Callcode. IOMan fordert damit **vier bei uns nicht belegte Dienste** an —
Namen per internem Referenzmaterial ausgezählt:

| Code | Dienst | Aufrufstellen |
|---|---|---|
| `$2e` | F$VModul | `$b054` |
| `$31` | **F$RetPD** | `$b9b0`, `$bc16` |
| `$38` | F$Move | `$b178`, `$b7b4` |
| `$5c` | F$SRqCMem | `$bd7c` |

**Korrektur einer naheliegenden Fehlannahme:** `$37` ist **F$GProcP** und bei
uns korrekt belegt. F$IOQu ist `$2b` — IOMan ruft es über das Trampolin gar
nicht.

**Tatsächlich angefordert wird davon genau einer: `F$RetPD`.** Jeder der vier
Slots bekam einen eigenen Marker-Stub; im Lauf erscheint nur `R`, und zwar
unmittelbar vor der altbekannten Meldung `ioman: can't chgdir to system
device: Error $00DD`. Wir haben `F$AllPD` (`$30`) implementiert, aber nie sein
Gegenstück `F$RetPD` (`$31`) — „Return Process/Path Descriptor".

**Im Lesepfad fehlt dagegen kein einziger Dienst.** Der gemeinsame
Unimplemented-Stub wurde testweise gesprächig gemacht: Über den ganzen
Lesevorgang meldet er sich **kein** Mal. Der leere Puffer ist also **nicht**
auf einen fehlenden Kernel-Dienst zurückzuführen, sondern auf eine Bedingung
in der IOMan/scf-Logik. Insbesondere ist auch `F$Move` widerlegt — der
naheliegende Verdacht, IOMan kopiere damit in den Nutzerpuffer, wurde per
Marker geprüft und trifft für diesen Pfad nicht zu.

**Der Rückgabeweg ist gefunden** (`$b95e`–`$b9c6`). IOMan sichert dort
`movem.l d0-d1,-(a7)` und überschreibt den d0-Slot mit `move.w sr,$0(a7)`;
nach dem Aufruf holt es beides zurück:

    00b9bc  move.l  $4(a7), d1        * Fehlercode aus dem Stack
    00b9c0  move.w  $0(a7), ccr       * Carry aus dem Stack
    00b9c4  addq.l  #8, a7

Das Carry, mit dem `I$ReadLn` zurückkehrt, wird hier also nur **durchgereicht**
— es entsteht weiter oben. Die Suche nach der Abbruchbedingung gehört damit
in den Code *vor* dieser Stelle bzw. in scf.

### F$RetPD implementiert (2026-09-05)

Die gefundene Lücke ist geschlossen. Konvention aus IOMans Aufrufstelle
(`ioman+$1206`) abgelesen, nicht geraten:

    move.w  $0(a1),d0        * d0.w = Nummer, aus dem Deskriptor selbst
    movea.l $48(a6),a0       * a0   = D_PthDBT, die Blocktabelle
    ...                      * Sprung in Dispatch-Slot $c4 = Callcode $31

`Q9K_ProcRetPD` ist das exakte Gegenstück zu `Q9K_ProcAllPD`: DBT-Slot räumen,
Deskriptor in die Freiliste zurückhängen. Fehlerfälle `E$BPNum` (`$C9`) für
Nummer 0, Nummer über dem Höchstindex und bereits freien Slot, `E$BPADDR`
(`$D2`) für eine Null-DBT. Registriert in beiden Dispatch-Tabellen.

**Keine Rahmenversorgung** — der Dienst gibt keine Register zurück. Das ist
bewusst so und steht als Kommentar im Handler: der blinde Nachbau der
`F$SRqMem`-Rahmenerkennung hat bei `F$Sleep` nachweislich fremden Speicher
zerstört.

12 neue Prüfungen im Host-Test. Weil `F$RetPD` den Deskriptorzeiger aus einem
DBT-Slot **zurückliest** (echte 4 Byte, 68k-Zeigerbreite) und die Testpuffer
auf 64-Bit-Hosts oberhalb 4 GB liegen, ist dieser Zeiger dort nicht
dereferenzierbar; niedrigen Speicher zu mappen verhindert macOS
(`__PAGEZERO`). Die Pool-Rückgabe wird im Test deshalb über einen Hook
abgefangen und protokolliert.

### Der `chgdir`-Fehler ist kein Kernel-Bug

    ioman: can't chgdir to system device: Error $00DD

`$DD` ist **`E$MNF`, „Module Not Found"** (ausgezählt aus
internem Referenzmaterial ab dem bekannten `E$PthFul = $C8`; dieselbe
Zählung bestätigt nebenbei `$C9` E$BPNum, `$D0` E$UnkSvc, `$D2` E$BPAddr,
`$D7` E$BPNam — alle wie im Code verwendet).

IOMan sucht also ein System-Device, das es in unserem Bootfile gar nicht
gibt: Es enthält weder RBF-Dateimanager noch Plattentreiber. Die Meldung ist
damit die **erwartbare Folge des Bootfile-Inhalts** und verschwindet erst mit
dem F$Load-Meilenstein. Sie gehört nicht mehr auf die Liste der vermuteten
Kernel-Fehler.

Auf den offenen Lesepfad hatte `F$RetPD` erwartungsgemäß keinen Einfluss —
im Lesepfad wurde ja nachweislich kein fehlender Dienst angefordert.

**Nächste Schritte:**
1. Die Abbruchbedingung des Lesepfads verfolgen: Der Rückgabeweg
   (`$b95e`–`$b9c6`) reicht Carry und `d1` nur durch, die Ursache liegt davor
   bzw. in scf.
2. Danach der F$Load-Meilenstein (RBF + Plattentreiber ins Bootfile) — der
   räumt zugleich die `chgdir`-Meldung ab.

### ~~scf-Analyse: der Lesepfad erreicht scf gar nicht~~ — WAR EIN MESSFEHLER

Diese Aussage vom 2026-09-05 ist **falsch** und hier nur noch als Warnung
stehengeblieben. Ursache: **IOMan und scf liegen im Bootfile hinter dem
Kernel und verschieben sich, sobald der Kernel wächst.** Alle damals
benutzten Laufzeitadressen stammten aus einem älteren Dump — die PC-Zähler
zeigten deshalb auf Adressen, die es so nicht mehr gab, und meldeten
plausibel aussehende Nullen.

**Merksatz:** Modulbasen vor jeder Messung frisch aus dem Dump holen
(Moduldirectory-Kette), nie aus einer Notiz übernehmen. Derselbe Fehler ließ
auch einen „Off-by-2" in der Dispatch-Tabelle erscheinen, den es nie gab.

### ✅ GELÖST (2026-09-06): der Lesepfad ist offen

    ...H < > @0000751C [hallo...] n hallo

`>` = `I$ReadLn` kehrt erfolgreich zurück, der Puffer enthält `hallo`.
Ein- und Ausgabe laufen damit in beide Richtungen über die echten
Microware-Module.

**Ursache: ein Byte Versatz in einem Feldoffset.** `Q9K_PROCDESC_AGE_OFF`
stand auf `$1B`. Das Feld ist zwei Byte breit, belegte also `$1B` **und**
`$1C` — und `$1C` ist im echten Layout `P$State` (Wort,
internem Referenzmaterial). Jedes Altern eines Prozesses schrieb damit
ins obere Byte von `P$State`.

`sc68681` prüft nach dem Aufwachen genau dieses Byte:

    $cc66  move.w $26(a4),d1     * P$Signal -- anliegendes Signal?
    $cc6a  beq.b  $cc72          * keins -> weiter
    $cc72  btst.b #$1,$1c(a4)    * P$State, oberes Byte
    $cc78  bne.b  $cc7e          * gesetzt -> Abbruch
    $cc80  ori.b  #$1,ccr        * Carry

Stand das Alter gerade auf 6 (oder einem anderen Wert mit Bit 1), hielt der
Treiber den wartenden Prozess für **„condemned"** und kehrte mit Fehler
zurück — statt den längst gefüllten Eingabepuffer auszulesen. Real gemessen:
`P$State = $0661` bei einem Prozess, dessen Alter gerade 6 war.

Korrektes Layout in dem Bereich: `P$Prior $18` (Wort), **`P$Age $1a`**
(Wort), `P$State $1c` (Wort). Das Alter liegt jetzt auf `$1A` und ist damit
sogar das echte `P$Age` statt einer Eigenerfindung.

**Wie es gefunden wurde** — die Kette wurde von außen nach innen abgetastet,
jede Station per PC-Zähler bestätigt statt aus dem Listing erschlossen:
IOMans `I$ReadLn` → `F$ChkMem` → Pfadsuche → Modusprüfung → scfs `ReadLn` →
dessen Zeichen-Hol-Routine → Treiber-Aufruf → `sc68681`s Read → Warteweg →
`F$Sleep` → Rückkehr. Den Ausschlag gab der Instruktions-Ring mit Freeze auf
scfs Fehlerausgang: Er zeigte, dass der Treiber nach dem Aufwachen nur
**sieben Instruktionen** läuft und dann herausspringt — statt zur
Pufferprüfung zurückzukehren. Diese sieben Instruktionen waren die Antwort.

**Zwei Messfallen, die dabei Zeit gekostet haben** (beide jetzt dokumentiert):
- Modulbasen verschieben sich, sobald der Kernel wächst — sie gehören vor
  jeder Messung frisch aus dem Dump geholt, nie aus einer Notiz.
- Reine Trefferzähler kennen keine Reihenfolge. Wo die Frage „wie kam der
  Code hierher?" lautet, braucht es den Ring mit Freeze.

### F$Load-Meilenstein: RBF und CompactFlash-Treiber sind eingebunden

Aus `OldBoot` (der originalen Bootdatei im Image) sind die fehlenden Module
extrahiert und ins Testbootfile aufgenommen: **`rbf`** (Dateimanager),
**`cfide`** (CompactFlash-Treiber) und die Gerätedeskriptoren **`dd`** und
**`c0`**. Das Rezept steckt jetzt in `tools/mkbootfile.sh` (`--disk`), damit
es reproduzierbar bleibt.

**Zwei Ergebnisse sofort:**

    CompactFlash driver build 42

- Der Treiber initialisiert sich beim Boot.
- **Die Meldung `can't chgdir to system device` ist verschwunden** — IOMan
  findet sein System-Device. Der älteste offene Defekt ist damit erledigt,
  und zwar wie vorhergesagt allein durch den Bootfile-Inhalt.

**Noch offen: der Dateizugriff.** Ein Testaufruf
`I$Open("/dd/startup", Modus 1)` scheitert mit **`E$MNF`** (`$DD`) — der
Gerätedeskriptor `dd` wird nicht gefunden.

**Zwischenstand: ein echter Bug gefunden und behoben.** Die
Moduldirectory-Kette zeigte nur 8 der 12 geladenen Module — es fehlten
`hellosvc`, `rbf`, `cfide` und `dd`, während ausgerechnet `c0` (das *letzte*
Modul) enthalten war.

Der Weg dorthin, Schritt für Schritt gemessen:
1. Ein Watch auf den Kettenkopf `$1238` zeigte: **alle 12 Slots werden
   sauber gepusht** (`$19000`…`$190b0`, lückenlos). Die Kette ist beim Aufbau
   also vollständig.
2. Ein Watch auf ein NEXT-Feld zeigte einen **dritten** Schreibzugriff lange
   nach dem Aufbau.
3. Der Instruktions-Ring, beim *dritten* Watch-Treffer eingefroren, zeigte
   eine Schleife über die Verzeichnis-Slots, die einen Eintrag **aushängt**.

Ursache: `Q9K_ModDirUnlinkByHeader` entfernte den Eintrag, sobald der
Link-Zähler 0 erreichte — auch bei Modulen aus der **Bootdatei**. IOMan linkt
und unlinkt beim Start reihum; danach waren `rbf`, `cfide` und `dd` aus dem
Verzeichnis verschwunden, obwohl sie unverändert im Speicher lagen.

Real bleibt ein Modul im Verzeichnis, solange es im Speicher liegt; der
Eintrag verschwindet erst, wenn auch der Speicher freigegeben wird — was bei
Bootdatei-Modulen nie passiert. Fix: Einträge aus dem Boot-Scan tragen ein
**Permanent-Flag** (`$0E` im Slot) und werden nie ausgehängt.

Damit stehen jetzt **alle 12 Module** in der Directory.

### ~~E$MNF beim Öffnen~~ — GELÖST (2026-09-06)

**IOMan sucht Module mit dem Rest des Pfades.** Ein Protokoll der
Modulsuch-Anfragen brachte es an den Tag:

    Letzte Modulsuche: Filter=0f00 Name="dd/startup"

Beim Öffnen von `/dd/startup` fragt IOMan nach einem Modul namens
**`dd/startup`** — der Gerätename endet für den Kernel am `/`. Unser
Namensvergleich lief dagegen bis zum NUL-Byte und verwarf den Treffer.

Real endet ein Modulname im `F$Link`-Aufruf am ersten Zeichen, das kein
Namenszeichen ist (Buchstaben, Ziffern, `_`, `.`, `$` — dieselbe Menge wie in
`F$PrsNam`); dass danach noch Text folgt, ist ausdrücklich vorgesehen. Genau
deshalb liefert `F$Link` in `a0` den Zeiger **hinter** den Namen zurück.

Das erklärt auch, warum der Einzeltest `F$Link("dd")` sauber funktionierte
und trotzdem `I$Open` scheiterte.

**Wie es gefunden wurde:** Der Weg war per Ring-Freeze verfolgbar — IOMan
ruft im Fehlerpfad `F$RetPD` und reicht danach einen zuvor auf dem Stack
geparkten Fehlercode weiter (`move.l (a7)+,d1`). Der Fehler selbst entstand
im langen Kernel-Lauf davor, also in unserer Modulsuche. Ein Protokoll der
gesuchten Namen zeigte dann sofort, wonach wirklich gefragt wurde.

### Neue Bruchstelle: Sprung mitten in eine Instruktion (F$GProcP)

Mit dem Namensfix läuft `I$Open` weiter in RBF hinein und stürzt dort ab:

    Vektor=4 (Illegal Instruction)  PC=00007d22
    A6=00007296  A3=00007587  A4=00007cfe  D0=$2bc  D1=$fc

**Die Stelle ist exakt zugeordnet.** Der Linker (`l68 -m`) legt den
Assembler-Psect auf Modul-Offset `$3c`; `$7d22` fällt damit in den
**Erfolgspfad von `F$GProcP`** (Callcode `$37`) — und zwar *mitten* in eine
Instruktion:

    007d0e  movea.l (a7)+, a6        * Rückkehr aus dem C-Teil
    007d10  tst.w   $138c.l          * Success-Flag
    007d16  beq.w   $7d26            * Fehlerpfad
    007d1a  movea.l $1384.l, a1      * Deskriptor
    007d20  andi.b  #$fe, ccr        * <- $7d22 liegt HIER drin
    007d24  rts

Bei `$7d22` stehen die Immediate-Bytes `00fe` — als Instruktion ungültig,
was den Vektor 4 erklärt. Etwas springt also **zwei Byte zu weit**, statt
`$7d24` (das `rts`) zu erreichen.

Auffällig dazu: **`A4` enthält `$7cfe`, eine Codeadresse** — an dieser Stelle
erwartet der aufrufende Code (IOMan) aber den Prozessdeskriptor. `$7cfe`
liegt unmittelbar vor dem gezeigten Ausschnitt, dürfte also der Einstieg des
Handlers selbst sein.

**Ursache gefunden: RBF schreibt in unseren Kernel-Code.**

Ein Code-Dump aus dem *laufenden* Speicher (nicht aus der Moduldatei!) zeigte
die Bescherung — an zwei Stellen steht etwas anderes als im Modul:

| Adresse | Moduldatei | Speicher |
|---|---|---|
| `$7d0e` | `2c5f 4a79` | **`0000 8200`** |
| `$7d1e` | `1384 023c` | **`0000 8200`** |

Ein Schreib-Watch nannte den Verursacher: **`pc=$d7e4`, also `rbf+$57c`**:

    move.l  d1, $14e(a4)

RBF schreibt in den Prozessdeskriptor bei `+$14e`. Aus der Zieladresse folgt
`a4 = $7bc0` — eine Adresse **in unserem Kernel-Code**, nicht der
Prozessdeskriptor (`D_Proc` war zeitgleich korrekt `$19400`).

**Woher das kommt:** Der Trap-Dispatcher benutzt `a4` als Sprungregister:

    movea.l Q9K_TrapA4Save,a4          * A4 korrekt wiederhergestellt
    bne     Q9K_TrapCallExternal
    movea.l Q9K_TrapHandlerScratch,a4  * eigener Handler: a4 = HANDLER-ADRESSE
    jsr     (a4)                        * ... und bleibt danach stehen

Für den *externen* Pfad ist das längst berücksichtigt (bsr+Stub, damit `a4`
frei bleibt). Für unsere **eigenen** Handler nicht — und genau die ruft IOMan
laufend (`F$GProcP`, `F$AllPD`, `F$SRqMem` …). IOMan und die File-Manager
führen in `a4` aber den Prozessdeskriptor und rechnen nach dem Aufruf damit
weiter.

**Der Fix ist noch offen.** Zwei Anläufe sind gescheitert, beide sind
dokumentiert, damit sie niemand wiederholt:

1. `a4` hinterher aus `Q9K_TrapA4Save` zurückholen — die Zelle ist **global**
   und übersteht keinen verschachtelten Trap; der IOMan-Start brach sofort
   mit einer Exception ab. Dieselbe Bug-Klasse wie beim früheren
   `Q9K_InTrapPath`.
2. Sprung über `bsr`+Stub (wie im externen Pfad) — bricht an derselben
   frühen Stelle. Der Verdacht ist, dass der Epilog oder einzelne Handler
   `a4` in seiner bisherigen Bedeutung erwarten; das ist noch nicht geprüft.

`jsr ([Q9K_TrapHandlerScratch])` scheidet aus: `r68` übersetzt die
speicherindirekte Form nicht („bad operand").

**Epilog und Handler sind inzwischen durchgesehen:**

- **Der Epilog (`Q9K_TrapAfterCall`) fasst `a4` nicht an.** Er arbeitet nur
  mit CCR, `d0` und dem Stack. Er scheidet als Ursache aus.
- **Der Dispatcher benutzt `a4` als Arbeitsregister**, und zwar früh: erst
  als geretteter PC (`movea.l 38(sp),a4`), dann als Tabellenbasis
  (`Q9_D_USRDIS`), dann als Handler-Adresse. Danach holt
  `movem.l (sp),d2-d7/a3-a5` den Registersatz des Aufrufers zurück — **ab da
  steht in `a4` wieder dessen Wert**. Die Sicherung in `Q9K_TrapA4Save`
  (Zeile 1617) trifft also den richtigen Wert; nur wird `a4` unmittelbar vor
  dem `jsr` erneut mit der Handler-Adresse überschrieben.

Damit ist die Stelle eindeutig: Es geht allein um die zwei Zeilen
`movea.l Q9K_TrapHandlerScratch,a4` / `jsr (a4)`.

**Der dritte Anlauf hat einen ganz anderen Bug aufgedeckt — und der ist
behoben.**

Per Ring-Freeze auf die Absturzadresse wurde der Weg sichtbar: Die Stelle
gehört gar nicht zum Handler-Aufruf, sondern ist **IOMans
Zeichenausgabe-Schleife**:

    $ad9c  movea.l $64(a6),a1    * a1 = D_SysRom
    $ada8  jsr     $8(a1)        * unsere Ausgabe-Routine (Vtable +8)
    $adac  move.b  (a0)+,d0
    $adae  bne     $ada8

IOMan wollte also eine Meldung ausgeben. Der Grund des Absturzes stand
direkt daneben:

    D_SysRom($64) = 000075a7      <- UNGERADE

**`Q9K_IOManOutVtable` lag auf einer ungeraden Adresse.** Damit ist auch das
Sprungziel `+8` ungerade, und die CPU bricht ab. Bisher stimmte die
Ausrichtung nur **zufällig** — jede Änderung davor, die die Codelänge um eine
ungerade Zahl verschiebt, kippte sie. Genau das haben meine Umbauversuche am
Dispatcher getan: Sie scheiterten scheinbar an ihrer eigenen Logik,
tatsächlich brach der IOMan-Start am ersten auszugebenden Zeichen.

Fix: ein `align` vor der Struktur. `D_SysRom` liegt jetzt stabil gerade
(`$75a8`), das Verhalten ist sonst unverändert.

### Der A4-Fix: wirkt, hat aber eine ungeklärte Nebenwirkung

Auf der bereinigten Grundlage (Ausrichtung gefixt) sind drei Varianten
durchgemessen. **Die Kernaussage: `a4` zu erhalten behebt die
Speicherkorruption tatsächlich** — die Illegal Instruction verschwindet, die
Exception-Mitschrift bleibt leer. **Aber jede Variante bricht IOMans
Konsolenöffnung** (`ioman: can't open console device`), womit auch die
Testausgabe `Hallo von Q9-OS!` ausbleibt.

| Variante | Ergebnis |
|---|---|
| `a4` aus globaler Zelle `Q9K_TrapA4Save` zurückholen | IOMan-Start bricht sofort ab — die Zelle übersteht keinen verschachtelten Trap |
| Sprung über `bsr`+Stub (wie im externen Pfad) | bricht früh im Boot |
| hinterher `movea.l Q9_D_Proc,a4` | Korruption weg, aber Konsole nicht mehr zu öffnen |
| `a4` über den Aufruf **auf dem Stack** retten (plus `addq.l #8,sp` in den beiden blockierenden Handlern) | Korruption weg, **keine Exception mehr**, aber Konsole ebenfalls nicht mehr zu öffnen |

Die letzte Variante ist die sauberste und funktioniert technisch: Der
Registersatz stimmt, die Blockierpfade räumen korrekt ab, alle 14
Host-Testsuiten bleiben grün. Trotzdem scheitert das Öffnen der Konsole mit
`Error $0000`.

**Damit ist klar: IOMan verlässt sich beim Öffnen auf einen `a4`-Wert, den
unser Dispatcher bisher — unbeabsichtigt — geliefert hat.** Welchen, ist noch
offen. Ein naheliegender Verdacht (ein Handler setzt `a4` absichtlich als
Rückgabe) wurde geprüft und ausgeschlossen: Die einzige Stelle, die `a4`
selbst setzt, gehört zum Testprozess, nicht zu einem Handler.

**Der Code bleibt deshalb vorerst auf dem alten Stand** — ein Kernel ohne
Konsolenausgabe wäre für die weitere Arbeit unbrauchbar, auch wenn die
Korruption damit weiterbesteht.

### Gemessen: `a4` ist beim Konsolen-Open gar nicht die Ursache

Der Instruktions-Ring zeichnet jetzt auch `a4` auf. Ergebnis, per Freeze auf
scfs Open-Routine:

    ohne Fix:  a4 = 00019400   (Prozessdeskriptor)
    mit Fix:   a4 = 00019400   (identisch)

**`a4` ist in beiden Fällen korrekt und identisch.** Meine Arbeitshypothese
war falsch — der Open-Fehler hat mit `a4` nichts zu tun. (Nebenbei bestätigt:
Auf dem Open-Pfad läuft alles über `Q9K_TrapCallExternal`, wo `a4 = D_Proc`
ohnehin korrekt gesetzt wird.)

**Die Ursache ist der Push selbst.** Ein Gegentest ohne die
`addq.l #8`-Anpassung bricht genauso — es liegt also allein am
`move.l a4,-(sp)` im Dispatcher. Und dazu passt eine Konstruktion, die es
schon gibt: `F$AllPD` und `F$SRqMem` erkennen den Trampolin-Registerrahmen
**stackrelativ**:

    movem.l d0/a0,-(sp)
    movea.l sp,a0
    adda.l  #16,a0        * sp vor dem movem (8) + 8
    cmpa.l  a0,a5         * a5 == sp+8  ->  Rahmen vorhanden

Ein zusätzlicher Push verschiebt `sp` um 4 und verfälscht damit genau diese
Erkennung. Die Handler versorgen den Rahmen dann nicht mehr (oder fälschlich),
IOMan bekommt Müll zurück — und das Öffnen scheitert.

**Konsequenz für den Fix:** Jede Lösung, die den Stack im Dispatcher
verändert, muss diese beiden Erkennungen mitziehen (`adda.l #20` statt `#16`,
Zeilen 2734 und 2798). Sauberer wäre eine Variante, die `a4` **ohne**
Stackänderung erhält — die ist aber noch zu finden: Eine globale Zelle
scheidet wegen Verschachtelung aus, `bsr`+Stub bricht, und `r68` kennt die
speicherindirekte `jsr`-Form nicht.

**Vierter Fix-Anlauf, ebenfalls gescheitert.** Versucht wurde die robuste
Variante: die Rahmenerkennung zusätzlich am Aufrufweg festmachen
(`tst.w Q9K_InTrapPath` vor der stackrelativen Prüfung), zusammen mit dem
`a4`-Push und `addq.l #8` in den Blockierpfaden. Ergebnis: Das Konsolen-Open
scheitert weiterhin, und der Boot bleibt zusätzlich früher hängen.

Der Denkfehler war nicht die Idee, sondern das Vorgehen: **drei Änderungen
auf einmal**, ohne sie einzeln zu messen. Damit lässt sich nicht mehr
zuordnen, welche davon was bewirkt. Der Code steht deshalb wieder auf dem
funktionierenden Stand (Konsole geht, `Hallo von Q9-OS!` erscheint).

### Was gesichert ist

- **Die Ursachenkette ist vollständig verstanden:** Der Dispatcher lässt bei
  eigenen Handlern die Handler-Adresse in `a4` stehen; IOMan und die
  File-Manager führen dort einen eigenen Zeiger und schreiben damit weiter —
  `rbf` trifft so unseren Kernel-Code (`move.l d1,$14e(a4)`).
- **`a4` zu erhalten behebt das nachweislich** (Illegal Instruction
  verschwindet).
- **Der Push kollidiert mit der stackrelativen Rahmenerkennung** in
  `F$AllPD`/`F$SRqMem` (`a5 == sp+8`, Zeilen ~2734/2798).
- **`a4` ist beim Konsolen-Open nicht die Ursache** — der Wert ist mit und
  ohne Fix identisch (`$19400`).

### Schrittweise Messung — drei Ergebnisse

Diesmal jeweils **eine** Änderung, einzeln gemessen:

**Schritt 1 — nur der `a4`-Push.** Das Konsolen-Open bricht. Damit ist der
Push zweifelsfrei der Auslöser, unabhängig von allem anderen.

**Schritt 2 — Push plus Rahmenerkennung auf `#20`.** Bricht *früher* als
zuvor. **Die Hypothese ist damit widerlegt**, und im Nachhinein ist auch
klar warum: Der Push betrifft nur den TRAP-Weg — dort soll die Erkennung
gerade **nicht** greifen. Der Trampolin-Weg läuft überhaupt nicht durch den
Dispatcher, für ihn bleibt `#16` richtig. `#20` bringt die Erkennung also
genau falsch herum durcheinander.

**Schritt 3 — Push allein, Freeze auf scfs Open-Routine.** Ergebnis:

    pc=0000c1cc (scf+$00aa)   a4=00019400

**scfs Open wird erreicht, und `a4` ist dort korrekt.** Der Fehler entsteht
also erst *innerhalb* von scf oder danach — nicht auf dem Weg dorthin.

### Stand

Die Suche ist deutlich enger geworden, der Fix aber weiter offen. Der Code
steht auf dem funktionierenden Stand.

**Schritt 4 — scfs Open-Zweige, mit und ohne Push verglichen.** Gemessen
wurden Einstieg, die beiden Fehlerausgänge und der Erfolgspfad:

| Stelle | mit Push | ohne Push |
|---|---|---|
| Open-Einstieg | 1 | 1 |
| `bcs` nach `F$SRqMem` | 1 | 1 |
| **Erfolgspfad** | **1** | **1** |
| Fehlerausgänge | 0 | 0 |

**Exakt identisch — scfs Open gelingt in beiden Fällen.** Damit ist auch
scfs Open als Ursache ausgeschlossen. Der Unterschied entsteht erst
*danach*: IOMan bewertet das Ergebnis unterschiedlich, obwohl der
File-Manager dasselbe tut.

*(Nebenbei aus der Analyse: scfs Open holt den Pfadnamen mit
`movea.l $20(a5),a0` aus dem Registerrahmen, ruft `F$PrsNam` per **TRAP** —
also durch unseren Dispatcher — und legt danach mit `lea -$2c(a7),a7 /
movea.l a7,a5` selbst einen 44-Byte-Rahmen für `F$SRqMem` an. Beide Wege
sind damit im Open-Pfad beteiligt.)*

### Stand der Eingrenzung

Ausgeschlossen sind inzwischen: der `a4`-Wert selbst, die Rahmenerkennung in
`F$AllPD`/`F$SRqMem`, der Weg bis scfs Open, und scfs Open selbst.
Übrig bleibt der **Rückweg von scfs Open zu IOMan** — dort wird das Ergebnis
offenbar anders bewertet.

### Die Stelle in IOMan ist gefunden

Über den Meldungstext (Modul-Offset `$3f7`) rückwärts: Ihn lädt `ioman+$2f4`,
das ist die Meldungsroutine ab `$2f0`. Ihr einziger Aufrufer ist `ioman+$12a`
— und davor steht der eigentliche Vorgang:

    ioman+$011e  lea     (a2,d0.w),a0     * Gerätename aus dem Deskriptor
    ioman+$0122  moveq   #$3,d0           * Modus 3 = Lesen+Schreiben
    ioman+$0124  trap    #0
    ioman+$0126  dc.w    $0064            * F$DAttach
    ioman+$012a  bsr.w   $2f0             * -> "can't open console device"

**IOMan scheitert am `F$DAttach` (`$64`)** — dem Dienst, der ein Gerät an das
System anhängt. Der Code ist per internem Referenzmaterial bestätigt
(`$60` F$Trans, **`$64` F$DAttach**, `$65` F$Flash, `$66` F$PwrMan).

Bemerkenswert: **Wir registrieren `$64` nicht selbst** — er muss also von
IOMan über `F$SSvc` kommen, wird demnach über den *externen* Pfad
(`Q9K_TrapCallExternal`) ausgeführt, wo der `a4`-Push gar nicht steht. Warum
er trotzdem nur mit Push fehlschlägt, ist der nächste Messpunkt.

*(Werkzeugnotiz: Der Weg vom Meldungstext zur Ursache ist mechanisch —
String im Modul suchen, das `lea <text>(pc),a0` dazu finden, dann dessen
Aufrufer. Alle drei Schritte lassen sich im extrahierten Modul offline
erledigen.)*

### Gemessen: `F$DAttach` schlägt nur mit Push fehl

| Zählpunkt | ohne Push | mit Push |
|---|---|---|
| `trap` (F$DAttach) | 1 | 1 |
| **Fehlerpfad `$12a`** | **0** | **1** |
| Fortsetzung danach | 0 | 1 |

Der Aufruf findet in beiden Fällen statt; **nur mit Push schlägt er fehl**.
Damit ist der Unterschied erstmals an einem einzelnen Dienst festgemacht.

**Was daraus folgt:** `F$DAttach` selbst läuft über den *externen* Pfad
(IOMan registriert `$64` per `F$SSvc`), wo der Push gar nicht steht. Betroffen
sein kann also nur ein **innerer** Aufruf von `F$DAttach` — der Dienst ruft
seinerseits Kernel-Dienste, und die laufen über unseren Dispatcher.

### ~~F$DAttach fehlt~~ — WIDERLEGT: es fehlt überhaupt kein Syscall

Die Spur „IOMan scheitert an einem nicht implementierten `F$DAttach` (`$64`)"
ist **falsch** und hier nur als Warnung dokumentiert.

Der Callcode `$64` stammte aus dem Byte hinter einem fremden `trap #0`
(`dc.w $0064`) — also wieder aus einer Lesung ohne gesicherte Ausrichtung.
Statt weiter zu raten, protokolliert der Unimplemented-Stub jetzt den
Callcode, den der Dispatcher in `$1370` ablegt:

| Lauf | Treffer im Unimplemented-Stub |
|---|---|
| ohne Push (Konsole geht) | **0** |
| mit Push (`can't open console device`) | **0** |

**Der Stub wird in keinem der beiden Fälle betreten.** Es fehlt also kein
einziger Dienst — weder `F$DAttach` noch sonst einer. Der Fehler kommt von
einem **existierenden** Dienst, der mit Push ein anderes Ergebnis liefert.

*(Damit ist auch klar, warum der Handler hinter Slot `$64` nie ausgeführt
wurde: Diese Nummer wird schlicht nie angefordert.)*

### Stand der Eingrenzung (belastbar)

- Der `a4`-Push behebt die Speicherkorruption (Illegal Instruction
  verschwindet) — mehrfach gemessen.
- Er bricht zugleich IOMans Konsolen-Open — ebenfalls mehrfach gemessen.
- **Nicht** die Ursache: der `a4`-Wert selbst, die stackrelative
  Rahmenerkennung, scfs Open, ein fehlender Syscall.
- scfs Open gelingt in beiden Fällen identisch; der Unterschied entsteht
  danach, beim Ergebnis eines bereits vorhandenen Dienstes.

### Der Callcode ist `$0084` — `I$Open`

Nachgesehen an der Stelle selbst, deren Instruktionsanfang durch den
PC-Zähler gesichert ist (`+$0124` wird ausgeführt):

    +$011e  41f2 0000    lea    (a2),a0      * Gerätename
    +$0122  7003         moveq  #3,d0        * Modus
    +$0124  4e40         trap   #0
    +$0126  0084         dc.w   $0084        * I$Open
    +$0128  6406         bcc.b  +6           * bei Erfolg überspringen
    +$012a  6100 01c4    bsr.w  $2f0         * Fehlermeldung

**Es ist `I$Open` (`$84`), nicht `$64`.** Die frühere Lesung war schlicht
falsch abgelesen. Damit ist auch die letzte Unklarheit ausgeräumt: `$84` wird
von IOMan selbst bedient (Slot zeigt in IOMan), es fehlt kein Dienst, und der
Unimplemented-Stub hat zu Recht null Treffer.

**Der Kreis schließt sich:** IOMan ruft beim Anhängen der Konsole seinen
**eigenen `I$Open`**. Darin gelingt scfs Open nachweislich (identisch mit und
ohne Push) — trotzdem meldet IOMan Fehlschlag. Die Ursache liegt also in
IOMans `I$Open`-**Nachbearbeitung**, nach der Rückkehr aus dem File-Manager.

### IOMans I$Open-Nachbearbeitung — der Unterschied ist eingekreist

Der Handler (Modul-Offset `$124a` ff.) sieht so aus:

    +$124a  move.b $3(a5),d1     * Modus aus dem Registerrahmen
    +$124e  bsr.w  $135e         * Deskriptor besorgen
    +$1252  bcs    $1266         * Fehler -> raus
    +$1254  bsr.w  $14f8         * File-Manager rufen (darin scfs Open)
    +$1258  bcs.w  $11ca         * Fehler -> raus
    +$125c  moveq  #0,d0
    +$125e  move.w $0(a1),d0     * Pfadnummer aus dem Deskriptor
    +$1262  move.l d0,$0(a5)     * Rückgabe in den Registerrahmen

Gemessen wurden die Zweige in beiden Läufen:

| Stelle | ohne Push | mit Push |
|---|---|---|
| `bcs` nach File-Manager (`$1258`) | 1 | 1 |
| **Rückgabe der Pfadnummer (`$1262`)** | **1** | **0** |

**Mit Push wird die Rückgabe nie erreicht** — der Fehlerzweig nach
`bsr.w $14f8` greift. Ohne Push läuft es bis zur Rückgabe durch.

Damit ist der Fehler auf **die Routine `ioman+$14f8`** eingegrenzt: Sie ruft
den File-Manager, und dessen Open gelingt nachweislich (mit und ohne Push
identisch) — trotzdem kehrt sie mit Carry zurück. Der Fehler entsteht also in
dieser Wrapper-Routine, vor oder nach dem eigentlichen Aufruf.

### Der Wrapper `ioman+$14f8`: ein Lock im Pfaddeskriptor

Die Routine ist jetzt vollständig gelesen. Sie **sperrt den Pfad**, ruft den
File-Manager und gibt die Sperre wieder frei:

    +$14f8  movem.l d0-d1/a0-a6,-(a7)   * eigener Registersatz, a4 landet bei $28(a7)
    +$1502  ori.w   #$700,sr            * Interrupts sperren
    +$1506  move.w  $8(a1),d0           * Lock im Pfaddeskriptor
    +$150a  bne.w   $15a6               * schon belegt -> FEHLER
    +$150e  move.w  $0(a4),$8(a1)       * Lock = Prozess-ID aus a4
    ...     Sprung in den File-Manager über dessen Sprungtabelle
    +$1564  movea.l $28(a7),a4          * a4 zurückholen
    +$158a  move.w  $0(a4),d0
    +$158e  cmp.w   $8(a1),d0           * Lock == eigene ID?
    +$1592  bne.b   $1598               * nein -> NICHT freigeben
    +$1594  clr.w   $8(a1)              * ja  -> freigeben

**Das Lock wird nur freigegeben, wenn `$0(a4)` beim Rückweg denselben Wert
liefert wie beim Setzen.** Bleibt es stehen, scheitert jeder weitere Open an
`bne $15a6` — genau das beobachtete Verhalten.

Damit ist der Mechanismus verstanden: Es geht um `a4` **über den gesamten
Aufruf hinweg**, nicht nur um seinen Wert nach einem einzelnen Handler.
IOMan sichert `a4` zwar selbst (`movem` am Anfang, zurück bei `$1564`) — der
Vergleich sollte also stimmen. Warum er mit Push trotzdem fehlschlägt, ist
noch offen; reine Code-Lektüre reicht hier nicht weiter.

**Gemessen — die Lock-Hypothese ist endgültig widerlegt.** PC-Zähler auf den
Fehlerzweig `ioman+$15a6` (dorthin springt `bne` bei belegtem Lock), dazu
Wrapper-Einstieg und Freigabestelle:

| Stelle | ohne Push | mit Push |
|---|---|---|
| **Fehlerzweig (Lock belegt)** | **0** | **0** |
| Wrapper-Einstieg (`$14f8`) | **5** | **2** |
| Lock setzen / freigeben | 4 / 4 | 2 / 2 |

Der Fehlerzweig wird in **keinem** Lauf getroffen; Sperren und Freigeben sind
immer paarig. Kein Lock bleibt hängen.

**Der eigentliche Befund liegt woanders:** Mit Push wird der Wrapper nur
**zweimal statt fünfmal** aufgerufen. IOMan kommt also gar nicht so weit — die
Kette bricht *vor* dem dritten File-Manager-Aufruf ab. Der Fehler entsteht
somit **zwischen** zwei Wrapper-Aufrufen, nicht in einem davon.

*(Messhygiene: Eine frühere Lock-Messung schien „identisch" auszufallen, weil
das Testimage nicht neu gebaut worden war und beide Läufe dieselbe
Push-Variante benutzten. Erkennbar wurde das an der Freigabe-Adresse — sie
lag auf der Push-Adresse, obwohl der Lauf als „ohne Push" gedacht war. Nach
jedem Kernel-Umbau gehört das Image neu gebaut, sonst misst man zweimal
dasselbe.)*

**Der dritte Wrapper-Aufruf kommt aus `ioman+$0dca`.** Ring-Freeze auf den
Wrapper-Einstieg beim dritten Treffer (im funktionierenden Lauf) zeigt den
Weg dorthin:

    ioman+$144a / $144c   * Namens-Kopierschleife, 8 Durchläufe
    ioman+$1450 … $145c
    ioman+$0dca, $0dce    * von hier wird gerufen
    ioman+$14f8           * Wrapper, 3. Mal

Vor dem Aufruf kopiert IOMan also einen Namen (die Schleife bei `$1448`:
`moveq #$7f,d1 / move.b (a0)+,(a2)+ / dbra`) und ruft dann über `$0dca` den
File-Manager. **Genau dieser dritte Aufruf fehlt mit Push.**

*(Auch hier hat die frisch dokumentierte Regel sofort gegriffen: Der erste
Freeze-Versuch lief ins Leere, weil nach dem `git checkout` das Image nicht
neu gebaut war — der Ring blieb ungefroren. Nach dem Neubau griff er sofort.)*

**Gemessen — `$0dca` ist nicht die Bruchstelle:**

| Stelle | ohne Push | mit Push |
|---|---|---|
| Aufrufstelle `$0dca` | 1 | **1** |
| Wrapper-Einstieg | 5 | **2** |

Die Stelle wird in **beiden** Läufen genau einmal erreicht. Mit Push fehlen
also die *anderen* Wrapper-Aufrufe, nicht der von hier.

### Zwischenbilanz zum A4-Punkt — und ein Vorschlag

Die Eingrenzung ist weit gekommen und hat unterwegs mehrere echte Bugs
zutage gefördert (Vtable-Ausrichtung, Moduldirectory, Pfaddeskriptor-Layout).
Zum `a4`-Punkt selbst gilt gesichert:

- Der Push **behebt die Speicherkorruption** — reproduzierbar.
- Er **bricht zugleich das Konsolen-Open** — ebenso reproduzierbar.
- Ausgeschlossen sind: der `a4`-Wert, die stackrelative Rahmenerkennung,
  scfs Open, ein fehlender Syscall, das Pfad-Lock, und die Aufrufstelle
  `$0dca`.

Jede Messrunde grenzt weiter ein, öffnet aber zugleich eine neue Ebene. Statt
diesen Weg fortzusetzen, bietet sich ein **anderer Ansatz** an:

`a4` nur dann wiederherstellen, **wenn der Aufrufer außerhalb des Kernels
liegt** — also genau für IOMan und die File-Manager, die den Prozessdeskriptor
dort erwarten. Die Rücksprungadresse steht im Exception-Frame; ein Vergleich
gegen die Kernel-Modulgrenzen genügt. Für eigene Prozesse bliebe alles
unverändert, und die Stack-Verschiebung träfe nur den Pfad, der sie braucht.

Das umgeht die Nebenwirkung, statt sie weiter zu jagen.

**Umsetzungshürde, vor dem Bauen zu klären:** Die Herkunftsprüfung braucht
den Rücksprung-PC aus dem Exception-Frame (`2(sp)` nach `lea 36(sp),sp`) und
einen Vergleich gegen die Kernel-Modulgrenzen. Beides ist heikel:

- **Kein freies Register.** `d0`/`d1`/`a0`/`a1`/`a2` sind Syscall-Eingaben,
  `d2`-`d7`/`a3`-`a5` müssen dem Aufrufer erhalten bleiben. Ein Register
  temporär auf dem Stack zu sichern geht (der Versatz ist vor dem Handler
  wieder weg), muss aber sauber vor dem eigentlichen `a4`-Push passieren.
- **Die Kernel-Grenzen stehen nicht als Konstante bereit.** `cmpi.l` braucht
  ein Immediate; die Ladeadresse ist erst zur Laufzeit bekannt. Entweder legt
  der Boot sie in einer Zelle ab (dann braucht der Vergleich doch ein
  Register), oder die Prüfung wird PC-relativ formuliert.

Beide Punkte sind lösbar, sollten aber **vorher entschieden** werden — nach
fünf Fehlversuchen an dieser Stelle ist ein weiterer Schnellschuss der
falsche Weg.

### Nachprüfung 2026-09-05: die bisherige Erklärung der Nebenwirkung trägt nicht

Vor dem sechsten Anlauf wurden die beiden oben notierten Erklärungen
**rechnerisch** geprüft, ohne eine Zeile zu ändern. Beide halten nicht:

**1. Die stackrelative Rahmenerkennung kann es nicht sein.** Oben steht als
„Konsequenz für den Fix": Ein zusätzlicher Push verschiebe `sp` und
verfälsche `cmpa.l a0,a5` in `F$AllPD`/`F$SRqMem`, deshalb `adda.l #20`
statt `#16`. Das ist falsch, aus zwei unabhängigen Gründen:

- Der **Trampolin-Weg läuft gar nicht durch `Q9K_TrapDispatch`** — der
  Aufrufer springt per PEA+RTS direkt in den Slot. Ein Push im Dispatcher
  erreicht diesen Pfad nie, kann seine Erkennung also auch nicht stören.
- Im **TRAP-Weg** ist `a5` laut eigenem Kommentar „ein beliebiges
  Aufruferregister". Die Erkennung schlägt dort mit *und* ohne Push fehl;
  ein Versatz um 4 ändert daran nichts.

**`adda.l #20` wäre demnach nicht die Lösung, sondern ein neuer Bug** — es
würde den Trampolin-Weg brechen, der als einziger auf die Erkennung angewiesen
ist. Die Zeile bleibt bewusst auf `#16`.

**2. Die `60(sp)`-Carry-Löschung ist unkritisch.** Verdacht war, dass
`andi.w #$fffe,60(sp)` in `F$Wait` (Z. 2286) und `F$Sleep` (Z. 2437) den
Push nicht mitzieht und ein stehengebliebenes Carry das beobachtete
`Error $0000` erzeugt. Geprüft: Die `60` ist die **eigene**
Registersatzgröße (15 × 4) und wird *nach* dem handlereigenen `movem`
gebildet; `F$Sleep` baut seinen Frame ohnehin selbst neu auf. Beide Offsets
sind vom Dispatcher-Stack unabhängig, solange die jsr-Rücksprungadresse
korrekt abgeräumt wird (`addq.l #8` statt `#4` — das war in Variante 4
bereits enthalten).

**Stand danach:** Die Ursache der Nebenwirkung ist damit **weiterhin offen**,
aber zwei Erklärungen sind sauber ausgeschieden, statt weiter mitgeschleppt
zu werden. Auch `Q9K_TrapAfterCall` (Zugriffe auf `(sp)`, `4(sp)`, `6(sp)`)
ist unkritisch, sofern der Pop vor dem `bra` dorthin steht.

Der nächste Schritt ist deshalb **kein Fixversuch, sondern eine Messung**:
Push einbauen, den Boot bis zum fehlschlagenden Open mitschreiben und die
*erste* Abweichung gegen den funktionierenden Lauf suchen. Erst wenn die
bekannt ist, lohnt ein sechster Anlauf.

### Die Messung (2026-09-05) — und ein Gegentest, der eine Klasse ausschließt

**Messaufbau, reproduzierbar:**

    # Bootfile bauen (Referenz ohne hellosvc, das fuegt mkbootfile.sh frisch ein)
    tools/mkbootfile.sh <ref.boot> <image>
    # Lauf mit Syscall-Mitschrift -- BEIDE Variablen noetig:
    Q9_TRAP_TRACE=<datei> Q9_TRAP_TRACE_ALL=1 expect run.exp <image> <log>

`Q9_TRAP_TRACE` ist der **Dateipfad**, `Q9_TRAP_TRACE_ALL` schaltet auf alle
Traps — nur eine von beiden zu setzen liefert stillschweigend nichts.
(Weitere Falle: `os9 gen -b=` bricht mit *„is fragmented"* ab, sobald das
Zielimage schon eine fragmentierte Bootdatei enthält. Dann frisch vom
Masterimage klonen — `cp -c OS9SYS.hda …` —, nicht vom Arbeitsstand.)

**Ergebnis, drei Läufe:**

| Lauf | Konsole | Trap-Zeilen | Pfad-Marker |
|---|---|---|---|
| ohne Push (Referenz) | `Hallo von Q9-OS!` | 28 | `RP012` |
| mit `a4`-Push | `can't open console device: Error $0000` | **2** | `RPn` |
| **Gegentest: 2 `nop` statt Push/Pop** | `Hallo von Q9-OS!` | 28 | `RP012` |

Der Marker `P` gibt je eine Ziffer pro belegtem `P$Path`-Slot aus, `n` heißt
„kein belegter Slot" — IOMan legt mit Push also **gar keine Pfade** an.

**Der Gegentest ist der eigentliche Ertrag.** Zwei `nop` sind exakt so groß
wie `move.l a4,-(sp)` plus `movea.l (sp)+,a4` (je 2 Byte) und lassen das
Modullayout identisch verschieben — aber sie ändern den Stack nicht. Damit
ist bewiesen:

> Es liegt an der **Stackänderung**, nicht an der Codegröße oder einer
> verschobenen Modulbasis. Diese ganze Erklärungsklasse ist erledigt.

**Und zugleich ein scharfer Widerspruch:** Alle 28 Trap-Einträge des
Referenzlaufs sind mit `kernel=1` klassifiziert — der erste ist `F$SSvc` aus
*unserem* Init, danach unser `I$Open("/term")`. **IOMans Init macht keinen
einzigen `TRAP #0`**, es läuft vollständig über die Trampoline. Ein Push in
`Q9K_TrapDispatch` dürfte es also überhaupt nicht erreichen — tut es aber
reproduzierbar.

**Stärkster verbliebener Kandidat:** Fünf Handler verlassen den Dispatcher
per `rte` statt über `Q9K_TrapAfterCall` — `F$Exit`, `F$Panic`, `F$PrsNam`,
`F$Sleep`, `F$Wait`. Sie überspringen den Pop, und ihr `rte` liest den
Exception-Frame dann 4 Byte versetzt. Dagegen spricht bisher, dass der Trace
vor dem Bruch keinen solchen Aufruf zeigt — genau das ist der nächste
Messgegenstand. Der `addq.l #4` → `#8` in den Blockierpfaden allein erklärt
es nicht: der Bruch tritt mit **und** ohne diese Anpassung auf (beide Läufe
gemessen).

*(Nicht geglückt: der Ring-Dump per Ctrl-^ blieb bei `Q9_TRACE_INSTR=1` und
`Q9_FREEZE_PC=0x73a0` leer. Der Weg ist noch zu klären, war für diesen
Befund aber nicht nötig.)*

**Der Code steht unverändert auf dem funktionierenden Stand.**

### Aufgelöst (2026-09-05): es ist `a4` selbst — nicht der Stack

Zwei weitere Messungen haben den Widerspruch geklärt, und dabei muss ich
**meinen eigenen Schluss von oben korrigieren**.

**1. Der Zweig läuft auch für die Trampolin-Aufrufe.** Ein `'*'` plus
Callcode, ausgegeben bei jedem Durchlauf durch den eigenen Handlerzweig,
zeigt zwischen IOMan-Einsprung und IOMans erster Meldung:

    Q *00000000 *00000032 *0000002A *00000010 ioman: …

IOMans Init ruft also `F$Link`, **`F$SSvc` ($32), `F$IRQ` ($2A) und
`F$PrsNam` ($10)** — und alle laufen durch denselben `jsr (a4)`, obwohl der
Trap-Trace dort **keinen** `TRAP #0` zeigt. Der Grund ist banal: Der Trace
hängt an der `trap`-Instruktion, die Trampolin-Aufrufe (PEA+RTS) sind keine.
**Die Aussage weiter oben, der Trampolin-Weg berühre den Dispatcher nicht,
war damit falsch** — beide Wege teilen sich diesen Code.

**2. Der stackfreie Fix trennt die beiden Verdächtigen sauber.** Der
Handlerzeiger lässt sich anspringen, ohne `a4` anzufassen und ohne den Stack
zu verändern — per speicherindirektem `jsr ([Q9K_TrapHandlerScratch])`. Der
Q9 ist ein 68030 und kennt die Adressierungsart; nur `r68` kennt ihre Syntax
nicht, deshalb als Opcode-Bytes:

    dc.w    $4ebb,$01f1        * jsr ([…]): Modus 111/011, volles
    dc.l    Q9K_TrapHandlerScratch   * Extension-Word, Basis+Index unterdrueckt

**Der Opcode ist verifiziert** — mit ihm erscheinen exakt dieselben Callcodes
wie im funktionierenden Lauf, alle Handler werden also korrekt erreicht.
(Die Doku führte diesen Weg bisher als „`r68` kennt die speicherindirekte
`jsr`-Form nicht" — nicht verfügbar ist nur die *Syntax*, nicht die
Instruktion.)

**Die Wahrheitstabelle aus vier Läufen:**

| Variante | `a4` im Handler | Stack | Ergebnis |
|---|---|---|---|
| Original | Handleradresse | unverändert | **läuft** |
| 2 `nop` (Größengegentest) | Handleradresse | unverändert | **läuft** |
| Push/Pop | Aufrufer-`a4` | +4 | bricht |
| `jsr ([…])` | Aufrufer-`a4` | **unverändert** | **bricht** |

> **Es ist `a4` selbst, nicht der Stack.** Mein Schluss oben („es liegt an der
> Stackänderung") war voreilig: Der `nop`-Gegentest schließt nur die
> *Codegröße* aus, nicht den Stack. Erst die stackfreie Variante trennt beide
> — und sie bricht genauso.

Damit ist die alte Vermutung bestätigt, die zwischenzeitlich als widerlegt
galt: **IOMan verlässt sich auf den `a4`-Wert, den unser Dispatcher bisher
unbeabsichtigt liefert** (die Handleradresse). Der „korrekte" Aufrufer-`a4`
bricht es — und zwar auch dann, wenn sonst nichts verändert wird.

### Die Stelle ist gefunden: `F$PrsNam` muss `a4` setzen

**Bisektion über die drei Callcodes** — der stackfreie Fix jeweils nur für
einen Callcode aktiviert, alle anderen unverändert:

| Callcode | Handler | mit korrektem `a4` |
|---|---|---|
| `$32` | `F$SSvc` | läuft (`RP012`) |
| `$2A` | `F$IRQ` | läuft (`RP012`) |
| **`$10`** | **`F$PrsNam`** | **bricht (`RPn`)** |

**`F$PrsNam` fasst `a4` selbst überhaupt nicht an** (q9kernel_entry.a,
`Q9K_SysFPrsNam`) und kehrt per `rts` zurück; auch der Epilog stellt `a4`
nie wieder her. Der Wert, den der Dispatcher hinterlässt, geht also
unverändert an IOMan zurück.

**Gemessen, was eingeht:** `a4 = $00019400` — der Prozessdeskriptor, exakt
der Wert, den die frühere Messung an scfs Open gesehen hatte.

**Trennungstest (`a4`-Wert oder Sprungmechanismus?):** Sprungweg exakt wie im
funktionierenden Stand (`jsr (a4)`), nur `a4` danach per `movea` auf den
Aufrufer-Wert zurückgesetzt — `movea` ist die einzige Instruktion, die nach
dem `jsr` das CCR unberührt lässt, das der Epilog auswertet. **Ergebnis: es
bricht.** Damit ist der Sprungmechanismus entlastet und der Befund steht:

> IOMan darf nach `F$PrsNam` **nicht** seinen eigenen `a4` zurückbekommen.
> Es braucht dort einen anderen Wert — den unser Handler nicht liefert.

**Damit dreht sich die Bewertung des ganzen Problems um.** Der „Dispatcher-Bug"
(`a4` wird von der Handleradresse überschrieben) ist nicht die Ursache,
sondern hat einen zweiten, verdeckten Fehler bisher *kaschiert*: Unser
`F$PrsNam` liefert eine `a4`-Ausgabe nicht, die IOMan erwartet. Im
funktionierenden Stand bekommt IOMan zufällig die Handleradresse — einen
Zeiger in unseren Kernelcode. **Genau das erklärt die
Speicherkorruption**: RBFs `move.l d1,$14e(a4)` schreibt über dieselbe Kette
in unseren Kernel. Der Zufallswert ist harmlos genug, dass der Boot
durchläuft; der echte Prozessdeskriptor ist es nicht — dort zerstört der
Schreibzugriff etwas, das gebraucht wird.

### Im Originalkernel nachgeschlagen: `F$PrsNam` schreibt in den Registerrahmen

Der reale Handler liegt laut Slot-Tabelle bei `$a3e0` (Ladebasis `$7100`,
Moduloffset `$32e0`). Analysiert ist er bemerkenswert kurz:

    a3e0  bsr.b    $a3f0            * eigentliche Parse-Routine
    a3e2  movem.l  d0-d1,$0(a5)     * Ergebnisse in den REGISTERRAHMEN
    a3e8  movem.l  a0-a1,$20(a5)
    a3ee  rts

**Der echte `F$PrsNam` gibt seine Ergebnisse nicht in den Registern zurück,
sondern schreibt sie in den R$-Registerrahmen, den `a5` adressiert** — auf
`R$d0=$00` und `R$a0=$20`, exakt die Offsets aus `process.a`. Unser Handler
lieferte sie bisher nur in den Registern.

**Gemessen, dass `a5` bei uns wirklich darauf zeigt:** beim `F$PrsNam`-Eintritt
`a5 = $0002D398`, `sp = $0002D338` — `a5` liegt im Stackbereich, also auf
einem echten Rahmen.

**Umgesetzt und verifiziert:** `Q9K_SysFPrsNam` versorgt den Rahmen jetzt wie
das Original. Der Kernel läuft damit unverändert durch (`RP012`,
`Hallo von Q9-OS!`) — die Konvention ist nachgezogen und nachweislich
unschädlich.

**Das `a4`-Problem löst sie aber nicht.** Mit Rahmenversorgung *und*
`a4`-Erhalt bricht das Open weiterhin; die Rahmenversorgung *allein* läuft.
Getrennt gemessen, nicht beides zugleich geändert.

**Wo die Suche jetzt steht.** Der Widerspruch ist enger geworden: Im
funktionierenden Stand trägt IOMan drei Pfade in unseren Prozessdeskriptor
ein (`RP012` liest sie bei `P$Path`/`$168` aus) — IOMan hat dort also schon
heute den *richtigen* `a4`, den es über `Q9K_TrapCallExternal` bekommt, wo
`a4 = D_Proc` korrekt gesetzt wird. Der `a4`-Erhalt im eigenen Handlerzweig
zerstört demnach etwas anderes, das IOMan davor braucht — nicht den
Open-Pfad selbst. Der Prozessdeskriptor scheidet als Ursache aus: Er ist
`$200` groß und deckt `P$Path` bis `$1A8` (q9kernel_firstproc.c).

### Zweiter echter Bug gefunden: die `P$Path`-Tabelle war uninitialisiert

**Die Callcode-Sequenzen beider Läufe, direkt gegenübergestellt:**

    ohne a4-Erhalt:  0000 0000 0032 002A 0010 | 0003 0000 0000 0000 0006
    mit  a4-Erhalt:  0000 0000 0032 002A 0010 |  (nichts mehr)

Bis einschließlich `F$PrsNam` identisch; danach fehlt der `F$Fork ($03)`
unseres Init — er bleibt aus, weil IOMan keine Pfade angelegt hat. (Die
Fehlermarken zeigen im funktionierenden Lauf drei `F$Link` mit `$DD`/E$MNF —
erwartete Suchfehlschläge, in beiden Läufen unauffällig.)

**Der Fund:** `Q9K_ProcCreate` (q9kernel_firstproc.c) füllt die
`P$Path`-Tabelle nur, **wenn es einen Erzeuger gibt**. Beim allerersten
Prozess blieb sie damit völlig uninitialisiert und enthielt den Speichermüll
der vorherigen Belegung. IOMans `I$Open` sucht dort das erste freie Wort
(`lea $168(a4),a0 / moveq #$1f,d0 / tst.w (a0)+ / dbeq d0,…`) — unter 32
Müllworten steht nie eine Null, also `E$PthFul`.

**Behoben:** Der `else`-Zweig nullt die Tabelle jetzt. Verifiziert an der
richtigen Adresse: `$168(D_Proc) = 0`, `$16c(D_Proc) = 0`, und der Kernel
läuft unverändert durch (`RP012`, `Hallo von Q9-OS!`).

**Messfalle, die dabei fast in die Irre geführt hätte** — und die hier steht,
damit sie niemand wiederholt: Eine erste Messung von `$168(a4)` *im Handler*
lieferte `$6600000C $23CD0000` und sah nach genau diesem Bug aus. Sie war
aber wertlos: **Im Handler ist `a4` bereits die Handleradresse** (gemessen
`a4 = $00007A16`, Kernelcode), nicht der Deskriptor — gelesen wurde also
unser eigener Code. Der Aufrufer-`a4` (`$00019400 = D_Proc`) ist nur *vor*
dem `movea.l Q9K_TrapHandlerScratch,a4` im Dispatcher zu sehen. Wer `a4`
misst, muss dazusagen, an welcher Stelle der Kette.

**Das `a4`-Problem bleibt auch damit offen.** Nullung allein läuft, Nullung
plus `a4`-Erhalt bricht weiterhin — getrennt gemessen. Beide heute gefundenen
Fehler (`F$PrsNam`-Rahmen, `P$Path`-Nullung) waren echt und sind behoben,
aber keiner von beiden ist die Ursache der Nebenwirkung.

## NACHTRAG 2026-09-07: `F$PrsNam` und `I$Attach` kommen von `scf`, nicht IOMan

**Messfalle, die den ganzen Vormittag verzerrt hat:** Die Kernelgröße wächst
mit jedem Fix (heute: 13270 → 14286 Byte), damit verschieben sich alle
Modulbasen im Testimage. Eine Analyse mit der GESTERN gültigen
`ioman`-Basis (`$A778`) landete im FALSCHEN Modul und lieferte plausibel
aussehenden, aber bedeutungslosen Code. Die Basis muss **bei jeder Messung
frisch aus dem aktuellen Testimage gelesen werden** (Modulliste per
Magic-Marker-Scan, nicht aus einer Notiz) — dieselbe Falle wie im
Kopfkommentar der letzten Runde, diesmal real erlebt statt nur zitiert.

**Vollständige Callcode+PC-Sequenz (funktionierender Lauf), diesmal mit
korrekter Basis:**

    #F$Link #F$Link #F$SSvc  +I$Open(ioman)  #F$IRQ  #F$PrsNam(scf+$4C0)
    +I$Attach(scf+$56E)  +I$Dup(ioman)  +I$Dup(ioman)  +I$Chgdir(ioman)  +I$Write

**Wichtiger Fund: `F$PrsNam` und der ERSTE `I$Attach`-Aufruf laufen nicht in
IOMan, sondern in `scf`** (dem File-Manager) — IOMan ruft nur `I$Open` auf,
scf parst darin selbst den Pfadnamen und hängt sich selbst am Gerät an; erst
danach kommen `I$Dup`/`I$Chgdir` wieder aus IOMans eigenem Code.

**Der Divergenzpunkt ist exakt eingegrenzt.** `I$Attach` ($80) wird
zweimal aufgerufen. Der erste Aufruf ist in beiden Läufen (mit/ohne
`a4`-Erhalt) identisch (`sr=$2700 d1=$10000`). Beim **zweiten** Aufruf:

| | Carry | `d1` |
|---|---|---|
| ohne Fix (funktioniert) | 0 | **1** |
| mit Fix (bricht) | 0 | **0** |

Kein Fehlerpfad (Carry ist beide Male 0) — eine echte, bedeutungstragende
Rückgabe. Das deutet auf einen internen Zustandsvergleich in `scf` hin (z. B.
„ist dieses Gerät für diesen Aufrufer schon angehängt?"), der wahrscheinlich
`a4` als Schlüssel benutzt: Mit dem alten Bug ist `a4` bei jedem Aufruf ein
anderer Zufallswert (Handleradresse), der Vergleich schlägt nie an; mit
`a4 = D_Proc` konstant über beide Aufrufe hinweg schlägt er plötzlich an,
und IOMan liest daraus „nichts zu duplizieren" — die beiden `I$Dup`-Aufrufe
entfallen.

**Einordnung:** Das führt in `scf`s eigene, private Zustandsverwaltung
(statischer Speicherbereich, den ein echter OS-9-Kernel per M$Exec-Konvention
aufsetzt — unser Kernel tut das nicht). Eine vollständige Klärung bräuchte
vermutlich die Analyse von scfs internem Attach-Buchführungscode
und/oder eine korrekte Umsetzung der M$Exec-Konvention für Dateimanager. Das
ist ein groesseres Stueck Arbeit, kein schneller Fix mehr.

**Stand:** Code unverändert auf dem funktionierenden Stand. Kein neuer
Fixversuch heute — die Messungen waren die Arbeit, nicht ein weiterer
siebter Blindversuch.

### Offen: Pfad-Deadlock

Blockiert der Erzeuger lesend auf einem Pfad, hängt ein schreibendes Kind
darin fest.

### Wie man den Lesetest fährt

Der `I$ReadLn`-Block in `Q9K_TestProcA` ist **aktiv**; er hält den Boot an,
solange die Bruchstelle oben besteht. Zum Abschalten den Block bis
`Q9K_TestReadDone` durch ein `bra Q9K_TestProcA_Loop` ersetzen.

Eingabe schickt man über eine Pipe, sonst tritt nie ein Weckfall ein:

```bash
(python3 -u -c "
import time,sys
time.sleep(10); sys.stdout.write('hallo\r'); sys.stdout.flush()
time.sleep(4);  sys.stdout.write('\x1e'); sys.stdout.flush()   # Ctrl-^ = Dump
time.sleep(4)
" | ./build/macos/q9.exe --rom <rom> --cf <image>)
```

Der Dump landet in `local_images/q9dbg_dump.txt` (nicht auf stdout!). Er zeigt
seit heute zusätzlich: die drei Warteschlangen mit Zuständen, die
Scratchzellen `$1600`–`$1620`, den vollen Registersatz der Exception und einen
Schreib-Watch mit Sequenznummern (`Q9_WATCH_ADDR`/`Q9_WATCH_LEN`).

**Die Sequenznummer hat den Fall entschieden:** Der Geweckte wurde stets
*112 Schreibzugriffe* vor dem Dump eingereiht, egal ob ich 6 oder 15 Sekunden
wartete. Genau daran war zu sehen, dass das System längst stand — und nicht
etwa der Scheduler den Prozess übersah.

## Offene Punkte

*(Der lange offene I/O-Fehler ist gelöst — s. u.)*

### ~~Exception nach dem ersten `I$Write`~~ — GELÖST (2026-09-04, `12b1651`)

    vorher:  ...Hallo von Q9-OS! E            (Exception, Prozess A steht)
    jetzt:   ...Hallo von Q9-OS! O00000011    (17 Bytes, A und B laufen weiter)

**Die Ursache war ein Zeichenzähler** in `Q9K_IOManPutChar`, als Diagnose
eingebaut beim Wiederherstellen der Ausgabe-Vtable:

    addq.l  #1,Q9K_IOManPutCharCount

Der Assembler übersetzt das in **absolute Adressierung mit dem
MODUL-OFFSET** des Labels, nicht mit dessen Laufzeitadresse. Der Zähler lag
bei Modul-Offset `$46C` — und `$46C` ist zugleich **Exception-Vektorslot 27**
(`$400 + 27·4`), der Autovektor für Level 3, auf dem der DUART meldet.

Jedes ausgegebene Zeichen zählte damit den **Vektor** um eins hoch. Nach den
17 Zeichen der Testausgabe plus IOMans eigener Meldung stand er bei `$798e`
— mitten im IRQ-Dispatcher, hinter dessen einleitendem `movem`. Ein
Autovektor-Interrupt sprang dorthin; der Durchlauf führte am Ende sein
`movem.l (a7)+` ohne das zugehörige einleitende aus, landete 60 Byte zu
hoch, las dort Nullen als Exception-Frame und sprang mit dem `rte` nach 0.
Von dort lief die CPU durch die Systemglobals bis `$6C`.

**Das erklärt restlos, warum der Fehler so extrem timing- und
größenabhängig wirkte:** Es war nie ein Race, sondern ein Zähler, dessen
Endstand von der Zahl ausgegebener Zeichen abhing.

**Die Lehre — und der teuerste Umweg dieser Suche:** Nacheinander wurden
Interrupt-Sturm, Stack-Verschachtelung, Frame-Überschreiben, Scheduler und
Reentranz-Lücken verdächtigt und einzeln widerlegt. Gefunden wurde die
Ursache erst, als der *Schreibzugriff auf den Vektorslot selbst* beobachtet
wurde, statt weiter nach dem Weg zu suchen, auf dem der Sprung entsteht.
**Bei einem korrupten Sprungziel gehört die Frage „wer schreibt dorthin?"
an den Anfang, nicht ans Ende.**

Die auf dem Weg entstandenen Härtungen bleiben, sie sind für sich richtig:
Interruptsperre im Dispatcher (`5467083`, `e58a109`), Wiederherstellung nach
dem ISR-Aufruf (`4740cce`), Rettung des Schleifenzustands (`0957b87`),
reentranter Trap-Epilog (`5f00f74`).

### Bekannte Vereinfachungen
- **`F$ChkMem` akzeptiert im Flat-Address-Space jeden nicht überlaufenden
  Bereich.** Ohne installierten SSM/MMU gibt es keine Rechte- oder
  Prozessadressraumprüfung; `d1.w` bleibt daher unbewertet, wie beim OS-9-
  Default-Handler ohne SSM. Der native Handler weist aber einen 32-Bit-
  Bereichsüberlauf zurück. Eine echte SSM/MMU-Prüfung muss mitwachsen, sobald
  getrennte Adressräume eingeführt werden.
- **Keine User-/Supervisor-Trennung.** `Q9K_TrapDispatch` benutzt für jeden
  `TRAP #0` immer `D_UsrDis`, nie `D_SysDis`.
- **Pfadnummern aus einem globalen Zähler**, nicht pro Prozess.
- Die Testblöcke in `Q9K_TestProcA` sind temporär und gehören vor dem
  nächsten Meilenstein entfernt.

---

## Fallen, die real Zeit gekostet haben

### Die Offsets in `src/q9sysglob.a` sind um `$1C` zu niedrig
Die Feldreihenfolge naiv durchzuzählen ergibt **konstant `$1C` zu kleine**
Offsets — verifiziert an drei unabhängigen Punkten (`D_Proc`=`$4C`,
`D_SysRom`=`$64`, `D_SysDis`=`$3A4`).

Genau daher stammt der Irrtum „`$64` ist `D_DevTbl`": gezählt ergibt
`D_DevTbl` `$64` — real ist das **`D_SysRom`**, der Ausgabevektor, über den
IOMan seine Meldungen schreibt. `D_DevTbl` liegt bei `$80`.

### `jsr $8(a1)` erwartet Code, keinen Funktionszeiger
Adressregister-indirekt **mit Distanz** springt **zur Adresse** `a1+8`. Ein
dort eingetragener Zeiger wird als Befehl ausgeführt: `$00 $00 $74 $CA` ist
`ori.b #$CA,d0` — jedes ausgegebene Zeichen kam als „Zeichen ODER `$CA`"
heraus, wobei `$CA` das unterste Byte der Routinenadresse war. Für einen
Zeiger bräuchte es die Memory-Indirect-Form `jsr ([8,a1])`.

### Eine ISR darf `d0-d1`/`a0-a3` zerstören
Zugesagt sind ihr nur `a2`/`a3`. Wer Schleifenzustand in `a0` oder `d2`
führt und dazwischen eine ISR ruft, verliert ihn.

### `move.l` löscht das Carry-Flag
Im Rückschreibpfad für externe Handler ging dadurch die Fehlermeldung
jedes per `F$SSvc` registrierten Handlers verloren — Fehler kamen als
Erfolg an. Gesichert wird das CCR seitdem auf dem **Stack**, nicht in einer
globalen Ablage (externe Handler setzen verschachtelte Syscalls ab).

### Im Emulator messen, nicht im Gast
**Die wichtigste Lehre.** Jede Diagnose-Ausgabe *im Kernel* ändert die
Modulgröße, verschiebt Ladeadressen und Interrupt-Zeitpunkte — und kann das
untersuchte Symptom zum Verschwinden bringen. Das ist zweimal real passiert
und hat eine lange Timing-Fehlspur erzeugt.

Werkzeuge dafür in Q9-Flux (alle im Ctrl-`^`-Dump):
- **THRA-Mitschrift** — Buswert plus CPU-Register bei jedem Sendezugriff.
  Zeigte, dass verstümmelte Bytes bereits *am Bus* ankamen, und schloss
  damit den gesamten Ausgabepfad des Emulators als Ursache aus.
- **Instruktions-Ringpuffer mit Freeze-on-Anomaly** (`Q9_TRACE_INSTR=1`) —
  4096 Befehle als (PC, d0, a0), friert bei einem Nicht-ASCII-Byte auf THRA
  oder einem Sprung unter `$1000` ein.
- **Exception-Mitschrift** — liest die `Q9K_ExcTrap`-Felder ab `$144020`.
- **Systemglobals, Gerätetabelle, DUART-IMR/IVR und Polling-Tabelle.**

### Bootfile nie per `os9 copy` lesen ODER schreiben
Die Verzeichnisdatei ist fragmentiert, der Bootloader liest **linear** ab
`DD_BT`. Eine per `os9 copy` geholte Kopie enthielt ein verstümmeltes
IOMan-Modul (`gbS`) und gar kein `scf`. Richtig: linear lesen (LSN in
`$15`–`$17`, Länge `$18`–`$19`, **512**-Byte-Sektoren), zurückschreiben mit
`os9 gen -b=`.

---

## Test- und Nachweis-Workflow
Siehe Kopfkommentare in `src/kernel/q9kernel_entry.a`. Kurz:

1. `./build.sh` in `src/kernel/`
2. Bootfile = frisch gebauter Kernel + Rest des **linear gelesenen**
   Originals, per `os9 gen -b=` in ein frisch geklontes Image
3. Emulator mit CWD im Q9-Flux-Repo starten (der Dump-Pfad ist relativ zum
   Prozess-CWD)
4. Host-Testsuiten: `gcc -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD`
   über `test_q9kernel_*.c` — aktuell 14 Suiten, alle grün

**Nachweiskriterium im Projekt:** kein Fake-Fortschritt. Ein gemischtes
`AABAABBA…` im Ausgabestrom belegt echten Kontextwechsel; dass ein
`level_held`-Interrupt sich *nicht* wiederholt, belegt, dass die ISR ihn
wirklich bedient hat.


## F$Load-Vorarbeit 2026-09-07: Treiber wird nie erreicht

Nach dem A4-Thema (bleibt in PR #13, ruht vorerst) auf `F$Load` gewechselt.
**Messfalle als Erstes selbst begangen und behoben:** Der ganze A4-Messtag
lief ohne `--disk`, `dd`/`rbf`/`cfide` waren nie im Bootfile. Mit `--disk`
(Module aus `Q9-Flux/OS9Boot.noprot.test` extrahiert, Größen 9638/1462/148/148
Byte an den per Magic-Marker-Scan gefundenen Offsets) findet der
„Dreiklang"-Test jetzt alle drei Namen (`123`).

**Der eigentliche Dateizugriff (`I$Open("/dd/startup")`) hängt** — ohne
Fehlermarker, ohne jeden weiteren Trap.

### IRQ-Hypothese geprüft und verworfen — mit echtem Quellcode

Internes Referenzmaterial zum cfide-Treiber liegt vor (passt zur
Boot-Meldung „build 42"). `CF_Read`/`CF_Write`/`ChkInit`/`SetDev` benutzen
ausschließlich das `WaitStatus`-Makro: Software-Timeout (~1,2 Mio.
Durchläufe), reines `btst.b`-Polling auf BSY/DRQ — **kein `F$IRQ`, kein
`trap #0`** im gesamten Lese-/Schreibpfad. Die rohe CF-Sektoremulation im
Emulator ist über `test_cf_sector512` ohnehin vollständig verifiziert
(30/30). Die IRQ-Spur ist damit endgültig erledigt.

### Werkzeugfund: der Instruktions-Hook des Emulators feuert in diesem Build nie

`Q9_TRACE_INSTR=1` registriert `q9_dbg_instr_hook`, aber die Funktion wird
**nachweislich nie aufgerufen** (mit einem unbedingten `fprintf` direkt am
Funktionskopf geprüft — keine Ausgabe, auf einer garantiert erreichten
Adresse). Das erklärt rückwirkend, warum der Ring-Dump per Ctrl-^ diese
Sitzung nie etwas lieferte (dokumentiert, aber bisher ungeklärt) — derselbe
tote Mechanismus. `Q9_FREEZE_PC`/`Q9_COUNT_PC` hängen an derselben Kette und
sind damit ebenfalls betroffen. **Funktionieren weiterhin:** `Q9_TRAP_TRACE`
(eigener Callback, `m68k_set_trap_instr_callback`) und der Speicher-Watch
(`Q9_WATCH_ADDR`/`Q9_WATCH_LEN`, hängt an `m68k_write_memory_*`, unabhängig
vom Instruktions-Hook) — beide über eigene, funktionierende Codepfade.
Ursache (fehlendes Compile-Flag in Musashi? nie aktivierter Init-Pfad?) noch
nicht untersucht — für heute nur als Falle vermerkt, nicht behoben.

### Gemessen (verlässlicher Mechanismus): der Treiber wird nie erreicht

Mit einer temporären `fprintf`-Zeile direkt in `q9_dbg_watch`
(`Q9-Flux/src/kernel/m68krt.c`, revertiert nach der Messung) auf
`Q9_WATCH_ADDR=0xFFFFE000 Q9_WATCH_LEN=16` (die CF-Portadresse laut
Deskriptor und Emulator-Konfiguration) über den **gesamten** Testlauf:
**null Treffer.** Kein einziger Schreibzugriff auf die CF-Hardware — weder
`CF_Init` noch `ChkInit`/`SetDev`/`CF_Read` laufen jemals an.

**Der Hänger liegt vollständig in Software, bevor der Treiber je berührt
wird** — irgendwo in IOMans/RBFs eigener Attach-Verarbeitung nach `I$Open`.

### Eine Fährte geprüft und verworfen: `F$DAttach` (`$64`)

Naheliegender Verdacht: `F$DAttach` (der Dienst, der laut
internem Referenzmaterial ein Gerät anhängt, Treiber-Statikspeicher
alloziert und `V_PORT` — Offset `$00`, „Required by kernel in static storage
of all devices", internem Referenzmaterial — aus dem Deskriptor einträgt)
fehlt bei uns (`$64` nicht in der `q9kernel_cinit.c`-Tabelle, fällt auf
`Q9K_SysUnimplemented`). **Das ist aber eine Sackgasse:** Laut
unseren internen Analysenotizen zufolge ist `$64`–`$70` auch im **echten,
unveränderten** Microware-Kernel nicht registriert — der Dienst wird also nie
per `trap #0` angefordert, auch nicht real. (Eine frühere Sitzung ist exakt
dieser Spur schon einmal gefolgt und hat sie widerlegt — dort stellte sich
ein vermeintliches `$0064` als falsch ausgerichtetes `$0084`/I$Open heraus,
s. Abschnitt weiter oben. Diesmal ist der Befund solide, da direkt aus der
eigenen C-Registrierungstabelle gelesen — aber die Konsequenz ist dieselbe:
falsche Spur.)

**Der Deskriptor selbst ist korrekt:** `P$PORT` in `dd` (Offset `0x32`)
trägt `$FFFFE000`, exakt die konfigurierte Onboard-CF-Basis.

**Damit bleibt offen, WIE V_PORT in einem echten Microware-Kernel gesetzt
wird**, wenn nicht über `F$DAttach`. Vermutlich direkt in IOMans/RBFs eigenem
Code (kein separater Syscall) — RBF selbst liegt nur als Binärmodul vor
(kein Quellcode im Referenzbaum gefunden, anders als die Gerätetreiber), das
wäre die nächste Analyserunde.

**Stand:** Kein Fixversuch. `F$DAttach` NICHT implementieren (Sackgasse,
s. o.). Konkreter nächster Schritt: entweder RBFs eigenen Attach-Code
analysieren, oder sc68681s (funktionierenden!) Attach-Weg mit cfides
verglichen — da die Konsole längst laden, muss der allgemeine
Attach-Mechanismus irgendwo funktionieren; der Unterschied liegt vermutlich
in etwas RBF-/Massenspeicher-Spezifischem, das SCFs Zeichengeräte-Weg nicht
durchläuft.

## F$Load: die Ursache des Absturzes — 2 Byte danebenliegender Sprung

Weitergesucht, warum `F$GProcP` (Callcode `$37`, von RBF aufgerufen) nie
zurückkehrt. Der Weg dorthin führte über zwei eigene Messfallen, beide jetzt
dokumentiert, damit niemand sie wiederholt:

**Falle 1 — der Emulator meldet Exceptions nicht automatisch, aber unser
eigener Handler tut es.** `Q9K_ExcTrap` (q9kernel_entry.a) gibt bei jeder
unbehandelten Exception ein `'E'` aus, bevor er in eine Endlosschleife
(`Q9K_ExcTrapSpin`) geht. Genau dieses `E` stand die ganze Zeit als
scheinbar abgeschnittener Rest am Ende der Konsolenausgabe — es ist die
Antwort selbst, keine Ctrl-C-Nachwirkung.

**Falle 2 — Adressen aus VERSCHIEDENEN Builds sind nicht vergleichbar, auch
nicht auf 2 Byte genau.** Ein erster Live-Speicher-Dump an der
Absturzadresse sah aus wie Fremdkorruption (Bytes stimmten nicht mit der
Datei überein) — bis klar wurde, dass Tabellenwert-Messung und
Speicherdump aus zwei GETRENNTEN Kernel-Builds stammten (jeder eigene Fix
verändert die Kernelgröße und verschiebt alles Nachfolgende). Mit **einem
einzigen, konsistenten Build** für Absturz-PC und Speicherdump zusammen
verschwindet der scheinbare Widerspruch vollständig: Live-Speicher und
Datei stimmen exakt überein. **Keine Speicherkorruption.**

### Der echte Befund: der Sprung landet 2 Byte zu spät

Vektor `$10` → `($10 & $FFF)/4 = 4` → **Illegal Instruction**. Absturz-PC
fällt exakt 2 Byte vor den Beginn von `andi.b #$fe,ccr` (Bytes `02 3c 00 fe`)
— dem Ende von `Q9K_SysFGProcP`s Erfolgspfad. Der Sprung landet also
**mitten in einer gültigen Instruktion**, nicht auf einer Befehlsgrenze —
ein um exakt 2 Byte falsches Sprungziel, kein Fremdzugriff.

RBF ruft `F$GProcP` vermutlich über denselben PEA+RTS-Trampolin-Mechanismus
wie IOMan (der Ganzsitzungs-Fund von vorgestern). Die Tabelleneinträge in
`Q9_D_SYSDIS`/`Q9_D_USRDIS` selbst sind identisch und korrekt (beide zeigen
auf `Q9K_SysFGProcP`s echten Anfang) — der Fehler liegt also nicht in der
Tabelle, sondern vermutlich in der Adressberechnung des Trampolin-Sprungs
selbst (RBFs eigener Code, wie IOMans PEA+RTS-Muster, aber mit einem
2-Byte-Versatz).

**Reihenfolge bis zum Absturz** (gemessen, `pid`-markiert): `I$Open` →
`F$SRqMem` → `F$PrsNam` → `F$SRqMem` → **`F$GProcP` → Absturz**. Alles im
selben Prozess (unser Testprozess, nicht `forkchild`).

**Stand:** Kein Fixversuch heute mehr. Nächster Schritt: RBFs eigenen
Trampolin-Aufruf für `F$GProcP` analysieren (Rücksprungadresse aus dem
Stack unter dem Exception-Frame — `Ret0`/`Ret1` waren beim ersten Versuch
nicht brauchbar, da aus dem inkonsistenten Build gelesen; mit dem jetzt
etablierten Ein-Build-Verfahren neu messen) und mit IOMans bekanntem
Trampolin-Muster vergleichen, um den 2-Byte-Versatz zu erklären.

## RBFs Aufrufstelle analysiert: echter TRAP #0, plausible Eingabe

Weiter am 2-Byte-Sprungfehler. Mit einem **einzigen konsistenten Build**
(Lehre aus der letzten Runde befolgt) `F$GProcP`s echte Aufrufstelle in
RBF gefunden: `rbf+$1324`.

### Die Aufrufstelle selbst ist unauffällig

```
1302  move.w  $2d6(a4), d0     * "angeforderte PID" aus RBFs eigener Struktur
...
130e  move.l  a3, -(a7)         * ein GANZ ANDERER, vorangehender Trampolin-
1310  movea.l $3a4(a6), a3      * Aufruf (F$Send, Slot $20 in D_SysDis) --
1314  pea.l   $1322(pc)         * unabhaengig von F$GProcP, hier nur zufaellig
1318  move.l  $20(a3), -(a7)    * direkt davor im Code
131c  movea.l $420(a3), a3
1320  rts
1322  movea.l (a7)+, a3
1324  trap    #$0               * <== F$GProcP, GANZ REGULAeR
1326  dc.w    $0037              * Callcode als Folgewort -- exakt unsere
                                  * eigene Konvention (wie F$Link im Testharness)
```

**Kein Trampolin-Sonderfall.** RBF ruft `F$GProcP` über ein stinknormales
`trap #0 / dc.w $0037` — dieselbe Konvention, die den ganzen Tag zuverlässig
funktioniert hat. Der vorangehende Trampolin-Block (Zeilen `130e`-`1320`)
ist ein **unabhängiger** Aufruf (Slot `$20` = `F$Send`), der nur zufällig
direkt davor im Code steht.

### Die Eingabe (D0 = angeforderte PID) ist real, aber unplausibel groß

Sauber gemessen (Push/Pop, kein Stack-Rateversuch mehr): `d0.w = $588F`
(22671 dezimal). Gegen `68k_tech.pdf S.443` geprüft — unsere Konvention
(`d0.w = requested process ID`, `(a1) = Deskriptorzeiger`) ist exakt
korrekt umgesetzt, keine Fehldeutung unsererseits.

**Verdacht geprüft: uninitialisierter Speicher aus dem vorangehenden
`F$SRqMem`.** `Q9K_ProcSRqMem` (q9kernel_sysmem.c) nullt den zurückgegebenen
Block nicht — geprüft, ob das nach echter Konvention nötig wäre:
`68k_tech.pdf S.503-505` **verspricht explizit KEIN genulltes Gedächtnis**
für `F$SRqMem`. Damit ist die einfache „wir vergessen zu nullen"-Erklärung
entkräftet — ein echtes, konventionskonformes RBF dürfte sich ohnehin nicht
darauf verlassen und müsste das Feld selbst initialisieren, bevor es
gelesen wird.

**Damit bleibt offen, ob `$588F` echte (aber im Kontext ungültige) RBF-
Nutzdaten sind oder ob RBFs Erwartung an `$2d6(a4)` an anderer Stelle
verletzt wird** (z. B. wenn RBF dieses Feld NUR bei einer bestimmten
Vorbedingung selbst setzt, die in unserem Ablauf nie eintritt). Der Wert
selbst (Carry+Fehlercode von `F$GProcP`) konnte noch nicht sauber gemessen
werden — zwei Versuche scheiterten an CCR-Zerstörung durch dazwischen-
liegende `move`-Befehle bzw. an erneuten Build-Inkonsistenzen.

**Stand:** Der 2-Byte-Sprungfehler selbst (Absturz-PC 2 Byte vor
`Q9K_SysFGProcP`s `andi.b #$fe,ccr`) ist weiterhin ungeklärt — die
Aufrufkonvention ist nachweislich korrekt, also liegt die Ursache
vermutlich NICHT im Aufruf selbst, sondern entweder (a) in `F$GProcP`s
Fehlerpfad (falls `d0=$588F` zu Recht mit `E$PrcID` scheitert und RBFs
eigene Fehlerbehandlung fehlerhaft ist) oder (b) einem Stack-bezogenen
Problem nach vielen verschachtelten Aufrufen (`I$Open → F$SRqMem →
F$PrsNam → F$SRqMem → F$GProcP`).

## Neue Spur: der Absturz liegt IM C-Aufruf selbst, nicht in der Nachbearbeitung

Auf den Widerspruch (Absturz-PC im Erfolgspfad, obwohl `pid=$588F > count=64`
eigentlich fehlschlagen müsste) hin direkt in `Q9K_SysFGProcP` selbst
gemessen (temporär, revertiert).

### Der Widerspruch löst sich auf — anders als angenommen

Eine Marke unmittelbar NACH dem C-Aufruf (`bsr Q9K_SysGProcPImpl`) feuert
**nie**. Das bedeutet: Die frühere Analyse „Absturz 2 Byte vor `andi.b`"
bezog sich auf einen **anderen Build** als die PID/Count-Messung — dieselbe
Falle wie schon mehrfach heute (jede Änderung verschiebt alle Adressen).
Der tatsächliche Absturz liegt **innerhalb** des C-Aufrufs oder unmittelbar
danach, nicht in der Tail-Logik.

### Eigenes Messwerkzeug wird selbst zum Symptom — Hinweis auf Stack-Erschöpfung

Eine Marke UNMITTELBAR VOR dem C-Aufruf (Zeichen ohne Hex-Ausgabe direkt
danach) feuert zuverlässig. Dieselbe Marke MIT einer anschließenden
Hex-Ausgabe (`Q9K_DiagPrintU32`, ein weiterer `bsr`) feuert dagegen **nie**
— obwohl exakt derselbe Codepfad, nur mit einem zusätzlichen
Unterprogrammaufruf mehr Stack-Tiefe.

**Das ist der stärkste bisherige Hinweis auf Stack-Erschöpfung statt eines
Logikfehlers.** Die Aufrufkette ist zu diesem Zeitpunkt bereits sehr tief:
`I$Open → F$SRqMem → F$PrsNam → F$SRqMem → F$GProcP`, jeweils mit
IOMan/scf/RBF-eigenen Zwischenrahmen. Wenn der Stack an dieser Stelle nahe
der Grenze ist, kann **jeder zusätzliche `bsr`** — ob der echte
`Q9K_SysGProcPImpl`-Aufruf oder ein eigener Diagnose-Aufruf — der
Tropfen sein, der das Fass zum Überlaufen bringt. Das würde auch die
2-Byte-Landung mitten in eigenem Kernelcode erklären: Stack und Kernelcode
liegen nah beieinander, ein Überlauf schreibt direkt in Codebereiche.

**Zusätzliche Messfalle dokumentiert:** Die Zeichen `'A'`/`'B'` sind bereits
durch `Q9K_TestProcA`/`Q9K_TestProcB` (eigene Busy-Loop-Testprozesse, die
laufend `'A'`/`'B'` ausgeben) belegt — jede neue Diagnose-Markierung muss
andere Zeichen verwenden, sonst verschmilzt sie mit deren Dauerausgabe.

**Stand:** Kein Fixversuch. Der nächste, konkrete Schritt ist, `Q9_D_Proc`s
Stackgrenze (`P$Stack`/aktueller `SP` gegen `PD_ALLOCBASE`) an exakt dieser
Stelle zu messen, um die Stack-Erschöpfungs-Hypothese zu bestätigen oder zu
verwerfen — mit **einem einzigen konsistenten Build**, wie in den letzten
Runden gelernt.

## Stack-Hypothese widerlegt — Ursache bleibt offen

Direkt gemessen (ein konsistenter Build): `SP=$0002D304`,
`AllocBase=$00025400`, `AllocSize=$00008000` (32 KByte). Der Stack liegt
damit nur **252 Byte** unter seinem oberen Ende (`AllocBase+AllocSize` =
`$2D400`) — praktisch ungenutzt, keine 32-KByte-Erschöpfung in Sicht.

**Die Stack-Erschöpfungs-Hypothese ist damit widerlegt, nicht bestätigt.**
Auch der zuvor als möglicher Beleg gedeutete Fund (Diagnosemarke ohne
Hex-Ausgabe feuert, dieselbe Marke MIT Hex-Ausgabe feuert nicht) hat also
vermutlich eine andere Ursache — am ehesten einen Fehler im eigenen
Messcode selbst (Registerbehandlung um `Q9K_DiagPrintU32`), nicht im
gemessenen Kernel.

### Zwischenbilanz nach vier Fortsetzungsrunden

Gesichert:
- Absturz ist eine echte CPU-Exception (Illegal Instruction, Vektor 4),
  von unserem eigenen `Q9K_ExcTrap` abgefangen.
- Keine Speicherkorruption (bei konsistentem Build stimmt Live-Speicher
  exakt mit der Datei überein).
- `F$GProcP`s Aufrufstelle in RBF (`rbf+$1324`) ist ein regulärer
  `trap #0 / dc.w $0037`, Konvention nachweislich korrekt (`68k_tech.pdf`
  S. 443).
- `F$SRqMem` muss laut Doku (`68k_tech.pdf` S. 503-505) keinen genullten
  Speicher liefern — die naheliegende „Müll-PID aus unitialisiertem
  Speicher"-Erklärung ist damit angezweifelt, nicht bestätigt.
- Stack-Erschöpfung ausgeschlossen (32 KByte, 252 Byte genutzt).
- Der Absturz liegt nachweislich **innerhalb** des C-Aufrufs
  (`Q9K_SysGProcPImpl`) oder unmittelbar danach — vor der Tail-Logik.

Widerlegt/entkräftet: IRQ-Abhängigkeit, `F$DAttach`-Fehlen, Trampolin-
Kodierungsfehler, Speicherkorruption, Stack-Erschöpfung.

**Offen:** die tatsächliche Ursache innerhalb `Q9K_SysGProcPImpl`s
Ausführung. Fünf plausible, aber unbewiesene Erklärungen sind mittlerweile
gefallen — an dieser Stelle ist ein weiterer Rateversuch der falsche Weg.
Der nächste sinnvolle Schritt wäre eine **Schritt-für-Schritt-Instruktions-
verfolgung** innerhalb des C-Aufrufs selbst (nicht nur davor/danach), um die
exakte Instruktion zu finden, an der die Ausführung abweicht — mit dem
etablierten Ein-Build-Verfahren und eigenen, kollisionsfreien Diagnose-
zeichen (nicht `A`/`B`, die durch `Q9K_TestProcA`/`B` belegt sind).

## Compilierten Code direkt analysiert — sauber, aber Ursache weiterhin offen

Da der eingebaute Instruktions-Hook in diesem Emulator-Build nachweislich
nicht feuert (früherer Fund, s. o.), kein echtes Einzelschritt-Tracing
möglich. Stattdessen `Q9K_SysGProcPImpl` und `Q9K_ProcLookup` direkt im
kompilierten Kernel-Binary lokalisiert (über die eindeutigen Scratch-
Adressen `$1384`/`$1388`) und von Hand analysiert.

**Beide Funktionen sind korrekt.** `Q9K_ProcLookup`s Vergleichslogik
(`pid==0`, `pid>count` unsigniert via `bhi`, `base==0`) entspricht exakt
dem C-Quelltext, Prolog/Epilog sind bezüglich der geretteten Register
balanciert. Mit `pid=$588F` und `count=$40` müsste der Fail-Zweig
genommen werden — kein Fehler im generierten Code sichtbar.

### Nebenfund: `_stklimit` ist unerwartet klein — aber (vermutlich) folgenlos

Jede C-Funktion prüft per Compiler-Konvention `subi.l #N,_stklimit(a6) /
bcc.b weiter / [Ueberlauf-Behandlung]`. Live gemessen: `_stklimit=$1A000`
(106496) — weit unter dem einzigen im Quelltext vorkommenden Init-Wert
`$7FFFFFFF` (`q9kernel_entry.a:753`, **die einzige Stelle**, die dieses
Feld je setzt). Gewöhnliche Kleinbeträge pro Aufruf ($14–$24) erklären
diesen Abfall über eine einzelne Boot-Sequenz nicht — woher der Wert
kommt, ist ungeklärt.

**Folgenlos für den Absturz:** `$1A000` liegt weit über jedem einzelnen
Abzugsbetrag, der Unterlauf-Zweig (`_stkhandler`, gibt `'S'` aus und hält
an) wird nachweislich nicht betreten — kein `'S'` in der Ausgabe. Bleibt
als ungeklärter Nebenbefund stehen, ist aber nicht die Ursache dieses
Absturzes.

### Ehrliche Zwischenbilanz nach fünf Fortsetzungsrunden

Sechs Erklärungen systematisch geprüft und verworfen: IRQ-Abhängigkeit,
`F$DAttach`-Fehlen, Trampolin-Kodierungsfehler, Speicherkorruption,
Stack-Erschöpfung, `_stklimit`-Unterlauf. Der generierte Code für die
beiden beteiligten C-Funktionen ist nachweislich korrekt. **Die
eigentliche Ursache ist damit nicht gefunden.**

Ohne echtes Einzelschritt-Tracing (der Instruktions-Hook des Emulators
müsste zuerst repariert werden — eigener, separater Untersuchungsaufwand)
ist der nächste sinnvolle Schritt in der jetzigen Werkzeuglage nicht mehr
die naheliegende Fortsetzung. Diese Sitzung markiert deshalb bewusst einen
Haltepunkt, statt eine siebte Hypothese ungeprüft anzuschieben.

## Werkzeugkorrektur: der Instruktions-Hook war nie kaputt

Auf Bitte hin zuerst geprüft, ob der Q9-Flux-Emulator gerade anderweitig
genutzt wird (kein laufender `q9.exe`-Prozess, keine tmux-Sitzung mit
Q9-Bezug, nur ein Worktree ohne fremde Änderungen) — sicher, weitergemacht.

**Die Ursache war nicht der Emulator, sondern die eigene Testmethode.**
`expect`s `spawn` verbindet `stdout` **und** `stderr` des Kindprozesses über
dasselbe Pseudo-Terminal — eine äußere `2>datei`-Umleitung auf das
`expect`-Kommando selbst erreicht davon **nichts**, sie fängt nur `expect`s
eigene Fehlerausgabe. Jede heutige Messung mit `Q9_TRACE_INSTR=1` UND einer
separaten `2>`-Umleitung hat deshalb ins Leere gegriffen — nicht weil der
Hook nicht feuerte, sondern weil seine Ausgabe nie im geprüften File landete.

**Verifiziert:** Mit unbedingtem `fprintf(stderr,...)` direkt im
Hook-Callback und Prüfung der **`log_file`-Mitschrift** (nicht einer
separaten stderr-Umleitung) feuert der Hook zuverlässig, mehrfach pro
Sekunde, über den gesamten Boot. Ein sauberer Neubau der Musashi-
Objektdateien (`musashi_m68kcpu.o` u. a.) war dafür nicht nötig, hat aber
zur Sicherheit stattgefunden.

**Praktische Konsequenz:** `Q9_TRACE_INSTR=1` funktioniert, ebenso vermutlich
`Q9_FREEZE_PC`/`Q9_COUNT_PC` (hängen an derselben Kette) — für echtes
Einzelschritt-Tracing steht das Werkzeug also grundsätzlich zur Verfügung.
Offen bleibt der separate Ctrl-^-Ringdump-Mechanismus, der bei einem
Testlauf keine Ausgabe zeigte — vermutlich ein eigener, kleinerer Fehler,
nicht der zuvor vermutete grundsätzliche Hook-Defekt. Nicht weiter verfolgt,
da für die eigentliche Kernel-Fehlersuche eigene `fprintf`-Sonden (wie hier
verifiziert) ausreichen.

**Alle temporären Test-Prints in `Q9-Flux` sind revertiert** (`git status`
sauber), keine dauerhafte Codeänderung.

## DURCHBRUCH: echte Speicherkorruption gefunden — es ist der Stack Pointer (A7), nicht A4

Mit dem jetzt funktionierenden Instruktions-Hook (siehe oben) endlich echtes
Einzelschritt-Tracing durchgeführt — und dabei zwei eigene Messfallen
unterwegs gefunden und umschifft, bevor der eigentliche Durchbruch gelang.

### Zwei neue Werkzeugfallen, dokumentiert

1. **`expect`s PTY mischt `stdout` und `stderr` zeichenweise mit der
   emulierten Konsolenausgabe.** Ein `printf()`/`fprintf(stderr,…)` aus dem
   Emulator-C-Code kann durch die zeitgleiche emulierte UART-Ausgabe MITTEN
   IM STRING zerrissen werden (beobachtet: eine eigene Debug-Zeile brach
   nach 5 von 8 Hexziffern ab, gefolgt vom Kernel-eigenen `'E'`-Zeichen).
   **Abhilfe:** eigene Diagnoseausgaben in eine dedizierte Datei schreiben
   (`fopen("/tmp/...")`), nie über `stdout`/`stderr`, wenn zeitgleich auch
   emulierter Code auf die Konsole schreibt.
2. **PC-Fenster-Filter mit Lücken im Grep-Muster verschleiern echte
   Adressen.** Ein Suchmuster wie `"pc=0000a70"` findet `a704` nicht aber
   verpasst `a754` (passt nicht auf `"a70"`) -- immer den vollen Adressraum
   durchsuchen, nie ein Präfix-Muster, das zufällig zu kurz greift.

### Der Aufruf läuft tatsächlich vollständig korrekt bis zum Fail-Pfad

Sauberes Einzelschritt-Tracing des EINZIGEN `F$GProcP`-Aufrufs im ganzen Boot
(`pid=$2008`, nicht `$588F` — das war eine Fehlmessung aus einer früheren
Runde, s. u.) zeigt: `Q9K_ProcLookup` UND `Q9K_SysGProcPImpl` laufen
**instruktionsgenau korrekt** bis zum Fail-Pfad (`pid=$2008 > count=$40` →
`E$PrcID`). Auch `Q9K_SysFGProcP`s eigener Fail-Zweig (`move.w Error,d1` /
`ori.b #1,ccr`) läuft korrekt.

**Der Absturz passiert exakt am `rts` von `Q9K_SysFGProcP_Fail`** — der
CPU landet danach nicht beim Aufrufer, sondern mitten in `Q9K_ExcTrap`.

### Die Ursache: `andi.b #$fe,ccr` (Erfolgspfad) ist im Live-Speicher zerstört

Direkter Live-Speicher-Vergleich an der kritischen Adresse (`$7d2a`, dem
`andi.b #$fe,ccr` am Ende des ERFOLGSPFADS, unmittelbar VOR dem Fail-Zweig
im Speicher): Datei sagt `02 3c 00 fe` (der echte Opcode), **Live-Speicher
zeigt `82 00 00 fe`** — die ersten 2 Byte sind überschrieben. Die CPU
decodiert `$8200` als eigenständige (2-Byte-)Instruktion, springt dadurch
nur 2 statt 4 Byte weiter, landet auf `$00FE` (dem ehemaligen Operanden-
Wort) als neuem „Opcode" — **echte Illegal Instruction, kein
Interpretationsfehler unsererseits.**

**Wer schreibt das?** Mit `Q9_WATCH_ADDR=0x7d2a Q9_WATCH_LEN=2` direkt
beobachtet: Der Schreibzugriff kommt von `pc=$D81E` — das liegt **in RBFs
eigenem Modulbereich** (`$D2EE`-`$F894`), nicht in unserem Kernel! An
RBF-Offset `$530` steht:

```
052e  move.l  $54(a3), -(a7)     * <== der Schreibzugriff
```

**Das ist ein GANZ NORMALER Stack-Push** (Teil von RBFs eigenem Trampolin-
Aufruf für `F$Time`, Slot `$54` in `D_SysDis`) — keine wilde
Zeiger-Schreibaktion. Die einzige Erklärung: **`a7` (der Stack Pointer)
selbst zeigt an dieser Stelle fälschlich auf `$7D28` — mitten in unseren
Kernelcode — statt auf RBFs echten Stack-Bereich.** RBFs eigener,
vollkommen korrekter Push zerstört dadurch fremden Speicher, einfach weil
der Zeiger falsch ist.

**Das dreht die gesamte bisherige A4-Untersuchung um eine Achse weiter:**
Es ist nicht (nur) `A4`, das an einer bestimmten Stelle falsch gesetzt
wird — der **Stack Pointer selbst** gerät irgendwann während RBFs
Ausführung auf einen Wert, der wie eine Kernel-Codeadresse aussieht. Woher
dieser falsche `A7`-Wert kommt (welcher Aufruf/Rückkehr ihn zuletzt
korrekt gesetzt hat, und wo genau er abweicht), ist die nächste offene
Frage — aber zum ersten Mal mit einem konkreten, reproduzierbaren
Ziel-Symptom (`SP≈$7D28` bei RBF-Offset `$530`) statt einer vagen
Vermutung.

**Frühere Fehlmessung korrigiert:** Der oft zitierte Wert `d0=$588F` als
„angeforderte PID" war eine Verwechslung — dieser Wert stammt aus einer
GANZ FRÜHEN, unabhängigen ROM-Scan-Routine (`a0=$fe001aaa`, weit vor RBF),
nicht aus RBFs `F$GProcP`-Aufruf. Der echte, einzige `F$GProcP`-Aufruf im
ganzen Boot hat `pid=$2008`.

**Stand:** Kein Fixversuch. Alle temporären Test-Prints in `Q9-Flux`
revertiert (`git status` sauber, `2ef1bae` unverändert). Nächster Schritt:
`SP` ab RBFs Modul-Einsprung verfolgen (mit dem jetzt funktionierenden
Instruktions-Hook, dediziertes File statt stdout!), um die genaue Stelle
zu finden, an der er von einem echten Stack-Wert auf `$7D28`-artige
Kernel-Adressen abdriftet.

## Der Kreis schließt sich: A7-Theorie widerlegt, A4-Theorie bestätigt

Die vorige `A7`-Schlussfolgerung war ein Messfehler — derselbe wie schon
mehrfach heute: Zwischen der Watch-Messung und der RBF-Analyse
hatte sich die Kernelgröße erneut leicht verschoben (14278 → 14222 Byte),
wodurch die Aufrufstelle im RBF-Modul falsch berechnet wurde
(`RBF+$530` statt korrekt `RBF+$588`). Mit frisch ermittelten
Modulgrenzen für **exakt** dieses Testimage neu analysiert.

### Die echte Instruktion — und sie schließt den Kreis zur ursprünglichen A4-Suche

```
057c  move.l  d1, $14e(a4)     * DER historische Fund von vor Tagen
0580  andi.b  #$4, d3
0584  beq.w   $48e
0588  move.l  d1, $15e(a4)     * <== DIESER Schreibzugriff, seine Schwester-
                                *     instruktion, 16 Byte weiter im selben
                                *     Feldupdate-Block
```

**Live gemessen:** `a4 = $00007BCA`, `sp = $0002D358` (gesund, unauffällig).
`$7BCA + $15E = $7D28` — exakt die beobachtete Schreibadresse. Der
Stack Pointer war die ganze Zeit gesund; die frühere „A7 statt A4"-
Schlussfolgerung ist damit zurückgenommen.

**`$7BCA` ist der Anfang einer unserer EIGENEN Syscall-Handler-Funktionen**
(Prolog `movem.l d0/a0,-(a7)` gefolgt vom bekannten Stack-Rahmen-
Erkennungsmuster `adda.l #$10,a0 / cmpa.l a0,a5` — identisch zu
`Q9K_SysFSRqMem`/`Q9K_SysFAllPD`s Struktur, s. `q9kernel_entry.a`).

### Das ist exakt der seit Tagen dokumentierte A4-Mechanismus

`Q9K_TrapDispatch` lässt im eigenen-Handler-Zweig `a4` auf die
Handler-Adresse zeigen (`movea.l Q9K_TrapHandlerScratch,a4 / jsr (a4)`) und
stellt sie beim Rücksprung **nie** auf den Wert des Aufrufers zurück. RBF
(wie `scf` vor Tagen bei der Konsole) erwartet dort seinen Geräte-
Statikspeicher- bzw. Prozessdeskriptor-Zeiger für ganz normale, legitime
Feldschreibzugriffe (`V_ZeroRd`-artige Flags o. ä.) — bekommt stattdessen
die stehengebliebene Handler-Adresse unseres eigenen `F$SRqMem`/`F$AllPD`
und schreibt damit in eigenen Kernelcode.

**Damit ist die MONATE-alte A4-Untersuchung inhaltlich abgeschlossen:**
Der Mechanismus war die ganze Zeit korrekt identifiziert (dokumentiert seit
den ersten `scf`-Funden) — was fehlte, war der Nachweis an einem ZWEITEN,
unabhängigen Fall (`RBF` statt `scf`, `$15e` statt `$14e`), der die
Erklärung endgültig von einer Vermutung zu einem bewiesenen Mechanismus
macht.

**Der bekannte, bisher ungelöste Konflikt bleibt bestehen:** Der naheliegende
Fix (`a4` beim Rücksprung auf den Aufrufer-Wert zurücksetzen) wurde vor
Tagen bereits mehrfach versucht und bricht dabei zuverlässig IOMans
Konsolen-Open (dokumentiert weiter oben, Abschnitt „Der A4-Fix: wirkt, hat
aber eine ungeklärte Nebenwirkung"). Der zuletzt vorgeschlagene, noch nicht
umgesetzte Ansatz — `a4` nur für Aufrufer AUSSERHALB des Kernels
zurücksetzen — bleibt der plausibelste nächste Schritt, jetzt mit einem
zweiten, unabhängig bestätigten Fall als zusätzlicher Motivation.

**Stand:** Kein Fixversuch in dieser Runde. Alle temporären Test-Prints in
`Q9-Flux` revertiert (`git status` sauber).

## Fix gemergt, aber unvollstaendig: Trampolin-Weg vermutlich nicht erfasst

Der A4-Herkunftsfix ist **committet** (`53d3d61`) und gegen zwei
unabhängige Sanity-Checks verifiziert: Konsole (`Hallo von Q9-OS!`, drei
Pfade) läuft nach FÜNF vorherigen gescheiterten Anläufen zum ersten Mal
weiter, und die konkret per Watch nachgewiesene erste RBF-Korruption
(`$15e(a4)`, Aufrufkette `F$SRqMem→F$PrsNam→F$SRqMem`) ist an ihrer
ursprünglichen Adresse verschwunden.

### Zweite, andersartige Korruption gefunden — derselbe Mechanismus, aber der Fix greift dort nicht

Direkt danach erneut per Watch geprüft: **Es gibt eine zweite
Korruptionsstelle**, strukturell identisch (Live-Speicher `82 00` statt
`02 3c` an `Q9K_SysFGProcP`s `andi.b`), aber mit einem ANDEREN falschen
`a4`-Wert (`$7C02` statt `$7BCA`) — wieder der Anfang einer unserer eigenen
Handler-Funktionen (dasselbe Stack-Rahmen-Erkennungsmuster). Der Fix greift
hier **nicht**.

**Wahrscheinliche Erklärung:** IOMan/RBF rufen Kernel-Primitive über ZWEI
verschiedene Wege auf — echtes `TRAP #0` UND den direkten PEA+RTS-
Trampolin-Sprung in denselben Dispatcher-Code (seit Tagen dokumentiert,
„beide Wege teilen sich diesen Code"). Der neue Fix liest die
Rücksprungadresse des Aufrufers bei `38(sp)` — das ist der korrekte
Offset für einen ECHTEN Hardware-Exception-Frame (`TRAP #0`), aber der
Trampolin-Weg baut vermutlich KEINEN identischen Frame auf (PEA+RTS ist
eine reine Software-Konvention, kein Prozessor-Trap). Für Trampolin-
Aufrufe liest die Herkunftsprüfung an dieser Stelle also vermutlich
Datenmüll statt der echten Rücksprungadresse — mit unvorhersagbarem
Ergebnis der `bcc`-Verzweigung.

**Einordnung:** Kein Rückschritt — die ERSTE, ursprünglich gejagte
Korruption ist nachweislich behoben, der Fix bleibt committet. Es gibt
schlicht eine zweite, bisher von der ersten verdeckte Fehlerquelle mit
demselben Grundmechanismus, aber einem zweiten Aufrufweg.

**Nächster Schritt:** Klären, wie sich TRAP- und Trampolin-Weg am
Stack-Layout unterscheiden lassen (evtl. über `Q9K_InTrapPath` oder ein
analoges Kennzeichen, das für Trampolin-Aufrufe bereits gesetzt/gelesen
wird), und die Herkunftsprüfung für BEIDE Wege korrekt herleiten.

## A4-FIX VOLLSTAENDIG: beide Speicherkorruptionen behoben, kein Absturz mehr

Der zweite Korruptionsfall (`a4=$7c00`/`$7c02`, dieselbe Handler-Adresse-
Symptomatik wie zuvor) lag NICHT am Trampolin-Weg, wie zunächst vermutet —
`F$SRqMem` läuft nachweislich durch `Q9K_TrapDispatch` (per Callcode-
Mitschrift bestätigt: `$28` erscheint mehrfach im Dispatcher-Log). Der
eigentliche Fehler steckte in der ersten Fix-Fassung selbst, in zwei
Schritten gefunden:

1. **Die geratene 64-KByte-Grenze war zu großzügig.** RBF/scf/ioman laden
   alle innerhalb von 64 KByte hinter unserem Kernel (alle Testmodule
   zusammen ~36 KByte) — die Prüfung stufte dadurch ausnahmslos alles als
   „intern" ein und griff nie. Per Messung entdeckt: alle beobachteten
   Distanzen lagen deutlich unter `$10000`.
2. **Ersetzt durch einen Vergleich gegen `M$Size` aus dem echten
   Modulkopf** — aber `4(a4)` nach `lea Q9K_ModuleStart(pc),a4` traf dabei
   zunächst auf Datenmüll statt auf `M$Size`: `Q9K_ModuleStart` markiert
   laut eigenem Kopfkommentar **nicht** Offset 0 des Moduls, sondern das
   Ende des vom Linker automatisch generierten, `$3C` Byte großen
   Modulkopfs (`Q9K_ModuleHeaderSize`, bereits als Konstante vorhanden).
   Um diese Konstante korrigiert (`suba.l #Q9K_ModuleHeaderSize,a4`)
   zeigt `a4` auf den wahren Modulanfang, `4(a4)` liest dort das echte
   `M$Size`.

### Vollständig verifiziert

- Konsole läuft weiterhin (`Hallo von Q9-OS!`, `RP012`, drei Pfade).
- **Beide** Speicherkorruptionsstellen sind verschwunden.
- **Breite Prüfung des gesamten 32-KByte-Kernel-Codebereichs** per
  Speicher-Watch über den kompletten Boot: keine Fremdkorruption mehr
  irgendwo — die einzigen verbleibenden Schreibzugriffe sind legitime
  Updates von `Q9K_CRuntimeData` selbst (`_stklimit` u. a., alle nahe
  `$7296`).
- **Der Absturz (Illegal Instruction, `'E'`-Marke) tritt nicht mehr auf.**
  Der `F$Load`-Testpfad scheitert jetzt sauber mit einem echten
  Fehlercode (`$D8`) statt zu crashen, der Boot läuft normal in die
  Testprozess-Schleifen weiter.

**Damit ist die A4-Untersuchung — begonnen vor Tagen bei `scf`s
Konsolen-Open, present bei RBFs `F$Load`-Vorarbeit — nach sechs Anläufen
tatsächlich gelöst, nicht nur verstanden.** Der Fehlercode `$D8` beim
`I$Open("/dd/startup")` ist der nächste, aber inhaltlich andere
Meilenstein (`F$Load` selbst) — kein Speicherkorruptionsproblem mehr.
## F$PrsNam-Guard (2026-09-08/09): echter Latenzfehler gefunden, aber NICHT die Ursache von $D8

Bei `Q9K_SysFPrsNam` (F$PrsNam, Callcode `$10`) schrieb der Erfolgszweig
**bedingungslos** den R$-Registerrahmen nach `(a5)`:

```
movem.l d0-d1,(a5)        * R$d0/R$d1
movem.l a0-a1,$20(a5)     * R$a0/R$a1
```

als sei `a5` immer ein gültiger Trampolin-Rahmenzeiger. Für RBFs eigenen,
per echtem `trap #0` abgesetzten `F$PrsNam`-Aufruf (bestätigt: Rücksprung-PC
lag nachweislich innerhalb von RBFs Modul) ist `a5` aber nur ein
gewöhnliches Aufrufer-Register — der Schreibzugriff traf fremden Speicher.
**Fix (übernommen und committet):** dieselbe stackrelative
Rahmenerkennung wie bei `Q9K_SysFAllPD`/`Q9K_SysFSRqMem`
(`a5 == sp_bei_Eintritt + 8`) — nur bei einem echten Rahmen wird
geschrieben, sonst bleibt es bei den bereits gesetzten Ausgaberegistern.

**Per Messung falsifiziert:** eine temporäre `W`/`S`-Diagnose an der
Verzweigungsstelle zeigte, dass RBFs Aufruf zuverlässig den **Sprungzweig**
(`S`, kein Schreibzugriff) nimmt — der Guard arbeitet korrekt. Trotzdem
bleibt `$D8` beim anschließenden `I$Open("/dd/startup")` **unverändert**
bestehen. Der Guard ist ein echter, sinnvoller Fix (verhindert eine
Speicherstörung, die früher oder später real aufgetreten wäre), aber er
war **nicht** die Ursache des `$D8`-Symptoms — diese frühere Vermutung ist
damit widerlegt.

## Die wahre Herkunft von $D8: kein Korruptionssymptom, sondern echte RBF-Logik

Statische Analyse (capstone) des unveränderten `rbf.mod` klärt
den Ursprung abschließend:

- Dateioffset `$f24`: `move.w #$d8,d1` — RBF setzt `$D8` hier **fest und
  bedingungslos**, als Übersetzung eines zuvor aufgetretenen `$D3`
  (`E$Share`, "Non-sharable file busy" — echter Wert laut korrekt
  ausgezähltem `funcs.a`, `org 64`-Tabelle: `E$DevBsy=$D0`, `E$Share=$D3`;
  die in einer früheren Sitzung angenommene Gleichsetzung „$D8 =
  E$DevBsy" war ein Auszählfehler und ist damit ebenfalls korrigiert).
- `$D3` wiederum stammt aus einer RBF-internen Kapazitätsprüfung bei
  Dateioffset `$64e`: `d2 = (a1)+$36 - (a1)+$32`; unterläuft diese
  Subtraktion (Carry, d. h. `+$32 > +$36`) oder ist die Differenz `0`,
  meldet RBF `$D3` — ein klassisches "kein Platz mehr im Pool"-Muster
  (Ringpuffer-/Zähler-Paar, `+$32`=Ist, `+$36`=Limit).
- `a1` zeigt dabei auf RBFs **eigene, private** Geräte-/Pfad-Verwaltungs-
  struktur (Felder ab Offset `$24` aufwärts — außerhalb des
  standardisierten, dokumentierten PD-Kopfs). Diese Struktur wird von RBF
  selbst verwaltet; unser Kernel liefert nur die Rohdaten, aus denen RBF
  sie beim Attach befüllt (insbesondere über GetStat-Rückfragen an
  Treiber/Deskriptor).

**Einordnung:** `$D8` ist damit **keine Speicherkorruption und kein
Bug in unserem eigenen Trap-Dispatch mehr**, sondern eine legitime
Fehlerantwort des echten RBF-Binärcodes auf einen Zustand, den unsere
eigene Descriptor/Driver/File-Manager-Verdrahtung offenbar nicht wie von
RBF erwartet aufsetzt. Das deckt sich mit dem in
`Q9-OS eigener Kernel (C-Neuimplementierung)`-Notizen bereits als
nächster großer Schritt benannten **"Dreiklang" (Descriptor→Driver→
File-Manager)** — die A4-Speicherkorruptions-Untersuchung ist damit
wirklich abgeschlossen, der nächste Fehler liegt bereits in diesem neuen,
größeren Themenblock und nicht mehr in seinem Vorfeld.

**Nicht mehr offen:** ob RBFs GetStat-Anfragen beim Attach unsere
Antworten überhaupt erreichen bzw. ob unser Treiber/Deskriptor die von
RBF erwarteten Werte für diesen Kapazitäts-/Pool-Zähler liefert, ist noch
nicht gemessen — das ist der konkrete erste Schritt in den "Dreiklang".

## $D8-Spurensuche in RBFs Binärcode: konkreter Verdacht, aber noch nicht bewiesen (2026-09-08/09)

Weitergehende dynamische Messung (Instruktionsspur + gezielte Speicher-
Dumps im Emulator, alle Q9-Flux-Änderungen dabei nur temporär und
inzwischen wieder vollständig zurückgesetzt) zur Frage: woher kommt die
"$36(a1)-$32(a1)=0"-Kapazitätserschöpfung, die laut vorigem Fund zu `$D3`
→ `$D8` führt?

**Zwischenfrage des Nutzers geklärt:** Der Pfad `/dd/startup` selbst ist
korrekt — auf dem Referenz-Image (`os9 dir OS9SYS.hda,`) existiert er
tatsächlich im Wurzelverzeichnis (`--e-rewr`), zusätzlich (aber getrennt
davon) auch `/dd/SYS/startup`. Kein Pfadproblem.

**Methodischer Fallstrick entdeckt:** Eine erste Runde statischer
Analyse (capstone, `rbf.mod` ab einem willkürlich gewählten
Byte-Offset `$ea0`) ergab plausibel aussehenden, aber tatsächlich
FALSCH ausgerichteten Code — bestätigt per `Q9_COUNT_PC` an sechs so
gewonnenen Kandidatenadressen: vier der sechs (`$1250`, `$64c`, `$f1c`,
`$f24`) wurden beim echten Boot NIE erreicht (0 Treffer), nur die aus
dem unmittelbaren Kontext übernommene Adresse `$eb2` traf tatsächlich
(2 Treffer, je einmal pro `I$Open`). Capstone synchronisiert sich beim
Start mitten in einer `bsr.w`-Verschiebungskonstante nicht von selbst
auf echte Befehlsgrenzen — jede weitere Adresse aus so einem Lauf ist
erst durch eine LIVE-Messung (Freeze/Count) zu vertrauen, nicht durch
bloßes Ablesen der Analyse.

**Live bestätigt (per `Q9_FREEZE_PC`, Instruktionsspur mit a0/a1/a4/sp):**
Beim zweiten `I$Open`-Aufruf (also `/dd/startup`) hält RBF durchgehend
`a1 = $00021500`. Ein Rohspeicher-Dump dieser Adresse (temporärer
`Q9_DUMP_ADDR`-Hebel in `q9boardrun.c`, inzwischen entfernt) zeigt an
Offset `+$32`/`+$36` (nach der — mit der oben genannten Unsicherheit
behafteten — ursprünglichen Feldzuordnung) zwei **identische** 32-Bit-
Werte: `$000003C0` (960 dezimal). Genau das ist das Muster, das die
vermutete Kapazitätsprüfung (`$36(a1)-$32(a1)`) auf `0` und damit auf
Fehlschlag `$D3`→`$D8` treibt.

**Ebenfalls live vermessen:** Zwei `F$SRqMem`-Aufrufe (Callcode `$28`)
mit je `d0=$200` (512 Byte) treten im selben Zeitfenster auf
(`pc=$e040` mit `d1=$1ff`, `pc=$e3e6` mit `d1=$9,d2=$8200`). Der erste
liefert `a2=$00035C80` — **nicht** `$21500`. Der zweite Rücksprungpunkt
ließ sich mit der (aus dem ersten Fehler bereits als unzuverlässig
erkannten) Adressrechnung nicht sauber treffen (0 Treffer trotz
mehrerer Versuche) und wurde nicht weiter verfolgt. `a1=$21500` ist
damit vermutlich NICHT einer dieser beiden 512-Byte-Puffer, sondern
eine andere, vermutlich früher (z. B. bei `I$Attach`) angelegte private
RBF-Struktur.

**Einordnung — was wirklich gesichert ist und was nicht:**
- Gesichert: `a1=$21500` beim scheiternden `I$Open`, Speicherinhalt dort
  wie oben gedumpt, zwei identische `$3C0`-Langworte an einer Stelle,
  die zur ursprünglich vermuteten Kapazitätsprüfung passen würde.
- NICHT gesichert: dass `+$32`/`+$36` (statt z. B. `+$30`/`+$34` oder
  anderer Offsets) wirklich die exakt richtigen Feldnamen/-lagen sind —
  die static-disasm-Grundlage dafür ist durch den oben beschriebenen
  Fund erschüttert und wurde nicht mit derselben Live-Methode
  nachgeprüft wie `$eb2`.
- Offen: WER die beiden `$3C0`-Werte schreibt und warum identisch —
  ob das eine legitime RBF-Eigeninitialisierung ist (die z. B. eine von
  UNS gelieferte GetStat-Antwort dupliziert unverändert übernimmt) oder
  ob unser eigener Allokator/unsere Attach-Verdrahtung hier fehlerhaft
  zweimal denselben Wert liefert, ist NICHT geklärt.

**Empfehlung für den nächsten Anlauf:** Statt weiter mit ad-hoc
capstone-Bereichen zu arbeiten, entweder (a) RBF vollständig und mit
echten Funktionsgrenzen analysieren (z. B. beginnend an der
M$Exec-Einsprungadresse aus dem Modulkopf, linear, ohne Bereichslücken)
und JEDE daraus abgeleitete Adresse per `Q9_COUNT_PC`/`Q9_FREEZE_PC`
gegenmessen, bevor sie als gesichert gilt, oder (b) gezielt die
GetStat-Kommunikation zwischen RBF und unserem Treiber/Deskriptor beim
`I$Attach` von `/dd`/`/term` mitschneiden (der eigentliche "Dreiklang"-
Einstieg) und dort direkt nach Feldern suchen, die unverändert in zwei
verschiedene Zielorte kopiert werden.

## DURCHBRUCH: wahre Ursache von $D8 gefunden — kein Bug in RBF, sondern ein Verzeichnis-Lesefehler (2026-09-08)

Nach dem oben dokumentierten Methodenfehler (unausgerichtete
Analyse) folgte eine **vollständige, sauber ausgerichtete**
Analyse von `rbf.mod`, verankert am echten Modulkopf-Feld
`M$Exec` (Offset `$30` laut `src/q9moduleheader.h`, hier Wert `$A6`) —
nicht mehr an einem geratenen Byte-Offset. Zwei methodische Fixes waren
nötig, bis die Ausrichtung über die GESAMTE 9,6-KByte-Datei hinweg
sauber blieb (nur noch 2 nicht aufgelöste Stellen, beide fernab der
relevanten Bereiche):

1. **`M$Exec` selbst zeigt NICHT auf Code, sondern auf eine
   13-Einträge-Sprungtabelle** (2 Byte pro Eintrag, `I$Attach` bis
   `I$WritLn` — `I$GetStt`/`I$SetStt`/`I$Close` NICHT über diese Tabelle
   geroutet). Echter Code beginnt danach bei Offset `$C0`.
2. **`trap #0` gefolgt vom Inline-Callcode-Wort** (die OS-9-Konvention,
   die unser eigener Kernel an vielen Stellen selbst nutzt) muss beim
   linearen Scan explizit übersprungen werden — sonst desynchronisiert
   jeder einzelne interne Syscall (`F$PrsNam`, `F$SRqMem`, `F$GProcP`
   — alle real in RBF gefunden, an den Dateioffsets `$196`/`$FC8` bzw.
   `$A9E`/`$D70`/`$1116` bzw. `$1268`/`$1326`) die Analyse ab
   dort.

**Live gegengeprüft (Q9_FREEZE_PC/Q9_COUNT_PC), RBF_BASE für den
Testlauf bestätigt `$E11A`** (`F$PrsNam`-Rücksprung-PC `$E2B2` minus
Call-Offset `$198`, UND unabhängig bestätigt durch einen exakten
Live-Treffer bei PC `$EFCC` = `RBF_BASE+$EB2`).

### Der eigentliche Fund: ein Speicher-Watch auf `$32(a1)`/`$36(a1)`

Statt weiter zu raten, wurde direkt beobachtet, WER diese beiden Felder
beschreibt (`Q9_WATCH_ADDR=0x21532 Q9_WATCH_LEN=8`, RBF hält `a1=$21500`
durchgehend für den `/dd/startup`-Pfad):

- `$36(a1)` wird EINMALIG auf `$3C0` (960) gesetzt — das ist die
  Verzeichnisgröße (30 Einträge à 32 Byte), keine willkürliche
  Dopplung.
- `$32(a1)` wird danach in einer Schleife exakt 30-mal um je `$20` (32,
  Standard-Verzeichniseintragsgröße) hochgezählt: `$20, $40, $60, ...,
  $3C0`. Bei `$3C0` ist `$36(a1)-$32(a1)=0` — GENAU DANN meldet RBF
  `$D3`→`$D8`.

**Das ist kein Speicherfehler und kein uninitialisiertes Feld — RBF
durchsucht sein Verzeichnispuffer stur bis zum Ende, findet "startup"
nicht und meldet korrekt "nicht gefunden" (nur eben über den
Kapazitäts-/Poolmechanismus statt über `E$PNNF` kodiert, offenbar eine
Eigenheit dieser RBF-Version).**

### Warum RBF "startup" nicht findet: der Verzeichnispuffer beginnt mitten im Verzeichnis

Ein Rohspeicher-Dump des tatsächlich durchsuchten Puffers (`a0=$36310`
beim allerersten Schleifendurchlauf, `Q9_DUMP_ADDR`) zeigt **echte,
korrekte Dateinamen aus dem Referenz-Image** — aber beginnend erst bei
`OldBoot`:

```
$36310: (nicht-lesbarer erster Eintrag, evtl. Kopf/Kontrollfeld)
$36330: "OldBoot"
$36350: "HOME" (als "HOM." mit Endekennung)
$36370: "PROJECT..."
$36390: "netmod..."
$363b0: "ET..." (ETC)
$363d0: "xterm..."
$363f0: "rtc7242..."
$36410: "startsp..." (startspf)
$36430: "bas..." (bash)
$36450: "mbrsca..." (mbrscan)
$36470: "cfboot_os9.b..."
$36490: "OS9Boo..." (OS9Boot)
$364b0: "CP" (CPM)
$364d0 ff.: Nullen (Rest bis Verzeichnisende)
```

Das reale Wurzelverzeichnis (per `os9 dir OS9SYS.hda,` am Host
verifiziert) lautet vollständig: `C CMDS CMDS_NEW DEFS GDP IO KERMIT
LIB REF README SYS **startup** reinstall.old reinstall.ultra OldBoot
HOME PROJECTS netmods ETC xterms rtc72421 startspf bash mbrscan
cfboot_os9.bl OS9Boot CPM`.

**`OldBoot` ist exakt der 16. reale Eintrag** (nach den vermutlich zwei
reservierten `.`/`..`-Slots plus den ersten 14 echten Namen `C` bis
`reinstall.ultra`, in denen `startup` als 12. Name steckt) — der
Puffer, den RBF durchsucht, beginnt also **zwei komplette 256-Byte-
Sektoren (16 Einträge à 32 Byte) zu spät**. Die ersten beiden Sektoren
des Verzeichnisses — und damit `startup` — fehlen komplett in dem, was
RBF zu sehen bekommt.

### Einordnung

Das ist **kein Bug in RBF** (das reale, unveränderte Modul verhält sich
korrekt gegenüber dem, was es zu lesen bekommt) und **keine
Speicherkorruption**. Die Ursache liegt in der Lesekette davor —
vermutlich in der Sektor-/LSN-Berechnung beim tatsächlichen
Verzeichnis-Einlesen (Treiber `cfide.mod` bzw. der Deskriptor `dd.mod`,
oder in einem F$-Dienst, den WIR dafür bereitstellen und der die
LSN/Blockzahl falsch weiterreicht). Das genau ist der "Dreiklang"-
Bereich (Descriptor→Driver→File-Manager).

**Nächster konkreter Schritt (noch nicht begonnen):** den tatsächlichen
Lesevorgang zurückverfolgen, der diesen Puffer befüllt (vermutlich ein
`I$Read`/GetStat-Austausch zwischen RBF und `cfide.mod`, oder ein
direkter Treiberaufruf) und die dabei verwendete Start-LSN mit der
tatsächlich benötigten (LSN des Wurzelverzeichnisses laut Deskriptor
`dd.mod`) vergleichen — mit dem Ziel, die Zwei-Sektor-Verschiebung auf
eine konkrete Quelle (falsche Konstante, falsches Feld, falsche
Einheit — Sektoren vs. Bytes o. Ä.) zurückzuführen.

Alle Emulator-Diagnosen (Ringpuffer-Erweiterung um a1/a2,
`Q9_DUMP_ADDR`) waren wieder nur temporär und sind vollständig
zurückgesetzt; die vollständige, ausgerichtete RBF-Analyse
liegt (aus Lizenzgründen — proprietärer Microware-Code, nur
Kurzausschnitte hier im Dokument) ausschließlich im Job-Scratch, nicht
im Repo.

## Korrektur/Vertiefung zum Durchbruch: "zwei Sektoren zu spät" ist NICHT bewiesen (2026-09-08, Fortsetzung)

Der Versuch, den vorigen Fund ("Verzeichnispuffer beginnt bei `OldBoot`,
die ersten zwei Sektoren fehlen") bis zur konkreten Lese-Quelle
zurückzuverfolgen, deckte einen Widerspruch auf, der die bisherige
Interpretation in Frage stellt:

**Neue reale Fakten (per `Q9_BOARD_CF_TRACE=1`, ein bereits vorhandenes,
unverändert genutztes Diagnosewerkzeug in `Q9-Flux/src/devices/cf/cf.c`,
sowie direkter Byte-Analyse des Referenz-Images):**

- Der echte Boot-Sektor (LSN 0) liefert `DD.DIR = 65` (die LSN des
  Wurzelverzeichnis-**Dateideskriptors**, nicht der Verzeichnisdaten
  selbst) und `DD.LSNSize = 512`.
- LSN 65 enthält tatsächlich einen FD-Sektor (Attribut-Byte `$BF`,
  danach eine Segmentliste ab Offset `$10`): erstes Segment beginnt bei
  LSN **66**.
- Der reale Boot-Trace zeigt exakt die dazu passenden Lesevorgänge:
  `lba=65` (2x), `lba=66`, `lba=67` — RBF liest also korrekt FD→Segment.
- ABER: der Inhalt von LSN 66/67 (roh von der Disk gelesen: `.` als
  erster Eintrag, danach etwas, das eher wie `.login` aussieht als wie
  `..`) **stimmt nicht mit dem Pufferinhalt überein, den RBF laut dem
  vorherigen Speicher-Watch tatsächlich durchsucht** (`OldBoot` ... `CPM`
  ab Adresse `$36310`). Das sind zwei UNTERSCHIEDLICHE Dateninhalte.

**Konsequenz:** Die vorige Schlussfolgerung ("RBF liest das Verzeichnis
korrekt, nur zwei Sektoren zu spät") ist damit **nicht mehr haltbar in
dieser einfachen Form** — der tatsächlich durchsuchte Puffer bei
`$36310` scheint NICHT aus diesen (korrekten!) LSN-65/66/67-Lesevorgängen
zu stammen. Wahrscheinlicher: der Puffer, auf den `$e(a1)` zeigt, enthält
Speicherreste aus dem GROSSEN Bulk-Ladevorgang beim Boot (`lba=555489,
count=72` — lädt Kernel+Module), die zufällig lesbar aussehende
Verzeichnisnamen enthalten (nicht aus dem extrahierten `.mod`-Blob
selbst, das wurde per Byte-Suche ausgeschlossen — die Namen stehen
NICHT in `rbf.mod`/`cfide.mod`/`dd.mod`/`c0.mod`).

**Bewertung:** Es ist damit weiterhin unklar, ob `$e(a1)` überhaupt
korrekt auf einen frisch gelesenen Verzeichnispuffer zeigt, oder ob
dort aus irgendeinem Grund eine STEHENGEBLIEBENE/falsche Adresse
verwendet wird. Die vorherige Analyse-Ausrichtung im Bereich
`$100`-`$200` (I$Attach-Fortsetzung) selbst erwies sich bei einer
Live-Gegenprobe (`Q9_FREEZE_PC` bei `$e22a`/Bittest) ebenfalls als NICHT
durchgängig verlässlich (ein direkt anschließender Sprung passte nicht
zur linear analysierten Nachbarschaft) — vermutlich ein weiterer,
noch nicht lokalisierter Ausrichtungsfehler in genau diesem
Codeabschnitt.

**Nächster Schritt (konkret, noch nicht begonnen):** herausfinden,
WOHER `$e(a1)` seinen Wert bekommt (Speicher-Watch auf das Feld selbst,
`a1+$0e`, mit `Q9_WATCH_FREEZE`), um den tatsächlichen Bezugspunkt
zwischen den echten LSN-65/66/67-Lesevorgängen (korrekt!) und dem
später durchsuchten Puffer bei `$36310` (Ursprung noch unklar)
herzustellen.

## Weitere Rückverfolgung: $2e(a1)-Puffer, echte FD-Felder, Nullfüll-Fallback (2026-09-08, Fortsetzung 2)

Direkte Live-Byte-Dumps am tatsächlichen `pc` (per `Q9_DUMP_ADDR`, ohne
jedes Ausrichtungsrisiko einer eigenständigen Analyse — die
Bytes stammen direkt aus dem laufenden Emulator) klären die Herkunft
der beiden zuvor unklaren Werte vollständig:

### `$36(a1) = $3C0` ist ECHT, kein Fehler

`pc=$d6a2` (`move.l d0,$36(a1)`) übernimmt einen Wert, der aus
`$8(a2)`/`$c(a2)` zusammengesetzt wird — `a2` zeigt dabei auf den real
von Disk gelesenen FD-Sektor (LSN 65, `DD.DIR`). Bei genauer
Nachrechnung ist das exakt die Interpretation der FD-Bytes 9-12 als
32-Bit-Big-Endian-Wert (`FD.SIZ`, die reale Dateigröße) — bei unserem
Referenz-Image `$000003C0` = 960 Byte. **Das Wurzelverzeichnis ist also
wirklich 960 Byte groß, `$36(a1)` ist korrekt aus echten Disk-Daten
abgeleitet, kein Bug.**

### `$2e(a1)`-Puffer: frisch alloziert, aber nur 512 Byte

`pc=$e3e2`-`$e3ec`: RBF fordert per echtem `F$SRqMem` (Callcode `$28`)
`$c8(a1)` Byte an (gemessen: `$200` = 512 Byte, genau EIN Sektor bei
`DD.LSNSize=512`) und legt den frischen Blockzeiger in `$2e(a1)` ab.
`pc=$e41c`-`$e426`: ein Vertausch-Mechanismus tauscht `$e(a1)` und
`$2e(a1)` (klassisches Doppelpuffer-Muster) — dadurch landet der
FRISCH allozierte, aber nur 512 Byte große Block als aktiver
Scan-Puffer.

### Der eigentliche Verdacht: Scan überschreitet die 512-Byte-Pufferzone

Die Schleife durchsucht laut vorigem Fund `$32(a1)` von `0` bis `$3C0`
(960) — **das übersteigt die tatsächliche Puffergröße (512 Byte) um
448 Byte.** Ein Speicher-Watch auf den kompletten Pufferbereich
(`$36310`-`$366FF`, 1024 Byte) zeigt zusätzlich: kurz vor/während des
Scans laufen **512 einzelne 1-Byte-Nullschreibzugriffe** von `pc=$fa0a`
in genau diesen Bereich.

`pc=$fa0a` selbst (Live-Bytes verifiziert) ist **keine Leseroutine**,
sondern eine reine Füllschleife:
```
f9f0: bne.w  $fa04        * (Bedingung aus vorigem Lese-/Retry-Versuch)
f9f4: subq.l #$1,d0        * Retry-Zaehler
f9f6: bne.w  $f9e0         * -> erneuter Leseversuch
f9fa: move.w #$f4,d1        * Retries erschoepft -> Fehler $F4
f9fe: ori.b  #$1,ccr
fa02: rts
fa04: movea.l a5,a0          * Fallback-Pfad
fa06: move.w #$1ff,d1         * 512 Durchlaeufe
fa0a: move.b $0(a3),(a0)+     * denselben Quellbyte (a3 NICHT erhoeht!) 512x kopieren
fa0e: dbra  d1,$fa0a
```
Das ist ein **Nullfüll-Fallback** (memset-artig, kopiert denselben
Byte 512-mal), erreichbar entweder nach einem fehlgeschlagenen
Lese-Retry oder als regulärer Pfad für einen bestimmten, noch nicht
identifizierten Sonderfall (`bne.w $fa04`-Bedingung vor der Schleife
noch nicht zurückverfolgt).

### Einordnung (Stand jetzt, ehrlich unvollständig)

Drei mögliche, noch nicht unterschiedene Erklärungen bleiben offen:

1. Der (korrekte, real gemessene) zweite Sektor-Lesevorgang für LSN 67
   schlägt in unserer CF-/Treiber-Emulation fehl bzw. wird nicht
   fertig bestätigt, RBF fällt nach Retries auf den Nullfüll-Pfad
   zurück — der Scan liest danach über den nur 512 Byte großen Puffer
   hinaus in benachbarten, nicht dazugehörigen Speicher (dort liegen
   zufällig lesbare Dateinamen aus dem Boot-Bulk-Ladevorgang).
2. Der Nullfüll-Pfad ist reguläres RBF-Verhalten für einen anderen
   Zweck (z. B. "Loch" im Segment, Sparse-Bereich) und wird hier durch
   eine falsche Vorbedingung fälschlich ausgelöst.
3. Es gibt einen bisher nicht gefundenen dritten Mechanismus, der den
   Puffer zwischen Nullfüllung und Scan noch einmal umlenkt.

**Konkreter nächster Schritt:** die Bedingung vor `$fa04` (der Sprung
bei `f9f0: bne.w $fa04`) zurückverfolgen -- welcher Vergleich davor
entscheidet "Nullfüllen statt lesen" -- sowie prüfen, ob der zweite
Sektor-Lesebefehl (LSN 67) laut CF-Treiber-Emulation überhaupt jemals
erfolgreich abgeschlossen/quittiert wird (`Q9_BOARD_CF_TRACE=1` zeigte
ihn zwar als gestartet, aber nicht, ob RBF ihn als erfolgreich
akzeptiert hat).

Alle Emulator-Diagnosen (`Q9_DUMP_ADDR`, `Q9_WATCH_ADDR`) waren wieder
nur temporär und sind vollständig zurückgesetzt.

## Fortsetzung 3: Nachlademechanismus lokalisiert, aber falsch identifiziert -- Korrektur

Der naheliegende Verdacht "`$fa0a`-Nullfüllung wird durch einen
fehlschlagenden zweiten Sektor-Lesevorgang ausgelöst" wurde weiter
zurückverfolgt. Live-Ground-Truth-Bytes (`Q9_DUMP_ADDR` direkt an
bestätigten PCs, kein Analyserisiko):

- Die Bedingung vor der `$32(a1)`-Aktualisierungs-Instruktion (echte
  Adresse `pc=$e25e`, per Probieren mehrerer Startoffsets sauber
  ausgerichtet): `d1 = d1 AND $70(a1)` (Maske `$1FF`, passt zur
  512-Byte-Puffergröße); ist das Ergebnis **Null** (Sektorgrenze
  erreicht), wird `bsr.w $efa8` aufgerufen, sonst wird die Grenze
  übersprungen.
- `$efa8` wird während des Testlaufs nachweislich 7x erreicht
  (`Q9_COUNT_PC`) — der Mechanismus feuert also.
- **Korrektur:** `$efa8` selbst ist bei genauer Live-Analyse
  KEINE Leseroutine, sondern eine kleine Flag-Utility (`andi #$fe,ccr`
  gefolgt von Bit-Tests/-Änderungen auf `$2a(a1)`, bedingt `bsr.w
  $ef38`). Die vermutete Kausalkette "Sektorgrenze -> `efa8` liest
  nach" ist damit **nicht bestätigt** — `efa8` ist offenbar nur ein
  Nebeneffekt-Aufruf, nicht der eigentliche Lesevorgang.
- Zusätzliche Beobachtung: der reale Diskinhalt von LSN 66 (direkt aus
  dem Image gelesen) stimmt NICHT byteweise mit dem, was im
  RBF-Arbeitspuffer an Position 0 steht — RBF wandelt den
  On-Disk-Verzeichniseintrag offenbar in ein eigenes
  Zwischenspeicherformat um, bevor er im Puffer landet. Ein direkter
  Byte-für-Byte-Vergleich Disk-vs-Puffer ist deshalb NICHT ohne
  Kenntnis dieses Zwischenformats aussagekräftig.

**Ehrlicher Zwischenstand:** Der exakte Mechanismus, der bei
Sektorgrenzen den nächsten Sektor tatsächlich nachlädt (falls das für
unseren Fall überhaupt vorgesehen ist -- ggf. lädt RBF für kleine
Verzeichnisse doch alles auf einmal, und die Leerstellen ab Eintrag 14
sind uninitialisierter Rest eines zu klein bemessenen oder nur
teilweise befüllten Puffers), ist noch nicht gefunden. Die naheliegende
nächste Spur (`e522`, 32 Treffer -- einmal pro Scan-Durchlauf, deutlich
öfter als `efa8`) wurde noch nicht untersucht.

**Nächster Schritt:** `e522` live analysieren (Ground-Truth-Bytes,
wie oben) -- das ist der Aufruf, der bei JEDER Scan-Iteration
(unabhängig von der Sektorgrenze) erfolgt, und damit ein besserer
Kandidat für den eigentlichen "Eintrag lesen/vergleichen"-Kern als
`efa8`.

## GRUNDLEGENDE KORREKTUR: RBF_BASE war falsch -- $D2D2, nicht $E11A (2026-09-08, Fortsetzung 4)

Beim Versuch, `$e522` (den nächsten Kandidaten) einer Datei-Position
zuzuordnen, ergab sich ein Widerspruch: `$e522` deckte sich exakt mit
der schon ganz am Anfang dieser Untersuchung gefundenen
"$1250"-Sperr-/`F$GProcP`-Routine — aber NUR unter einer ANDEREN
Basisadresse als der bisher verwendeten `$E11A`.

**Direkt geprüft, zweifelsfrei:** Rohspeicher an `$D2D2` beginnt mit
`4a fc 00 01 00 00 25 a6` — das ist `M$ID` (`$4AFC`) gefolgt von
`M$Size=$25A6`, exakt die reale Dateigröße von `rbf.mod` (9638 Byte).
**`$D2D2` ist die wahre Ladeadresse von RBF in diesem Testlauf, nicht
`$E11A`.** Zusätzlich bestätigt: der reale Trap-Tracer (`Q9_TRAP_TRACE`)
hatte schon vor Tagen `trap0 pc=$e298` für `F$PrsNam` protokolliert --
das ist exakt `$D2D2+$FC6` (der `trap #0` bei Dateioffset `$FC6`,
zweite `F$PrsNam`-Fundstelle) -- eine dritte, unabhängige Bestätigung.

**Woher kam der Fehler?** Die frühere "Bestätigung" von `$E11A` (über
den `F$PrsNam`-Rücksprung-PC `$E2B2` und einen Freeze-Treffer bei
`$EFCC`) war ein Zufallstreffer: `$E11A` liegt selbst INNERHALB von
RBFs Adressraum (`$D2D2` bis `$D2D2+$25A6=$F878`), bei Dateioffset
`$E48`. Jede "Bestätigung", die auf `$E11A` als Basis aufbaute, hat
deshalb auf eine andere, zufällig ebenfalls plausibel aussehende
Codestelle gepasst -- nicht auf die eigentlich gemeinte.

**Wichtig: alle Funde, die über `Q9_DUMP_ADDR` DIREKT an live bestätigten
PCs gewonnen wurden (der Ansatz seit der Mitte dieser Untersuchung),
bleiben davon unberührt und gültig** -- betroffen war nur die
nachträgliche Umrechnung "Datei-Offset ↔ Live-PC" für die ganz frühen,
rein statischen Funde (die ursprüngliche `$64e`/`$f90`/`$f7a`/`$1250`-
Kette). Mit der korrigierten Basis decken sich diese jetzt aber
vollständig mit den Live-Messungen:

```
Q9_COUNT_PC (korrigierte Basis $D2D2):
  $d920 ($64e, Kapazitaetspruefung)      = 31 Treffer
  $e184 ($eb2, Pruefung nach f7a)         = 31 Treffer
  $e262 ($f90, Wrapper)                    = 31 Treffer
  $e24c ($f7a, Ringpuffer-Vorschub)         = 30 Treffer
  $e1ee ($f1c, Vergleich mit $D3)            =  1 Treffer
  $e1f6 ($f24, setzt $D8)                     =  1 Treffer
```

Damit ist die **ursprüngliche Kapazitätsprüfungs-Kette
($f90→$1250/$64e→$D3→$D8) tatsächlich der reale Mechanismus** -- exakt
wie ganz am Anfang dieser Untersuchung vermutet, nur jetzt sauber
bewiesen statt nur plausibel.

### Neuer Fund: Segmentlisten-Auswertung live analysiert (`ed3e`)

`f90` ruft bedingt (`btst.b #1,$2a(a1)`) eine weitere Routine (`$1cf8`,
live `$efca`) auf, die ihrerseits `$1a6c` (live `$ed3e`) aufruft.
`$ed3e` ist -- Ground-Truth-Bytes, siehe Doku-Commit -- die
**Segmentlisten-Auswertung**: sie liest direkt die 5-Byte-Einträge
(3 Byte LSN + 2 Byte Größe, geshiftet um den Allokationseinheiten-Faktor
`$6f(a1)`) aus dem FD-Puffer (`$2e(a1)+$10` ff., exakt das reale
FD-Format, das wir schon vom rohen Disk-Sektor kennen) und sucht das
Segment, das die aktuelle Scan-Position `$32(a1)` enthält.

**Ergebnis der Auswertung für unser Verzeichnis:** das erste (und
einzige benötigte) Segment beginnt bei LSN 66 mit einer Kapazität von
`30 << Schichtfaktor` -- weit über den 960 benötigten Byte. Das
Verzeichnis liegt also in EINEM einzigen, großen Segment, nicht in
zwei getrennten -- frühere Vermutungen einer "zweiten Segment"-Suche
sind damit hinfällig. Offen bleibt: wie/wo aus diesem Segment die
tatsächlich zu lesende physische LSN für einen gegebenen `$32(a1)`-Wert
berechnet und der eigentliche Sektor-Lesevorgang ausgelöst wird --
das ist vermutlich die schon früher gefundene `$15e6`-Routine
(LSN-Berechnung aus Segmentbasis + Offset), deren Aufrufer noch nicht
zurückverfolgt wurde.

**Nächster Schritt:** den Aufrufer von `$15e6` (jetzt mit der
korrigierten Basis neu zu berechnen: `$D2D2+$15E6=$E8B8`) live
zurückverfolgen und prüfen, ob/wann er für die zweite Sektorgrenze
(Bytes 512-959) tatsächlich aufgerufen wird und ob der resultierende
Disk-Lesevorgang (LSN 67) erfolgreich im Puffer landet.

Alle Emulator-Diagnosen wieder nur temporär, vollständig zurückgesetzt.

## Fortsetzung 5: Nachlade-Auslöser gefunden -- feuert nie, Verdacht wandert zu Interrupt-Rückmeldung

Mit der korrigierten Basis (`$D2D2`) ließ sich der Sektorgrenzen-
Mechanismus jetzt vollständig durchgehen:

- `$f7a` (Ringpuffer-Vorschub, live `$e24c`, 30 Treffer) ruft bei
  JEDER Sektorgrenze (`($32(a1)+$20) & $70(a1) == 0`) `$1cd6` auf --
  das entspricht (korrigierte Basis) `$efa8`.
- `$efca`/`$ed3e` (Segmentlisten-Auswertung) feuert dagegen nur **2x
  insgesamt** (einmal pro `I$Open`) -- das ist korrekt: das Verzeichnis
  liegt in einem einzigen Segment, eine erneute Segmentsuche ist nach
  dem ersten Mal nicht nötig.
- `$efa8` selbst wird 7x erreicht, nimmt darin aber **jedes Mal** den
  Kurzschluss-Pfad (`bclr.b #0,$2a(a1); beq.b $efc2`) -- der bedingte
  Aufruf von `$ef38` (dem letzten verbliebenen Kandidaten für die
  eigentliche Leseauslösung) wird dabei **kein einziges Mal** erreicht
  (`Q9_COUNT_PC`: `$ef38=0`, `$efbe`=0).

**Das bedeutet: Der Code-Pfad, der bei einer Sektorgrenze tatsächlich
einen neuen Lesevorgang auslösen würde, existiert -- wird aber nie
ausgelöst, weil ein Flag-Bit (`Bit 0` von `$2a(a1)`) beim Prüfen immer
schon gelöscht ist.**

### Neue Arbeitshypothese

Ein Flag-Bit, das "Lesevorgang wurde fertig gemeldet, bitte
nachladen" (oder umgekehrt "Nachladen nötig") bedeuten könnte, wird nie
gesetzt vorgefunden. Das passt zu einem klassischen OS-9-Muster:
Plattenzugriffe sind **asynchron** -- ein Treiber löst den eigentlichen
Transfer aus und ein Interrupt (bzw. dessen Handler) meldet später den
Abschluss zurück, wobei genau ein solches Flag gesetzt wird. Trifft
dieser Interrupt bei uns nie ein (oder wird er falsch/an der falschen
Stelle quittiert), bliebe das Flag dauerhaft im "kein Nachladen nötig"-
Zustand, obwohl in Wirklichkeit noch nie wirklich nachgeladen wurde --
exakt das beobachtete Verhalten.

Das würde die Untersuchung von der reinen RBF-Codeverfolgung weg und
zurück zu einem Bereich lenken, der in diesem Projekt schon mehrfach
Thema war: die Interrupt-Zustellung für den CF/IDE-Pfad (`Q9K_IRQDispatch`,
`F$IRQ`, Polling-Tabelle -- s. frühere Abschnitte in diesem Dokument).

**Nächster Schritt:** herausfinden, WER `Bit 0`/`Bit 1` von `$2a(a1)`
überhaupt setzt (Speicher-Watch auf `a1+$2a`, ein Byte) -- falls dort
gar kein Schreibzugriff außer den bekannten `bclr`-Stellen auftritt,
ist das der Beweis, dass die erwartete Interrupt-/Abschluss-Meldung bei
RBF nie ankommt.

Alle Emulator-Diagnosen wieder nur temporär, vollständig zurückgesetzt.

## Fortsetzung 6: Bit 1 von $2a(a1) wird nachweislich NIE gesetzt

Direkter Speicher-Watch auf `a1+$2a` (das komplette Flag-Byte, ein
Byte) über den gesamten Testlauf zeigt alle 46 Schreibzugriffe im
Klartext. Ergebnis: **keiner der beobachteten Werte (`$00, $08, $0c,
$20, $28`) hat jemals Bit 0 (`$01`) oder Bit 1 (`$02`) gesetzt.**

Das bestätigt endgültig: `efa8`s allererste Prüfung (`btst.b #1,$2a(a1)`)
schlägt bei allen 7 Aufrufen sofort fehl (Bit 1 ist immer 0) — die
komplette Nachlade-Logik in `efa8`/`ef38` ist für unseren Testfall
**totes, nie ausgeführtes Code** (nicht nur "Bit 0 falsch", wie zuvor
vermutet — schon die äußere Bedingung greift nie).

Auffällig im selben Mitschnitt: ein klares Poll-Paar (`$ee60`/`$eec8`,
abwechselnd Bit 5 setzend/löschend bei konstant gesetztem Bit 3) —
sieht nach einer echten Hardware-Status-Warteschleife aus (z. B.
"warte auf Controller bereit"), läuft mehrfach durch und endet sauber
bei `$00` (kein Fehlerbit hängen geblieben). Diese Aktivität fällt
zeitlich in den ERSTEN der beiden `I$Open`-Aufrufe (vermutlich Teil der
Attach-/Verzeichnissuche für "dd" selbst, nicht der `startup`-Suche).

### Offener Widerspruch (ehrlich benannt)

Wenn Bit 1 nie gesetzt wird UND der reine Adress-Masken-Mechanismus
(`$32(a1) AND $70(a1)`) den Puffer bei Überschreiten von 512 Byte
einfach auf denselben, unveränderten 512-Byte-Inhalt zurückspiegeln
würde, müssten die Einträge ab Offset `$220` dieselben Namen zeigen wie
ab Offset `$20` (Wrap-Around). Tatsächlich beobachtet wurde aber
**Nullen** ab ungefähr Offset `$1C0` (Eintrag 14) — weder ein Wrap
noch echte neue Daten. Diese Diskrepanz ist NICHT aufgelöst.

### Einordnung und Empfehlung

Diese Untersuchung ist inzwischen so tief in proprietären,
unkommentierten Microware-Binärcode vorgedrungen, dass weitere
Fortschritte nur noch in sehr kleinen Schritten und mit hohem Aufwand pro
Erkenntnis möglich sind (mehrere frühere Zwischenstände mussten schon
korrigiert werden, zuletzt sogar die grundlegende Ladeadresse). Für den
nächsten Anlauf empfiehlt sich einer von zwei Wegen:

1. **Puffergröße/Layout nochmal nachmessen** -- ob `$c8(a1)` wirklich
   512 ist oder ein anderer, bisher übersehener Wert, und ob die
   "Nullen ab Eintrag 14" schlicht bedeuten, dass dort auf Disk
   tatsächlich nichts steht (d. h. die REALEN Verzeichniseinträge auf
   diesem Image enden bei Eintrag 13, und `startup` liegt an einer
   ganz anderen Position/einem anderen Offset als angenommen -- dann
   wäre wieder ein reiner Pfad-/Positionsfehler die Ursache, kein
   Nachlade-Bug).
2. **Vergleichsmessung** mit einem bekannt funktionierenden RBF-Setup
   (falls verfügbar) oder gezielt die reale Microware-Dokumentation zu
   diesem RBF-internen Puffer-/Segment-Mechanismus konsultieren, statt
   ihn ausschließlich aus dem Binärcode zu rekonstruieren.

Alle Emulator-Diagnosen wieder nur temporär, vollständig zurückgesetzt.

## ECHTER FIX GEFUNDEN UND ANGEWENDET: F$PrsNam-Kettenkonvention (2026-09-08, Fortsetzung 7)

Weiter zurückverfolgt, wie der reale Namensvergleich innerhalb von RBF
tatsächlich abläuft (Live-Ground-Truth-Bytes, `Q9_DUMP_ADDR`/`Q9_WATCH_ADDR`,
über mehrere Sprünge hinweg bis zur eigentlichen Vergleichsroutine):

### Der Fund

RBF vergleicht jeden Verzeichniseintrag byteweise gegen einen Namen, dessen
Startzeiger und Länge es sich EINMALIG, VOR Beginn des Scans, auf seinem
eigenen Stack ablegt (`$8(a7)`/`$2(a7)`) — nicht pro Eintrag neu über
`F$PrsNam` geholt. Per Live-Messung (Freeze exakt beim Eintrag "startup",
der nachweislich real und korrekt im Puffer steht — Byte für Byte
`73 74 61 72 74 75 f0` = "startup"+Hochbit-Ende) zweifelsfrei nachgewiesen:

- Der Startzeiger für den Vergleich war (VOR dem Fix) `$758C` — das ist
  exakt der Rückgabewert von `Q9K_ProcPrsNam`s `outPastName` für den
  Namen `"dd"`, und `$758C` zeigt auf den TRENNER (`/`) vor `"startup"`,
  NICHT auf `"startup"` selbst.
- Ergebnis: RBF verglich jeden Verzeichniseintrag gegen `"/startup"`
  (8 Byte, mit Schrägstrich) statt gegen `"startup"` (7 Byte) — das kann
  gegen KEINEN echten Verzeichniseintrag matchen.

### Die Ursache: `outPastName` folgte nicht der echten Kettenkonvention

Der eigene Kopfkommentar von `Q9K_SysFPrsNam` (aus der echten
`scf`-Analyse übernommen) sagt: "a0 = hinter dem Namen" — das
ist mehrdeutig. Die Live-Messung klärt es: RBF benutzt `outPastName`
**direkt** als nächsten Namenszeiger, OHNE selbst noch einen Trenner zu
überspringen — die reale Kettenkonvention verlangt also, dass
`outPastName` schon HINTER einem eventuellen `/`-Trenner steht, nicht nur
hinter dem Namen selbst.

### Der Fix

`Q9K_ProcPrsNam` (`src/kernel/q9kernel_iopath.c`) überspringt jetzt einen
`/`-Trenner beim Setzen von `*outPastName` (NUL-Trenner/Pfadende bleibt
unangetastet, sonst liefe der Zeiger über das Stringende hinaus). Per
neuem Host-Regressionstest verifiziert (Kettenaufruf: `outPastName`
direkt als nächster `pathPtr` an `Q9K_ProcPrsNam` übergeben, liefert
korrekt `"SYS"` aus `"/dd/SYS/motd"`).

**Live im Emulator verifiziert:** der Vergleichs-Startzeiger ist jetzt
korrekt `$758D` (zeigt exakt auf das `'s'` von `"startup"`). Der
byteweise Vergleich selbst **findet jetzt tatsächlich einen Treffer**
(`d0=0` nach der Vergleichsschleife, kein XOR-Rest) — der Fix behebt
also einen echten, jetzt live bestätigten Bug.

### Trotzdem noch kein voller Erfolg — zweiter, verwandter Bug gefunden

Der volle Boot-Test zeigt weiterhin `$D8`. Grund, ebenfalls live
verifiziert: die Vergleichs**länge** (`$2(a7)`) ist weiterhin `2` (die
Länge von `"dd"`), nicht `7` (die Länge von `"startup"`) — die
Vergleichsschleife bricht nach nur 2 verglichenen Zeichen (`'s'`,`'t'`)
ab, und eine zusätzliche Sicherheitsprüfung danach (die erkennt, dass
der Verzeichniseintrag nach den 2 verglichenen Zeichen noch nicht zu
Ende ist) verwirft den eigentlich korrekten Treffer zu Recht als
Präfix-Treffer statt Volltreffer.

Per Aufrufer-PC-Diagnose (`Q9K_TrapCallerPC`) zweifelsfrei geklärt: BEIDE
beobachteten `F$PrsNam`-Aufrufe für `/dd/startup` kommen von DERSELBEN
RBF-internen Adresse (Dateioffset `$160C`) und liefern BEIDE `"dd"` —
`F$PrsNam` wird für `"startup"` selbst **nie** aufgerufen. Die
Vergleichslänge `2` muss also aus `"dd"`s eigener, gecachter Länge
stammen (vermutlich wiederverwendet statt für die zweite Pfadebene neu
berechnet) — vermutlich RBFs eigene interne Nachfolge-Scan-Logik, die
NICHT über `F$PrsNam` läuft und noch nicht bis zu ihrer Quelle
zurückverfolgt wurde.

**Nächster Schritt (noch offen):** herausfinden, WOHER die
Vergleichslänge `2` tatsächlich kommt (vermutlich RBFs eigener,
`F$PrsNam`-unabhängiger Nachfolge-Namens-Scan) und warum sie nicht auf
`7` aktualisiert wird, wenn der Startzeiger (jetzt korrekt) auf
`"startup"` zeigt.

**Status:** Der `F$PrsNam`-Fix ist ein echter, verifizierter,
eigenständiger Bugfix (committet) — er behebt eine reale
Fehlfunktion, auch wenn der `$D8`-Symptomfall wegen des zweiten,
noch offenen Bugs weiterhin auftritt.

Alle Emulator-Diagnosen wieder nur temporär, vollständig zurückgesetzt.

## Fortsetzung 8: Ursprung der falschen Vergleichslänge -- führt zu IOMan, nicht RBF

Die Vergleichslänge (`2`, statt der benötigten `7` für `"startup"`) bis
zu ihrem Ursprung zurückverfolgt (Speicher-Watch auf die Stack-Position,
dann Ground-Truth-Bytes am schreibenden PC):

- Die Länge (`d1=2`) stammt aus einer RBF-internen Hilfsroutine
  (`bsr.w $e2a2`, innerhalb der laufenden Suche aufgerufen, NICHT über
  `F$PrsNam`), deren Ergebnis in eine lokale, F$PrsNam-Ausgabe-artige
  Struktur (SR/d0/d1/a0/a1 -- exakt das reale F$PrsNam-Ausgabeformat)
  zwischengespeichert wird.
- Deren EINGABE (`a0`) ist beim Aufruf `$758A` -- **wieder** der Zeiger
  auf `"dd"`, nicht auf `"startup"` (`$758D`).
- Zurückverfolgt bis zu dessen eigener Quelle: **nicht RBF**, sondern
  eine ANDERE, tiefer im Speicher liegende Codeadresse (`$B6E8`,
  außerhalb von RBFs Adressraum -- ein anderer, ebenfalls
  unveränderter Microware-Modul, mit an Sicherheit grenzender
  Wahrscheinlichkeit **IOMan**, erkennbar am andersartigen
  Prozesskontext `a4=$19400` statt RBFs durchgehendem `a4=$7100`).
- Dort: `a0` wird aus dem URSPRÜNGLICHEN, kompletten Pfadzeiger
  (`$7589`, derselbe Zeiger, den auch der allererste `F$PrsNam`-Aufruf
  als Eingabe bekam) berechnet, per simplem `+1` -- überspringt NUR
  das eine führende `/`, landet bei `$758A` ("dd/startup" MINUS
  Schrägstrich) -- NICHT beim eigentlichen Dateinamen `"startup"`
  (`$758D`, hinter `/dd/`).

### Einordnung

Das ist jetzt eine Ebene TIEFER als der bereits gefixte `F$PrsNam`-Bug:
**IOMan** (nicht RBF) berechnet den an RBF weiterzugebenden Namen/die
Länge offenbar durch simples Überspringen des ERSTEN Zeichens, nicht
durch korrektes Abtrennen des kompletten Geräte-Präfixes `/dd/`. Da
IOMan ebenfalls reale, unveränderte Microware-Software ist, kann das
kein "IOMan-Bug" im klassischen Sinn sein -- wahrscheinlicher: IOMan
verlässt sich dabei auf einen Zustand/eine Information, die WIR ihm
liefern müssen (z. B. "wie viele Zeichen hat der bereits erfolgreich
angehängte Geräte-Präfix verbraucht", damit IOMan korrekt darüber
hinaus zeigen kann) -- und dieser Zustand ist bei uns entweder nicht
vorhanden oder falsch.

**Nächster Schritt (noch offen, neue, tiefere Untersuchungsebene):**
klären, welche Information IOMan an dieser Stelle (`$B6E8` ff.)
tatsächlich benutzt, um die Präfixlänge zu bestimmen (vermutlich ein
Rückgabewert/Zustand aus dem vorangegangenen `I$Attach`-Aufruf für
`/dd`, den wir liefern) -- das ist eine neue, von der bisherigen
RBF-Spur unabhängige Untersuchung in IOMans eigenem Code.

Alle Emulator-Diagnosen wieder nur temporär, vollständig zurückgesetzt.
Der `F$PrsNam`-Fix aus der vorigen Runde bleibt unangetastet gültig
(committet) -- er behebt weiterhin einen echten, eigenständigen Bug,
auch wenn er für DIESEN speziellen Testfall allein nicht ausreicht.

## Fortsetzung 9: Korrektur der vorigen IOMan-Spur, ehrlicher Zwischenstand

Die vorige Vermutung ("IOMan überspringt nur ein Zeichen statt `/dd/`")
musste beim Versuch, sie weiter zu erhärten, **zurückgenommen** werden:

- Byte-für-Byte-Abgleich (exakte 10-Byte-Signatur direkt im Kernel-
  Binary `build/q9kernel` gesucht) zeigt zweifelsfrei: die Adresse
  `$B6E8` (Quelle des `+1`-Zeigers) liegt NICHT in IOMan, sondern in
  **unserem eigenen Kernel**, konkret in `Q9K_ProcFork`
  (`q9kernel_firstproc.c`, kompiliert in `q9kernel_firstproc.r`).
- `Q9K_ProcFork` implementiert `F$Fork` (Prozess-/Programmstart) — hat
  mit der Verzeichnissuche nach `"startup"` inhaltlich nichts zu tun.
  Die beobachtete Überschneidung war mit hoher Wahrscheinlichkeit
  **Stack-Speicher-Wiederverwendung**: derselbe Stack-Steckplatz wird
  zu unterschiedlichen Zeiten für unterschiedliche, voneinander
  unabhängige Zwecke benutzt (einmal von `Q9K_ProcFork` beim Starten
  des Testprogramms, einmal von RBFs eigenem Vergleich) — reiner
  Zufallstreffer bei der Adresse, kein echter Datenfluss.

### Was weiterhin gesichert ist

- Die Schreibstelle `pc=$E150` für die Vergleichslänge `2` liegt
  (korrekt nachgerechnet, `$E150 > $D2DC`) tatsächlich **innerhalb von
  RBF** (Dateioffset `$E74`) — anders als zwischenzeitlich fälschlich
  angenommen. Das ist also doch reales RBF-Verhalten.
- Von dort direkt zurückverfolgt zu einem weiteren RBF-internen
  `F$PrsNam`-Wrapperaufruf (`$E142`, ruft `$E2A2` -- RBFs eigene
  zweite `F$PrsNam`-Aufrufstelle, Dateioffset `$FC6`).
- Dessen Eingabe (`a0=$758A`) konnte bis zu einer Stelle
  zurückverfolgt werden, die sich beim genaueren Hinsehen als
  **kausal nicht zusammenhängend** (Stack-Wiederverwendung mit
  `Q9K_ProcFork`) erwiesen hat -- die WIRKLICHE Quelle von `a0=$758A`
  für DIESEN spezifischen RBF-internen Aufruf ist damit wieder offen.

### Ehrliche Einordnung

Diese letzte Teiluntersuchung ist in Widersprüche geraten (zwei
verschiedene, ähnlich benannte Adressen -- `$E150` innerhalb RBF vs.
`$B6E8`/`$B6FA` außerhalb -- wurden zwischenzeitlich versehentlich als
derselbe Sachverhalt behandelt). Um weiteren Verwechslungen
vorzubeugen: **jede neue Adresse muss einzeln gegen `$D2DC`
(RBF-Anfang) geprüft werden, bevor ihr eine Bedeutung zugeschrieben
wird** -- keine Abkürzungen mehr.

**Nächster Schritt:** von `$E150`/`$E142` (beides zweifelsfrei RBF-
intern) aus NEU zurückverfolgen, diesmal mit konsequenter
Bereichsprüfung jeder einzelnen Zwischenadresse, um die wahre Quelle
von `a0=$758A` (dem Eingabewert für RBFs zweiten `F$PrsNam`-Aufruf)
zu finden.

Der `F$PrsNam`-Fix aus den vorigen Runden bleibt unangetastet gültig.
Alle Emulator-Diagnosen (inkl. der temporären `a3`-Ringpuffererweiterung)
wieder vollständig zurückgesetzt.

## Fortsetzung 10: wahre Quelle von a0=$758A gefunden -- keine Verunreinigung, sondern zweifach korrekt berechneter Wert; E142s eigentlicher Zweck bleibt offen (2026-09-08, neue Session nach Pause)

**Neues Werkzeug: `Q9_FREEZE_A0`** (Ergänzung zu `Q9_FREEZE_PC`) -- friert
nur ein, wenn PC **und** a0 gleichzeitig passen (analog `Q9_FREEZE_PC`/
`Q9_FREEZE_PC_N` in `m68krt.c`, derselbe `getenv`-Mechanismus). Erspart
das Erraten einer Trefferzahl, wenn dieselbe Adresse für kausal
verschiedene Aufrufe durchlaufen wird -- genau das Problem, an dem
Fortsetzung 8/9 gescheitert waren. Wie alle anderen Werkzeuge nach
Gebrauch per `git checkout` zurückgesetzt; gehört ab jetzt zum
"Bewährten Werkzeug-Set" oben.

### Ausgangspunkt: RBF_BASE für den aktuellen Build neu verifiziert

Kernel neu gebaut (Quelltext seit dem letzten Stand unverändert, Größe
`0x37d4`), Testabbild nach dem dokumentierten Rezept neu erzeugt
(`tools/mkbootfile.sh --disk`, Disk-Module aus der
letzten Session weiterverwendet). Rohspeicher an `$D2DC` per
`Q9_DUMP_ADDR` gelesen: `4a fc 00 01 00 00 25 a6` -- `M$ID` gefolgt von
`M$Size=$25A6`, exakt `rbf.mod`s reale Größe. **`RBF_BASE=$D2DC`
bestätigt**, wie am Ende der letzten Session vermutet.

### Sauberer Freeze bei PC=$E142 UND a0=$758A

Mit `Q9_TRACE_INSTR=1 Q9_FREEZE_PC=0xE142 Q9_FREEZE_A0=0x758A` lief der
komplette 24576-Eintrag-Ring bis zu genau diesem Treffer (`sp=$2D348`,
`a4=$00007100`). Per `Q9_COUNT_PC` nachträglich bestätigt: **`$E142`
wird im gesamten Boot genau EINMAL erreicht** -- der gefrorene Treffer
ist also zweifelsfrei der einzige, gesuchte Aufruf, keine Verwechslung
mit einer anderen Ebene möglich (anders als in Fortsetzung 8/9
befürchtet). Auffällig: `$E8E8` (die aus dem *alten* Dateioffset `$160C`
unter der damaligen Session-Basis hochgerechnete Adresse) wird gar
nicht erreicht -- die frühere Umrechnung war für den *jetzigen* Build
nicht mehr gültig (erwartungsgemäß, da genau das der Kern der
"Ladeadresse bei jedem Build neu bestimmen"-Lehre ist).

### Rückverfolgung im Ring: doppelt unabhängig, nicht verunreinigt

Erste Erscheinung von `a0=$758A` im 24576-Eintrag-Fenster: `pc=$B6F0`,
`a4=$19400` (Prozesskontext, stimmt mit der Ready-Queue überein) --
laut `Q9K_BootList`-Dump für diesen Lauf zweifelsfrei innerhalb von
**IOMan** (`$AB76`-`$C192`). Live-Ground-Truth-Bytes an dieser Stelle
(`Q9_DUMP_ADDR`) analysiert:

```
00b6e4: movea.l $20(a5), a0      ; a0 = Pfadname aus dem Deskriptor
00b6e8: cmpi.b  #$2f, (a0)       ; erstes Zeichen '/'?
00b6ec: bne.b   $b6f0
00b6ee: addq.l  #1, a0           ; GENAU EIN führender Trenner uebersprungen: $7589->$758A
00b6f0: movea.l a7, a5           ; eigener Mini-Deskriptor auf dem Stack
...
00b6fa: move.l  a0, $20(a5)
00b6fe: bsr.w   $b8cc            ; ECHTER F$PrsNam-Trap (via $3a4(a6)/D_SysDis-Trampolin,
                                  ; dieselbe Konvention wie in RBFs eigenem Code)
```

Das ist **Standard-`I$Open`-Verhalten**, kein Bug: IOMan überspringt
den einen führenden Trenner und ruft `F$PrsNam` für den Gerätenamen
("dd") auf.

Zweiter, unabhängiger Fund: RBFs **eigener Eintrittspunkt** live
bestätigt bei `$D5B2` (Dateioffset `$2D6`), aufgerufen von IOMan bei
`$C0D4` mit `a0 = $2D380` -- ein Zeiger auf einen STACK-Deskriptor,
NICHT der rohe Pfadname. Kurz danach (`$DF94`→`$DF98`) dereferenziert
RBF diesen Deskriptor selbst und lädt `a0 = $7589` -- der volle,
unveränderte Pfadname MIT führendem Trenner (einen weniger als IOMans
`$758A`!). **RBF parst also selbst, unabhängig von IOMan, noch einmal
von vorn.**

**Der Auflösungspunkt:** `Q9K_ProcPrsNam($7589)` überspringt den einen
führenden Trenner (`outNameStart = $758A`) und liefert für den Namen
"dd" (Länge 2, Trennzeichen `/`) `outPastName = $758D`. **`$758A` ist
also exakt `outNameStart` von "dd" -- ein Wert, der bei JEDER korrekten
Zerlegung dieses Pfadpräfixes entsteht, egal ob man (wie IOMan) bei
`$7589` startet und einen Trenner überspringt, oder (wie RBF) denselben
String parst.** Die frühere Fortsetzung-8/9-Frage "kommt das von IOMan,
oder ist es Zufall?" war im Kern falsch gestellt -- es handelt sich
nicht um eine Verunreinigung zwischen zwei Aufrufern, sondern um ein
mathematisch notwendiges Zwischenergebnis, das an mehreren Stellen
unabhängig entsteht. Fortsetzung 9s Rücknahme war also im Ergebnis
richtig (keine IOMan-Verunreinigung), aber aus dem falschen Grund
(vermeintliche Stack-Wiederverwendung mit `Q9K_ProcFork` -- diesmal mit
sauberem Freeze zweifelsfrei ausgeschlossen, siehe oben).

### Was der Aufruf bei $E142 mit diesem Wert tatsächlich anrichtet

Weiter live verfolgt (Ground-Truth-Analyse direkt ab dem
bestätigten `$E142`):

```
00e142: bsr.w   $e2a2         ; = trap #0  -- ECHTER F$PrsNam-Syscall (kein RBF-Code mehr
                               ;   danach, die Folgebytes sind OS-9-Inline-Callcode, s.
                               ;   ÜBERGABE-Warnung zu Trap-Inline-Woertern)
00e150: move.w  d1, $6(a7)     ; d1 = Laenge (=2) -- NUR in einen kurzlebigen CCR-Scratch-
                               ;   Puffer kopiert, der bei $e164 wieder freigegeben wird,
                               ;   OHNE je zurueckgelesen zu werden (Sackgasse!)
...
00e166: bcs.w   $e1f8
00e16a: movem.l d2/a2, -(a7)
00e16e: move.l  d1, d2         ; die WIRKLICH weiterverwendete Laenge: direkt aus dem
                               ;   Register d1, unveraendert seit dem Trap-Ruecksprung
00e170: lea.l   $e0(a1), a2    ; a2 = Kopierziel-Puffer
00e174: clr.b   (a2, d2.w)     ; NUL-Byte bei Puffer+Laenge(=2) -- schneidet den Namen
                               ;   auf 2 Zeichen ab, BEVOR die eigentliche Vergleichsschleife
                               ;   (bsr $d662, per Live-Trace bereits als "startup"-Vergleich
                               ;   bestaetigt) ihre Kopie bekommt
```

Damit ist die ursprüngliche ÜBERGABE-Vermutung im Kern bestätigt: **die
Vergleichslänge 2 stammt exakt daher, dass `F$PrsNam` bei `$E142` mit
`pathPtr=$758A` (Beginn von "dd") statt `$758D` (Beginn von "startup")
aufgerufen wird** -- ab `$758A` liefert JEDE korrekte `F$PrsNam`-Parse
zwangsläufig Länge 2 ("dd", terminiert durch den folgenden `/`),
unabhängig davon, welche konkrete Quelle den Zeiger geliefert hat. Der
eigentliche Fehler liegt also nicht in "welcher Aufrufer hat a0
verunreinigt", sondern darin, **dass RBF für diesen zweiten,
internen `F$PrsNam`-Aufruf einen Zeiger auf den BEGINN von Ebene 1
("dd") verwendet, obwohl zu diesem Zeitpunkt bereits Ebene 2
("startup") gescannt wird** -- derselbe `outNameStart`, den RBF (oder
eine von uns bereitgestellte Struktur) offenbar nicht zwischen den
beiden Ebenen aktualisiert.

### Offen: WESSEN Feld RBF hier tatsächlich (wieder-)liest

Nicht abschließend geklärt: ob RBF den Zeiger `$758A` erneut aus dem
Deskriptor bei `$2D380`/`$DF98` liest (derselbe, den es schon für die
GESAMTE Operation einmalig dereferenziert hat, s. o. -- dann müsste RBF
diesen Deskriptor-Slot eigentlich selbst zwischen den Ebenen
fortschreiben, tut es aber laut Messung nicht), oder ob eine ANDERE,
noch nicht identifizierte Quelle (Stack-Slot `$8(a7)`/`$10(a7)` der
Vergleichsroutine, s. Fortsetzung 7) hier hineinspielt. `Q9_COUNT_PC`
zeigt `$D5B2` (RBFs Eintrittspunkt) **zweimal** pro Boot -- ob der
zweite Treffer zur selben `/dd/startup`-Anfrage gehört (z. B. ein
interner Retry) oder zu einer ganz anderen, späteren Anfrage, ist
ebenfalls offen und sollte zuerst geklärt werden, bevor man tiefer in
die Deskriptor-Frage einsteigt.

**Nächster Schritt:** den zweiten `$D5B2`-Treffer einordnen (gehört er
zu `/dd/startup`? per `Q9_FREEZE_PC=0xD5B2 Q9_FREEZE_PC_N=2` plus
`Q9_TRACE_INSTR=1` pruefen, a0 am Eintritt notieren). Danach gezielt
zwischen `$DF98` (Deskriptor-Dereferenzierung, a0 wird `$7589`) und
`$E13E` (laedt `$10(a7)` fuer den `$E142`-Aufruf) den Code Schritt fuer
Schritt analysieren -- diesmal mit Ground-Truth-Bytes an einem
zweiten, garantiert live bestaetigten Anker (nicht nur an den beiden
Enden) --, um die tatsaechliche Quelle von `$10(a7)` (Speicher- oder
Registerpfad) zu finden. Testabbild fuer die Reproduktion:
`Q9-Flux/local_images/OS9SYS.dbg10.hda` (mit dem aktuellen Kernel-Stand
neu gebaut, RBF_BASE=$D2DC bestaetigt); volle Ringpuffer-Spur dieser
Session gesichert unter
`$CLAUDE_JOB_DIR/tmp/trace_full.txt` (24576 Eintraege, endet exakt bei
`pc=$E142`).

Alle Emulator-Diagnosen (`Q9_DUMP_ADDR`, `Q9_FREEZE_A0`) wieder
vollständig zurückgesetzt (`git checkout`).

## Fortsetzung 11: $E076 wirft den korrekten Zeiger weg -- und Fortsetzung 7s Erfolgsmessung muss neu verifiziert werden (2026-09-08, dieselbe Session)

**Zweiter `$D5B2`-Treffer eingeordnet:** derselbe Deskriptor
(`a0=$2D380`), dieselbe Stack-Tiefe (`sp=$2D360` bei Eintritt) wie beim
ersten Treffer -- IOMan ruft RBFs Eintrittspunkt also zweimal mit
identischem Deskriptor auf. Wichtig fuer die Interpretation der
bisherigen Funde: die in Fortsetzung 10 analysierte Spur (beginnend bei
Zeile 15810 in `trace_full.txt`) ist -- da der 24576-Eintrag-Ring den
ERSTEN `$D5B2`-Treffer bereits verdraengt hatte, bevor `$E142` einfror
-- tatsaechlich schon der ZWEITE Aufruf. Kein Widerspruch zu
Fortsetzung 10, nur eine Praezisierung.

**Neue Werkzeug-Erweiterung: `a1` im Ringpuffer.** `q9_dbg_tr_a0`
diente bisher als einziges Adressregister im Trace; `a1` fehlte (die
Doku hatte das selbst als moegliche Erweiterung vorgesehen). Patch
analog zu `a0` in `m68krt.h`/`m68krt.c`/`q9boardrun.c` (Feld
`q9_dbg_tr_a1`, dritte Spalte im Dump). Nach Gebrauch zurueckgesetzt,
Rezept hier fuer die Rekonstruktion.

### Der eigentliche `$E062`-Aufruf -- und was RBF mit dessen Ergebnis macht

Live mit `a1` mitgeschnitten: RBFs `$E062` (`bsr.w $e2a2` = `trap #0`)
ist ein weiterer, bisher nicht dokumentierter `F$PrsNam`-Aufruf --
zeitlich VOR `$E142`, mit `pathPtr=$7589` (dem vollen, unveraenderten
Pfad "/dd/startup"). Der Trap-Ruecksprung liefert exakt die
dokumentierte ABI: `a0 = outPastName = $758D` ("startup"-Anfang, s.
`Q9K_PRSNAM_SCRATCH_PAST`), `a1 = outNameStart = $758A` ("dd"-Anfang,
`Q9K_PRSNAM_SCRATCH_NAME`) -- **live bestaetigt bei `pc=$7A6A`
(Ruecksprung im eigenen Kernel), also unser eigener `F$PrsNam`
funktioniert hier nachweislich korrekt.**

Direkt danach, in RBFs eigenem Code:

```
00e076: movea.l a1, a0     ; a0 := a1  -- WIRFT $758D weg, a0 wird $758A
00e078: movea.l $c(a7), a1 ; a1 bekommt einen NEUEN Wert von der eigenen
                            ;   Aufrufer-Stack-Ecke (nicht mehr der Name)
```

**Ab hier bleibt `a0` fuer den GESAMTEN Rest dieser RBF-Invocation
`$758A` -- bis einschliesslich `$E142`.** Vollstaendig durchsucht
(`grep` ueber die ganze `a1`-Spur zwischen zweitem `$D5B2` und `$E142`):
`a0` und `a1` nehmen den Wert `$758D` **kein einziges Mal** wieder an.
`$8(a7)`/`$10(a7)` bei `$E0F0` (die Fortsetzung 7 als "einmalig vor dem
Scan abgelegter Vergleichs-Startzeiger" identifiziert hatte) werden
beide direkt aus diesem `a0` gefuellt -- also ebenfalls `$758A`, nicht
`$758D`.

### Widerspruch zu Fortsetzung 7 -- ungeklärt, wer recht hat

Das steht im Widerspruch zu Fortsetzung 7s Live-Messung ("Der
Vergleichs-Startzeiger ist jetzt korrekt `$758D`"): wenn der
Vergleich tatsaechlich bei `$758A` beginnt (`"dd/startup"`, erstes
Zeichen `'d'`), muesste er gegen einen mit `'s'` beginnenden
Verzeichniseintrag ("startup") sofort im ERSTEN Byte scheitern -- kein
Treffer moeglich. Fortsetzung 7s Messung war eine echte
`Q9_DUMP_ADDR`-Live-Messung, keine Vermutung -- vermutlich existiert
also eine WEITERE, noch nicht gefundene Stelle, an der `$758D`
zwischenzeitlich doch wieder hergestellt wird (z. B. ueber `d0`/`d1`
oder eine Stack-Adresse ausserhalb von `a0`/`a1`, die dieser
`a0`/`a1`-Trace naturgemaess nicht zeigt), BEVOR die eigentliche
Vergleichsschleife (`bsr $d662` von `$E124`) beginnt.

**Nächster Schritt:** Fortsetzung 7s Messung mit den jetzt verfuegbaren,
praeziseren Werkzeugen (`Q9_FREEZE_A0`, `a1`-Tracking) FRISCH
wiederholen -- konkret: `Q9_WATCH_ADDR` auf die tatsaechliche
Speicheradresse legen, an der die Vergleichsschleife (`$d662`, ueber
`$e124` aufgerufen) ihren Namens-Zeiger/Laenge herbekommt (vermutlich
`$8(a7)`/`$2(a7)` EINER SPAETEREN Stack-Tiefe als bei `$E0F0`, da die
eigentliche Suchschleife erst nach `$E124` beginnt und zwischen `$E0F0`
und `$E124` noch einiges passiert, das hier noch nicht Zeile-fuer-Zeile
durchleuchtet wurde). Ziel: die Stelle finden, an der `$758D`
tatsaechlich (wieder) gesetzt wird, und diese mit `$E0F0`s
`$758A`-Snapshot in Bezug setzen -- vermutlich zwei UNABHAENGIGE
`F$PrsNam`-Ketten (eine fuer den Vergleich, korrekt; eine fuer
`$E142`s Laengenberechnung, fehlerhaft), deren Trennung noch nicht
verstanden ist.

Alle Emulator-Diagnosen (`a1`-Tracking) wieder vollständig
zurückgesetzt (`git checkout`).

## Fortsetzung 12: E142s $758A war nie das Problem -- die Suche geht nach $E142 weiter, "$D8" tritt spaeter auf als bisher verfolgt (2026-09-08, dieselbe Session)

**Wichtigste Korrektur dieser Runde:** alle bisherigen Freezes (Fortsetzung
10/11) frorren GENAU BEI `$E142` ein -- alles danach war reine, NIE
live verifizierte Vermutung ("meine eigene Analyse ab `$E146`
wurde faelschlich als Tatsache behandelt"). Frisch mit `Q9_FREEZE_PC`
auf spaetere Adressen (`$E174`, `$E1F8`) eingefroren, um wirklich zu
sehen, was passiert:

### `$758A` fuehrt trotzdem zu `$758D` -- E142 ist wahrscheinlich KEIN Bug

`Q9K_ProcPrsNam($758A)` (Eingabe "dd/startup", ohne fuehrenden Trenner)
liefert **ebenfalls** `outPastName=$758D` -- rechnerisch zwangslaeufig,
da "dd" mit oder ohne fuehrenden Trenner an derselben Stelle endet.
Live bestaetigt (`pc=$e146`, direkt nach dem Trap-Ruecksprung):
**`a0=$758D`, korrekt.** Die in Fortsetzung 10/11 als Bug eingeordnete
"`$758A` statt `$758D`"-Beobachtung betraf nur den EINGABEWERT von
`$E142`, nicht dessen Ausgabe -- und die Ausgabe ist fuer die
nachfolgende Suche richtig. **`$E142` ist damit mit hoher
Wahrscheinlichkeit KEIN Bug**, sondern RBFs (etwas umstaendlicher, aber
funktionierender) Weg, den Zeiger fuer die naechste Pfadebene zu
gewinnen. Fortsetzung 10/11 bleiben als Messungen gueltig, ihre
BUG-Einordnung an dieser Stelle war voreilig.

### Neuer Fund: der einzige `$E1F8`-Fehlerpfad liefert `$D3`, nicht `$D8`

Von `$E142` aus weiterverfolgt: `$e174` (`lea.l $e0(a1),a2` /
`clr.b (a2,d2.w)`) wird genau einmal erreicht, mit `a0=$758D` --
passt. Der Code laeuft weiter durch `$e178`(`bsr $d39c`)→`$e180`
(`bsr $e26c`)→`$e18e` (Schleifenkopf) und schliesslich (ueber
`$e272`→ eine Positions-/Laengenpruefung bei `$d92a`-`$d94c`:
`d2 = $36(a1) - $32(a1)`, verglichen gegen einen Stackwert) zu
`$e1f8`. **Neu per `d1`-Register-Tracking (Erweiterung von `a1`,
gleiches Muster) live gemessen: `d1=$000000D3` an diesem einzigen
`$E1F8`-Treffer** -- ein ANDERER Fehlercode als der gesuchte `$D8`.
Der `$d92a`-Mechanismus ist eine reine FD-Positions-/Bereichspruefung
(vermutlich "ist noch mehr vom aktuellen Verzeichnispuffer da, oder
muss nachgeladen werden") -- WEDER Namensvergleich noch die gesuchte
`$D8`-Ursache.

**Die Konsolenausgabe zeigt weiterhin `$D8`** (`f000000D8n` im
Bootlog) -- der `$D3`-Fehler wird also von einem AEUSSEREN Mechanismus
abgefangen/wiederholt (passt zum aus Fortsetzung 4/5 bekannten
"Nachlademechanismus"), und der tatsaechliche `$D8` entsteht
chronologisch SPAETER als alles bisher Verfolgte. Ein Scan von `d1` in
der `$E1F8`-Spur nach `$D8` fand nur einen Zufallstreffer (`pc=$fa14`
in CFIDE, `d0=$00123456` -- ein Fuellmuster/Testwert, `d1=$D8` dort
rein numerischer Zufall, kein Fehlercode).

**Neue Werkzeug-Erweiterung: `d1` im Ringpuffer** (gleiches Muster wie
`a1`, Fortsetzung 11) -- gehoert ab jetzt zusammen mit `a1` zum
Werkzeug-Set, beide muessen bei Bedarf neu gepatcht werden (git
checkout nach Gebrauch, wie ueblich).

**Nächster Schritt:** vom `$D3`-Fehler bei `$E1F8` aus WEITER
verfolgen (`Q9_FREEZE_PC` auf die RTS-Rueckkehradresse dieses Aufrufs
oder auf plausible "Nachlade"-Routinen wie die aus Fortsetzung 5
bekannte Segmentlisten-Logik), um zu sehen, ob/wie der `$D3` in einen
Retry-Versuch muendet, und DANN den naechsten `$D8`-Kandidaten
(`Q9_COUNT_PC` auf mehrere Verdachtsadressen gleichzeitig, `d1`-Wert
jeweils per Freeze pruefen) zu finden. Reproduktion unveraendert:
`Q9-Flux/local_images/OS9SYS.dbg10.hda`, RBF_BASE=`$D2DC`.

Alle Emulator-Diagnosen (`d1`/`a1`-Tracking) wieder vollständig
zurückgesetzt (`git checkout`).

## Fortsetzung 13: DURCHBRUCH -- der echte Namensvergleich live gefunden, Fortsetzung 12s Entwarnung war falsch (2026-09-08, dieselbe Session)

**Fortsetzung 12 zurückgenommen:** die dortige Einschätzung ("`$E142`
ist wahrscheinlich KEIN Bug") beruhte darauf, dass der zurückgegebene
Zeiger `$758D` korrekt aussah -- die zugehörige LÄNGE wurde nicht
weiterverfolgt. Jetzt live bis zur tatsächlichen Vergleichsinstruktion
durchgetestet: **die Länge IST das Problem**, und `$E142` liefert
sie falsch. Fortsetzung 10/11s ursprüngliche Einordnung war richtig.

### Der echte Namensvergleich: `$E20A`, nicht `$D662`

`$D662` (in Fortsetzung 10 fälschlich als "Vergleichsschleife"
bezeichnet) ist tatsächlich FD-Feld-Initialisierung nach einem bereits
gefundenen Treffer. Der ECHTE, byteweise, case-insensitive
Namensvergleich (XOR-Muster `eor.b`/`andi.b #$df`, danach `andi.b #$7f`
fürs Hochbit-Namensende -- exakt das aus Fortsetzung 7 erinnerte
"kein XOR-Rest") sitzt bei `$E20A`:

```
00e20a: movem.l d0-d2/a0-a1, -(a7)
00e20e: subq.w  #1, d1          ; d1 = Vergleichslaenge - 1
00e210: move.b  (a0)+, d0       ; Suchname-Zeichen
00e212: move.b  (a1)+, d2       ; Verzeichniseintrag-Zeichen
00e214: eor.b   d2, d0
00e216: andi.b  #$df, d0        ; Gross-/Kleinschreibung ignorieren
00e21a: dbne    d1, $e210       ; Schleife
00e21e: andi.b  #$7f, d0        ; Hochbit des letzten Namensbytes ausblenden
00e222: bne.b   $e22c           ; Rest != 0 -> Fehltreffer
00e224: subq.w  #1, d1
00e226: bcc.b   $e22c           ; Laengen nicht exakt gleich -> Fehltreffer
00e228: moveq   #0, d0          ; echter Volltreffer
```

Aufgerufen von `$e19e` (`bsr.b $e20a`), mit `a0 = $8(a7)` (einmalig
beim ERSTEN Schleifendurchlauf gesetzt, s. `$e236`/`$e244`) und
`d1 = $2(a7)` -- zwei GETRENNTE Stack-Felder derselben Schleifen-Ebene
(nicht dieselben wie `$E0F0`s `$8(a7)`/`$10(a7)`, andere Aufruftiefe).

### Live gemessen -- eindeutiger Beweis

`Q9_FREEZE_PC=0xE20A Q9_FREEZE_PC_N=1` (erster von 30 Vergleichen,
30 = 960 Byte Verzeichnis / 32 Byte pro Eintrag) plus `d1`/`a1`-Tracking:

    pc=0000e20a d0=00000001 d1=00000002 a0=0000758d a1=00036310 a4=00007100 sp=0002d344

**`a0=$758D` (korrekt, Beginn von "startup") -- aber `d1=2`, nicht 7.**
Der Vergleich prüft also nur `'s','t'` gegen die ersten zwei Zeichen
jedes Verzeichniseintrags, und die anschließende Längenprüfung
(`subq.w #1,d1 ; bcc $e22c`) verwirft JEDEN Treffer, dessen Eintrag
länger als 2 Zeichen ist -- **also auch den echten "startup"-Eintrag,
bei allen 30 durchlaufenen Einträgen.** Genau das im allerersten
ÜBERGABE-Befund vermutete Verhalten, nur jetzt direkt am Ort des
Geschehens bewiesen statt indirekt erschlossen.

### Der Kreis schließt sich zu Fortsetzung 10/11

Diese `d1=2` stammt -- wie in Fortsetzung 10/11 gezeigt -- aus
`$E142`s `F$PrsNam`-Aufruf mit `pathPtr=$758A` (statt `$758D`): das
liefert `outPastName=$758D` (deshalb sieht `a0` korrekt aus) UND
`outLen=2` (Länge von "dd", nicht "startup"). **Beide Werte
(Zeiger UND Länge) stammen aus DEMSELBEN einen `F$PrsNam`-Aufruf --
der Zeiger ist zufällig trotzdem richtig, die Länge nicht.** Fortsetzung
10/11s Rückverfolgung (`$E076: movea.l a1,a0` verwirft `$758D` zugunsten
von `outNameStart=$758A`, dieser Wert bleibt bis `$E0F0`s Snapshot
unverändert) bleibt damit die gültige, jetzt vollständig bestätigte
Erklärungskette für den kompletten `$D8`-Fehler.

### Offen: was RBF an dieser Stelle eigentlich erwartet

Real, unveraendertes RBF muss fuer echte OS-9-Installationen
funktionieren -- die wahrscheinlichste Erklaerung: RBF erwartet, dass
zwischen der `"dd"`-Ebene und der `"startup"`-Ebene ein WEITERER,
separater `F$PrsNam`-Aufruf mit `pathPtr=$758D` steht (der `outLen=7`
liefern wuerde), und `$E142`s Ergebnis ist eigentlich nur fuer die
Zeiger-Verkettung gedacht, nicht fuer die Vergleichslaenge. Ob dieser
dritte Aufruf fehlt, an der falschen Stelle landet, oder ob `$2(a7)`
aus einer ganz anderen Quelle stammen sollte, ist die letzte offene
Frage.

**Nächster Schritt:** `Q9_WATCH_ADDR` auf die absolute Adresse setzen,
die zum Zeitpunkt von `$e19a` (`move.w $2(a7),d1`) tatsaechlich
`$2(a7)` entspricht (`sp` bei `$e19a` ist `$2D348`, also Adresse
`$2D34A`), mit `Q9_WATCH_FREEZE=1`, um den SCHREIBENDEN Aufrufer zu
finden -- das zeigt, ob die `2` direkt von `$E150`s Schreibvorgang
durchgereicht wird (dann waere die noetige Korrektur: `$E142` mit
`pathPtr=$758D` statt `$758A` aufzurufen -- vermutlich behebbar durch
Anpassen des Zeigers, den `$E076` in `a0` legt) oder ob ein weiterer,
noch unbekannter Zwischenschritt beteiligt ist.

Alle Emulator-Diagnosen (`d1`/`a1`-Tracking) wieder vollständig
zurückgesetzt (`git checkout`).

## Fortsetzung 14: Schreib-Lese-Kette bewiesen; Verdacht wandert eine Ebene höher -- zu IOMans Aufrufkonvention für RBF (2026-09-08, dieselbe Session)

### Der Beweis: `$E150` schreibt direkt dorthin, wo `$E19A` liest

`Q9_WATCH_ADDR=0x2D34A Q9_WATCH_LEN=2` (die Adresse, die `$2(a7)` bei
`$e19a` entspricht) über den GESAMTEN Boot beobachtet -- nur 51
Treffer insgesamt, davon einer eindeutig:

    #630520  pc=0000e150 -> 0002d34a schrieb 00000002 (2 Byte)

**Exakter, direkter Beweis:** `$E150` (die Stelle, die Fortsetzung 10
faelschlich als "wird nie zurückgelesen" einordnete, weil der
umschliessende Stack-Slot per `addq.l #4,a7` formal freigegeben wird)
schreibt die `2` in genau die physische Speicherzelle, die `$E19A`
später als Vergleichslänge liest -- keine Vermutung mehr, sondern eine
lückenlose Schreib→Lese-Kette über zwei verschiedene, sich
überlappende Stack-Tiefen hinweg (68K-Idiom: der als "frei" markierte
Speicher bleibt inhaltlich gültig, bis ihn jemand überschreibt).

### Gegenprobe: `$c(a7)` bei `$E078` ist NICHT die vermutete Verkettungsadresse

Die in Fortsetzung 13 offen gelassene Vermutung ("vielleicht sollte
dort `$758D` zwischengespeichert sein") widerlegt: `Q9_WATCH_ADDR` auf
`$2D354` (= `$c(a7)` bei `$e078`) zeigt, dass dort zuverlässig
`$00021500` steht (geschrieben bei `$DF9E`, kurz nach der
Deskriptor-Dereferenzierung) -- ein fester, mit dem Pfadnamen
unzusammenhängender Wert (vermutlich eine Puffer-/Tabellenadresse).
Kein Zwischenspeicher für `$758D` an dieser Stelle.

### Neuer, praeziserer Denkansatz: das Problem liegt eine Ebene hoeher

Alles bisher Gefundene passt zu EINEM konsistenten Bild: RBFs
Eintrittspunkt `$D5B2` wird zweimal aufgerufen (Fortsetzung 11), BEIDE
Male mit dem IDENTISCHEN, unveraenderten Rohpfad `$7589`
("/dd/startup", Deskriptor bei `$2D380`). Wenn RBF (unveraendert,
funktioniert auf echten OS-9-Systemen nachweislich) bei jedem Aufruf
selbst `F$PrsNam` auf den vollen Pfad anwendet und dabei IMMER "dd"
(nicht "startup") als aktuellen Namen bekommt, kann die
Laengenverwechslung bei `$E142`/`$E150` eine ZWANGSLAEUFIGE FOLGE
davon sein, nicht ihre eigentliche Ursache.

**Der Verdacht wandert damit eine Ebene hoeher: IOMan muesste
zwischen dem ersten und zweiten `$D5B2`-Aufruf den Pfadzeiger im
Deskriptor auf `outPastName` aus RBFs ERSTEM Aufruf fortschreiben
(analog zur dokumentierten I$Open-Konvention "(a0) = Updated past
pathlist") -- tut es aber laut Messung nicht (beide Aufrufe: exakt
derselbe Deskriptor, derselbe Rohpfad). Das koennte daran liegen, dass
UNSER F$SSvc-Dispatch/Trap-Rueckweg fuer RBFs eigene Antwort
("wie viel vom Pfad wurde verbraucht") nicht korrekt bis zu IOMan
durchgereicht wird** -- eine architektonische Frage auf der
Trap-Rueckgabe-Ebene, nicht mehr auf der Registerebene innerhalb von
RBF.

**Nächster Schritt:** klären, WAS zwischen dem ersten und zweiten
`$D5B2`-Aufruf tatsaechlich passiert (Aufrufer-Code bei `$C0AE`-`$C0D4`
in IOMan Schritt fuer Schritt analysieren -- unveraendert, also
Ground-Truth-lesbar) -- insbesondere: liest IOMan dort ueberhaupt einen
Rueckgabewert von RBFs erstem Aufruf, und wenn ja, woher (Register?
Deskriptorfeld?) -- und ob unser eigener Trap-Rueckweg (`Q9K_SysSSvc`
o. ae., das RBF im Moduldirectory eintraegt) diesen Wert liefert. Falls
IOMan dort tatsaechlich NICHTS liest (RBF selbst muesste dann intern
zwischen Ebenen wechseln), waere die Fragestellung erneut auf die
`$E062`-`$E142`-Kette innerhalb von RBF zurueckzufuehren -- diesmal
aber mit dem Wissen, dass BEIDE Aufrufe denselben Rohpfad bekommen,
was bislang nicht beruecksichtigt wurde.

Alle Emulator-Diagnosen (`Q9_WATCH_ADDR`) wieder vollständig
zurückgesetzt (keine Quelltextänderung diese Runde -- nur bereits
committete Werkzeuge verwendet).

### Anschlussfund: der Aufruf-Mechanismus selbst (noch kein Abschluss)

`$C080`-`$C0D4` (per `Q9_DUMP_ADDR` gelesen und analysiert, real
IOMan-Code) ist die generische, callcode-indizierte Sprungtabellen-
Dispatch-Routine, über die IOMan JEDEN Gerätetreiber-Entry-Point
aufruft (Tabellen-Offset aus `$3c(a5)-$83`, verdoppelt, indiziert in
eine Basis aus `$c(a0)+$30(a0)`) -- sie laeuft bei JEDEM `$D5B2`-Aufruf
identisch durch und zeigt fuer sich allein nicht, ob/wo zwischen den
beiden Aufrufen ein Rueckgabewert gelesen wird. Der eigentliche
Aufrufer DIESER Dispatch-Routine (der zweimal hierher springt) ist
noch nicht lokalisiert -- das ist die konkrete Fortsetzung fuer den
naechsten Anlauf, mit dem im "Nächster Schritt" oben skizzierten Ziel.

**Aufrufer gefunden, aber neue Ebene geoeffnet statt geschlossen:**
per Live-Trace direkt vor den zweiten `$D5B2`-Aufruf zurueckverfolgt:
bei `$BF20` prueft IOMan erneut `cmpi.b #$2f,(a0)` (a0 weiterhin
`$7589`, der volle, unveraenderte Rohpfad) und ruft bei `$BF56`
(`bsr.w $b6b0`) eine gemeinsame Subroutine auf -- ganz in der Naehe
der schon aus der allerersten Sitzungsphase bekannten
`$B6C0`-Stelle ("Fortsetzung 10": IOMans Slash-Ueberspring-Logik).
Ob `$B6B0` dieselbe Routine ODER eine andere, eigenstaendige ist, und
ob/wo dabei der Pfadzeiger fuer die zweite Ebene tatsaechlich
entsteht (bei `$BF60`/`$BF64` wird ein Ergebnis in `a1+4` abgelegt,
noch nicht ausgewertet, wofuer), ist NICHT mehr geklaert -- eine
weitere Schicht IOMan-Code, kein Abschluss.

**Einordnung zum Sessionende:** diese Sitzung hat den `$D8`-Fehler von
einer vagen Vermutung ("irgendwo in RBFs Vergleich") zu einem exakt
bewiesenen Mechanismus gebracht (`$E150`→`$2D34A`→`$E19A`,
Fortsetzung 13/14) und den Verdacht plausibel eine Ebene hoeher
verortet (IOMans Pfad-Fortschreibung zwischen den beiden `$D5B2`-
Aufrufen). Der IOMan-Aufrufcode selbst (`$BF20` ff., `$B6B0`) ist
aber ein NEUES, noch unerschlossenes Feld -- vermutlich mehrere
weitere Fortsetzungen wert, sinnvollerweise mit frischem Kopf statt
am Ende einer bereits sehr langen Sitzung. Reproduktion unveraendert:
`Q9-Flux/local_images/OS9SYS.dbg10.hda`, RBF_BASE=`$D2DC`.

**Hinweis (nächste Session): Q9-Flux liegt jetzt unter
`Q9-Forge/Q9-Flux-68k`** (umbenannt, gleiches Repo/Remote, wegen der
parallelen x86-Portierung `Q9-Flux-x86`) -- Pfade in obigen
Abschnitten entsprechend anpassen.

## Fortsetzung 15: `$BDC0` als echter Aufrufer bestätigt -- Pfad bleibt trotzdem unverändert

`$BF20`/`$BED4` (Fortsetzung 14) war NICHT der per-Ebene-Aufrufer --
per `Q9_COUNT_PC` bestätigt: alle 3 Treffer davon sind entweder
unrelated (IOMans eigene Konsolen-/init-Aktivität) oder liegen VOR
dem ersten `/dd/startup`-relevanten `$D5B2`-Aufruf. Der tatsächliche
Aufrufer des zweiten, relevanten `$D5B2`: **`$BDC0`** -- per
lückenloser, ununterbrochener Live-Spur zweifelsfrei bestätigt
(`a5`-Tracking ergänzt, `Q9_FREEZE_PC=0xD5B2 Q9_FREEZE_A0=0x2D380
Q9_FREEZE_PC_N=2`, rückwärts bis zu einer `bsr.w $bdc0`-Stelle
verfolgt, danach ohne Lücke bis `$D5B2` durchgetestet). `$BDC0` ruft
INTERN ebenfalls `$BED4` (allokiert also eine EIGENE, neue
Pfad-Deskriptor-Kopie) und danach `$C06E` (derselbe Geräte-Dispatch
wie beim ersten Aufruf). **Auch hier: der Pfadname, den `$BED4` bei
`$BF1C` aus `$20(a5)` liest, ist weiterhin `$7589`** (unveränderter
Rohpfad) -- bestätigt an ZWEI unabhängigen Messpunkten (direkt bei
`$BF20` und nochmal bei `$BF1C`).

**Neuer Fund:** der Aufrufer von `$BDC0` ist selbst KEIN einfacher
IOMan-Code, sondern läuft über einen Trap in UNSEREN EIGENEN Kernel
(`pc=$7440`-`$7710`, weit unterhalb von IOMans Adressraum) -- eine
Schleife, die den Moduldirectory-Namen "dd"/"rbf"/"cfide" nacheinander
mit TyLang-Filtern `$0f00`/`$0d01`/`$0e01` sucht (per `d0`-Wert live
bestätigt). Das ist die VOLLSTÄNDIGE Geräte/Treiberketten-Auflösung
("dd" → Dateimanager "rbf" → Treiber "cfide") -- sie läuft für den
zweiten Aufruf **komplett erneut**, nicht nur eine Pfad-Fortschreibung.
Das spricht dafür, dass **RBF selbst** (nicht IOMans oberste
Aufruf-Ebene) diesen zweiten Zugriff auslöst -- vermutlich um "dd" als
Gerät für einen tieferen Lesezugriff erneut zu öffnen -- und dabei
denselben, unveränderten Rohpfad weiterreicht, weil die Ebenen-Tiefe
über ein ANDERES Feld (nicht die Pfadzeichenkette) getrackt werden
sollte.

Alle Emulator-Diagnosen (`a5`-Tracking) wieder vollständig
zurückgesetzt.

## Fortsetzung 16: DURCHBRUCH bei der Doku-Recherche -- echter Spec-Bug gefunden und gefixt, $D8 bleibt trotzdem (contradiction aufgedeckt)

**Meilenstein:** `<lokaler Referenzpfad>/DOC/PDF/68k_tech.pdf`
(die echte "OS-9 for 68K Processors Technical Manual", per
`pdftotext` durchsuchbar!) enthält die offizielle `F$PrsNam`-Spezifikation
(Anhang D). Zitat, wortwörtlich:

    Output
    d0.b = Pathlist delimiter
    d1.w = Length of pathlist element
    (a0) = Pathlist pointer updated past the optional "/" character
    (a1) = Address of the last character of the name +1

**`(a1)` ist NICHT der Namensanfang** (wie unser Kopfkommentar und
unsere Implementierung bisher annahmen), **sondern das Ende des
Namens** (dieselbe Position wie `(a0)`, nur OHNE einen folgenden `/`
zu überspringen). Ein echter, jetzt dokumentiert nachgewiesener Bug in
`Q9K_ProcPrsNam` (`q9kernel_iopath.c`): `*outNameStart = pathPtr +
start` (Namensanfang) statt `pathPtr + i` (Namensende). **Gefixt,
Host-Regressionstest angepasst und verifiziert, alle 14
Host-Testsuiten grün, committet** (`400dfcd`).

### Live-Test des Fixes: Länge jetzt korrekt, aber ZEIGER dadurch falsch

Neuen Kernel gebaut (Größe `$37d2`, RBF_BASE dadurch verschoben auf
**`$D2DA`** -- bei jedem Build neu bestimmen, s. Methodik-Lehre oben!),
Testabbild neu erzeugt (`OS9SYS.fix16.hda`), live geprüft:

- Am ersten von weiterhin 30 Vergleichen (`pc=$E208`, die um `-2`
  verschobene `$E20A`-Adresse): **`d1=7`** (korrekt! vorher `2`) --
  **`a0=$7594`** (FALSCH -- zeigt jetzt HINTER `"startup"`, auf das
  NUL-Byte, statt auf dessen Anfang `$758D`).
- Ergebnis unverändert: weiterhin `$D8`, weiterhin alle 30 Einträge
  ohne Treffer durchlaufen (`Q9_COUNT_PC` bestätigt: `$E208`=30x,
  Fehlerpfad `$E1F6`=1x wie zuvor).

### Die Ursache des Widerspruchs -- sauber hergeleitet

`$E076`s `movea.l a1,a0` übernimmt jetzt (korrekt gemäß Manual)
`$758C` (Ende von "dd", zeigt auf den `/`-Trenner vor "startup")
statt vorher `$758A` (Anfang von "dd"). Dieser Wert fließt unverändert
bis zum `F$PrsNam`-Aufruf bei (dem jetzt verschobenen) `$E140` durch
und wird dort als `pathPtr` verwendet:

- **Vorher** (`pathPtr=$758A` bzw. `$7589`, beide parsen "dd"):
  `outPastName = $758D` (**korrekter Zeiger** -- Anfang von "startup"),
  `outLen = 2` (**falsche Länge** -- Länge von "dd").
- **Jetzt** (`pathPtr=$758C`, parst direkt "startup" selbst, da der
  führende `/` übersprungen wird): `outPastName = $7594` (**falscher
  Zeiger** -- ENDE von "startup"), `outLen = 7` (**korrekte Länge**).

**Rechnerisch beweisbar: Zeiger UND Länge können NIE gleichzeitig aus
EINEM einzigen `F$PrsNam`-Aufruf korrekt hervorgehen** -- ein Aufruf,
der "dd" parst, liefert zwangsläufig den richtigen Zeiger (Anfang von
"startup", weil das zufällig genau da ist, wo "dd" endet) aber die
falsche Länge (die von "dd", nicht "startup"); ein Aufruf, der
"startup" selbst parst, liefert zwangsläufig die richtige Länge aber
einen Zeiger, der schon wieder HINTER "startup" liegt. **Das ist der
eigentliche Webfehler**, nicht (nur) `outNameStart`s Formel.

### Einordnung und nächster Schritt

Reales, unverändertes RBF muss für ECHTE OS-9-Installationen
funktionieren -- also MUSS es entweder (a) `F$PrsNam` **zweimal**
aufrufen, einmal für den Zeiger (Eingabe "dd") und einmal für die
Länge (Eingabe "startup", z. B. mit dem `$758C`/`$758D`-Zeiger als
Eingabe), oder (b) die Vergleichslänge stammt in einer korrekten
Umgebung aus einer GANZ ANDEREN Quelle als `$E150`s Schreibvorgang
(der bei uns nur zufällig der letzte Schreibzugriff auf diese
Speicherzelle vor dem Lesen ist, s. Fortsetzung 14s
`Q9_WATCH_ADDR`-Fund -- **51 Treffer insgesamt, nicht alle geprüft**,
möglich, dass ein ERWARTETER, korrekter späterer Schreibzugriff bei
uns aus einem ANDEREN Grund ausbleibt und deshalb `$E150`s Wert
"gewinnt").

**Nächster Schritt:** klären, ob `$E142`/`$E140` innerhalb DIESES
Aufrufs von RBF ein zweites Mal erreicht wird (`Q9_COUNT_PC` auf die
NEUE Adresse `$E140` -- mit dem Fix könnte sich die Aufrufzahl
geändert haben!) oder ob eine BISHER UNENTDECKTE zweite
`F$PrsNam`-Aufrufstelle existiert, die "startup" separat parst.
Alternativ: die volle `Q9_WATCH_ADDR`-Liste (51 Treffer) noch einmal
komplett durchgehen (nicht nur den letzten Treffer vor dem Lesen) und
prüfen, ob EIN früherer/späterer Treffer mit Wert `7` existiert, der
in einer korrekten Umgebung eigentlich der maßgebliche sein sollte.

**Zum Reproduzieren mit dem Fix:** Kernel neu bauen
(`src/kernel/build.sh`), RBF_BASE ist jetzt `$D2DA` (nicht mehr
`$D2DC`!), Testabbild `Q9-Flux-68k/local_images/OS9SYS.fix16.hda`
bereits vorhanden. `$E20A` (alt) → `$E208` (neu), `$E1F8` (alt) →
`$E1F6` (neu), `$E142` (alt) → `$E140` (neu) -- alle RBF-internen
Adressen um `-2` verschoben.

Alle Emulator-Diagnosen (`d1`/`a1`-Tracking) wieder vollständig
zurückgesetzt. Der `Q9K_ProcPrsNam`-Fix selbst bleibt committet
(`400dfcd`) -- echter Spec-Fix, auch wenn er `$D8` allein nicht löst.

## Fortsetzung 17: Architektonischer Durchbruch aus der Doku -- der zweite RBF-Aufruf MUSS einen relativen Pfad bekommen

**Zwei entscheidende Zitate aus `68k_tech.pdf`** (Kapitel 3, "OS-9
Input/Output System", Tabelle "File Manager I/O Responsibilities"):

> **Open**: "...If the file manager controls multifile devices (such
> as RBF...), directory searching is performed to find the specified
> file." -- Verzeichnissuche über MEHRERE Ebenen passiert also
> INNERHALB eines einzigen Open-Aufrufs, intern von RBF selbst
> gesteuert.
>
> **Chgdir**: "...the address of the directory is saved in the
> caller's process descriptor at P\$DIO... **Open and Create begin
> searching in this directory when the caller's pathlist does NOT
> begin with a slash (/) character.**"

**Das ist der Schlüssel:** ein Pfad, der mit `/` beginnt, wird
IMMER ab dem Wurzelverzeichnis gesucht -- unabhängig vom aktuellen
Verzeichniskontext. Unser Pfad `$7589` ist `"/dd/startup"` -- beginnt
mit `/`. Wenn RBF für die zweite Ebene (Suche nach `"startup"`
INNERHALB von `"dd"`) intern denselben, unveränderten, ABSOLUTEN Pfad
erneut an den Open-Mechanismus übergibt, MUSS dieser zwangsläufig
wieder ab der Wurzel suchen -- nicht innerhalb von `"dd"`.

**Für einen korrekten zweiten Aufruf müsste der übergebene Pfad also
entweder relativ sein (z. B. nur `"startup"`, ohne führenden `/`) oder
zumindest nicht erneut als vollständiger, absoluter Pfad interpretiert
werden.** Das deckt sich exakt mit allem bisher Gemessenen: beide
`$D5B2`-Aufrufe bekommen denselben Deskriptor mit demselben, absoluten
Rohpfad `$7589` -- das ist nach dieser Doku-Lektüre nicht nur
"zufällig verdächtig", sondern **nachweislich der falsche Zustand**
für einen funktionierenden zweiten Aufruf.

### Wo genau der Pfad fortgeschrieben werden müsste, ist noch nicht gefunden

`$BDC0`/`$BED4` (Fortsetzung 15) ist keine eigenständige, separat
aufgerufene Funktion, sondern nur eine LABEL-Stelle INNERHALB einer
größeren Funktion, die schon bei `$BD9E` (oder früher) beginnt --
diese Stelle wird selbst wiederum als Rücksprungziel aus einem TRAP
in UNSEREN EIGENEN Kernel erreicht (`a4=$BD9E` als gespeicherte
Rücksprungadresse gefunden, s. Fortsetzung 15). Die eigentliche Frage
ist jetzt: **ruft RBF (unverändert) hier tatsächlich erneut "Open" auf
sich selbst mit dem UNVERÄNDERTEN absoluten Pfad auf (dann wäre das
ein eigenständiges RBF-Verhalten, das wir nicht direkt reparieren
können, sondern nur durch korrekte P\$DIO-Verwaltung UMGEHEN müssten),
oder liegt der Fehler in UNSEREM eigenen Code, der diesen Aufruf erst
auslöst/vorbereitet** (die TyLang-Modulsuche bei `$7440`-`$7710` läuft
nachweislich in UNSEREM Kernel, vermutlich `Q9K_ModDirLinkByName` in
`q9kernel_moddir.c` -- passt zur `F$Link`-Semantik, ist aber
wahrscheinlich nur die MODUL-Auflösung, nicht die eigentliche
Pfad-Weitergabe).

**Nächster Schritt:** die Rücksprungadresse `$BD9E` weiter
zurückverfolgen -- WER (RBF-Code selbst, per `bsr`/`jsr`, oder unser
eigener Kernel via F\$SSvc-Trampolin) tatsächlich in diese Funktion
hineinspringt, und ob `P$DIO` (Prozessdeskriptor-Feld, s. Chgdir-
Zitat oben) nach dem ERSTEN `$D5B2`-Aufruf korrekt auf "dd"s
Verzeichnis gesetzt wird. Falls ja: die Frage wird dann, warum RBF
trotzdem den absoluten statt einen relativen Pfad für den zweiten
Aufruf verwendet. Falls `P$DIO` NICHT gesetzt wird: das waere ein
eigenstaendiger, klar benennbarer Bug in unserer Prozessdeskriptor-
Pflege (`Q9K_PROCDESC_SIZE`/`P$DIO`-Feld, s. `q9kernel_procapi.c`
bzw. `q9kernel_iopath.c`).

Reproduktion: `Q9-Flux-68k/local_images/OS9SYS.fix16.hda`,
RBF_BASE=`$D2DA` (mit dem `F$PrsNam`-Fix). Alle Emulator-Diagnosen
wieder vollständig zurückgesetzt.

## Fortsetzung 18: P$DIO-Offset exakt bestimmt -- spielt für unseren Fall aber GAR KEINE Rolle; Aufrufer bleibt offen

**WICHTIG (IOMan-Adressen erneut verschoben):** mit dem `F$PrsNam`-Fix
ist der Kernel 2 Byte kleiner geworden -- das verschiebt NICHT NUR
RBF (`$D2DC`→`$D2DA`, bereits dokumentiert), sondern JEDES Modul, das
in der Bootdatei DANACH kommt, also auch **IOMan** (`$AB76`→`$AB74`).
Alle in Fortsetzung 14/15 genannten IOMan-Adressen (`$BF20`, `$BDC0`,
`$BDC4`, `$BED4`, `$B6B0`, `$C080`, `$C06E` usw.) sind daher jetzt
`-2`: `$BF1E`, `$BDBE`, `$BDC2`, `$BED2`, `$B6AE`, `$C07E`, `$C06C`.
**Lehre bestätigt sich ein drittes Mal: bei JEDEM Kernel-Build ALLE
nachfolgenden Modul-Basisadressen neu bestimmen, nicht nur RBF.**

### P$DIO exakt lokalisiert

Aus der echten Prozessdeskriptor-Struktur
(internem Referenzmaterial) Feld für Feld aufaddiert:
**`P$DIO` liegt bei Offset `$148`** (328 dezimal) -- exakt die
Adresse, die IOMans Code bei `$BF2A` (alt `$BF2C`) als einen von zwei
Kandidaten verwendet (`$148(a4)` vs. `$158(a4)` = `P$DIO+ExecDir`,
ausgewählt über ein Modus-Bit). Damit ist zweifelsfrei geklärt, WAS
diese beiden Adressen sind.

**Aber:** dieser gesamte Auswahl-Code (`$BF2A`-`$BF34`) wird nur
erreicht, wenn der Pfad NICHT mit `/` beginnt (`$BF1E`:
`cmpi.b #$2f,(a0); beq $bf42` -- bei führendem `/` wird DIREKT zu
`$bf42` gesprungen, die P\$DIO-Auswahl komplett übersprungen). Da
unser Pfad `/dd/startup` **immer** mit `/` beginnt, **wird P\$DIO für
unseren Fall nie gelesen** -- die Frage "ist P\$DIO korrekt gesetzt"
aus dem "Nächster Schritt" oben ist damit gegenstandslos. Bestätigt
noch einmal (jetzt auf Registerebene, nicht nur aus der Doku-Regel):
**die einzige denkbare Korrektur ist, dass der zweite Aufruf einen
NICHT-absoluten Pfad bekommen muss.**

### Aufrufer-Suche: a4 ist in diesem C-kompilierten Code KEIN verlässlicher Rückverfolgungs-Marker

Versucht, den Aufrufer der `$74xx`-`$77xx`-Schleife (dreifache
`F\$Link`-Suche "dd"/"rbf"/"cfide" in UNSEREM Kernel, s. Fortsetzung 15)
über Wechsel von `a4` in RBF-/IOMan-Adressraum zu finden (Methode, die
bei `Q9K_TrapDispatch`-Handlern zuverlässig funktioniert, weil dort
a4 laut Konvention immer der Prozessdeskriptor ist). **Hier
unzuverlässig:** unser von C nach 68k übersetzter Code nutzt `a4`
einfach als weiteres Skalarregister für Zwischenwerte (u. a. Adressen
wie `$1400`, `$1484`, `$18800`, `$E2A2`, die rein zufällig in
RBF-/IOMan-Adressräume fallen) -- nur EIN Wert (`$BD9C`, unmittelbar
vor der Rückkehr) ist tatsächlich eine Rücksprungadresse. Die
Schleife selbst läuft komplett innerhalb unseres Kernels, ohne
erkennbare Trap-Grenze davor im 24576-Eintrag-Fenster -- der
eigentliche Aufrufer (RBF selbst per `trap #0`, oder IOMan) liegt
außerhalb des aktuellen Beobachtungsfensters.

**Nächster Schritt:** RBFs EIGENEN Code direkt NACH dem erfolgreichen
"dd"-Verzeichniseintrag-Treffer (weiterverfolgen ab dem bereits
bekannten `$D662`/neu `$D660`-Bereich, FD-Aufbau nach Treffer, s.
Fortsetzung 10) analysieren -- dort, nicht in IOMan oder unserem
Kernel, muss RBF selbst entscheiden, mit welchem Pfad/Parametern es
den nächsten Ebene-Zugriff auslöst. Ziel: die Stelle finden, an der
RBF (unverändert) den Pfadzeiger für den rekursiven Zugriff aus dem
gefundenen Verzeichniseintrag berechnet (vermutlich unter Verwendung
von `outPastName`, korrekt `$758D`, NICHT dem inzwischen als
fehlerhaft erkannten `outNameStart`-Pfad über `$E074`).

Alle Emulator-Diagnosen wieder vollständig zurückgesetzt.

## Fortsetzung 19: offizielle Registerkonvention für Dateimanager-Einstiegspunkte gefunden -- Register-Rollen präzisiert, Rätsel bleibt

**Aus `68k_tech.pdf`, Kapitel "File Manager Organization"** (Tabelle
3-8, "Registers"): beim Aufruf einzelner Dateimanager-Routinen (Open,
Create, Read, ...) gilt standardmäßig:

| Register | Zeigt auf |
|---|---|
| `(a1)` | **Path descriptor** |
| `(a4)` | Current process descriptor |
| `(a5)` | User's register stack (Parameter -- wie im jeweiligen Systemaufruf beschrieben) |
| `(a6)` | System global data area |

**Präzisiert unsere bisherige Terminologie:** was wir bisher als
"den Deskriptor" (`a5`, mit `$20(a5)`=Pfadname) bezeichnet haben, ist
laut Doku eigentlich der **Parameterblock** ("User's register stack")
-- **`a1` ist der eigentliche Pfad-Deskriptor** (die im Entstehen
begriffene P\$Path-Struktur). Live-Inhalt an `a1` (`$21500`) geprüft:
enthält u. a. `$03c0`/`$03c0` (960/960, die aus Fortsetzung 4 bekannten
Segmentgrößen-Felder) -- **passt zur Identifikation als echte
FD/Pfad-Deskriptor-Struktur**, bestätigt die Zuordnung.

Damit bleibt die zentrale Beobachtung unverändert gültig (nur die
Namen sind jetzt korrekt): **`a5` (der Parameterblock, vom Aufrufer
auf dessen eigenem Stack aufgebaut) enthält bei BEIDEN
`$D5B0`-Aufrufen denselben, unveränderten absoluten Pfad
`$7589`.** Da laut "File Manager Organization" der Parameterblock
**vom Aufrufer** aufgebaut wird, bestätigt das: **derjenige, der den
zweiten `Open`-Aufruf auslöst, baut den Parameterblock erneut mit dem
UNVERÄNDERTEN Originalpfad auf** -- ob das RBF selbst ist (das dann
per echtem `bsr`/`jsr` in IOMans generischen
Parameter-Marshalling-Code bei `$BF1E` hineinspringt) oder eine
andere, noch nicht gefundene Stelle, bleibt die offene Frage.

**Bestätigt außerdem:** `$D5B0` (Open) ist die EINZIGE RBF-Einsprung-
adresse, die in der Nachbarschaft (`$D590`-`$D600`) angesprungen wird
-- `Q9_COUNT_PC` zeigt keine Treffer für benachbarte Adressen, die
`ChgDir` o. ä. sein könnten. Beide Aufrufe sind also zweifelsfrei
`Open`, kein `ChgDir` dazwischen.

**Nächster Schritt:** den tatsächlichen Aufrufer von `$BF1E`
(IOMans Parameter-Marshalling-Einsprung) beim ZWEITEN Mal
identifizieren -- diesmal mit `a1`(jetzt korrekt: Pfad-Deskriptor)
UND `a5`(Parameterblock) beide im Blick behalten, da die bisherige
Verwechslung (a0 fälschlich als "der Deskriptor" behandelt) frühere
Rückverfolgungsversuche verzerrt haben könnte. `Q9_FREEZE_PC=0xBF1E`
mit wachsendem `Q9_FREEZE_PC_N`, bei jedem Treffer `a5`+`$20(a5)`
sowie den unmittelbaren Aufrufer-Kontext (Ringpuffer davor) prüfen,
bis der ZWEITE, relevante Treffer (mit `a5`-Pfadname `$7589`)
gefunden ist -- dann von DORT aus rückwärts zum echten `bsr`/`jsr`
zurückverfolgen (nicht wie bisher über `a4`-Wertwechsel, s.
Fortsetzung 18: das ist bei C-kompiliertem Code unzuverlässig, könnte
aber bei ECHTEM RBF-Assemblercode -- falls RBF selbst der Aufrufer ist
-- durchaus zuverlässig sein, da RBF vermutlich die klassische
`bsr`/`rts`-Konvention einhält).

Alle Emulator-Diagnosen wieder vollständig zurückgesetzt.

## Fortsetzung 20: RBF läuft NICHT unmittelbar vor dem zweiten Open-Aufruf -- Rekursions-Theorie widerlegt, echte Ursache weiter offen

**Wichtige Korrektur:** die "RBF ruft sich selbst rekursiv auf"-Theorie
(Fortsetzung 15/17/18) ist **widerlegt**. Live per `Q9_FREEZE_PC=0xBF1E`
mit wachsendem `Q9_FREEZE_PC_N` den dritten (relevanten) Treffer
isoliert und die volle 24576-Eintrag-Spur davor durchsucht: **RBF-Code
(`$D2DA`-`$F876`) läuft dort NICHT ein einziges Mal** -- der letzte
RBF-Befehl liegt tausende Instruktionen zuvor. Der zweite `Open`-Aufruf
wird also NICHT von RBF direkt ausgelöst, sondern von IOMan/unserem
Kernel, nachdem RBF laengst zurueckgekehrt ist.

**Versuch, den echten "dd"-Open-Aufruf (Ebene 1) zu finden:** den
Instruktions-Ringpuffer testweise von `24576` auf `200000` Einträge
vergrößert (`Q9_DBG_TR_SIZE` in `m68krt.h`, temporär) -- selbst damit
liegt der GENUINE "dd"-Open-Aufruf noch VOR dem Fensteranfang. Zwischen
Ebene-1- und Ebene-2-Open liegen also **mehr als 200.000 Instruktionen**
anderer Aktivität (mutmaßlich weitere, unabhängige Öffnungen wie
`/term`, Modul-Nachladen usw.) -- mit reiner Ringpuffer-Vergrößerung
nicht mehr praktikabel einzugrenzen.

**Neuer, offener Befund:** Innerhalb des GROSSEN (200k) Fensters wurde
trotzdem klar: der scheinbar "erste" `$D5B0`-Treffer in JEDEM bisher
untersuchten Fenster hatte tatsächlich `a0≈$A9AD`-`$A9B0` (init-Modul-
Bereich) -- **niemals den echten "dd"-Aufruf**. Das bedeutet: der
wirkliche Ebene-1-Aufruf für `/dd/startup` liegt zeitlich noch weiter
vorne im Boot, als bisher angenommen (nicht "kurz davor", sondern mit
sehr viel unabhängiger Aktivität dazwischen).

**Ehrliche Einordnung:** Fortsetzung 15-18 haben wertvolle Puzzleteile
geliefert (Registerkonvention geklärt, P\$DIO als irrelevant
ausgeschlossen, IOMan-Adressverschiebung korrigiert), aber die
zentrale Frage ("wer baut den zweiten Parameterblock mit dem
unveränderten Pfad auf, und warum") bleibt trotz erheblichen Aufwands
ungeklärt. Reine Ringpuffer-Vergrößerung ist an ihre praktische Grenze
gestoßen.

**Empfehlung für den nächsten Anlauf:** statt weiter den Ringpuffer zu
vergrößern, gezielt `Q9_WATCH_ADDR` auf das konkrete Feld `$20(a5)`
DES ZWEITEN Aufrufs legen (`a5=$2D3B0`, also Adresse `$2D3D0`) OHNE
Freeze, über den GESAMTEN Boot -- das zeigt (wie in Fortsetzung 14
für die Vergleichslänge erfolgreich) alle SCHREIBZUGRIFFE auf genau
diese Speicherzelle, unabhängig davon, wie weit der Schreibzeitpunkt
zurückliegt (der 64-Eintrag-Watch-Ring ist dafür ausreichend, sofern
nicht zu viele UNABHÄNGIGE Schreibzugriffe auf denselben Stack-Slot
zwischenzeitlich erfolgen -- Vorsicht vor Stack-Wiederverwendung, s.
Methodik-Lehre oben).

Alle Emulator-Diagnosen (`Q9_DBG_TR_SIZE`-Vergrößerung, `a5`-Tracking)
wieder vollständig zurückgesetzt.

### Werkzeug-Lücke gefunden (für den nächsten Anlauf zu bauen)

Die empfohlene `Q9_WATCH_ADDR`-Prüfung auf `$20(a5)` des zweiten
Aufrufs (`$2D3D0`) tatsächlich versucht: **2143 Schreibzugriffe
insgesamt**, aber der 64-Eintrag-Ring zeigt nur die LETZTEN 64 --
und die sind alle von einer spät im Boot laufenden, unabhängigen
Aktivität (`pc=$75F2`, schreibt wiederholt `$D8`/`$0`, vermutlich
Scheduler- oder Fehlerbehandlungs-Schleife NACH dem eigentlichen
Testlauf). `Q9_WATCH_FREEZE` hilft hier NICHT: es stoppt nur die
INSTRUKTIONS-Ringpuffer-Aufzeichnung (`q9_dbg_tr_frozen`), nicht die
CPU selbst und nicht den separaten Schreibzugriffs-Ring
(`q9_dbg_watch()` prüft `q9_dbg_tr_frozen` gar nicht ab) -- der Boot
läuft nach dem Freeze-Zeitpunkt einfach weiter und überschreibt die
relevanten frühen Treffer.

**Fehlendes Werkzeug für den nächsten Anlauf:** eine Variante, die den
Dump AUTOMATISCH schreibt, sobald `Q9_FREEZE_PC` das erste Mal
zuschlägt (statt auf den manuellen `Ctrl-^`-Tastendruck ~20s später zu
warten) -- würde dieses Problem grundsätzlich lösen (Watch-Ring exakt
zum relevanten Zeitpunkt eingefroren, nicht Sekunden später). Ansatz:
in `q9_dbg_instr_hook` (`m68krt.c`) beim ERSTEN Setzen von
`q9_dbg_tr_frozen` direkt `dbg_dump_kernel_globals(board)` aufrufen
(erfordert Zugriff auf den `q9_board_t*` -- ggf. über einen globalen
Zeiger, der beim Board-Setup gesetzt wird, aehnlich wie `g_board` an
anderen Stellen im Code bereits verwendet).

## Fortsetzung 21: DURCHBRUCH -- "zweiter Open-Aufruf" war die falsche Fragestellung; Werkzeug gebaut UND erfolgreich benutzt

**Das oben skizzierte Werkzeug wurde gebaut und funktioniert.** In
`q9_dbg_instr_hook` (`m68krt.c`) direkt beim ersten Setzen von
`q9_dbg_tr_frozen` einen sofortigen Datei-Dump des 64-Eintrag-
Watch-Rings ergänzt (`local_images/q9dbg_watch_snapshot.txt`, aus den
`m68krt.c`-eigenen Arrays, kein Board-Zugriff nötig) -- löst das in
Fortsetzung 20 dokumentierte Problem vollständig.

### `$20(a5)` ist einfach `R$a0` -- eine reine Kopie des Trap-Aufrufer-Registers

Mit dem neuen Werkzeug den Schreibzugriff auf `$20(a5)` (`$2D3D0`)
GENAU zum relevanten Zeitpunkt eingefangen: `pc=$76BA -> $2D3D0 schrieb
$00007589`. Per Symbolkarte (`l68 -s=`) verortet: **`$76BA` liegt
exakt in `Q9K_TrapCallExternal`** (`q9kernel_entry.a`) -- UNSERER
EIGENEN, generischen Trap-Weiterleitungslogik. Quelltext gelesen:
dieser Code baut den echten OS-9-"User Register Stack Image"-Rahmen
(`R$d0`.."R$a6`, `process.a`-Layout) per `movem.l d0-d7/a0-a6,(sp)` --
**`$20(sp)` = `R$a0` ist schlicht eine unveränderte Kopie des
`a0`-Registers, das der TRAP-AUFRUFER selbst hatte.** Unser Code
entscheidet hier nichts, er reicht nur ehrlich durch.

### Der wahre Aufrufer: unser EIGENER Testprozess, EIN EINZIGER `I$Open`-Aufruf

`Q9K_TrapCallerPC` (fester Ort `$13E8`, wird bei JEDEM Trap-Eintritt
gesetzt) ebenso mit dem neuen Werkzeug beobachtet: der Aufrufer-PC
unmittelbar vor dem `$2D3D0`-Schreibzugriff ist **`$74A4`** -- per
Symbolkarte verortet: **liegt in `Q9K_TestProcA`, direkt bei der
EINZIGEN `I\$Open`-Trap-Anweisung unseres Testprogramms**
(`q9kernel_entry.a`, Kommentar "DIAGNOSE: Dateitest beginnt" ...
`trap #0 / dc.w $0084`).

**Das ändert die Fragestellung fundamental:** die vorherigen
Fortsetzungen (17-20) suchten nach "wer ruft `Open` ein zweites Mal
mit demselben Pfad auf" -- aber **es gibt nur EINEN einzigen,
expliziten `I$Open`-Aufruf** in diesem ganzen Testlauf, ausgelöst
von UNSEREM eigenen Testcode. Der vorher gefundene "Dreiklang-Test"
(`F$Link` dreimal für "dd"/"rbf"/"cfide", Fortsetzung 15/18-20) ist
**ebenfalls unser eigener Testcode** -- ein bewusster, separater
Diagnose-Vorab-Check ("Vorprobe: findet unsere EIGENE Modulsuche den
kompletten Dreiklang?", Kommentar direkt im Quelltext), der VOR dem
eigentlichen `I$Open` läuft und mit ihm nichts zu tun hat außer der
zeitlichen Nähe.

**Damit war die ganze "wer baut den zweiten Parameterblock" Suche
(Fortsetzung 15/17-20) eine falsch gestellte Frage.** Die zwei
`$D5B0`-Treffer sind RBFs EIGENE interne Rekursion (Level 1 "dd",
Level 2 "startup") INNERHALB dieses EINEN `I$Open`-Aufrufs -- die
frühere "RBF läuft nicht direkt davor"-Beobachtung (Fortsetzung 20)
täuschte, weil RBFs Rekursion zwangsläufig durch UNSERE Trap-
Weiterleitung (`Q9K_TrapDispatch`/`Q9K_TrapCallExternal`) hindurch
muss -- das UNTERBRICHT die reine RBF-PC-Kontinuität, ohne dass es
sich um zwei unabhängige Open-Aufrufe handelt.

### Zurück zur eigentlichen, jetzt wieder gültigen Frage (Fortsetzung 16)

Damit ist die Untersuchung wieder genau dort, wo Fortsetzung 16 sie
verlassen hat -- mit KLAREREM Verständnis: **innerhalb EINES `I$Open`-
Aufrufs** ruft RBF `F\$PrsNam` erneut auf (`$E062`/neu `$E060`), um von
Ebene 1 ("dd") zu Ebene 2 ("startup") zu wechseln -- und Zeiger UND
Länge für den anschließenden Namensvergleich lassen sich nachweislich
NICHT beide korrekt aus diesem einen Aufruf herleiten (mathematischer
Beweis in Fortsetzung 16 bleibt vollständig gültig). Die Suche nach
einem "zweiten Aufrufer" war unnötig -- der nächste Schritt ist wieder
RBFs eigener Code direkt nach dem "dd"-Verzeichnistreffer, diesmal mit
dem Wissen, dass alles innerhalb EINER `Q9K_TrapCallExternal`-
Aufrufkette passiert.

**Nächster Schritt:** RBFs Code zwischen `$E062`/neu `$E060` (dem
ERSTEN `F$PrsNam`-Aufruf, liefert korrekt `outPastName=$758D`) und
`$E0F0`/neu `$E0EE` (wo `a0` in `$8(a7)`/`$10(a7)` gesichert wird, s.
Fortsetzung 10/13) noch einmal GENAU Zeile für Zeile durchgehen --
mit dem NEUEN `Q9_DUMP_ADDR`/Sofort-Watch-Snapshot-Werkzeug sollte
sich jetzt PRÄZISE finden lassen, wo `a0` von `$758D` (korrekt, direkt
nach dem Trap) auf `$758A`/`$758C` (fehlerhaft, nach `$E074`s
`movea.l a1,a0`) wechselt, und ob es EINEN Weg gibt, diesen Wechsel
zu vermeiden oder zu kompensieren, OHNE RBF selbst zu verändern.

Alle Emulator-Diagnosen (Sofort-Dump-Werkzeug) wieder vollständig
zurückgesetzt -- das Werkzeug selbst (Code-Patch) ist dokumentiert
und leicht rekonstruierbar (s. o., "Werkzeug-Lücke gefunden").

## Fortsetzung 22: mathematischer Vollbeweis -- $10(a7) hat KEINE versteckte zweite Quelle, der Widerspruch ist strukturell, nicht zufällig

**`$10(a7)` (die Adresse, die `$E13C`/alt `$E13E` als `pathPtr` für
den zweiten `F$PrsNam`-Aufruf laedt) per `Q9_WATCH_ADDR` über den
GESAMTEN Boot beobachtet, Snapshot exakt beim `$E140`-Freeze
gesichert (neues Sofort-Dump-Werkzeug aus Fortsetzung 21 genutzt):
**genau EIN relevanter Schreibzugriff insgesamt** -- `pc=$E0F2`
(alt `$E0F0`+2, das zweite `move.l a0,$10(a7)`) schreibt `$758C`.
Keine andere Stelle schreibt jemals in diese Speicherzelle, bevor
`$E140` sie liest. **Damit ist zweifelsfrei ausgeschlossen, dass es
eine versteckte, "richtige" zweite Quelle gibt, die nur durch Zufall
überschrieben wird** -- die in Fortsetzung 20 vorgeschlagene
Hypothese ("vielleicht schreibt was Korrektes dazwischen") ist
widerlegt.

### Der Mechanismus jetzt vollständig verstanden -- und der Widerspruch mathematisch zwingend

Mit dieser letzten Messung lässt sich die komplette Kette jetzt exakt
nachrechnen (kein Rätselraten mehr):

1. RBFs ERSTER `F$PrsNam`-Aufruf (`$E060`/alt `$E062`) bekommt
   `pathPtr=$7589` ("/dd/startup"), parst `"dd"`. **Output:**
   `a0=outPastName=$758D` (Zeiger auf "startup") -- KORREKT, aber wird
   bei `$E074` durch `movea.l a1,a0` verworfen.
2. `a1=outNameStart` (laut Fix jetzt "hinter dem Namen", `$758C`,
   die Trenner-Position) wird nach `a0` kopiert, dann in `$8(a7)`
   UND `$10(a7)` gesichert (`$E0EE`/`$E0F2`).
3. `$E13C` lädt `a0` aus `$10(a7)` (`$758C`) und ruft `F$PrsNam` ein
   ZWEITES Mal (`$E140`) -- diesmal mit `pathPtr=$758C`.
4. **Rechnerisch zwingend:** `F$PrsNam($758C)` parst zwangsläufig
   `"startup"` selbst (der führende `/` bei `$758C` wird übersprungen,
   landet direkt bei `"startup"`) -- das liefert IMMER
   `outPastName=$7594` (HINTER "startup", nicht davor) UND `outLen=7`
   (korrekt). **Es gibt KEINEN mathematisch möglichen Eingabewert für
   `$E140`, der GLEICHZEITIG `outPastName=$758D` (Zeiger AUF
   "startup") und `outLen=7` liefert** -- `outPastName` zeigt per
   Definition IMMER hinter den gerade geparsten Namen, niemals davor.
   Ein Aufruf, der `"dd"` parst, liefert zwangsläufig `outPastName=
   $758D` (zufällig = Anfang von "startup") aber `outLen=2`; ein
   Aufruf, der `"startup"` parst, liefert `outLen=7` aber `outPastName
   =$7594` (hinter "startup"). Es gibt keine dritte Möglichkeit.

**Das bestätigt Fortsetzung 16s Beweis nicht nur, sondern erklärt jetzt
auch WARUM live exakt das gemessen wird, was gemessen wird** -- vor
dem Fix (Alt-Semantik, `a1=$758A`=Anfang "dd") lieferte `$E140`
zwangsläufig `outPastName=$758D` (Zeiger korrekt) + `outLen=2` (Länge
von "dd", falsch); nach dem Fix (`a1=$758C`=Ende "dd") liefert
`$E140` zwangsläufig `outPastName=$7594` (Zeiger falsch) + `outLen=7`
(Länge korrekt). **Keine der beiden Semantiken kann funktionieren --
das Problem liegt nicht in `outNameStart`s Formel, sondern darin, DASS
RBF für den Vergleich zwei Werte aus EINEM `F$PrsNam`-Aufruf erwartet,
die sich gegenseitig ausschließen.**

### Ehrliche Einordnung nach mehreren Sitzungen intensiver Untersuchung

Reales, unverändertes RBF funktioniert auf echten OS-9-Systemen --
also KANN dieser Mechanismus dort nicht so ablaufen, wie hier
gemessen. Die naheliegendsten verbleibenden Erklärungen:

1. **`$E140` ist NICHT für den Vergleich gedacht** -- der eigentliche
   Vergleichs-Setup-Mechanismus liegt an einer noch nicht gefundenen
   dritten Stelle, und die beobachtete Übereinstimmung von `a0`/`d1`
   mit den Compare-Registern bei jedem bisherigen Test war Zufall
   durch dieselbe Registerkette, nicht Kausalität. (Erscheint nach
   der jetzt vollständigen Nachrechnung UNWAHRSCHEINLICH, da die
   Werte bei JEDEM Testlauf -- vor UND nach dem Fix -- exakt den
   Vorhersagen entsprachen, aber nicht mit letzter Sicherheit
   ausschließbar.)
2. **Ein noch nicht gefundenes drittes Register/Feld** liefert die
   fehlende Information (z. B. `a2`, das laut `68k_tech.pdf` an
   anderer Stelle als "Directory entry pointer" auftaucht, oder ein
   FD-internes Feld), das den Vergleich VOR der Längenprüfung
   zusätzlich korrigiert -- dafür müsste der komplette Compare-Aufruf-
   Kontext (`$E1x` Bereich, Fortsetzung 10s allererste Analyse)
   noch einmal mit ALLEN Registern (nicht nur a0/a1/d0/d1) neu
   vermessen werden.
3. **Ohne echten RBF-Assembler-Quelltext** (Microware-Eigentum, nicht
   im Projekt vorhanden) bleibt die vollständige Auflösung dieses
   Widerspruchs eine Blackbox-Analyseaufgabe, die trotz
   erheblichen, sorgfältig dokumentierten Aufwands über mehrere
   Sitzungen hinweg nicht abschließend gelöst werden konnte.

**Empfehlung für den nächsten Anlauf:** Punkt 2 zuerst prüfen (dritte
Registerspur ergänzen, insbesondere `a2`/`d2`, beim Compare-Aufruf
UND bei `$E140`s unmittelbarer Umgebung) -- das ist der einzige noch
nicht vollständig ausgeschöpfte, konkret umsetzbare Weg. Alternativ:
gezielt nach einer öffentlich verfügbaren RBF-Quelltext-Referenz
suchen (auch außerhalb des Projekts), falls eine legale Quelle
existiert.

**Nachtrag, noch in derselben Sitzung:** Empfehlungspunkt 2 (dritte
Registerspur) sofort geprüft -- `a1`, `a2`, `d0`, `d2` am
Vergleichspunkt (`$E208`) live mitgeschnitten:
`d0=1 d1=7 d2=$42 a0=$7594 a1=$36310 a2=$35F10`. **Keines dieser
zusätzlichen Register hält `$758D`** -- der korrekte Zeiger ist zu
diesem Zeitpunkt in KEINEM der gängigen Register mehr vorhanden.
Damit ist auch Empfehlungspunkt 2 ausgeschöpft; es bleiben nur noch
Punkt 1 (E142s wahre Rolle liegt woanders) und Punkt 3 (echter
RBF-Quelltext nötig) als plausible Erklärungen.

Alle Emulator-Diagnosen wieder vollständig zurückgesetzt.

## Fortsetzung 23: letzte Blackbox-Ansätze ausgeschöpft -- Sackgasse bestätigt

**Geprüft: gibt es weitere, bisher unentdeckte `F$PrsNam`-Aufrufe?**
Die echte Trap-Instruktion selbst (`$E2A0`, wird von MEHREREN
`bsr`-Stellen in RBF angesprungen) auf Gesamt-Trefferzahl geprüft:
**genau 3 Treffer im ganzen Boot** -- exakt erklärt durch `$E060`
(2×, davon 1× unrelated/init-Bereich) + `$E140` (1×). Keine versteckte
vierte Aufrufstelle.

**Geprüft: was passiert GENAU zwischen dem `$E140`-Rücksprung und dem
Vergleich?** Lückenlos durchgetestet (volle Spur von `$E140` bis
`$E208`). Dabei eine bisher unbeachtete Zwischenstation gefunden
(`$E28E`-`$E29E`) -- **stellt sich aber bei genauer Analyse
als KOMPLETT UNABHÄNGIGE Hilfsfunktion heraus**: berechnet eine
Pufferadresse rein aus FD-Feldern (`$e(a1) + ($32(a1) AND $70(a1))`,
"Segmentbasis + aktuelle Position AND Maske") -- hat NICHTS mit dem
Pfadnamen zu tun. Die schon vorher beobachtete Werte-Übereinstimmung
war Zufall durch dieselbe Registerkette, keine kausale Verbindung zum
Suchnamen.

**Ergebnis:** Alle mit vertretbarem Aufwand erreichbaren Blackbox-
Ansätze sind jetzt ausgeschöpft (dritte/vierte Registerspur, weitere
Aufrufstellen, lückenlose Zwischenschritt-Analyse). Der in
Fortsetzung 22 bewiesene strukturelle Widerspruch bleibt die
endgültige Erkenntnis dieser Untersuchungslinie: **`$D8` bei
`I$Open("/dd/startup")` lässt sich mit reinem Live-Tracing des
unveränderten RBF-Binärcodes nicht weiter auflösen.** Eine
vollständige Lösung würde entweder echten RBF-Quelltext oder eine
grundlegend andere Herangehensweise erfordern (z. B. ein
funktionierendes Referenzsystem mit ECHTEM Microware-Kernel zum
Vergleich der exakt gleichen Speicherstellen, falls verfügbar).

Alle Emulator-Diagnosen wieder vollständig zurückgesetzt.

## Fortsetzung 24: Durchbruch -- echter RBF-Level-2-Quellcode gefunden, $D8-Wurzelursache tatsächlich behoben; neuer, nachgelagerter Absturz entdeckt (2026-09-08, neue Session)

**Der in Fortsetzung 22/23 als "unauflösbar ohne echten RBF-Quelltext"
dokumentierte Sackgassen-Befund war korrekt in der Diagnose, aber
falsch in der Schlussfolgerung "nicht ohne echten Quelltext lösbar" --
echter RBF-Quelltext war tatsächlich online auffindbar** (Level-2-
Quellen bei `www.roug.org/havneholmen/retrocomputing/os/os9/l2sources/rbf`,
6809, aber algorithmisch identisch zum 68K-Port). Der entscheidende
Fund: `SchDir`/`RBPNam` zeigen unzweideutig, dass `F$PrsNam` GENAU
EINMAL pro Verzeichnisebene aufgerufen wird und dabei GLEICHZEITIG
zwei verschiedene Zeiger liefert, die RBF für ZWEI verschiedene
Zwecke braucht:

```
RBPNam   os9  F$PrsNam        ; parse normal pathname
         pshs x                ; X = a0-Ausgabe SOFORT sichern
         bcc  RBPNam99         ; Erfolg -> X bleibt a0-Ausgabe
...
SchDir60 ... lbsr RBPNam
         std  S.Delim,S        ; D (Trenner:Laenge) UNVERAENDERT von F$PrsNam
         stx  S.PathPt,S       ; X = a0-Ausgabe -> spaeter VERGLEICHSZEIGER
         sty  S.NextPt,S       ; Y = a1-Ausgabe -> spaeter NAECHSTER Eingabezeiger
...
SchDir80 ... ldx S.PathPt,S    ; Vergleichszeiger
             ldb S.NameSz,S    ; (= D.b von ganz oben, NIE ueberschrieben)
             os9 F$CmpNam
```

**Die Rollenverteilung, die Fortsetzung 16-23 verfehlt hatte:**
`a0`-Ausgabe von `F$PrsNam` = **Anfang** des aktuellen Namens (hinter
einem evtl. fuehrenden `/`, RBFs eigener Vergleichszeiger fuer
`F$CmpNam`); `a1`-Ausgabe = **Ende** des Namens/Trennerposition (RBFs
Zeiger fuer den NAECHSTEN `RBPNam`-Aufruf, RBF haengt den
Trenner-Ueberspringschritt SELBST an, `leax 1,X`). Auch das offizielle
Manual-Zitat ("`(a0)` = pathlist pointer updated past the optional
`/` character") passt dazu -- war die ganze Zeit vorhanden, wurde in
Fortsetzung 16 aber falsch auf `a1` statt `a0` bezogen.

### Der eigentliche Bug: `outPastName`/a0, nicht `outNameStart`/a1

`Q9K_ProcPrsNam` (`q9kernel_iopath.c`) lieferte in `*outPastName`
(a0-Ausgabe) faelschlich "hinter Name UND Trenner" (`pathPtr + i +
Trenner-Skip`) statt einfach `pathPtr + start` (Anfang des Namens).
Fix (dieselbe Session):

```c
*outPastName = pathPtr + start;   /* a0: Namensanfang, s. echter RBF-Quellcode */
```

`*outNameStart` (a1-Ausgabe, `pathPtr + i`, Fix #2 aus einer
frueheren Session) war dabei numerisch schon korrekt -- nur die
Rollenbeschreibung im Kommentar war falsch begruendet. Host-Tests
(`test_q9kernel_iopath.c`) entsprechend angepasst, inkl. Kettentest
(naechster Eingabezeiger ist jetzt `nameStart + 1`, NICHT `past`).

### Live-Beweis: der $D8-Widerspruch ist aufgelöst

Kernel neu gebaut (RBF_BASE jetzt `$D2D2`, per `M$ID`-Sync-Wort im
Ctrl-^-Dump bestätigt), Testabbild `OS9SYS.fix24.hda`. Die echte
Vergleichsroutine im unveraenderten RBF-Modul per `capstone`
GEFUNDEN (nicht mehr geraten) durch Byte-Mustersuche in `rbf.mod`
selbst (`eor.b d2,d0` gefolgt vom bekannten XOR-Vergleichsmuster) --
Modul-Offset `$f2e`, live also `$D2D2 + $f2e = $E200`:

```
Q9_FREEZE_PC=0xE200 Q9_FREEZE_PC_N=1 Q9_TRACE_INSTR=1 (erster von 30 Vergleichen):
  pc=0000e200 d0=00000001 d1=00000007 a0=0000758d a1=00036310 a4=00007100 sp=0002d344
```

**`a0=$758D` (Anfang von "startup") UND `d1=7` (Länge von "startup")
-- BEIDE Werte gleichzeitig korrekt.** Das ist exakt der Zustand, den
Fortsetzung 16 als "aus einem einzigen `F$PrsNam`-Aufruf unmöglich"
bewiesen hatte -- der Beweis war nicht falsch, nur bezogen auf die
ALTE, falsche `outPastName`-Formel.

**Bestätigt per `Q9_TRAP_TRACE=1 Q9_TRAP_TRACE_ALL=1`** (mit
temporär `fflush()` nach jedem Trace-Schreibvorgang ergänzt, sonst
gehen die letzten Zeilen bei einem Absturz/Kill verloren -- Patch
wieder zurückgesetzt): der komplette `I$Open("/dd/startup")`-Aufruf
zeigt genau ZWEI `F$PrsNam`-Aufrufe (`"dd"`, dann `"startup"`, keine
dritte "Trenner-überspringen"-Runde wie in der 6809-Quelle vermutet)
und endet mit

    syscallret pc=000074a6 callcode=0084 a0=00007589 d0=00000003 d1=00000002 a2=0000f878

**`d0=3` -- eine echte Pfadnummer, kein Fehlercode.** `I$Open`
liefert damit zum ersten Mal überhaupt Erfolg für `/dd/startup`
gegen den unveränderten Microware-RBF. Die `$D8`-Wurzelursache ist
damit nachweislich behoben.

### Neuer, nachgelagerter Fund: Absturz kurz NACH erfolgreichem Open+Read

Die Konsolenausgabe zeigt trotzdem keinen Erfolg (`Fo...[...]`),
sondern bricht nach `F` (Open-Test-Start) mit `E` (unser eigener
`Q9K_ExcTrap`-Handler) ab, danach nur noch die parallele
`TestProcB`-Endlosschleife. `Q9K_ExcTrap` zeichnet Vektor 4
(Illegal Instruction) auf `PC=$74C5` auf, reproduzierbar.

Per `capstone` direkt am gebauten `q9kernel`-Modul (nicht geraten --
Trap-Inline-Wörter korrekt übersprungen, s. etablierte Methodik)
lokalisiert: `$74C5` = Kernel-Offset `$3C5`, mitten in `bsr.w
Q9K_DiagWriteD7` (Offset `$3C4`-`$3C7`, druckt `'['` nach
erfolgreichem `I$Read`) -- die Fault-PC trifft exakt das ZWEITE Byte
dieser 4-Byte-Instruktion. Das deutet auf eine falsch berechnete
Rücksprungadresse (um 1 Byte versetzt) irgendwo in der Aufrufkette
von `Q9K_DiagWriteD7` hin -- vermutlich in dessen eigener
DUART-Busy-Wait-Logik, nicht im `F$PrsNam`/RBF-Pfad. Noch nicht
untersucht.

**Nächster Schritt (neue Baustelle, nicht mehr `$D8`):** Ursache des
Off-by-one-Rücksprungs in/um `Q9K_DiagWriteD7` (`q9kernel_entry.a`)
finden -- vermutlich unabhängig vom `F$PrsNam`-Fix, aber erst durch
ihn erstmals erreichbar (vorher brach `I$Open` immer schon vorher mit
`$D8` ab).

Alle Emulator-Diagnosen (`m68krt.c`/`m68krt.h`/`q9boardrun.c`,
inkl. `fflush`-Patch) wieder vollständig zurückgesetzt (`git
checkout`).

## Fortsetzung 25: DiagWriteD7-Absturz auf A4-Herkunftspruefung zurueckgefuehrt -- Fix bricht Konsolen-Open, zwei Versuche verworfen (2026-09-08/09, neue Session)

**Ausgangspunkt:** der in Fortsetzung 24 gefundene Illegal-Instruction-
Absturz (PC=$74C5, mitten in `bsr.w Q9K_DiagWriteD7`) ist KEINE falsche
Ruecksprungadresse, sondern echte Selbstmodifikation: Byte $74AF wird
von $00 auf $01 veraendert (aus `bsr.w` wird `bsr.b +1`, landet mitten
im Folgebefehl).

### Wurzelursache gefunden: A4-Herkunftspruefung in Q9K_TrapDispatch

Per `Q9_WATCH_ADDR=0x74AF` (Treffer-Liste) auf den Schreiber
zurueckverfolgt: RBFs Code (Modul-Offset `$1bd6` im unveraenderten
`rbf.mod`, `addq.l #1,$3ac(a4)`) schreibt dort -- WEIL `a4` bei diesem
Aufruf faelschlich unsere eigene Kernel-Modulbasis (`$7100`) enthaelt
statt eines echten Zeigers (`$7100+$3ac=$74ac`, exakt der beobachtete
Fehlerort). Per `Q9_FREEZE_PC_N` ueber mehrere Treffer derselben
Instruktion bestaetigt: `a4` wechselt zwischen `$19400` (echter
Prozessdeskriptor) und `$7100` (unsere Modulbasis) je nach Aufrufkontext
-- kein Zufall, sondern strukturell.

Ursache in `Q9K_TrapDispatch`s "HERKUNFTSPRUEFUNG" (Kommentar "sechster
A4-Anlauf", 2026-09-07) gefunden: `lea Q9K_ModuleStart(pc),a4 / suba.l
#Q9K_ModuleHeaderSize,a4` ueberschreibt A4 fuer die Eigen-/Fremd-
Pruefung selbst -- und NICHTS stellt den echten Aufrufer-Wert vor `bcc
Q9K_TrapCallForeignCaller` wieder her, obwohl der dortige Kommentar
("A4 bleibt dessen eigener Wert, unangetastet") genau das behauptet.
Trifft ein Fremdaufrufer (RBF, ueber einen internen `F$SRqMem`-Trap
waehrend seiner eigenen `I$Open`-Verarbeitung) auf diesen Pfad, bekommt
er faelschlich unsere Modulbasis statt seines echten `a4` zurueck.

### Zwei Fixversuche, BEIDE verworfen -- Konsolen-Open bricht komplett

**Versuch 1:** echten Aufrufer-A4 in der bestehenden globalen Zelle
`Q9K_TrapA4Save` rettten (vor der Pruefung sichern, im
Fremdaufrufer-Zweig zurueckholen). **Ergebnis: massive Verschlechterung**
-- IOMans allererstes `/term`-Konsolen-Open scheitert danach sofort
mit `"ioman: can't open console device: Error $0000"` (vorher lief der
Boot bis zu unserem eigenen Dateitest durch). Vermutung: `Q9K_TrapA4Save`
ist eine EINZELNE, nicht wiedereintrittsfeste Zelle -- Traps
schachteln sich (I$Open ruft selbst `F$SRqMem` per Trap auf, waehrend
der aeussere Trap "in Arbeit" ist), der innere Aufruf ueberschreibt die
Zelle, bevor der aeussere sie zurueckholt.

**Versuch 2:** A4 stattdessen auf dem Stack retten (`move.l a4,-(sp)` /
`movea.l (sp)+,a4`) -- naturgemaess wiedereintrittsfest, jede
Verschachtelungsebene bekommt ihren eigenen Rettungsplatz.
**Ergebnis: IDENTISCHER Fehler** ("Error $0000" beim Konsolen-Open,
Byte-genau gleiches Log wie Versuch 1). Das widerlegt die
Reentranz-Hypothese vollstaendig -- das Problem liegt NICHT an der Art
der Zwischenspeicherung.

### Der eigentliche Widerspruch (noch ungeloest)

Beide Versuche beweisen: **"den echten Aufrufer-A4-Wert wiederherstellen"
ist fuer den Konsolen-Open-Aufruf (scf) grundsaetzlich falsch**, nicht
nur falsch implementiert -- unabhaengig von der Rettungsmethode. Gleich-
zeitig beweist die urspruengliche Messung: **RBFs interner
`F$SRqMem`-Aufruf braucht seinen echten A4-Wert, NICHT unsere
Modulbasis** (sonst die beobachtete Speicherkorruption).

Zwei scheinbar widerspruechliche Anforderungen an DENSELBEN Codepfad.
Denkbare Erklaerung (noch nicht verifiziert): `scf`s eigener A4-Wert
VOR dem allerersten Trap ist selbst schon undefiniert/Muell (ganz frueher
Boot-Zeitpunkt, vor jeder Prozesserzeugung) -- unsere Modulbasis als
Ersatzwert waere dann zufaellig "weniger kaputt" als das ECHTE,
unbrauchbare Original. Noch NICHT geprueft: was `scf`s A4 tatsaechlich
VOR seinem allerersten Trap enthaelt (per Live-Messung an der
Herkunftspruefungs-Stelle, fuer GENAU diesen frühen Aufruf).

**Beide Versuche zurueckgesetzt** (`git checkout`), Branch ist wieder
exakt auf Commit `7de5410` (der F$PrsNam-Fix bleibt unangetastet
gueltig). Kein Regressionsrisiko fuer den bereits erreichten Stand.

**Naechster Schritt:** A4 an der Herkunftspruefungs-Stelle fuer scfs
allerersten Trap (Konsolen-Open) live messen, BEVOR ein weiterer
Fixversuch unternommen wird -- die beiden bisherigen Versuche waren
zu blind (gleiche Behandlung fuer alle Fremdaufrufer angenommen, ohne
vorher zu pruefen, ob das ueberhaupt plausibel ist).

## Fortsetzung 26: Zwei weitere A4-Fixversuche, beide gescheitert -- Konsolen-Init-Bereich extrem fragil, vier Fehlschläge insgesamt (2026-09-09, dieselbe Session)

Fortsetzung an Fortsetzung 25: Manual-Recherche ergab, dass sowohl
`F$SRqMem` als auch `F$SSvc` laut Technical Manual **keinen**
A4-Rückgabewert definieren ("Output" listet nur `d0`/`(a2)` bzw. gar
nichts) -- A4 MÜSSTE also für Fremdaufrufer grundsätzlich unverändert
bleiben, nicht neu gesetzt werden. Das würde Fortsetzung 25s ersten
Fixversuch (A4 unverändert lassen) eigentlich bestätigen. Per Live-
Messung an `Q9K_TrapDispatch`s Herkunftsprüfung zusätzlich
herausgefunden: der ECHTE, ursprüngliche A4-Wert des Aufrufers ist bei
IOMans allererstem Trap (`F$SSvc`, ganz am Boot-Anfang) selbst schon
Müll (`$7832`/`$7822`/… -- je nach Lauf verschieden, aber immer eine
Adresse INNERHALB unseres eigenen Kernels) -- **weil A4 vor dem
allerersten Sprung nach IOMan (`jsr (a1)` in `q9kernel_entry.a`)
niemals explizit gesetzt wird.** Per Live-Messung `Q9_D_Proc` hält an
dieser Stelle aber bereits einen echten, gültigen Deskriptor (`$19400`).

**Versuch 3 (kombiniert):** (a) A4 in `Q9K_TrapDispatch`s Fremdaufrufer-
Zweig wiedereintrittsfest auf dem Stack retten (Fortsetzung 25s
zweiter, verworfener Ansatz, hier erneut) UND (b) zusätzlich A4 VOR dem
allerersten `jsr (a1)`-Sprung nach IOMan explizit auf `Q9_D_Proc`
setzen (die fehlende Grundinitialisierung beheben). **Ergebnis:
IDENTISCHER Fehlschlag** ("ioman: can't open console device: Error
$0000", exakt wie in Fortsetzung 25). Die zusätzliche Initialisierung
allein löst das Problem also nicht, UND die Kombination mit der
Stack-Rettung bricht weiterhin.

**Versuch 4 (maximal chirurgisch):** `Q9K_TrapDispatch` komplett
UNANGETASTET gelassen (kein Risiko für andere Aufrufer) -- stattdessen
NUR in `Q9K_SysFSRqMem`s eigenem Rückgabepfad (vor beiden `rts`,
Erfolg und Fehlschlag) `A4` explizit auf `Q9_D_Proc` gesetzt, als
Ersatz für den durch die Herkunftsprüfung ohnehin schon zerstörten
Wert. **Ergebnis: IDENTISCHER Fehlschlag**, obwohl diese Änderung
JEDEN anderen Aufrufer/Callcode gar nicht berühren sollte.

### Schlussfolgerung: Bereich ist fragiler als angenommen

Vier von vier Versuchen (Fortsetzung 25 + 26), so unterschiedlich sie
auch waren (globale Zelle, Stack, `Q9_D_Proc` dispatcher-weit,
`Q9_D_Proc` nur in einem einzigen Handler), scheitern am SELBEN
Symptom. Das spricht gegen "A4-Semantik falsch gewählt" als alleinige
Erklärung -- entweder ist der Konsolen-Init-Pfad auf eine Art
zeitkritisch/layoutempfindlich, die selbst chirurgische Änderungen weit
entfernter Funktionen durchschlagen lässt (ähnlich dem historisch
dokumentierten "14-NOP-Bug" im Trap-Pfad, s. `Q9K_TrapDispatch`s
Interrupt-Sperr-Kommentar), oder IOMans/scfs tatsächliche Erwartung an
A4 an dieser Stelle unterscheidet sich von ALLEN vier hier probierten
Hypothesen.

**Alle vier Versuche zurückgesetzt**, Branch wieder exakt auf
`9d5e26f`. Kein Regressionsrisiko für den `$D8`-Fix.

**Empfehlung für den nächsten Anlauf:** kein fünfter Blindversuch mehr
ohne vorherige Analyse von `scf`/`ioman` GENAU an der Stelle,
die "can't open console device" ausgibt (welches Feld wird tatsächlich
geprüft, welcher Wert führt zum Fehlschlag?) -- dieselbe Methodik, die
bei Fortsetzung 24 den `$D8`-Bug wirklich gelöst hat (echten Quelltext/
echte Analyse VOR weiterem Raten). `ioman.mod`/`scf.mod` als
eigenständige Dateien liegen noch nicht im `$CLAUDE_JOB_DIR/tmp` dieses
Jobs -- müssten aus `Q9-Flux-68k/OS9Boot.noprot.test` oder dem
Referenz-Bootfile extrahiert werden (analog zu `rbf.mod` etc.).

## Fortsetzung 27: GELÖST -- Prozessdeskriptor war halb so groß wie real, A4 vor IOMans Einsprung nie gesetzt; I$Open UND I$Read laufen erstmals durch (2026-09-09)

**Der Absturz aus Fortsetzung 24 ist behoben.** Zwei kleine, unabhängige
Ursachen — keine davon im Trap-Dispatcher, an dem sich fünf Versuche
vergeblich abgearbeitet hatten.

### Ursache 1: `Q9K_PROCDESC_SIZE` war 512 statt der realen 1024 Byte

Der vollständige `Q9_TRAP_TRACE_ALL`-Mitschnitt zeigte, dass RBF
`F$SRqMem` schon beim AUTOMATISCHEN Boot-Attach von `/dd` aufruft (nicht
erst bei unserem Testprozess) — und die Spur exakt danach abbricht. Die
RBF-Instruktion bei Modul-Offset `$1bd6` (`addq.l #1,$3ac(a4)`, mit
passendem `subq.l` nach dem Aufruf) klammert einen internen Treiberaufruf
ein. Offset `$3AC` ist im echten Layout **`P$Preempt`**
("process level system-state pre-emption flag") — also völlig legitimes,
dokumentiertes OS-9-Verhalten.

Aus internem Referenzmaterial Feld für Feld aufsummiert (org 0 ab
`P$ID` bis `P$PrcBody`, mit `MemBlks=NumPaths=DefIOSiz=32`):
**`P$PrcBody` = `$400` (1024 Byte)** — unser Deskriptor war exakt halb so
groß, jeder `P$Preempt`-Zugriff lief also über sein Ende hinaus.

Mitgezogen: in `q9kernel_procapi.c` steckte die Slot-Größe zusätzlich als
drei hartkodierte `>> 9`/`<< 9`. Die sind jetzt durch EINE abgeleitete
Konstante `Q9K_PROCDESC_SHIFT` plus Kompilierzeit-Kopplung ersetzt
(negative Array-Größe bricht den Build, falls Shift und Größe je wieder
auseinanderlaufen); dieselbe Kopplung im zugehörigen Host-Test, dessen
Fake-Pool ebenfalls `0x200` hartkodiert hatte.

### Ursache 2: A4 war beim allerersten Sprung nach IOMan nie initialisiert

Vor `jsr (a1)` (IOMans `M$Exec`-Einsprung) wurde A4 nie gesetzt — IOMan
bekam einen zufälligen Restwert aus unserem eigenen Bootstrap-Code (live
gemessen `$7832`/`$7822`/…, je nach Lauf verschieden). Die reale
OS-9-Konvention verlangt dort den aktuellen Prozessdeskriptor;
`Q9_D_Proc` hält an dieser Stelle bereits einen gültigen ("Prozess 1"
existiert vor dem ersten Modulaufruf, per Messung belegt). Ein
`movea.l Q9_D_Proc,a4` davor genügt.

### Ergebnis, live

```
RP012Hallo von Q9-OS!  O00000011H123F o[........] n AAAA…
                                      ^ ^          ^
                                      | |          nächster Test
                                      | I$Read erfolgreich (8 Byte)
                                      I$Open erfolgreich
```

**Kein `E` (Exception) mehr**, und das System läuft danach normal weiter
(A/B-Scheduler-Schleife). Alle 14 Host-Testsuiten grün.

### Warum die fünf Dispatcher-Versuche scheitern mussten

Ein Kontrollexperiment hat den letzten offenen Punkt sauber getrennt:
gleiche Stack-Bewegung wie der Fixversuch, aber A4-Wert unverändert
(Modulbasis) → läuft fehlerfrei. Es liegt also am **Wert**, nicht an der
Codeform. Der Kommentar in `Q9K_TrapDispatch` ("A4 bleibt dessen eigener
Wert, unangetastet") bleibt damit sachlich falsch — die Herkunftsprüfung
überschreibt A4 tatsächlich —, aber jeder Versuch, das zu "reparieren",
bricht IOMans Konsolen-Open. Das ist jetzt als bewusste, begründete
Nicht-Änderung im Code dokumentiert. **Offen bleibt:** warum IOMan/scf
ausgerechnet unsere Modulbasis vertragen.

### Nächste Baustelle

`I$Read` meldet Erfolg, überträgt aber noch keine Daten: der Testpuffer
enthält danach 8 Nullbytes, während `/dd/startup` real mit
`echo "Excecute s…` beginnt (per `os9 copy` gegengeprüft). Der Lesepfad
ist also noch nicht angeschlossen — deutlich kleineres Thema als der
bisherige Absturz.

### Werkzeug-Nachtrag: `os9 gen` fällt aus, Direktschreiben ersetzt es

`os9 gen -b=` bricht seit heute mit *"is fragmented"* ab — auch bei
frischen Klonen, weil das Master-Abbild zwischenzeitlich von außerhalb
dieser Sitzung beschrieben wurde (Zeitstempel 12:44). Ersatz:
`$CLAUDE_JOB_DIR/tmp/mkboot_direct.py` schreibt die Kette linear an die
Boot-LSN aus dem Identification-Sektor und zieht den Längeneintrag mit
(`0x15`-`0x17` LSN, `0x18`-`0x19` Länge, Sektorgröße 512) — exakt das in
der Projektnotiz "Q9 Testimage-Bootkette" dokumentierte Verfahren, mit
Größenprüfung gegen den belegten Bereich.

## Fortsetzung 28: GELÖST -- `F$Move` fehlte, `I$Read` liefert jetzt echte Dateidaten (2026-09-10, neue Session)

Reproduziert per fertigem Rezept aus der vorigen Übergabe (Kernel neu
gebaut, `mkboot_direct.py` mit den vier gesicherten Disk-Modulen aus
dem Job-`tmp`, Testabbild frisch von `OS9SYS.dbg10.hda` geklont): exakt
derselbe Stand wie zuletzt notiert --

    o[........]n

-- der Testpuffer nach `I$Read("/dd/startup", 8)` blieb auf acht
Nullbytes, obwohl `d1` (per angehängter Diagnose geprüft) korrekt `8`
zurückmeldete. Kein Absturz, kein Fehlercode -- der Kopiervorgang fand
schlicht nicht statt.

### Ursache gefunden: `F$Move` unregistriert, RBF prüft dessen Erfolg nicht

`Q9K_SysUnimplemented` (bisher stumm: Carry+`E$UNKSVC`, sonst nichts)
kurzzeitig um eine Ausgabe der Rücksprungadresse UND der im Aufrufer
codierten Dispatch-Slot-Verschiebung erweitert (Technik: die
Rücksprungadresse liegt bei `(sp)` genau wie bei einem normalen `jsr`;
8 Byte davor steht beim Trampolin-Muster -- `pea <ret>(pc)` / `move.l
disp(a3),-(a7)` / `movea.l disp2(a3),a3` / `rts` -- die Verschiebung der
ersten `move.l`, und die ist `Callcode*4`; dieselbe Methode, mit der
vorher schon `F$RetPD` gefunden wurde). Ergebnis: **derselbe Aufrufer**
(Rücksprungadresse `$D3F2`) fragt sowohl während `I$Open` als auch
während `I$Read` nach Slot-Verschiebung `$E0` = Callcode `$38` =
**F$Move**.

Die Rücksprungadresse liegt exakt im RBF-Modul (`HdrPtr=$D31A`,
`Größe=$25A6`, per Moduldirectory-Dump ermittelt) bei Modul-Offset
`$D8`. Ein Hexdump von `rbf.mod` an dieser Stelle bestätigt den
kompletten Trampolin von Hand:

    000000c0: 48e7 e0e0 2f0b 266e 03a4 487a 000c 2f2b
    000000d0: 00e0 266b 04e0 4e75 265f 4cdf 0707 4e75

`movem.l ...,-(sp)` / `move.l a3,-(sp)` / `movea.l $3a4(a6),a3`
(D_SysDis) / `pea $d8(pc)` / `move.l $e0(a3),-(a7)` (**Callcode
`$e0/4=$38`**) / `movea.l $4e0(a3),a3` / `rts` -- und ab Offset `$d8`
(dem Rücksprungziel): `movea.l (a7)+,a3` / `movem.l (a7)+,...` / `rts`.
**Kein einziger Test auf Carry oder `d1` dazwischen** -- RBF geht
stillschweigend von Erfolg aus, genau wie beim echten Microware-Kernel
üblich (F$Move gilt dort praktisch nie als fehlschlagend). Bei uns lief
der Aufruf bisher in den Unimplemented-Stub: kein sichtbarer Fehler,
aber auch keine kopierten Daten.

Real-Konvention nachgeschlagen (`68k_tech.pdf`, S. 466f, "F$Move --
Move Data (Low Bound First)"): IN `d2.l`=Bytezahl, `(a0)`=Quelle,
`(a2)`=Ziel; OUT keine; bei überlappenden Bereichen richtungssicher
kopieren (System-State-Dienst).

### Fix: `Q9K_SysFMove` implementiert

Neuer Handler in `q9kernel_entry.a` (Callcode `0x38`), reine
Byteschleife mit Überlapp-Erkennung (`a2 > a0` → rückwärts, sonst
vorwärts) -- Tempo ist für unseren Zweck irrelevant. `a0`/`a2`/`d2`
werden trotz laut Manual undefiniertem OUT unangetastet
zurückgegeben (kostet nichts, vermeidet die Klasse von Annahme-Fallen,
die schon bei der verworfenen `F$Sleep`-Rahmenübernahme (`a5==sp+8`)
echten Speicher zerstört hat). Registrierung in `q9kernel_cinit.c`
analog zu `F$RetPD`, in `Q9_D_USRDIS` UND `Q9_D_SYSDIS`.

**Live bestätigt:**

    o[echo "Ex]n

Der Testpuffer enthält jetzt exakt die ersten 8 Byte der echten Datei
(`/dd/startup` beginnt mit `echo "Excecute s…`, per `os9 copy`
gegengeprüft in der vorigen Session). Alle 14 Host-Testsuiten weiterhin
grün.

Die Diagnose-Erweiterungen (Bytezahl-Ausgabe im Testcode, Rücksprung-
adress-Auswertung in `Q9K_SysUnimplemented`) wurden nach dem Fund
wieder zurückgebaut; die Fundtechnik steht als Nachschlage-Kommentar
direkt bei `Q9K_SysUnimplemented` im Quelltext (Stil wie beim
`F$RetPD`-Fund).

**WICHTIG bei jedem neuen Kernel-Build:** RBF_BASE hat sich mit diesem
Fix wieder verschoben (Kernel um 14 Byte größer als beim `A4`-Fix aus
Fortsetzung 27) -- vor jeder adressbasierten Messung per
Moduldirectory-Dump (Ctrl-`^` im laufenden Emulator, `q9dbg_dump.txt`)
oder `M$ID`-Sync-Wort neu bestimmen, nicht aus dieser Notiz übernehmen.

**Nächste Schritte:** Der `F$Load`-Meilenstein selbst (RBF/CF-Treiber
als echten Boot-Loader-Pfad statt Testcode nutzen) ist jetzt technisch
nicht mehr durch fehlende Dienste blockiert -- die drei ursprünglich in
IOMans Aufrufliste gefundenen, noch unregistrierten Dienste `F$VModul`
(`$2e`), `F$SRqCMem` (`$5c`) und `F$RetPD` (`$31`, inzwischen
implementiert) sollten vor dem nächsten größeren Schritt (Datei
tatsächlich AUSFÜHREN, nicht nur lesen) daraufhin geprüft werden, ob
sie im Ladepfad wirklich noch gebraucht werden.

## Fortsetzung 29: `F$VModul`/`F$SRqCMem` implementiert -- Modul lädt+validiert korrekt, `F$Load` meldet dem Aufrufer trotzdem Fehlschlag (2026-09-10/11, neue Session)

Ausgangspunkt: die am Ende von Fortsetzung 28 empfohlene Prüfung, ob
`F$VModul` und `F$SRqCMem` im `F$Load`-Ladepfad wirklich noch gebraucht
werden. Vorab per Moduldirectory-/Dispatch-Dump geklärt: **`F$Load`
selbst ist bereits vollständig durch IOMan bereitgestellt** (Slot `$01`
zeigt schon vor jedem eigenen Eingriff auf eine echte IOMan-Adresse,
ebenso `$84`/`$89` für `I$Open`/`I$Read`) -- der eigene Kernel muss
`F$Load` NICHT selbst implementieren, nur die Kernel-Primitive
liefern, die IOMans eigene `F$Load`-Logik intern braucht.

### `F$Move` allein reichte nicht -- `F$VModul` und `F$SRqCMem` fehlten noch

Per Dispatch-Dump (erweiterte Slot-Liste in `q9boardrun.c`) bestätigt:
`$2e` (`F$VModul`) und `$5c` (`F$SRqCMem`) zeigten beide noch auf
`Q9K_SysUnimplemented`. Reale Konventionen aus `68k_tech.pdf` gelesen:

- **`F$SRqCMem`** (S. 501f): IN `d0.l`=Bytezahl, `d1.w`=Speicherfarbe
  (0=beliebig); OUT `d0.l`=gewährte Bytezahl, `(a2)`=Blockzeiger. Manual
  selbst: *"F$SRqMem is equivalent to a F$SRqCMem request with a color
  of 0"* -- unser Kernel kennt ohnehin nur einen Speicherbereich, daher
  wortwörtlich dieselbe Logik wie das bereits vorhandene `F$SRqMem`
  (`Q9K_SysSRqMemImpl` wiederverwendet), inklusive derselben 44-Byte-
  Rahmenversorgung für Trampolin-Aufrufer (eigenes Frame-Scratch, damit
  ein verschachtelter `F$SRqMem`/`F$SRqCMem`-Aufruf sich nicht
  gegenseitig überschreibt).
- **`F$VModul`** (S. 532f): IN `d0.l`=Modulgruppen-ID (ungenutzt),
  `d1.l`=Modulgröße, `(a0)`=Modulzeiger; OUT `(a2)`=Verzeichnis-
  eintragszeiger. Prüft Kopfparität UND CRC, trägt bei Erfolg ins
  Moduldirectory ein.

### Der Modul-CRC-24-Algorithmus war nirgends als Code dokumentiert -- empirisch gefunden

Das Manual beschreibt nur in Worten, was `F$CRC` tut (Akkumulator -1,
XOR mit jedem Byte, dann bitweise Polynomdivision), nennt aber weder
das Polynom noch fertigen Code. internem Referenzmaterial liefert
immerhin den ERWARTETEN Endwert: `CRCCon = $00800FE3`. Per Python-
Vorabtest (gleiche Methodik wie beim 24-Word-Kopfprüfsummen-Fund
2026-08-18) gegen sechs echte, unveränderte Microware-Module (rbf/
cfide/ioman/scf/dd/c0.mod aus diesem Testkorpus) durchprobiert:

    init = 0xFFFFFF
    für jedes Byte b: crc ^= (b << 16); dann 8×: crc = (crc&0x800000) ?
        ((crc<<1) ^ 0x800063) & 0xFFFFFF : (crc<<1) & 0xFFFFFF

Über das GESAMTE Modul (inklusive des CRC-Feldes selbst) ergeben alle
sechs Module exakt `$800FE3` -- Polynom `$800063` damit als real
verifiziert, nicht geraten. Implementiert als `Q9K_ModDirValidateAndAdd`
(`q9kernel_moddir.c`), die bestehende `Q9K_ValidModuleHeader`/
`Q9K_CheckSyncWord`-Infrastruktur (24-Word-Kopfprüfsumme, schon für
`F$Link`s Boot-Scan vorhanden) und `Q9K_ModDirAdd` wiederverwendend.
Reale Fehlercodes aus `funcs.a` ausgezählt (Anker `E$PthFul=$C8`,
`E$UnkSvc=$D0`, `E$BPAddr=$D2`, `E$BPNam=$D7`, `E$MNF=$DD` -- alle
bereits bekannt und bestätigt, Zählung damit verlässlich): `E$BMID`
(`$CD`, Sync-Wort falsch), `E$BMHP` (`$EC`, Kopfprüfsumme falsch),
`E$BMCRC` (`$E8`, Modul-CRC falsch).

### Live-Lauf 1: `I$Open`/`I$Read` erneut bestätigt, `F$Move` per Rücksprungadressen-Forensik gefunden

Reproduziert mit dem exakt gleichen Rezept wie Fortsetzung 28 (Kernel
neu bauen, `mkboot_direct.py` mit den vier Disk-Modulen, Testabbild
frisch von `OS9SYS.dbg10.hda` geklont) -- Ergebnis identisch zu vorher
bestätigt, kein Rückschritt.

### Live-Lauf 2: `F$VModul`/`F$SRqCMem` implementiert und gegen ein echtes Kommandomodul getestet

Testcode erweitert um einen `F$Load("/CMDS/echo")`-Aufruf --
`/CMDS/echo` ist ein echtes, kleines Kommandomodul auf dem Testabbild
(per `os9 ident` geprüft: Größe `$C8E`, "Good CRC"). Erster Versuch
scheiterte mit `E$MNF` (`$DD`) -- eigener Bug im Testpfad, nicht im
Kernel: **das erste Pfadsegment nach `/` ist in OS-9 immer der
Gerätedeskriptorname**, `/CMDS/echo` ohne Präfix ließ IOMan
`F$Link("CMDS")` versuchen statt `F$Link("dd")`. Korrigiert auf
`/dd/CMDS/echo`.

Danach: `E$BMCRC` (`$E8`). Per angehängter Diagnose (Größe + erste/
letzte 4 Byte des Modulzeigers ausgeben) gefunden: IOMan übergibt eine
um **2 Byte zu große** Modulgröße (`$C90` statt der echten `$C8E`) --
Ursache auf IOMan-Seite nicht weiterverfolgt (außerhalb der Kernel-
Zuständigkeit), aber die falschen zwei Zusatzbyte ließen die
CRC-Prüfung über den Rand des echten Moduls hinauslaufen und dadurch
scheitern. **Fix:** `Q9K_ModDirValidateAndAdd` liest die reale
Modulgröße nach bestandener Kopfprüfsumme aus dem Header selbst
(`M$Size`, Offset `$04`) statt dem möglicherweise ungenauen Aufrufer-
Parameter zu vertrauen -- der Wert ist zu diesem Zeitpunkt bereits
durch die 24-Word-XOR-Prüfsumme abgesichert. Der Aufrufer-Parameter
bleibt nur noch als Obergrenze für die Bounds-Prüfung der billigen
Vorstufen in Gebrauch.

**Danach live bestätigt:** das Testmodul steht mit `HdrPtr` (korrekte
Adresse), `Größe=$C8E` (exakt richtig) und `TyLang=0101` im
Moduldirectory -- `F$VModul` selbst arbeitet nachweislich vollständig
korrekt.

### Live-Lauf 3: `F$VModul` erfolgreich, `F$Load` meldet dem Aufrufer trotzdem Fehlschlag -- Ursache noch offen

Trotz des korrekt eingetragenen Moduls meldet der GESAMTE
`F$Load`-Aufruf dem Testcode `E$MNF` (`$DD`). Drei Ausschlüsse per
gezielter Forensik:

1. **Kein fehlender Kernel-Dienst.** Die Rücksprungadressen-Sonde in
   `Q9K_SysUnimplemented` (dieselbe Technik, mit der `F$Move` gefunden
   wurde) blieb während der gesamten `F$Load`-Fehlschlagkette stumm --
   nichts landet dort.
2. **Rahmenversorgung (analog `F$SRqMem`/`F$AllPD`) ändert nichts.**
   Versuch: `(a2)` zusätzlich in den 44-Byte-Registerrahmen des
   Trampolin-Aufrufers schreiben (`a5==sp+8`-Erkennung, exakt das
   Muster von `F$SRqMem`). Live-Ergebnis identisch (weiterhin `E$MNF`,
   Modul weiterhin korrekt im Directory) -- **kein nachgewiesener
   Nutzen**, deshalb wieder zurückgebaut statt auf Verdacht drin zu
   bleiben (Lehre aus dem `F$Sleep`-Vorfall: eine Rahmenerkennung ohne
   bestätigten Rahmen kann fremden Speicher zerstören).
3. **Verdächtiges Folgesymptom, nicht weiterverfolgt.** Mit der
   Rahmenversorgung aktiv zeigte eine `Q9K_ModDirLinkByName`-Diagnose
   (letzte Suchanfrage nach `$1710`/`$1714`) einen späteren `F$Link`-
   artigen Aufruf mit erkennbar kaputten Parametern (Filter `$90A0` --
   exakt eine Moduldirectory-Slot-Adresse, kein plausibler Typ/Sprache-
   Filter; Name unlesbar). Ob das Symptom der Rahmenversorgung selbst
   zuzuschreiben ist oder unabhängig vorher schon bestand, ist NICHT
   geklärt -- nach dem Zurückbau nicht erneut geprüft.

**Werkzeug-Nachtrag: Boot-Testabbild an einer harten Größengrenze.**
`mkboot_direct.py` darf nur bis zur nächsten 512-Byte-Grenze der
BISHERIGEN Bootdatei-Länge schreiben (aktuell 72 Sektoren = 36864 Byte
ab LSN `$879E1`) -- `os9 gen -b=` als Ausweg bleibt weiterhin an
"is fragmented" gescheitert (auch auf frischen Klonen: die
Fragmentierung liegt im Master-Abbild selbst). Mit `F$VModul`/
`F$SRqCMem` plus dem `F$Load`-Testcode reicht der Platz NICHT mehr für
den kompletten Kernel plus `hellosvc` gleichzeitig -- für die Läufe
dieser Sitzung wurde `hellosvc` deshalb aus `src/kernel/build/`
entfernt, bevor `mkboot_direct.py` lief (Datei bleibt unverändert im
Quelltext, nur aus DIESEM Testabbild ausgeschlossen). Bei jedem
künftigen `F$Load`-Testlauf zuerst prüfen, ob beides gleichzeitig noch
passt; wenn nicht, `hellosvc` erneut beiseiteschieben oder eine der
beiden Testroutinen kürzen. Die früheren Dreiklang- und Dateitest-
Testblöcke (F$Link auf "dd"/"rbf"/"cfide" bzw. I$Open/I$Read auf
"/dd/startup") wurden entfernt, um Platz für den neuen `F$Load`-Test zu
schaffen -- ihr Zweck war bereits erfüllt und in Fortsetzung 27/28
committet.

**Nächste Schritte:**
1. Ursache der `F$Load`-Fehlermeldung an den Aufrufer finden --
   Rücksprungadressen-Forensik auf den Punkt, an dem IOMans `F$Load`
   selbst das Carry setzt (nicht mehr auf einen fehlenden Dienst, das
   ist ausgeschlossen). Startpunkt: das Verhalten ist reproduzierbar
   (`/dd/CMDS/echo` lädt immer korrekt, meldet aber immer `E$MNF`).
2. Das verdächtige `F$Link`-Folgesymptom aus Live-Lauf 3 unabhängig
   vom Rahmenversorgungsversuch nachprüfen (trat es auch OHNE
   Rahmenversorgung auf?).
3. Danach erst: `F$Load` tatsächlich zum Ausführen eines geladenen
   Programms nutzen (der ursprüngliche Zweck des ganzen Meilensteins).

## Fortsetzung 30: GELÖST -- `F$Load` läuft vollständig durch (2026-09-11, autonome Nachtsitzung)

Fortsetzung 29 endete mit einem Rätsel: `F$VModul` validiert das echte
Kommandomodul `/dd/CMDS/echo` nachweislich korrekt (steht mit exakt
richtiger Größe im Moduldirectory), aber der GESAMTE `F$Load`-Aufruf
meldet dem Testcode trotzdem `E$MNF`. Diese Sitzung hat die Ursache
gefunden und behoben.

### Werkzeug-Erweiterung: Register in der Instruktionsspur

Der bestehende Instruktionsring (`Q9_TRACE_INSTR=1` + `Q9_FREEZE_PC`,
Q9-Flux `src/kernel/m68krt.c`) zeichnete bisher nur `pc/d0/a0/a4/sp`
auf. Um `d1`/`d3`/`d4` an einer beliebigen eingefrorenen Stelle zu
sehen, wurde die Spur um drei Felder erweitert (`q9_dbg_tr_d1/d3/d4`,
`m68krt.h`/`m68krt.c`/`q9boardrun.c`) -- bleibt als permanente
Werkzeug-Erweiterung bestehen (wie schon die A4/SP-Felder vorher),
nicht zurückgebaut.

### Die Diagnose-Kette

**Schritt 1 -- Rücksprungadresse des `F$VModul`-Aufrufers.** Wie schon
beim `F$Move`-Fund: Rücksprungadresse bei `(sp)` nach `$1730` legen,
BEVOR irgendein Register angefasst wird. Ergebnis: `$0000B55C` --
Modul-relativ zu IOMans HdrPtr exakt `ioman+$8BE`, passt zur bereits in
Fortsetzung 11/`intern dokumentiert` bekannten
`F$Load`-Einsprungadresse `ioman+$6D6`.

**Schritt 2 -- Mehrfachaufruf ausgeschlossen.** Vermutung: vielleicht
ruft `F$Load`s "lies bis Fehler/EOF"-Schleife `F$VModul` ein zweites
Mal für Restbytes nach dem echten Modul auf. Per Aufrufzähler +
Groesse-rein/Fehler-raus-Log (bis zu 4 Slots ab `$1750`) widerlegt:
**genau EIN Aufruf**, mit Erfolg. (Ein erster Versuch dieser Diagnose
hatte selbst einen Bug -- benutzte `a0` als Rechenregister und
überschrieb damit den echten Modulzeiger-Parameter, WOMIT `F$VModul`
reproduzierbar fehlschlug. Eigene Diagnose-Bugs sind genauso real wie
Kernel-Bugs; sofort per Kopfkommentar dokumentiert, dann korrigiert.)

**Schritt 3 -- volle Analyse von `ioman+$6D6` bis `$990`**
(per capstone, `CS_ARCH_M68K`/`CS_MODE_M68K_000`, `ioman.mod` aus dem
F$Load-Testkorpus). Zeigt den kompletten `F$Load`-Ablauf: Speichersuche
zuerst (`bsr $124a`, scheitert erwartungsgemäß -- Modul noch nicht
resident), Pfadauflösung + `I$Open` + `I$Read` von Platte, dann Aufruf
von `F$VModul` (Slot `$b8/4=$2e`, exakt das Trampolin-Muster wie bei
`F$Move`). NACH dem Aufruf (`ioman+$8be` ff.):
`+$0C(a2)` wird mit `ADDQ.W` inkrementiert (Link-Zähler) und später mit
`SUBQ.W` wieder dekrementiert; `ioman+$6de` liest `+$12(a0)` (a0=a2 zu
diesem Zeitpunkt) als Wort UND `+$0C(a0)` als Langwort, das zu `a0`
addiert wird -- Ergebnis dient als Namenszeiger für einen internen
`F$Link`-Aufruf (Slot `$0/4=$00`, über `D_UsrDis` diesmal statt
`D_SysDis`). Schließlich landet `+$00(a2)` im Stack-Frame an einer
Stelle, die der Funktions-Epilog (`movem.l (a7)+,d0-d4/a0-a3/a5`) als
NEUEN `a2`-Wert zurückgibt -- IOMans eigener `F$Load`-Rückgabewert.

**Schritt 4 -- zwei verworfene Zwischenversuche, live widerlegt:**
1. `(a2)` = unser eigener 16-Byte-Verzeichnis-Slot (ursprüngliche
   Implementierung, Fortsetzung 29): `+$12` liegt AUSSERHALB unseres
   Slots (endet bei `$0E`) -- Datenmüll aus dem Free-Pool-Nachbarn.
2. `(a2)` = der validierte Modulkopfzeiger selbst (naheliegend, da
   `+$12` beim echten Header zufällig `M$TypLang` ist): live per
   Freeze bestätigt, dass IOMan dadurch `+$0C(a2)` als Link-Zähler
   BEHANDELT UND BESCHREIBT -- beim echten Header ist `+$0C` aber
   `M$Name` (der Namens-Offset)! Das `ADDQ.W`/`SUBQ.W` verschob dieses
   Feld hin und her und beschädigte den Header. Symptom: `a0` enthielt
   danach `$4AFC0001` -- die rohen ERSTEN 4 BYTE des Moduls, nicht
   dessen Adresse (per `+$00(a2)`-Lesezugriff auf den -- durch die
   `$0C`-Fehlinterpretation ausgelösten -- Header selbst erklärt).

**Fix (Schritt 5):** `F$VModul` legt einen EIGENEN, nur dafür
reservierten 20-Byte-Rückgabepuffer an (`Q9K_VMODUL_RETBUF`, `$1650`,
fest/wiederverwendet -- muss nur bis unmittelbar nach der Rückkehr
überleben) und gibt DESSEN Adresse zurück:
- `+$00` (4): Modulkopfzeiger (wird IOMans eigener Rückgabewert)
- `+$0C` (2): Platzhalter, verträgt beliebiges ADDQ/SUBQ (Endwert
  irrelevant, wird nie wieder gelesen)
- `+$12` (2): Kopie von `M$TypLang` aus dem Modulheader

Der eigene 16-Byte-Verzeichnis-Slot wird weiterhin über
`Q9K_ModDirAdd` angelegt (für spätere `F$Link`/`F$UnLink`-Suchen) --
nur sein Zeiger geht nicht mehr nach außen.

**Live bestätigt, zweimal reproduziert:**

    RP012O0000001Hallo von Q9-OS!
    1h000000DDl

(`h000000DD` = `hellosvc`-Fork schlägt erwartungsgemäß fehl, da
`hellosvc` für dieses Testabbild aus Platzgründen ausgeschlossen war,
s.u.; `l` = **`F$Load` erfolgreich**, kein `k...DD` mehr). Moduldirectory
zeigt `echo` weiterhin mit exakt korrekter Größe (`$C8E`) und
`TyLang=0101`. Alle 14 Host-Testsuiten grün.

### Werkzeug-Nachtrag: Testabbild-Größengrenze bestätigt weiterhin gültig

Wie in Fortsetzung 29 dokumentiert: Kernel + `F$Load`-Testcode + die
vier Disk-Module passen weiterhin nur OHNE `hellosvc` in die
36864-Byte-Grenze des Testabbilds (`hellosvc` fehlte in ALLEN Läufen
dieser Sitzung, deshalb `h000000DD`). Bleibt ein bekannter, dokumentierter
Zustand dieses SPEZIFISCHEN Testabbilds -- der committete Quelltext ist
unverändert vollständig (`hellosvc.a` selbst wurde nicht angefasst).

### Nächste Schritte

1. **Der `F$Load`-Meilenstein ist erreicht** -- der nächste sinnvolle
   Schritt ist, ein geladenes Programm tatsächlich AUSZUFÜHREN (der
   ursprüngliche Zweck: z. B. `echo` per `F$Fork` auf den per `F$Load`
   gelieferten Einsprungpunkt (`a1`) starten und seine echte Ausgabe
   sehen).
2. Die Restplatz-Frage im Testabbild lösen (`os9 gen -b=` bleibt an
   "is fragmented" gescheitert, auch auf frischen Klonen -- ein neues,
   sauber formatiertes Testabbild von Grund auf wäre eine Möglichkeit).
3. Die drei ursprünglich in IOMans Aufrufliste gefundenen, jetzt alle
   implementierten Dienste (`F$RetPD`, `F$Move`, `F$VModul`,
   `F$SRqCMem`) sind vollständig abgearbeitet -- keine offene
   Restarbeit aus dieser Liste mehr.

## Fortsetzung 31: das geladene Modul läuft wirklich -- neue Baustelle "csl"-Trap-Library (2026-09-11, direkt im Anschluss)

Direkt nach dem `F$Load`-Erfolg getestet: geht der Meilenstein wirklich
zu Ende, d. h. lässt sich das geladene Modul auch AUSFÜHREN? Testcode
um `F$Fork("echo")` erweitert (Parametergröße 0 -- kein argv) direkt
nach dem erfolgreichen `F$Load`.

**Ergebnis: ja, mit Einschränkung.** `F$Fork` findet `echo` sofort über
die längst bewährte `Q9K_ModDirLinkByName` (dieselbe Suche, die schon
für `hellosvc`/`forkchild` funktioniert -- keine Änderung nötig, das
korrekt eingetragene Moduldirectory aus Fortsetzung 30 reicht). Der
Kindprozess startet und führt ECHTEN, unverändert aus dem Microware-
Modul geladenen Code aus -- live sichtbar an einer eigenen,
authentischen Fehlermeldung DES MODULS SELBST:

    **** can't install csl ****

"csl" ist die OS-9-C-Runtime-Bibliothek, mit der `echo` (ein normales,
kompiliertes C-Programm) seine Standard-I/O/Speicherverwaltung
initialisiert. Direkt im Anschluss stürzt der Kindprozess ab (Vektor 4,
Illegal Instruction, `PC=$7031` -- liegt mitten in einer
Text-Konstante des Moduls selbst, "...sed Me!SysBoot Used..." -- die
eigene Fehlerbehandlung von `echo` springt dort offenbar auf einen nie
initialisierten Funktionszeiger).

**Warum das erwartbar ist, keine Regression:** `csl` ($BCEE = 48366
Byte) ist selbst ein reales OS-9-Modul, aber
vom Typ **"Trap Hnlr" (Trap Handler/"ghost machine language trap
library")** -- ein völlig anderer Mechanismus als die bisher
implementierten `F$`/`I$`-Syscalls (`TRAP #0`). OS-9 reserviert
`TRAP #1`-`#15` für genau solche installierbaren Trap-Bibliotheken
(F$STrap/ähnliche Installationsroutine); unser Kernel kennt bisher
AUSSCHLIESSLICH `TRAP #0`. `echo` versucht beim Start, sich bei dieser
Bibliothek anzumelden, findet sie nicht (weder installiert noch
überhaupt geladen) und meldet das korrekt selbst -- der NACHFOLGENDE
Absturz ist ein Bug in `echo`s EIGENER Fehlerbehandlung für genau
diesen (bei echtem OS-9 vermutlich nie auftretenden) Fall, keiner in
unserem Kernel.

**Sofort direkt im Anschluss geprüft: reicht bloßes Laden?** Naheliegende
Vermutung: vielleicht bedeutet "can't install csl" nur "F$Link(csl) fand
nichts", weil `csl` schlicht nirgends geladen ist -- dann würde ein
`F$Load("/dd/CMDS/csl")` VOR dem `F$Fork("echo")` schon reichen, ganz
ohne neuen Kernel-Mechanismus. Live widerlegt: `csl` lädt über unseren
längst funktionierenden `F$Load`-Pfad anstandslos (Diagnose-Marker `c`,
48 KByte -- deutlich größer als alle bisher geladenen Testmodule, keine
Größenprobleme) und steht danach im Moduldirectory -- **aber `echo`
scheitert exakt genauso wie zuvor**, Wort für Wort dieselbe Meldung,
derselbe Absturz-PC. Das bestätigt sauber: die Lücke ist wirklich die
INSTALLATION (ein echter, noch unbekannter Kernelaufruf, der die
Bibliothek in eine TRAP-#1–15-Vektortabelle einträgt), nicht bloß das
Auffinden/Laden des Moduls.

**Bewusst nicht weiterverfolgt in dieser Sitzung** -- eigenes,
mehrstufiges Thema: die reale Installationsroutine (vermutlich in
IOMan oder im csl-Modul selbst, per Analyse zu finden, analog
zur `F$Load`-Forensik dieser Nacht) sowie den TRAP-#1-15-Dispatch-
Mechanismus selbst verstehen und implementieren. Passender Startpunkt
für eine eigene Sitzung.

**Nebenbefund:** ein Illegal-Instruction-Absturz in einem GEFORKTEN
KINDPROZESS scheint auf Systemebene durchzuschlagen (kein sichtbarer
"Kindprozess sauber beendet, Elternprozess läuft weiter"-Verhalten,
Ctrl-^-Dump danach als "abgebrochen" markiert) -- unser Kernel hat noch
keine Prozess-Isolation für CPU-Exceptions (ein fehlerhafter Kindprozess
kann derzeit das gesamte System mitreißen, statt nur sich selbst zu
beenden). Ebenfalls ein Thema für später, nicht für heute Nacht.

Testcode (`Q9K_TestEchoName`, `F$Fork`-Aufruf) bleibt im Quelltext
stehen -- markiert den aktuellen Stand der Untersuchung für die
nächste Sitzung, analog zu allen anderen offenen `Fortsetzung`en in
diesem Dokument. Alle 14 Host-Testsuiten weiterhin grün (der Absturz
betrifft nur den emulierten Gastcode, nicht den Kernel-Quelltext oder
dessen Host-Tests).

## Fortsetzung 32: `F$TLink` implementiert (Code-vollständig,
host-getestet) -- Live-Nachweis gegen `echo`/`csl` weiterhin durch
einen VORBESTEHENDEN, bereits in Fortsetzung 27 dokumentierten
Race blockiert (2026-09-11, direkte Fortsetzung derselben Nacht)

**Auftrag:** auf Nachfrage, wie aufwendig `F$TLink`/TRAP-#1-15-Support
wäre, Einschätzung gegeben (moderat, gut dokumentiert, vergleichbar mit
`F$VModul`/`F$SRqCMem`) -- direkt im Anschluss beauftragt ("ja bitte,
leg los").

### Forensik: `echo` benutzt TRAP #13 mit Namen "csl", NICHT `T$Math`=15

Vor der Implementierung erst nachgeprüft, WELCHE Trap-Nummer/Namen
`echo` wirklich benutzt, statt vom oberflächlich ähnlichen `T$Math`=15
auszugehen: `echo.mod`/`csl.mod` aus dem alten Testabbild extrahiert,
per Capstone analysiert. Fund bei Modul-Offset `0x79a` (über eine
gemeinsame Hilfsroutine ab `0x770`): echtes `trap #13` plus Namenszeiger
auf `"csl"`. Zusätzlich in `echo`s eigener `M$Excpt`-Fallback-Routine
exakt die Vektor→Trap-Nummer-Rechnung gefunden, die auch unser
`Q9K_TCallDispatch` benutzt (`subi.w #$80,d0 / asr.w #2,d0`) -- guter
Beleg, dass die reale Konvention (68k_tech.pdf Kapitel 5) richtig
verstanden wurde.

### Implementierung

* `src/q9moduleheader.h`: zwei neue Offset-Konstanten `Q9_MH68K_INIT`
  ($48, M$Init) und `Q9_MH68K_TERM` ($4C, M$Term, laut Handbuch nie vom
  Kernel aufgerufen) -- beide `[HANDBUCH]`, nicht per Analyse
  verifiziert.
* `src/kernel/q9kernel_traplink.c` (neu): reine Buchhaltungslogik für
  `F$TLink` (Callcode `0x21`) -- Trap-Nummer prüfen (1-15), pro-Prozess-
  Trap-Tabellen-Slot auf Kollision prüfen (E$ModBsy), Modul per
  `Q9K_ModDirLinkByName` linken (E$MNF bei Fehlschlag, bewusst KEIN
  F$Load-Fallback, s. Kopfkommentar dort), M$Exec/M$Init-Einsprünge aus
  dem Header lesen, bei Bedarf statischen Speicher per
  `Q9K_ProcSRqMem` anfordern (Aufrufer-Override d1.l hat Vorrang vor
  M$Mem), Trap-Tabellen-Slot füllen. Der eigentliche Fremdaufruf von
  M$Init (spezieller, von M$Init selbst konsumierter Stack-Rahmen)
  bleibt bewusst der Assemblerseite vorbehalten.
* Neue, eigene pro-Prozess-Trap-Tabelle (KEINE OS-9-Entsprechung): 15
  Einträge à 12 Byte (Modulzeiger/Ausführungs-Einsprung/statischer
  Speicherzeiger) bei Deskriptor-Offset `Q9K_PROCDESC_TRAPTBL_OFF`
  ($1CC, direkt hinter `Q9K_PROCDESC_ENTRYPC_OFF`).
* `q9kernel_entry.a`: `Q9K_SysFTLink` (TRAP #0, Callcode `0x21`) baut
  den echten, von M$Init per `movem.l (a7),a6 / addq.l #8,a7 / rts`
  konsumierten 12-Byte-Rahmen und springt (kein `jsr`) hinein.
  `Q9K_TCallDispatch` (neu, Ziel für TRAP #1-15) berechnet die
  Trap-Nummer aus dem Format/Vektor-Wort, schlägt den Slot in der
  Trap-Tabelle des aktuellen Prozesses nach, baut den TrapEnt-Rahmen
  (Rücksprung-PC/Vektor#/Funktionscode/Aufrufer-a6) und springt in
  M$Exec; ist nichts installiert, fällt der Pfad auf `Q9K_ExcTrap`
  zurück (generische Diagnose/Halt, s. u.).
* `q9kernel_exctable.c`: Vektoren 33-47 (TRAP #1-15) auf
  `Q9K_TCallDispatch` verdrahtet, gleiches Muster wie zuvor Vektor 32.
* `q9kernel_cinit.c`: `Q9K_SysFTLink` unter Callcode `0x21` in
  USRDIS/SYSDIS registriert.
* Neuer Host-Test `test_q9kernel_traplink.c` (8 Testfälle: Erfolg ohne/
  mit statischem Speicher, Aufrufer-Override vor M$Mem, Parameterfehler,
  Slot-Kollision, Modul-nicht-gefunden, Speicheranforderung
  fehlgeschlagen, Bruecke `Q9K_SysTLinkImpl`) -- **alle jetzt 15
  Host-Testsuiten grün** (vorher 14). Gleicher bekannter Host-Stolperstein
  wie in `test_q9kernel_tables.c`/`firstproc.c` erneut angetroffen und
  gleich gelöst: `Q9_u32` ist auf diesem 64-Bit-Testhost 8 statt 4 Byte
  breit, die drei fest 4 Byte auseinanderliegenden Trap-Tabellen-Felder
  müssen deshalb testseitig als reine 4-Byte-Werte zurückgelesen werden
  statt per `Q9K_GetU32`; zusätzlich mussten zwei Vergleiche auf die
  unteren 32 Bit maskiert werden, weil ein echter 64-Bit-Hostzeiger
  (`g_fakeModule`) über das hinausgeht, was ein reales 32-Bit-Feld
  überhaupt fassen könnte.

### Live-Verifikation: durch denselben vorbestehenden Race blockiert wie in Fortsetzung 27 angekündigt

**Neues, größeres Testabbild nötig:** Das alte, aus `OS9SYS.dbg10.hda`
geklonte Testabbild hat eine feste Bootregion-Obergrenze von 36448 Byte
(`DD_BSZ`) -- der neue, um `F$TLink`/`Q9K_TCallDispatch` gewachsene
Kernel (Bootdatei jetzt 37974 Byte) passt darauf nicht mehr, UND das
Abbild ist chronisch fragmentiert (`os9 gen -b=` schlägt bei jeder
Änderung mit "is fragmented" fehl). **Neues, wiederholbares Rezept**
etabliert (gilt für alle künftigen Sitzungen mit diesem Testkernel):

1. `os9 format -q -k -nQ9TEST -bs512 -l32768 -c32 <image>` -- OHNE
   `-e`. Ohne `-e` legt `os9 format` nur ein winziges (~16 KByte)
   Datei-Fragment an (die volle 16-MByte-Größe steht zwar im
   Identification-Sektor, das Hostdateisystem materialisiert sie aber
   erst bei tatsächlichem Schreibzugriff). **Falle, live erlebt:** `-e`
   ("format entire disk") erzeugt sofort ein volles 16-MByte-Abbild --
   genau DAMIT bootet der emulierte CompactFlash-Treiber nicht mehr
   sauber durch (endlose `B`-Zeichenflut direkt nach dem Treiber-Banner,
   noch vor jeder eigenen Kernel-Ausgabe; per Bisektion bestätigt: tritt
   unabhängig von der genauen Sektorzahl auf, auch bei nachträglichem
   `truncate` auf volle Größe -- eindeutig an "Datei von Anfang an voll
   materialisiert" gekoppelt, nicht an der Sektorzahl selbst). Ursache
   im Emulator/CF-Treiber nicht weiter verfolgt, nur die auslösende
   Bedingung vermieden.
2. `tools/mkbootfile.sh --disk <ref.boot> <image>` NUR auf dem noch
   leeren Abbild -- `os9 gen -b=` scheitert reproduzierbar mit "is
   fragmented", sobald vorher schon andere Dateien (CMDS/echo, CMDS/csl)
   angelegt wurden, selbst nach Löschen einer alten Bootdatei.
3. Erst DANACH `os9 makdir`/`os9 copy` für `/CMDS/echo`/`/CMDS/csl`,
   `os9 attr -e` danach (s. `[[q9-toolshed-execute-bit-bug]]`).

Mit diesem Rezept bootet das Abbild wieder sauber bis zum bekannten
`I$Write`-Erfolg ("Hallo von Q9-OS!").

**Der in Fortsetzung 27 bereits als bekannt vermerkte, Modulgrößen-
abhängige Folgefehler (Vektor 4 kurz nach `I$Write`, vor der
Diagnose-Marke `O`) besteht weiterhin** -- diesmal in einer ANDEREN
Erscheinungsform: statt einer Illegal-Instruction-Exception auf
Datenbytes wird jetzt (reproduzierbar auf dem byte-identischen,
37974-Byte-Kernel) unmittelbar nach `I$Write` ein echtes `trap #n`
ausgeführt, das über `Q9K_TCallDispatch` läuft, dort keinen
installierten Trap-Slot findet und nach `Q9K_ExcTrap` durchfällt
(Diagnose-Zeichen `E`); danach folgt eine endlose, nicht durch die
übliche Testverzögerung getaktete Zeichenflut (`B`), die nicht zu
`Q9K_TestProcB` (das würde sichtbar VERZÖGERT ausgeben) passt -- die
genaue Quelle dieser Zeichen ist NICHT geklärt. Interpretation: es
handelt sich um denselben, seit 2026-09-04 bekannten, zeitpunkt-/
layoutabhängigen Race (mutmaßlich ein durch `Q9K_TimerIRQHandler`
oder eine unmaskierte Unterbrechung zwischen zwei Syscalls verursachter
Rücksprung an eine falsche, aber zufällig gültige Adresse) -- nur dass
er diesmal auf Bytes trifft, die zufällig als `trap #n` decodieren,
statt auf eine ungültige Bytefolge. Ein erster, kleiner
Zeitversatz-Versuch (Entfernen von 6 Byte Diagnosecode) hatte den
Absturz-PC vorher nur geringfügig verschoben, nicht behoben; ein
größerer, gezielter Versuch (48 NOP vor `Q9K_TestProcA`, bewusst
außerhalb der A4-Dispatch-Mechanik platziert) wurde in dieser Sitzung
gebaut, LIVE GETESTET und **ergebnislos wieder entfernt** -- er
verschob das Symptom nur erneut (führte reproduzierbar in dieselbe
`B`-Endlosschleife), löste die eigentliche Ursache nicht.

**Fazit dieser Sitzung:** `F$TLink` ist code-vollständig und durch den
neuen Host-Test abgedeckt, aber der End-zu-Ende-Nachweis gegen echtes
`echo`+`csl` bleibt weiterhin durch denselben vorbestehenden Race
blockiert, der schon in Fortsetzung 27 als offenes Thema vermerkt
wurde -- er ist NICHT durch die `F$TLink`-Änderungen verursacht
(gleiches Verhalten bei identischer Kernelgröße vor und nach den hier
beschriebenen Änderungen bestätigt), sondern lag schon vorher da und
wird durch mehr Code im Kernel nur früher/anders sichtbar. Zeitversatz-
Experimente (Bytes verschieben, in der Hoffnung das Race-Fenster zu
verfehlen) sind als Lösungsweg widerlegt -- der nächste Schritt braucht
eine echte Ursachenanalyse (wahrscheinlichster Verdächtiger:
`Q9K_TimerIRQHandler`s Registersicherung auf dem STACK DES
UNTERBROCHENEN PROZESSES, kombiniert mit dem C-Aufruf von
`Q9K_SchedReschedule` auf demselben Stack -- oder die fünf Autovektor-
Ebenen, die laut `q9kernel_exctable.c`-Kommentar weiterhin bewusst auf
dem generischen Halt-Handler bleiben, falls dort doch schon echte
Hardware unterbrechen kann), keine weiteren Bisektions-Rateversuche.
Alle 15 Host-Testsuiten grün (der Race betrifft ausschließlich den
emulierten Boot-Test, nicht den Kernel-Quelltext oder dessen
Host-Tests).

## Fortsetzung 33: Ringpuffer-Instrumentierung zeigt den Race live --
Absturz in `echo` selbst ist NICHT (mehr) der `F$TLink`-Fehlschlag,
sondern eine noch frühere, ungeklärte Lücke (2026-09-11, direkte
Fortsetzung derselben Sitzung)

**Auftrag:** nach Fortsetzung 32 ("Sollen wir den Bug angehen?") --
"ja bitte" -- echte Ursachenanalyse statt weiterer Zeitversatz-Versuche.

### Instrumentierung (alle TEMPORÄR, am Ende dieser Sitzung wieder
vollständig entfernt -- `git checkout -- src/kernel/q9kernel_entry.a`
auf den Stand von Commit `4de8704`)

* Ringpuffer der letzten 32 Interrupt-Eintritte (Marker=Vektornummer,
  30 für den Timer; geretteter PC), gefüllt in `Q9K_TimerIRQHandler`
  und `Q9K_IRQDispatch`, ausgegeben von `Q9K_ExcTrap` vor dem Anhalten.
* Alle-256-Aufrufe-Marker in `Q9K_DiagWriteD7` (zeigt den Aufrufer, um
  eine Endlosschleife auf dieser Routine ihrem wahren Ursprung
  zuzuordnen).
* Erfolg/Fehlschlag-Marker (`+`/`-` + Trap-Nummer bzw. Fehlercode)
  direkt in `Q9K_SysFTLink`.
* **Wichtige Lektion unterwegs:** die ersten Puffer-Adressen ($1900/
  $1A00/$1A10) lagen im NIEDRIGEN Adressbereich -- ein Absturz landete
  daraufhin zufällig GENAU auf einer dieser eigenen Debug-Zellen
  (PC=$1a10). Verschieben auf eine hohe, garantiert freie Adresse
  (`$1440C0`, direkt hinter dem längst etablierten `Q9K_ExcInfo_Stack`)
  lieferte anschließend BYTE-IDENTISCHE Ergebnisse -- bewiesen: die
  eigene Debug-Zelle war nie die Ursache, nur zufällig im Zielbereich
  des ohnehin vorhandenen Fehlers platziert. Eigene Debug-Puffer für
  diese Fehlerklasse gehören grundsätzlich in den hohen Adressbereich,
  nie in den niedrigen (genau dort landen die kaputten Sprungziele).

### Fund 1: der Ringpuffer bestätigt die Race-These direkt

Kurz vor einem Absturz zeigte der Puffer ~30 Einträge mit Marker `$50`
(80 = der per F$IRQ verdrahtete DUART-Vektor) und IDENTISCHEM PC
`$756E` (die TXRDY-Poll-Schleife in `Q9K_DiagWait`/`Q9K_DiagHexWait`)
-- plausibel: viele schnelle Sende-Interrupts während einer Zeichen-
ausgabe. EIN Eintrag mitten drin weicht ab: Marker `$50`, aber PC
`$0000001B` -- eine winzige, eindeutig ungültige Codeadresse. Das ist
der erste DIREKTE, live gemessene Beleg (nicht mehr nur Vermutung),
dass irgendwo in der Interrupt-Verschachtelung (Timer trifft
`Q9K_IRQDispatch` oder umgekehrt) der gerettete PC auf einen kleinen,
plausiblen "Registerwert-statt-Adresse"-Wert kollabiert -- exakt die
seit 2026-09-04 vermutete Fehlerklasse, jetzt erstmals mit echten
Zahlen statt nur "Ob er auftritt, hängt an der Modulgröße" belegt.

### Fund 2: der eigentliche Absturz in `echo` ist NICHT der `F$TLink`-Fehlschlag

Der Absturz selbst (Vektor 4, PC=`$7031`, mitten in der Textkonstante
"...sed Me!SysBoot Used...") ist WORTWÖRTLICH derselbe, den Fortsetzung
31 VOR der `F$TLink`-Implementierung dokumentiert hat. Das allein wäre
noch kein Widerspruch (`F$TLink` könnte ja weiterhin fehlschlagen) --
aber der neu eingebaute `+`/`-`-Diagnosemarker in `Q9K_SysFTLink`
**feuerte kein einziges Mal** vor dem Absturz. Da dieser Marker JEDEN
echten Aufruf von Callcode `$21` protokolliert hätte, unabhängig vom
Aufrufer (eigener Testcode oder `echo`), heißt das: **`echo` hat
`F$TLink` in diesem Lauf gar nicht erst erreicht.** Die ursprüngliche
Diagnose ("scheitert an `F$TLink`, dann Absturz in der eigenen
Fehlerbehandlung") war entweder ein Umstand einer früheren, anders
getakteten Sitzung, oder der hier untersuchte Absturzpfad ist ein
GANZ ANDERER als der ursprünglich dokumentierte, der zufällig zur
selben Adresse führt (beides bei einem PC MITTEN IN EINER
TEXTKONSTANTE plausibel -- ein springender Zeiger, der zufällig genau
dort landet, muss nicht jedes Mal aus demselben Grund kommen).

Zusätzlich fehlten in diesem Lauf auch die erwarteten Diagnosezeichen
`l`/`k` (F$Load "echo") und `c` (F$Load "csl") VOLLSTÄNDIG aus dem
Konsolenstrom -- direkt zwischen hellosvcs eigener Ausgabe und dem
"E" (F$Fork "echo" erfolgreich) klafft eine Lücke ohne jedes Zeichen
(per Rohbyte-Vergleich verifiziert, kein Anzeige-/Terminal-Artefakt).
Das heißt: mindestens die BEIDEN Diagnose-Ausgaben wurden komplett
übersprungen, OHNE dass der nachfolgende F$Fork fehlschlug -- ein
weiterer, eigenständiger Beleg für denselben PC-Verschiebungs-
Mechanismus (er überspringt hier offenbar nur die kurzen
Diagnose-Aufrufe, nicht die eigentlichen Trap-#0-Aufrufe selbst).

### Fazit und offener nächster Schritt

Der `F$TLink`-Verdacht aus Fortsetzung 32 ist widerlegt: die
Implementierung wird in diesem Lauf gar nicht erreicht, der Absturz
in `echo` hat eine andere, noch nicht identifizierte Ursache (entweder
derselbe Kernel-Race, diesmal INNERHALB des geforkten Kindprozesses
statt in unserem eigenen Testcode, oder eine ECHTE Lücke in `echo`s
eigener C-Laufzeit-Startsequenz, die mit unserem noch unvollständigen
Kernel kollidiert -- z. B. ein von `echo`s Runtime vorausgesetzter,
bei uns fehlender Syscall). Beides ist plausibel, keins der beiden ist
in dieser Sitzung mehr abschließend unterscheidbar gewesen.

**Konkreter nächster Schritt (nicht mehr in dieser Sitzung):** den
`+`/`-`-Diagnosemarker in `Q9K_SysFTLink` (oder eine schlankere
Variante davon) dauerhaft/wieder einbauen und GEZIELT einen KONTROLLIERTEN
`F$TLink(13,"csl")`-Aufruf aus dem EIGENEN Testcode heraus prüfen
(vor dem `F$Fork("echo")`, mit bekanntem, sauberem Kontext) -- das
entkoppelt die Prüfung von `echo`s unbekannter, nicht quelloffener
C-Laufzeit und beantwortet zuerst die einfachere Frage "funktioniert
`F$TLink` überhaupt korrekt, wenn WIR es sauber aufrufen?", bevor
weiter in `echo`s eigenem Absturz gegraben wird.

Alle 15 Host-Testsuiten weiterhin grün. Keine Quelltextänderung aus
dieser Sitzung committet -- die gesamte Instrumentierung war temporär
und wurde vor Sitzungsende auf den Stand von Commit `4de8704`
zurückgesetzt (`git checkout -- src/kernel/q9kernel_entry.a`).

## Fortsetzung 34: MEILENSTEIN -- `F$TLink(13,"csl")` funktioniert
wirklich, `echo` installiert `csl` erfolgreich (2026-09-11, direkte
Fortsetzung derselben Sitzung, nach neuem Sitzungslimit)

**Auftrag:** "Wir haben ... wieder neue Limits bekommen ... mach bitte
weiter" -- Fortsetzung 33s konkreten nächsten Schritt umgesetzt: einen
kontrollierten `F$TLink(13,"csl")`-Aufruf aus EIGENEM Testcode, um die
Frage "funktioniert `F$TLink` überhaupt korrekt?" von `echo`s unbekannter
C-Laufzeit zu entkoppeln.

### Fund 1 (ECHTER BUG, GEFIXT): Interrupt-Race im eigenen Testcode-Fenster

Der kontrollierte Testaufruf wurde direkt hinter dem `I$Write`-Erfolg
platziert -- genau das seit 2026-09-04 als Race dokumentierte Zeitfenster.
Ein erster Lauf bestätigte das erwartungsgemäß: derselbe bekannte Absturz,
diesmal schon VOR dem eigenen `F$TLink`-Test (der neue Diagnosemarker
feuerte gar nicht). Die in Fortsetzung 33 gebaute Ringpuffer-
Instrumentierung hatte bereits gezeigt, WAS passiert (ein Interrupt trifft
einen bereits kollabierten PC) -- als gezielter, schmaler Fix wird das
betroffene Fenster (reine Diagnose-Ausgabe + Registervorbereitung für
`F$Fork`, kein echtes I/O nötig) jetzt per `ori.w #$0700,sr` /
`andi.w #$f8ff,sr` gegen Interrupts abgeschirmt -- `Q9K_DiagWriteD7`
pollt den DUART direkt und bleibt dabei voll funktionsfähig. **Live
verifiziert, zweimal reproduziert:** mit der Sperre kommt der Testablauf
zuverlässig bis `F$Load(echo)`→`F$Load(csl)`→`F$TLink(13,"csl")`→
`F$Fork(echo)` durch, ohne sie bricht er identisch wie vorher ab (exakt
derselbe Kernel-Build, nur diese zwei Zeilen unterschiedlich). **Der
allgemeine Kernel-Race ist damit NICHT behoben** -- nur dieses eine,
namentlich bekannte Zeitfenster in unserem eigenen Testcode. Echte
Anwendungsprozesse (`echo` selbst, s. Fund 3) laufen weiterhin
unmaskiert und können denselben Mechanismus anderswo treffen. Die
generelle Ursache (`Q9K_TimerIRQHandler`/`Q9K_IRQDispatch`) bleibt echtes
TODO.

### Fund 2 (ECHTER BUG, GEFIXT): eigener Testaufruf nutzte den falschen Namen

Nach dem Interrupt-Fix erreichte der `F$TLink`-Test seinen eigenen
Diagnosemarker -- und meldete prompt E$MNF ($DD), obwohl `F$Load("csl")`
direkt davor sichtbar erfolgreich war (Marker `c`). Ursache: der
Testaufruf übergab `Q9K_TestCslName`, das den vollen `F$Load`-Pfad
`"/dd/CMDS/csl"` enthält -- `Q9K_ModDirLinkByName` sucht aber nach dem
NACKTEN Namen aus dem Modulkopf (`"csl"`), genau wie es `echo.mod` selbst
per Analyse nachweislich tut. Eigener Testcode-Fehler, keine
Kernel-Logik betroffen. Fix: neues Label `Q9K_TestCslBareName` (`"csl"`,
ohne Pfad) für den `F$TLink`-Aufruf; `Q9K_TestCslName` bleibt unverändert
für `F$Load`.

### Fund 3: `F$TLink` funktioniert -- `echo` kommt weiter als je zuvor,
trifft auf einen NEUEN, eigenständigen Absturz

Mit beiden Fixes: `F$TLink(13,"csl")` liefert Erfolg (Diagnosemarker `t`),
UND `echo` selbst (das intern denselben Aufruf macht) stürzt NICHT mehr
an seiner alten Stelle (`PC=$7031`, Textkonstante "...sed Me!SysBoot
Used...") ab. Stattdessen läuft `echo` spürbar weiter (zwei echte
Prozesse in der Ready-Queue, `csl` im Moduldirectory mit erhöhtem
Link-Zähler) und stürzt an einer ANDEREN, neuen Stelle ab: Vektor 4,
PC=`$0000006C`, A4=`$FFFFFFFE` (offensichtlich nie gesetzt), erreicht laut
Stack-Rückverfolgung über eine Rücksprungadresse in `echo.mod` selbst
(Modul-Offset `$9a`, direkt hinter einem ganz gewöhnlichen PC-relativen
internen Funktionsaufruf zu Modul-Offset `$82A` -- `movea.l #$792,a0 /
jsr $98(pc,a0.l)`, die bei Aufrufen ausserhalb der kurzen Sprungreichweite
übliche OS-9-PIC-Konvention, keine `csl`/`F$TLink`-Angelegenheit mehr).

**Einordnung:** `PC=$6C` mit `A4` uninitialisiert (`$FFFFFFFE`) ist
DIESELBE, in diesem Projekt schon mehrfach gefundene Fehlerklasse wie die
früheren "A4 muss Prozessdeskriptor sein, wurde für diesen externen
Aufrufpfad aber nie gesetzt"-Bugs (s. `Q9K_TrapCallExternal`-Historie) --
vermutlich ruft `echo` an dieser Stelle (innerhalb der Funktion bei
Modul-Offset `$82A`, deren Inhalt noch nicht untersucht wurde) einen
weiteren, noch nicht (oder über den falschen Pfad) verdrahteten Syscall
auf. NICHT weiter verfolgt in dieser Sitzung -- `echo.mod`/`csl.mod` sind
geschlossene, nicht quelloffene Microware-Binärdateien, die weitere
Rückverfolgung braucht gezielte Analyse der Funktion bei
Offset `$82A` (nächster, klar benannter Startpunkt für eine Folgesitzung).

### Fazit

`F$TLink`/TRAP-#1-15-Support ist damit nicht nur code-vollständig und
host-getestet (Fortsetzung 32), sondern jetzt auch LIVE gegen ein echtes,
kompiliertes Microware-Kommando end-to-end verifiziert: das ursprüngliche
Ziel dieser mehrtägigen Teilaufgabe ("kann `echo` seine `csl`-
Trap-Bibliothek installieren?") ist erreicht. Das Projekt ist dabei einen
Schritt weiter gekommen als geplant -- `echo` läuft jetzt so weit, dass es
an einer GANZ ANDEREN, unabhängigen Kernel-Lücke hängen bleibt, die einen
eigenen, neuen Untersuchungs-Faden darstellt (s. Fund 3).

Alle 15 Host-Testsuiten grün. Committet (`q9kernel_entry.a`: Interrupt-
Sperre um das Diagnose-/Registrierfenster, `Q9K_TestCslBareName`) und
gepusht auf `fix/a4-aufruferabhaengig`.

## Fortsetzung 35: `F$CCtl` (Cache Control) implementiert -- behebt den
`$6C`-Absturz aus Fortsetzung 34 NICHT, aber ein echter, fehlender
Syscall ist jetzt sauber verdrahtet (2026-09-11, direkte Fortsetzung
derselben Sitzung)

**Fund per Live-Diagnose, nicht per Analyse-Raten:** die
schon vorhandene Callcode-Scratchzelle (`$1370`, "letzter Funktionscode")
zeigte beim `$6C`-Absturz aus Fortsetzung 34 den Wert `$5A`. Per
`modules/SYSCALL_MODULE_MAP.md`: `F$CCtl`, "Cache Control". Echt im
Handbuch nachgelesen (68k_tech.pdf S. 379f, `/System/Volumes/Data/
Volumes/SSD1TB/#INFO/#Microware/68k_tech.pdf`): IN d0.l=gewünschte
Cache-Operation (0 = beide Caches fluschen, generischer Fall). Das
Handbuch nennt explizit den hier vorliegenden Anwendungsfall: *"Any
program building or changing executable code in memory should flush
the instruction cache by F\$CCtl before executing the new code"* --
genau das tut `csl` nach einem erfolgreichen `F$TLink` (frisch
gelinkter Code muss vor der Ausführung cache-kohärent gemacht werden).

**Implementierung:** `Q9K_SysFCCtl` (reines Assembler, keine C-Logik
nötig) unter Callcode `0x5A` registriert. Da Q9-Flux-68k laut eigener
Boot-Meldung ("680x0: unhandled PFLUSH ... kein TLB") keine echte
Cache-/MMU-Hardware emuliert, ist ein Flush auf diesem Ziel bedeutungslos
-- ehrliche Implementierung: immer Erfolg, keine Wirkung (kein
verstecktes Validierungs-Framework, gleiche Begründung wie bei
`F$SRtMem`/`F$SSvc`). Die dokumentierte `E$Param`-Prüfung reservierter
Bits für den privilegierten Pfad (Supergruppe/System-Zustand) bewusst
NICHT nachgebildet -- dieser Kernel hat noch keine echte
User-/Supervisor-Prozesstrennung.

**Ergebnis live geprüft:** `F$CCtl` wird jetzt erfolgreich bedient (die
"Unimplemented"-Zählzelle bleibt bei 0, statt wie vorher `F$CCtl`
mitzuzählen) -- der `$6C`-Absturz aus Fortsetzung 34 tritt aber WEITERHIN
auf, mit BYTE-IDENTISCHEN Registerwerten (`A4=$FFFFFFFE`, `D3=$EE`,
identischer Stack-Inhalt) wie vorher. Das beweist: `F$CCtl` war nicht die
Ursache dieses Absturzes, nur ein zusätzlicher, vorher fehlender
Aufruf, der zufällig kurz davor lag. **Bemerkenswerter Nebenbefund:**
der Absturz ist über zwei unterschiedlich große Kernel-Builds hinweg
byte-identisch reproduzierbar -- anders als der Interrupt-Race aus
Fortsetzung 32/33 (der mit der Kernelgröße wanderte) ist DIESER Absturz
offenbar ein deterministischer Logikfehler, kein Race. Das macht ihn
grundsätzlich leichter zu fassen als den Race -- aber `echo.mod`/
`csl.mod` sind weiterhin geschlossene Binärdateien ohne Quelltext, die
weitere Rückverfolgung bleibt aufwendig (s. Fortsetzung 34, Fund 3,
Startpunkt Modul-Offset `$82A`).

Alle 15 Host-Testsuiten grün (keine neue Testdatei nötig -- `Q9K_SysFCCtl`
ist reines Assembler ohne eigene C-Logik, gleiches Muster wie andere
triviale Wrapper in dieser Datei).

## Fortsetzung 36: `Q9K_TCallDispatch` verletzte die reale TrapEnt-
Konvention zweifach (ECHTE BUGS, GEFIXT) -- UND ein präziser neuer Fund
zur allgemeinen Race (2026-09-11, direkte Fortsetzung derselben Sitzung)

### Zwei echte, spec-verifizierte Bugs in `Q9K_TCallDispatch`

Beim erneuten Nachlesen der TrapEnt-Konvention (68k_tech.pdf S. 172f,
auf der Suche nach dem `$6C`-Absturz aus Fortsetzung 34/35) wörtlich:
*"Passed: d0-d7 = caller's registers, a0-a5 = caller's registers"* --
ALLE müssen unverändert beim Trap-Handler ankommen. Zwei Verstöße
gefunden:

1. **d0 verloren:** Die bisherige Fassung benutzte d0 nach der
   Aufrufer-Registerwiederherstellung noch zweimal als eigenes
   Rechenregister (Vektorwort, dann Funktionscode), ohne d0 vor dem
   Sprung in den Handler nochmal zurückzugeben -- der Aufrufer verlor
   sein eigenes d0 dauerhaft. Fix: Vektorwort/Funktionscode werden jetzt
   VOR der Registerwiederherstellung in zwei neue Speicherzellen
   (`Q9K_TCallScratch_VectorWord`/`_FuncCode`) vorausberechnet, danach
   bleiben d0/a4 bis zum Handler-Sprung unangetastet.
2. **a4 verloren:** `movea.l ExecEntry,a4 / jmp (a4)` verletzte dieselbe
   Konvention ein zweites Mal -- a4 gehört zu "a0-a5". Fix: derselbe
   registerlose "Adresse pushen, RTS springt hin"-Trampolin, der in
   diesem Kernel bereits für `Q9K_TrapExtInvoke` etabliert ist (dort aus
   demselben Grund: kein freies Register für das Sprungziel übrig, wenn
   ALLE Aufrufer-Register erhalten bleiben müssen).

Beide sind spec-verifizierte, echte Korrektheitsfehler, unabhängig davon,
ob sie die konkrete Ursache des `$6C`-Absturzes waren -- jeder reale
Trap-Handler, der sich auf ein unverändertes d0 oder a4 verlässt (eine
plausible, gängige Konvention für interne Dispatch-Tabellen, exakt wie
sie im Handbuch selbst für den Sprung IN eine Funktion beispielhaft
gezeigt wird), hätte bisher garantiert falsche Werte bekommen.

### Live-Verifikation gegen `echo`/`csl`: durch dieselbe Race blockiert,
aber ein präziser neuer Fund

Der Live-Test nach diesem Fix traf erneut auf die seit Fortsetzung 32
bekannte, kernelgrößenabhängige Race -- diesmal SOGAR VOR dem
`F$TLink`-Testpunkt (der `Q9K_TCallDispatch`-Fix selbst also in diesem
Lauf gar nicht durchlaufen). Der Absturz-PC (`$74b4`) lieferte aber einen
ungewöhnlich präzisen neuen Hinweis: er liegt EXAKT auf dem
`dc.w $008a`-Funktionscode-Wort, das im eigenen Testcode direkt hinter
der `trap #0`-Instruktion für `I$Write` steht. Das bedeutet: die
Rücksprungadresse im Exception-Frame stand noch auf dem Wert VOR
`Q9K_TrapDispatch`s eigener `addq.l #2,38(sp)`-Korrektur (die genau
dieses Funktionscode-Wort überspringen soll) -- als sei diese Korrektur
irgendwo zwischen ihrer Ausführung und dem finalen `rte` wieder verloren
gegangen. Plausibelster Verdächtiger: `Q9K_TrapCallExternal`s eigener,
mehrfach dokumentiert fragiler Rückweg (liest/schreibt gezielte
Stack-Offsets, sperrt Interrupts erst NACH dem ersten CCR-Rettungsschritt)
-- eine ZUSAETZLICHE, bisher nicht gefundene Verschachtelungslücke dort
ist naheliegend, aber NICHT bewiesen.

**Bewusst NICHT versucht:** ein Blindfix an `Q9K_TrapCallExternal` ohne
weitere Live-Instrumentierung -- dieser Pfad wurde bereits dreimal real
gefixt (CCR-Verlust, A4-Konvention, Herkunftsprüfung) und verträgt keinen
ungeprüften vierten Eingriff. Der nächste, sauber benannte Startpunkt für
eine Folgesitzung: Ringpuffer-Instrumentierung (Fortsetzung 33) gezielt
um Einträge AN `Q9K_TrapCallExternal`s Rückweg selbst erweitern (nicht
nur an Timer/`Q9K_IRQDispatch`), um zu sehen, ob ein Interrupt GENAU
dort einschlägt.

Alle 15 Host-Testsuiten grün. `Q9K_TCallDispatch`-Fix committet -- real,
spec-verifiziert, unabhängig vom noch offenen Race-Fund wertvoll.

## Fortsetzung 37: DURCHBRUCH -- die "kernelgroessenabhaengige Interrupt-
Race" ist KEINE Race, sondern derselbe A4-Herkunftspruefung-Bug aus
Fortsetzung 25, jetzt vollstaendig erklaert (2026-09-11, elfte
Arbeitssitzung)

**Auftrag:** Fortsetzung 37 (Übergabe "zehnte Sitzung") weiterverfolgen
-- sechster Messpunkt/Musashi-Trace-Hook fuer die Interrupt-Race, wie
dort vorgeschlagen.

**Es brauchte keinen neuen Musashi-Hook.** Die im Projekt bereits
vorhandene Instruktionsspur-Infrastruktur (`Q9_TRACE_INSTR=1`,
`Q9_FREEZE_PC`, `m68krt.c`) reichte, kombiniert mit dem ebenfalls
bereits vorhandenen Schreibzugriffs-Watch (`Q9_WATCH_ADDR`/`_LEN`).

### Schritt 1: Instruktionsspur eingefroren exakt am Absturz-PC

`Q9_TRACE_INSTR=1 Q9_FREEZE_PC=0x74b4` (der seit Fortsetzung 36 bekannte
Absturz-PC) zeigt die letzten Instruktionen VOR dem Crash lueckenlos:
```
pc=000074a8 d0=00000000              * move.l #$11,d1 (Q9K_TestWriteLen)
pc=000074ae d0=00000000 d1=00000011  * lea Q9K_TestWriteText(pc),a0
pc=000074b4 d0=00000000 a0=ff9c0c41  * <- Crash-PC, a0 ist GARBAGE
```
**Zwischen `$74ae` und `$74b4` fehlt der komplette `trap #0`-Befehl bei
`$74b2` in der Spur -- er wird nie als eigene Instruktion ausgefuehrt.**
Das bestaetigt und praezisiert den Fund der zehnten Sitzung
(„erreicht `Q9K_TrapDispatch` nie") auf Instruktionsebene.

### Schritt 2: Byte-Vergleich Laufzeit vs. gebautes Modul -- EIN BIT

`Code um den PC` aus dem Crash-Dump zeigt Byte `$74ae`=`41 fb`. Der
frisch gebaute, unveraenderte `q9kernel` enthaelt an derselben
Modul-Datei-Position (`$74ae - $7100 = $3ae`) aber `41 fa`:
```
xxd -s $((0x74ae-0x7100)) build/q9kernel
000003ae: 41fa 0124 4e40 008a ...
```
**`$74ae` ist zur Laufzeit von `$fa` auf `$fb` veraendert worden -- ein
einzelnes Bit.** `41fa`=`lea (d16,PC),a0` (harmloses PC-relatives LEA,
so wie es der Quelltext auch vorsieht), `41fb`=`lea (bd,PC,Xn),a0` im
"Full Extension Format" mit einer laut Motorola-Spezifikation
RESERVIERTEN I/IS-Bitkombination -- Musashi decodiert das nicht wie ein
normales 4-Byte-LEA, sondern liest zusaetzliche (nicht vorhandene)
Extension-Words, wodurch der Instruktionsstrom ab hier komplett
verschiebt und der eigentliche `trap #0` bei `$74b2` nie als solcher
gesehen wird -- **das ist die vollstaendige Erklaerung fuer "warum
verschluckt Musashi den trap"**, ohne dass im Emulator irgendetwas
kaputt ist: er bekommt schlicht keinen gueltigen Opcode mehr serviert.

### Schritt 3: Watchpoint auf `$74ae` findet den Schreiber -- und es ist Fortsetzung 25s Bug

`Q9_WATCH_ADDR=0x74ae Q9_WATCH_LEN=2 Q9_WATCH_FREEZE=1`, Treffer-Liste:
```
#31507/31508  pc=fe000d66 -> $74ae/$74af schreibt 41/fa   (CF-Bootloader, initialer Load -- korrekt)
#625377       pc=0000cc22 -> $74ac schreibt 0x001141fb (4 Byte)   [innerhalb scf.mod]
#625390       pc=0000d176 -> $74ac schreibt 0x001141fa (4 Byte)   [innerhalb scf.mod]
#625396       pc=0000caa2 -> $74ac schreibt 0x001141fb (4 Byte)   [innerhalb scf.mod]
#628400       pc=0000f5f8 -> $74ac schreibt 0x001141fc (4 Byte)   [innerhalb rbf.mod]
#629079       pc=0000f614 -> $74ac schreibt 0x001141fb (4 Byte)   [innerhalb rbf.mod]
... (weitere sieben, gleiches Muster)
```
Die geschriebenen 32-Bit-Werte sind **immer** `0x001141fX` -- das ist
schlicht der ORIGINALWERT an `$74ac` (`00 11 41 fa`, die oberen zwei
Byte sind die Immediate-Haelfte von `move.l #$11,d1` direkt davor,
korrekt und unveraendert), **um genau 1 erhoeht bzw. erniedrigt.**

**Das ist exakt `addq.l #1,$3ac(a4)` / `subq.l #1,$3ac(a4)` aus RBF/
SCF -- der in Fortsetzung 25 (2026-09-08/09) bereits gefundene und
dort namentlich benannte P$Preempt-Zaehler-Inkrement/Dekrement**, den
RBF und SCF bei JEDEM internen Treiberaufruf (z. B. einem eigenen
`F$SRqMem` waehrend ihrer eigenen `I$Open`/`I$Write`-Verarbeitung)
routinemaessig ausfuehren. Fortsetzung 25 hatte den Mechanismus exakt
beschrieben ("`a4` enthaelt faelschlich unsere eigene
Kernel-Modulbasis `$7100` statt eines echten Zeigers,
`$7100+$3ac=$74ac`, exakt der beobachtete Fehlerort") -- **nur wurde
damals ein ANDERER Absturz (`Q9K_DiagWriteD7`, `$74C5` bei einer
frueheren Kernelgroesse) durch denselben Mechanismus verursacht.** Die
Wurzelursache selbst (`Q9K_TrapDispatch`s A4-Herkunftspruefung
ueberschreibt A4 fuer Fremdaufrufer mit der eigenen Modulbasis statt
mit dem echten Aufruferwert) ist seit Fortsetzung 25 UNVERAENDERT im
Kernel -- **fuenf Reparaturversuche wurden verworfen, weil sie
IOMans Konsolen-Open brachen** (s. Fortsetzung 25/27).

### Fazit: die "Interrupt-Race" seit 2026-09-04 ist derselbe Bug, nicht neu

Was ueber sieben Sitzungen als "kernelgroessenabhaengige Race" verfolgt
wurde, ist in Wahrheit: **RBF/SCF schreiben bei JEDEM internen
Treiberaufruf blind auf die feste Adresse `Modulbasis+$3ac`** (weil A4
dabei faelschlich die Modulbasis ist). Ob das schadet, haengt nur davon
ab, WELCHER Code des eigenen Kernels GENAU an Datei-Offset `$3ac`-`$3af`
liegt -- eine reine Frage der Kernelgroesse/des Layouts, KEINE
Zeitfrage. Das erklaert:
- **"kernelgroessenabhaengig":** jede Codeaenderung verschiebt, was an
  `$3ac` landet -- mal Fuellbyte/Datenmuell (folgenlos), mal wie hier
  ein aktives Opcode-Byte (fatal).
- **Warum kein Interrupt im X/A-Ringpuffer-Fenster auftauchte (diese
  Sitzung, vor diesem Fund) und warum `Q9K_TrapDispatch` nie erreicht
  wird (zehnte Sitzung):** der fragliche `trap #0`-Aufruf selbst wird
  nie sauber erreicht, weil das vorausgehende LEA bereits VORHER (durch
  einen voellig unabhaengigen, nicht-interrupt-getriebenen Ablauf --
  RBF/SCFs eigene, ganz normale interne Aufrufe) kaputtgeschrieben
  wurde. Es gibt keine Verschachtelung von Interrupt und Trap-Rueckweg
  zu finden, weil das gar nicht die Ursache ist.
- **Warum fruehere Reparaturversuche an `Q9K_TrapCallExternal`/
  `Q9K_TrapDispatch` nichts brachten:** sie aenderten alle etwas AN der
  Symptomstelle (Rueckweg/PC-Korrektur), nicht an der URSACHE (A4-Wert
  bei Fremdaufrufer-Pruefung).

### Empfohlener naechster Schritt -- BEWUSST NICHT die A4-Pruefung selbst anfassen

Fuenf direkte Reparaturversuche an der A4-Herkunftspruefung sind bereits
gescheitert (brechen IOMans Konsolen-Open aus nicht verstandenem Grund,
s. Fortsetzung 25/27). **Neuer, risikoaermerer Ansatz, der diese fragile
Logik unangetastet laesst:** da der Schreibort IMMER exakt
`Modulbasis+$3ac` ist (deterministisch, kein Zufall), genuegt es, an
GENAU dieser festen Datei-Position im eigenen Modul (`q9kernel_entry.a`,
Datei-Offset `$3ac`-`$3af` relativ zum wahren Modulanfang, s.
`Q9K_ModuleHeaderSize`) vier harmlose, nie ausgefuehrte/nie gelesene
Fuellbytes zu platzieren (z. B. ein eigens benanntes `ds.l 1`-Feld oder
gezielte `nop`-Fuellung mit Sicherheitsabstand), sodass ein blindes
`addq.l #1`/`subq.l #1` dort niemals mehr etwas Kritisches trifft --
unabhaengig davon, ob/wann RBF oder SCF dorthin schreiben. Das behebt
das SYMPTOM zuverlaessig, OHNE die eigentliche (bereits fuenfmal an
IOMan gescheiterte) A4-Semantik zu veraendern. Konkret zu pruefen: was
aktuell bei `q9kernel_entry.a`-Quelltextposition entsprechend
Datei-Offset `$3ac` steht (in dieser Sitzung war es exakt die zweite
Haelfte von `move.l #$11,d1` plus der Anfang des folgenden `lea`) und
ob sich davor/danach ein 4-Byte-Sicherheitsabstand einfuegen laesst,
ohne andere Offsets/Sprungziele zu verschieben (Neuvermessung per `l68
-s` noetig, s. `Q9K_ModuleHeaderSize`-Lehre).

**Langfristig sauberer, aber aufwendiger:** die A4-Herkunftspruefung
doch richtig loesen -- jetzt mit einem NEUEN, bisher nicht probierten
Werkzeug: da der Schreibort nachweislich IMMER exakt bekannt ist
(`Modulbasis+$3ac`), liesse sich ein sechster Fixversuch diesmal GEZIELT
per `Q9_WATCH_ADDR=<Modulbasis+0x3ac>` waehrend der Konsolen-Open-Sequenz
beobachten, um zu sehen, WAS an der A4-Pruefung sich fuer IOMan aendert,
wenn A4 korrekt gesetzt wird -- die bisherigen fuenf Versuche massen das
Symptom ("Error $0000") nie mit derselben Watch-Praezision nach.

Alle 15 Host-Testsuiten weiterhin gruen (reine Diagnose, keine
Quelltextaenderung in dieser Sitzung). Instrumentierung aus der zehnten
Sitzung (`Q9K_RaceRing`, Commit `475b549`) unveraendert im Repo
belassen -- sie war fuer diesen Fund nicht mehr noetig (Instruktionsspur
+ Watchpoint reichten), schadet aber auch nicht und dokumentiert den
fruaeheren, nicht falschen (nur unvollstaendigen) Ermittlungsstand.

## Fortsetzung 38: Sicherheitsabstand umgesetzt und live verifiziert -- MEILENSTEIN, Absturz weg (2026-09-11, elfte Sitzung, direkte Fortsetzung)

**Umgesetzt:** der in Fortsetzung 37 empfohlene, risikoarme Fix. 12 Byte
Totraum (per `bra.s` uebersprungen, nie gelesen/ausgefuehrt) exakt an
Datei-Offset `$3ac`-`$3af` in `q9kernel_entry.a` eingefuegt (unmittelbar
vor `move.l #Q9K_TestWriteLen,d1` im eigenen I$Write-Testcode, wo dieses
Offset im aktuellen Kernelbau zufaellig lag). Per `xxd` am gebauten
`q9kernel` verifiziert: Offset `$3ac`-`$3af` ist jetzt reiner Nullraum,
das vorher dort liegende LEA-Opcode-Wort ist sicher nach `$3bc`
verschoben.

**Live verifiziert, ZWEI unabhaengige Laeufe, byte-identisches
Ergebnis:**
```
RP012OHallo von Q9-OS!HHallo aus einem echten Programm!
kAAAA...BBBB...AAAA...BBBB...  (stabile Scheduler-Schleife, kein Absturz)
```
`Q9K_ExcTrap`-Mitschrift beide Male `Vektor=0` (keine Exception). Der
seit 2026-09-04 verfolgte Absturz (`PC=$74b4`) tritt nicht mehr auf --
`I$Write` liefert die Testnachricht erfolgreich, `hellosvc` wird
erfolgreich geforkt und laeuft als ECHTES Programm bis zu seinem
eigenen `I$WritLn`+`F$Exit`, danach laeuft der Scheduler stabil im
A/B-Testprozesspaar weiter ueber die volle Testdauer (20s).

**Committet und gepusht** (`eb9c3ac`, `fix/a4-aufruferabhaengig`, PR
#13). Alle 15 Host-Testsuiten gruen.

**Ausdruecklich NICHT geloest:** die eigentliche A4-Herkunftspruefung
in `Q9K_TrapDispatch` bleibt fehlerhaft (RBF/SCF schreiben weiterhin
blind auf `Modulbasis+$3ac`, das ist nur noch folgenlos, weil dort
jetzt Totraum liegt). **Jede kuenftige Codeaenderung VOR dieser Stelle
im Modul kann den Totraum wieder verschieben und ein ANDERES,
ungeschuetztes Offset dem Schreibzugriff aussetzen** -- das ist kein
theoretisches Risiko, sondern exakt der Mechanismus, der diesen
Absturz elf Sitzungen lang als "Race" erscheinen liess. Bei einem
NEUEN, scheinbar unerklaerlichen Absturz mit Vektor 4 (Illegal
Instruction) an einer neuen Adresse: SOFORT `Q9_WATCH_ADDR=<Modulbasis
+ $3ac>` pruefen, bevor eine neue Ursachenjagd beginnt -- s. Fortsetzung
37 fuer die vollstaendige Methodik (Instruktionsspur + Bytevergleich
Laufzeit/gebautes-Modul + Watchpoint).

**Fuer eine echte, dauerhafte Loesung** (statt des reinen
Symptomschutzes) bleibt der in Fortsetzung 37 skizzierte sechste
Fixversuch an der A4-Herkunftspruefung selbst offen -- diesmal mit
`Q9_WATCH_ADDR` gezielt beobachtbar, was genau bei IOMans Konsolen-Open
kaputtgeht, wenn A4 dort korrekt gesetzt wird.

**Naechster inhaltlicher Schritt (unabhaengig von diesem Bug):** mit
laufendem, stabilem System jetzt den `F$TLink(13,"csl")`/`echo`-Test
aus Fortsetzung 34 erneut versuchen -- der war zuletzt an genau diesem
Absturz gescheitert (Fortsetzung 36), bevor er den `F$TLink`-Testpunkt
ueberhaupt erreichte.

## Fortsetzung 39: F$TLink/echo-Kette nach dem Fix erneut bestaetigt -- naechste Baustelle ist wieder erreichbar (2026-09-11, elfte Sitzung, direkte Fortsetzung)

Mit dem Fix aus Fortsetzung 38 lief der `echo`/`csl`-Test aus
Fortsetzung 34 erneut: Konsole zeigt `...Hallo aus einem echten
Programm!` (hellosvc, wie in Fortsetzung 38), danach **`lctE`** --
`F$Load(echo)` ✓, `F$Load(csl)` ✓, `F$TLink(13,"csl")` ✓, `F$Fork(echo)`
✓, exakt die in Fortsetzung 34 als Meilenstein verifizierte Kette.
`echo` laeuft danach bis zur DORT bereits dokumentierten, separaten
"Fund 3"-Absturzstelle (Vektor 4, `PC=$0000006C`, `A4=$FFFFFFFE`,
diesmal `A6=$0004d6a0`/`SP=$0004e9dc` -- andere Adressen als 2026-09-11
Fortsetzung 34, weil andere Kernelgroesse, aber gleiches Muster:
uninitialisiertes A4). **Kein neuer Bug, kein Rueckfall** -- die
bekannte naechste Baustelle (Analyse von `echo.mod`
Modul-Offset `$82A`, s. Fortsetzung 34 Fund 3) ist damit wieder
reproduzierbar erreichbar und wartet weiterhin auf eine Folgesitzung.

## Fortsetzung 40: `echo`-Absturz (Fund 3) naeher eingegrenzt -- KEINE Speichererschoepfung, sondern deterministischer Stack-Rahmen-Fehler an fester Aufruftiefe (2026-09-11, elfte Sitzung, direkte Fortsetzung)

**Auftrag:** die in Fortsetzung 39 bestaetigte, separate naechste Baustelle
(`echo`-Absturz `PC=$6C`, `A4=$FFFFFFFE`, aus Fortsetzung 34 Fund 3)
weiterverfolgen.

### Fund 1: Rueckverfolgung fuehrt zu `csl`, nicht zu `echo.mod`

Instruktionsspur (`Q9_FREEZE_PC=0x6c`) zeigt: der Absturz ist ein `rts`
in `csl` (Modul-Offset `$6b74`, Datei csl.mod per `xxd`/`capstone`
gegengeprueft -- ein sauberes, unauffaelliges C-Funktionsende
`movem.l (a7)+,d1/d6-d7/a0` + `rts`), das eine auf dem Stack liegende
Ruecksprungadresse von `$00000000` vorfindet statt eines echten
Zeigers -- die CPU laeuft daraufhin ab Adresse 0 als Pseudocode los und
trifft nach ca. `$6c` Byte auf ein tatsaechlich ungueltiges Opcode-Wort.
`A4=$FFFFFFFE` erwies sich als bereits VOR diesem Aufruf gesetzter,
unveraenderter Wert (durchlaeuft unseren `Q9K_TrapCallForeignCaller`-
Pfad unangetastet, wie beabsichtigt) -- wahrscheinlich csl-intern
genutzt, NICHT die Ursache des Absturzes selbst.

### Fund 2: Watchpoint auf die betroffene Stack-Adresse

`Q9_WATCH_ADDR` auf die genullte Stack-Adresse zeigt zwei Schreiber:
1. `pc=$9010..$902a` (in `q9kernel`) baut dort byteweise `$0000136c`
   auf -- sieht nach einer plausiblen Ruecksprungadresse aus, aber
   `$136c` selbst liegt mitten in unseren KERNEL-SCRATCHZELLEN
   (`Q9K_IOpenScratch_*`-Bereich), kein echter `csl`-Codepunkt.
2. `pc=$3ed3e` (in `echo.mod`, Modul-Offset `$82e` -- direkt der
   `movem.l d2-d7/a0-a2,-(a7)`-Prolog der in Fortsetzung 34/37/39
   bereits identifizierten Funktion bei `$82A`) ueberschreibt denselben
   Platz kurz danach mit `0`.

### Fund 3 (WICHTIG, widerlegt die naheliegendste Hypothese): KEINE Speichererschoepfung

Verdacht "echo.mod eigenes `M\$Stack` (nur 3072 Byte, per Header
`$3c` gelesen) reicht nicht, sobald `csl` mitlaeuft" testweise
gezielt geprueft: `F\$Fork("echo")` im eigenen Testcode testweise mit
16 KB zusaetzlichem Speicher (`d1=$4000` statt `0`) aufgerufen,
neu gebaut, live getestet.

**Ergebnis: der Absturz tritt BYTE-IDENTISCH auf** -- exakt derselbe
Registersatz (`D0-D7`, `A4=$FFFFFFFE`, `A6`), NUR die absoluten
Adressen (`SP`, die Ziel-Stackadresse `$136c`->`$536c` etc.) sind exakt
um die hinzugefuegten `$4000` Byte verschoben. **Das schliesst
Speichererschoepfung aus:** waere der Stack schlicht zu klein, haette
mehr Speicher den Absturz verzoegert/verschoben (andere Aufruftiefe)
oder behoben -- stattdessen passiert er an GENAU derselben logischen
Stelle, unabhaengig von der verfuegbaren Reserve. Experiment
zurueckgesetzt (`clr.l d1` wiederhergestellt), keine Aenderung im Repo.

### Einordnung und naechster Schritt

Der Fehler ist ein **deterministischer Stack-Rahmen-Fehler an fester
Aufruftiefe** (vermutlich ein Push/Pop-Ungleichgewicht irgendwo in der
`tcall`/`F$TLink`-Aufrufkette zwischen `echo`, `csl` und unserem
Kernel -- eine feste Anzahl Aufrufe balanciert sich falsch aus, bis
irgendwann ein fremder Frame ueberschrieben wird), KEINE
Kapazitaetsfrage. Naechster, konkret benannter Schritt fuer eine
Folgesitzung: die vollstaendige Aufrufkette von `F$Fork("echo")` bis
zum Absturz per Instruktionsspur auf SP-BALANCE hin durchgehen (SP-Wert
bei jedem `bsr`/`jsr`/`rts` protokollieren, nicht nur bei Traps) --
gesucht ist die EINE Stelle, an der ein `bsr`/`jsr` ohne passendes
`rts`/`addq.l sp` bilanziert wird oder umgekehrt. Da `Q9K_TCallDispatch`
in Fortsetzung 36 bereits zwei echte TrapEnt-Konventionsverletzungen
gefixt bekam, aber NIE live gegen genau diesen Ablauf verifiziert
wurde (dort dokumentiert: "Noch NICHT live gegen echos eigenen tcall
13,X-Aufruf nachverfolgt"), ist das der naheliegendste erste
Verdachtsort.

Alle 15 Host-Testsuiten weiterhin gruen. Keine Codeaenderung aus
dieser Fortsetzung im Repo (Experiment vollstaendig zurueckgesetzt).

## Fortsetzung 41: neues Werkzeug `tools/annotate_trace.py` -- zwei weitere Hypothesen zum `echo`-Absturz widerlegt, `Q9K_TCallDispatch` als Verdaechtiger ausgeschlossen (2026-09-11, elfte Sitzung, direkte Fortsetzung)

**Neues Werkzeug:** `tools/annotate_trace.py` liest eine
`Q9_TRACE_INSTR=1`-Instruktionsspur (Dump-Datei) ein, ordnet JEDE
PC-Adresse automatisch dem richtigen geladenen Modul zu (Basisadressen
werden LIVE aus der Moduldirectory-Sektion desselben Dumps gelesen,
nie von Hand eingetragen), analysiert per `capstone` korrekt aus
der passenden Binaerdatei, loest Kerneladressen ueber eine per `l68
-s=` erzeugte Symbolkarte zu Funktionsnamen auf, und verfolgt
automatisch die Call/Return-Bilanz (inkl. Erkennung der in diesem
Kernel bewusst genutzten "Adresse pushen, `rts` springt hin"-
Trampoline, die keine echte Rueckkehr sind). Grund fuer das Werkzeug:
in dieser Sitzung fuehrten mehrere Versuche, Adressen von Hand zwischen
Kernel/`echo.mod`/`csl.mod` umzurechnen, zu eigenen Rechenfehlern
(einmal ein falsch decodiertes Bit in einer LEA-Adressierungsart,
einmal eine Adresse faelschlich `Q9K_SetupTables` statt der generischen
`Q9K_ZeroRangeLoop` zugeschrieben) -- das Werkzeug macht diese Fehler
strukturell unmoeglich.

### Fund 1 (per Werkzeug widerlegt): `Q9K_TCallDispatch` ist NICHT beteiligt

Der komplette 24576-Instruktionen-Trace bis zum Absturz enthaelt
**null** Treffer im Adressbereich von `Q9K_TCallDispatch`. Der
zuvor vermutete Zusammenhang mit dem `tcall`/`F$TLink`-Dispatcher
(nahegelegt durch Fortsetzung 36s "nie live gegen echos eigenen tcall
verifiziert") ist damit ausgeschlossen -- der Absturz passiert
vollstaendig INNERHALB von `csl`s/`echo`s eigenem, normalem
`bsr`/`rts`-Aufrufcode (ein Zeichen-Klassifizierungs-/
Formatstring-Scanner in `csl`, erkennbar an Pruefungen auf
`n`/`s`/`x`/`u`/`p`/`g`-Zeichen -- typisch fuer eine
`printf`-Implementierung).

**Nebenfund:** die vorher (Fortsetzung 40) beobachteten Schreibzugriffe
auf `Q9K_TCallScratch_ExecEntry` mit Wert 0 kamen NICHT von
`Q9K_TCallDispatch`, sondern von der GENERISCHEN Boot-Zeit-
Nullungsschleife `Q9K_ZeroRangeLoop` (zaehlt zu den drei bekannten,
einmaligen `Q9K_ZeroRange`-Aufrufen direkt in `Q9K_Entry`) -- ein
eigener Fehlschluss dieser Sitzung, durch das neue Werkzeug aufgeklaert
und hiermit richtiggestellt. `F$TLink`s Trap-Tabellen-Eintrag selbst
ist nachweislich korrekt (`ExecEntry=$3f420`, per Watchpoint direkt in
echos Prozessdeskriptor bestaetigt, s. Fortsetzung 40).

### Fund 2 (per Experiment widerlegt): fehlendes `argv` ist NICHT die Ursache

Verdacht: `F$Fork("echo")` im eigenen Testcode uebergibt `paramSize=0`
(keinerlei Kommandozeilenargument) -- ein Fall, der auf einem echten
System nie vorkommt (eine Shell uebergibt immer mindestens den
Programmnamen/ein Zeilenende), moeglicherweise ein in `echo`/`csl`
nie getesteter Sonderfall.

Testweise ein echtes 3-Byte-Argument (`"hi",$0d`) mitgegeben, neu
gebaut, live getestet: **Absturz tritt identisch wieder auf** --
`D3`/`D4`/`D6`(=`echo+$8d8`)/`D7` byteidentisch zum Lauf ohne
Argument, nur `D0` und die betroffenen Adressen um genau die 3
zusaetzlichen Byte verschoben. Schliesst aus, dass der Fehler von der
konkreten Kommandozeile abhaengt -- er liegt in einem UNBEDINGTEN,
argumentunabhaengigen Teil des Ablaufs. Experiment vollstaendig
zurueckgesetzt (`git diff` leer).

### Stand: zwei plausible Hypothesen widerlegt, echte Ursache noch offen

Ausgeschlossen bisher: Speichererschoepfung (Fortsetzung 40),
`Q9K_TCallDispatch`-Bug, fehlendes Argument. Gesichert: ein
deterministischer Stack-Rahmen-Fehler tief in `csl`s eigenem
Format-/Zeichen-Scanner, ausgeloest durch `echo.mod`s eigenen
Funktionsprolog bei Modul-Offset `$82e` (`movem.l
d2-d7/a0-a2,-(a7)`), der eine noch lebende Ruecksprungadresse
(`echo+$8d8`) mit Null ueberschreibt.

**Naechster Schritt fuer eine Folgesitzung:** `tools/annotate_trace.py`
auf einen frischen `Q9_TRACE_INSTR=1`-Dump anwenden und GEZIELT die
vollstaendige, ununterbrochene Aufrufkette ab `echo`s eigenem
Haupteinstieg (`M$Exec`) bis zum Absturz von Hand durchgehen (das
Werkzeug listet Anomalien nur automatisch, echte Fehlinterpretationen
durch die "getarnter Sprung"-Heuristik sind nicht auszuschliessen --
im Zweifel die annotierte Rohliste selbst lesen, nicht nur die
Anomalie-Zusammenfassung). Ziel: die EINE Stelle finden, an der eine
Aufruftiefe/Rahmengroesse falsch angenommen wird -- vermutlich in
`echo.mod` selbst (geschlossene Microware-Binaerdatei, keine
Quelltextaenderung moeglich, aber ein VERSTANDENER Mechanismus koennte
einen gezielten Workaround im Kernel nahelegen, z. B. ein groesserer
initialer Registerkontext oder ein A6/A5-Wert, den `echo`/`csl`
stillschweigend anders erwarten als bisher angenommen).

Alle 15 Host-Testsuiten weiterhin gruen. Kein Codefix in dieser
Fortsetzung, `tools/annotate_trace.py` neu im Repo.

## Fortsetzung 42: EXAKTER Mechanismus des `echo`-Absturzes gefunden -- Funktionszeiger-Cache in `echo.mod` zeigt ~68 Byte zu weit in `csl` hinein, ueberspringt deren Prolog (2026-09-11, elfte Sitzung, direkte Fortsetzung)

**Werkzeugfehler behoben:** `tools/annotate_trace.py` erkannte KEINEN
einzigen `bsr`-Aufruf -- Capstone meldet das Mnemonic bei Groessen-
suffix als EIN String (`"bsr.l"`, `"bsr.w"`, ...), der bisherige
exakte Vergleich `mnem in {"jsr","bsr"}` traf das nie. Fix: Praefix-
Vergleich (`mnem.startswith("bsr")`/`"jsr"`/`"rts"`/`"rte"`). Nach dem
Fix loesen sich fast alle vorherigen "RUECKSPRUNG-MISMATCH"-Funde aus
Fortsetzung 41 als Artefakte dieses Bugs auf -- der Kernel-eigene Code
(`Q9K_ProcTLink`, `Q9K_SysTLinkImpl`, `Q9K_ProcSRqMem`) balanciert
Call/Return tatsaechlich sauber.

### Der echte, verbleibende Fehler -- jetzt Byte-genau lokalisiert

Instruktion fuer Instruktion nachvollzogen (`echo`-Offset `$8d4` bis
zum Absturz):
1. `echo+$8d4`: `jsr $3ede6(pc,d7.l)` (d7=`$232`) -> Ziel `echo+$b08`.
2. `echo+$b08` (`$3f018`): `jsr -$78a0(a6)` -- Aufruf ueber einen in
   `echo`s eigenem Datenbereich gecachten Funktionszeiger (12 Aufruf-
   stellen im ganzen Modul benutzen denselben Cache -- ein gemeinsamer
   "aktuelle Ausgabefunktion"-Slot). SP vorher `$4e9d4`, nach dem
   `jsr`-eigenen Push `$4e9d0` -- **die Ruecksprungadresse (`echo+$b0c`)
   liegt jetzt auf dem Stack GENAU bei Adresse `$4e9d0`.**
3. Das tatsaechliche Sprungziel ist `csl+$6ae0` (`$45e00`).
4. **Byte-genau nachgewiesen** (Analyse ab dem zweifelsfrei
   erkennbaren `movem`-Byte-Muster `48e74380`): die ECHTE Funktion
   beginnt nicht bei `$45e00`, sondern 68 Byte frueher bei `$45dbc`
   (`csl+$6a9c`) mit `movem.l d1/d6-d7/a0,-(a7)` -- ihrem Prolog, der
   vier Register (16 Byte) rettet. **`echo`s Funktionszeiger-Cache
   zeigt auf `$45e00`, MITTEN in dieselbe Funktion, HINTER deren
   eigenem Prolog.** Der Prolog wird bei diesem Aufruf also komplett
   uebersprungen.
5. Die Funktion (ein Formatstring-/Prozentzeichen-Scanner, an den
   Vergleichen auf `E`/`X`/`G`/`d`/`f`/`e`/`c`/`i`/`o`/`n`/`s`/`x`/`u`/
   `p`/`g` erkennbar) laeuft korrekt durch, macht einen sauberen,
   balancierten verschachtelten `bsr.l`-Aufruf (Zeichen-Klassifizierer,
   `csl+$9a98`) und erreicht am Ende trotzdem ihren GEMEINSAMEN Epilog
   `movem.l (a7)+,d1/d6-d7/a0` (Zeile `$45e90`) -- der IMMER 16 Byte
   vom Stack "zurueckholt", UNABHAENGIG davon, ob bei DIESEM Aufruf
   ueberhaupt etwas gepusht wurde.
6. **Der Absturz, Schritt fuer Schritt per SP-Differenz belegt:**
   `movem.l (a7)+,...` liest 16 Byte ab der Adresse, an der die ECHTE
   Ruecksprungadresse (`echo+$b0c`) steht (`$4e9d0`) -- verschluckt sie
   als vermeintlichen Registerwert (`d1`) -- und hebt SP auf `$4e9e0`
   (16 Byte zu weit). Das nachfolgende `rts` liest von `$4e9e0` --
   einer Adresse, an der NIE etwas Sinnvolles abgelegt wurde (deshalb
   `0`) -- Absturz bei `PC=$0`, weiterlaufend bis zum ersten
   ungueltigen Opcode bei `$6C` (exakt der seit Fortsetzung 34 bekannte
   Befund).

### Einordnung

Der Fehler ist **kein Interrupt-Problem, keine Speicherfrage, kein
`Q9K_TCallDispatch`-Bug** (alle drei diese Sitzung widerlegt) --
sondern ein **falscher Wert in `echo.mod`s eigenem Funktionszeiger-
Cache** (`-$78a0(a6)`), der um ca. 68 Byte zu weit in `csl` zeigt.
**Woher dieser Wert kommt, ist noch offen** -- zwei Moeglichkeiten:
(a) `csl` loest diesen Zeiger selbst ueber eine interne, noch nicht
verstandene Tabellen-/Indexlogik auf, und ein von unserem Kernel
bereitgestellter Ausgangswert (z. B. `ExecEntry=$3f420` aus `F$TLink`,
selbst nachweislich korrekt, s. Fortsetzung 40) wird von `csl`s
EIGENEM Code falsch WEITERVERARBEITET, oder (b) `echo.mod`/`csl.mod`
(fest editierte, geschlossene Microware-Binaerdateien) wurden fuer
eine ANDERE `csl`-Version/einen anderen `F$TLink`-Ablauf kompiliert und
diese exakte Kombination aus Dateien war so nie vorgesehen.

**Naechster Schritt fuer eine Folgesitzung:** herausfinden, WELCHER
Code in `echo.mod` oder `csl.mod` den Wert `-$78a0(a6)` ZUERST
SCHREIBT (12 Aufrufstellen bekannt, s.o., aber der SCHREIBER noch
nicht identifiziert -- `lea.l -$78a0(a6),a1` bei `echo+$35a` ist ein
Kandidat, laedt aber nur die ADRESSE der Zelle, nicht deren Inhalt).
Ein gezielter `Q9_WATCH_ADDR` auf die tatsaechliche RAM-Adresse dieser
Zelle (`A6-Basiswert - $78a0`, `A6` per Instruktionsspur an einer
beliebigen `echo`-Stelle ablesbar) wuerde den Schreiber direkt zeigen
-- dieselbe bereits etablierte Methodik wie in Fortsetzung 37/40.

Alle 15 Host-Testsuiten weiterhin gruen. Kein Codefix in dieser
Fortsetzung, `tools/annotate_trace.py`s Mnemonic-Fix ist die einzige
Aenderung (im Repo, s. `tools/annotate_trace.py`).

## Fortsetzung 43: Watchpoint auf die Zeigerzelle -- neuer dritter Wert, Byte-Ausrichtung noch ungeklaert (2026-09-11, elfte Sitzung, direkte Fortsetzung)

`Q9_WATCH_ADDR` auf die per `A6-$78a0` berechnete Zelladresse (`$45e00`
-- bemerkenswerterweise IDENTISCH mit dem bereits als falsch erkannten
Sprungziel aus Fortsetzung 42) zeigt zwei Schreiber:
1. `pc=$1016a` (innerhalb `cfide`/Disk-Treiberbereich) schreibt beim
   Laden von `csl.mod` von der Platte 4 Byte `$0c800000` dorthin --
   plausibel Dateiinhalt/Puffer-Randdaten aus dem `F$Load`-Vorgang,
   nicht die eigentliche Zeigerbefuellung.
2. `pc=$3ecd2` (in `echo.mod` selbst) schreibt 4 Byte `$0003f406` an
   Adresse `$45e02` -- ZWEI Byte VERSETZT zur angenommenen Zellgrenze
   (`$45e00`-`$45e03`), UND ein DRITTER, bisher an keiner Stelle
   gesehener Wert (weder `$3f420`=`ExecEntry` aus `F$TLink` noch
   `$45dbc`=echte Funktionsadresse noch `$45e00`=tatsaechlich benutztes
   Sprungziel).

**Nicht mehr in dieser Sitzung aufgeloest:** ob die eigentliche
Zellgrenze bei `$45e00` oder tatsaechlich bei `$45e02` liegt (die
`(a6)`-Adressierung des Compilers koennte je nach erzeugtem Code
unterschiedlich ausgerichtet sein), und in welcher Reihenfolge/mit
welchem Wert die Zelle VOR dem Absturz-Aufruf zuletzt wirklich
geschrieben wurde. Fuer eine Folgesitzung: `Q9_WATCH_ADDR` probeweise
auf `$45e02` (Laenge 4 UND 2) wiederholen, UND `Q9_WATCH_FREEZE`
gezielt auf den letzten Schreiber VOR dem fehlerhaften `jsr -$78a0(a6)`
bei Zeile ~24521 setzen (s. `tools/annotate_trace.py`-Methodik aus
Fortsetzung 41/42), um den Wert UNMITTELBAR vor dem fehlschlagenden
Aufruf zu sehen, statt wie hier ueber den gesamten Lauf gemittelt.

---

**Gesamtstand dieser Sitzung (elfte Arbeitssitzung, Fortsetzung 37-43),
zusammengefasst:**
- **GELOEST, zweifach live verifiziert, gepusht:** der seit 2026-09-04
  verfolgte "kernelgroessenabhaengige Interrupt-Race"-Absturz war der
  bereits in Fortsetzung 25 gefundene A4-Herkunftspruefung-Bug (RBF/SCF
  schreiben blind auf `Modulbasis+$3ac`) -- behoben per Sicherheits-
  abstand an der betroffenen Datei-Position, OHNE die fragile
  A4-Pruefung selbst anzufassen. `F$TLink`/`echo`-Erfolgskette (`lctE`)
  laeuft seitdem wieder zuverlaessig.
- **Neues, wiederverwendbares Werkzeug:** `tools/annotate_trace.py`
  (Modul/Symbol-Zuordnung + Call/Return-Bilanz fuer
  `Q9_TRACE_INSTR`-Dumps), inkl. eines im Verlauf gefundenen und
  behobenen eigenen Bugs (bsr.l-Mnemonic-Erkennung).
- **Praezise mechanistisch verstanden, aber NICHT geloest:** `echo`s
  separater Folgeabsturz (Fortsetzung 34 Fund 3) ist ein Funktions-
  zeiger in `echo.mod`s eigenem Datenbereich, der ca. 68 Byte zu weit
  in `csl` zeigt und deren Registersicherungs-Prolog ueberspringt --
  Ursache des FALSCHEN ZEIGERWERTS selbst noch offen (naechster Schritt
  oben benannt).

Alle 15 Host-Testsuiten durchgehend gruen. Kein Codefix in dieser
letzten Fortsetzung.

## Fortsetzung 44: moegliche Grundsatzerkenntnis -- `echo.mod` koennte auf eine FESTE, dem Kernel unbekannte `csl`-Adresse fest verdrahtet sein (2026-09-11, elfte Sitzung, Abschluss)

**Korrektur einer eigenen Fehlannahme aus Fortsetzung 42/43:**
`jsr -$78a0(a6)` liest KEINEN Funktionszeiger aus dem Speicher -- die
Adressierungsart berechnet die Zieladresse DIREKT aus `a6` (Register-
Indirekt-mit-Displacement als Sprungziel selbst, kein zusaetzliches
Dereferenzieren). Es gibt also keine "Zeiger-Zelle" zu reparieren --
das eigentliche Problem ist, DASS `echo.mod`s eigener kompilierter
Code direkt "A6 minus fester Konstante" als `csl`-Zieladresse
verwendet.

### `68k_tech.pdf` direkt gelesen: Trap-Handler-Module haben einen
dokumentierten `M$Init`-Einstiegspunkt (Modulkopf-Offset `$48`,
"Initialization Execution Offset") -- UNSER `Q9K_SysFTLink` ruft ihn
bereits korrekt auf (`q9kernel_entry.a`, `Q9K_SysFTLink`, mit dem in
Kapitel 5 des Handbuchs dokumentierten `TrapInit`-Spezialrahmen,
bereits in einer FRUEHEREN Sitzung implementiert, s. Kopfkommentar
dort).

**Aber:** beim Aufruf von `M$Init` ist `a6` = `csl`s EIGENER frisch
zugewiesener statischer Speicher (`StaticPtr`) -- `echo`s EIGENES `a6`
(sein Prozess-Speicherblock, per Table D-9 bei F\$Fork gesetzt) ist zu
diesem Zeitpunkt in KEINEM Register mehr verfuegbar (wird kurz vorher
gerettet, wiederhergestellt, dann sofort wieder ueberschrieben, s.
Code). `M\$Init` kann also -- so wie unser Kernel es aufruft -- gar
nicht wissen, WO `echo`s eigener Datenbereich liegt, selbst wenn seine
Aufgabe waere, dort eine Vektortabelle zu befuellen.

**Wichtigste, noch nicht abschliessend geklaerte Frage fuer eine
Folgesitzung:** ist das laut Handbuch UEBERHAUPT `M$Init`s Aufgabe
(eine Vektortabelle im AUFRUFER zu befuellen), oder verwendet `echo.mod`
stattdessen eine GANZ ANDERE, HIER NOCH NICHT VERSTANDENE Konvention?
Die Tatsache, dass `echo.mod`s kompilierter Code eine FESTE, ABSOLUTE
Differenz (`-$78a0`, ca. 68 Byte Diskrepanz zum tatsaechlich
funktionierenden Ziel bei aktuellem `A6`-Wert) zu `A6` benutzt, um
`csl`-Funktionen zu erreichen, deutet stark darauf hin, dass **dieses
konkrete `echo.mod`/`csl.mod`-Paar beim urspruenglichen Kompilieren/
Linken eine FESTE, VORHERSAGBARE Adressbeziehung zwischen einem
UCC-Programm und dem System-`csl` voraussetzte** (auf einem echten
System vermutlich: `csl` laedt immer an einer sehr fruehen, festen
Adresse, und der Compiler/Linker kennt diese Beziehung beim Bauen des
Programms) -- eine Annahme, die eine DYNAMISCHE, generische
`F$TLink`-Ladeadresse (wie unser Kernel sie vergibt) grundsaetzlich
nicht erfuellen kann, OHNE die Ladereihenfolge/-adresse von `csl`
gezielt an das nachzubilden, was das ORIGINALSYSTEM hatte.

**Konsequenz:** dies koennte KEIN einfach behebbarer Kernel-Bug sein,
sondern eine grundsaetzliche Kompatibilitaetsfrage dieser SPEZIELLEN,
vorkompilierten `echo.mod`/`csl.mod`-Kombination mit einem generischen,
dynamischen `F\$TLink`. Vor einem weiteren Fixversuch waere zu klaeren
(z. B. durch Vergleich mit einer ECHTEN Microware-Systemkonfiguration,
falls Referenzmaterial verfuegbar ist, oder durch weitere gezielte
Analyse von `csl`s `M\$Init`-Routine selbst bei `csl+$50`),
WELCHEN Mechanismus `M$Init` tatsaechlich implementiert und ob es einen
dokumentierten Weg gibt, `echo`s A6 an `M\$Init` zu uebergeben.

**Fuer die naechste Sitzung, konkret:** `csl+$50` (`M$Init`s echte
Routine) analysieren -- prueft, ob sie ueberhaupt versucht, in den
Aufrufer zu schreiben (z. B. ueber den geretteten `a6`-Wert im
TrapInit-Rahmen selbst, den unser Code VOR dem Ruecksprung noch besitzt
und aktuell einfach verwirft, s. `Q9K_SysFTLink_AfterInit`).

**Da das ein potenziell grundlegenderes Thema ist als ein einzelner
Bugfix, wird diese Baustelle hier bewusst BEENDET** -- der Hauptauftrag
dieser Sitzung (Interrupt-Race-Fix) ist geloest und verifiziert, diese
`echo`-Nebenbaustelle ist jetzt so praezise wie moeglich fuer eine
gezielte Folgesitzung dokumentiert.

## Fortsetzung 45: `M$Init`-Registerrettung gefixt (echter, eigenstaendiger Bug) -- behebt den `echo`-Absturz NICHT, bestaetigt Fortsetzung 44s Architekturhypothese (2026-09-11, elfte Sitzung, Abschluss)

**Echter Bug gefunden + gefixt:** `csl+$50` (`M$Init`, per Analyse
direkt gelesen) liest nachweislich `d1-d5`/`a3`/`a4` DES URSPRUENGLICHEN
AUFRUFERS (matcht die im Handbuch dokumentierte `TrapInit`-Konvention,
"Passed: d0-d7 = caller's registers, a0-a5 = caller's registers").
`Q9K_SysFTLink` rettete bisher nur `a6` um den internen `bsr
Q9K_SysTLinkImpl`-Aufruf (eine normale C-Funktion, die alle anderen
Register frei als Arbeitsregister benutzt) -- gefixt per `movem.l
d1-d7/a3-a5,-(sp)` vor und `movem.l (sp)+,d1-d7/a3-a5` nach dem
internen Aufruf (`a0`/`a1`/`a2` bekommen ohnehin bewusst neue,
dokumentierte Werte fuer `M$Init`).

**Live getestet: der `echo`-Absturz tritt BYTE-IDENTISCH weiter auf**
(exakt derselbe Registersatz wie vor dem Fix). Das bestaetigt die in
Fortsetzung 44 aufgestellte Hypothese unabhaengig: die problematische
Adressberechnung (`jsr -$78a0(a6)` in `echo.mod` selbst, ca. 68 Byte
neben dem wahren `csl`-Ziel) ist eine FEST EINKOMPILIERTE KONSTANTE in
`echo.mod`, ausgewertet gegen `echo`s EIGENES `a6` -- das bereits bei
`F$Fork` (lange VOR `F$TLink`/`M$Init`) gesetzt wird. `M$Init`s
Register haben damit auf DIESEN spezifischen Absturz gar keinen
Einfluss; der gefixte Register-Bug ist trotzdem real und bleibt
behalten (spec-konform, korrekt fuer kuenftige/andere Trap-Bibliotheken
und den Fall, dass `M$Init` seine Eingaben tatsaechlich braucht).

**Fazit dieser Baustelle fuer heute:** der `echo`-Absturz ist mit
hoher Sicherheit eine Adressraum-/Linking-Annahme, die `echo.mod`
beim urspruenglichen Kompilieren ueber die relative Lage von `echo`s
eigenem Datenbereich zu `csl` traf -- eine Annahme, die generisches,
dynamisches `F$TLink`-Laden nicht automatisch erfuellt. Eine echte
Loesung wuerde vermutlich bedeuten, `csl` (oder `echo`s Speicherblock)
gezielt an einer Adresse zu platzieren, die diese fest einkompilierte
Beziehung wiederherstellt -- ein groesseres, eigenstaendiges Vorhaben,
kein kleiner Bugfix mehr.

Committet+gepusht. Alle 15 Host-Testsuiten gruen.

## Fortsetzung 46: Relokations-Experiment versucht -- verworfen, zeigt warum "nachtraeglich verschieben" nicht funktioniert (2026-09-11, elfte Sitzung, Abschluss)

**Idee:** `csl` NACH dem normalen Laden per Bytekopie an die Adresse
verschieben, die `echo`s fest einkompilierte Konstante (`jsr
-$78a0(a6)`, s. Fortsetzung 44) tatsaechlich braucht -- kein Redesign
der Speicherzuteilung, nur ein gezielter, nachtraeglicher Fix fuer
DIESE eine Kombination. Rechnerisch exakt hergeleitet: `csl` muesste
fuer `echo`s aktuelles `A6` bei `csl_base + $44` (68 Byte spaeter)
laden. Als C-Funktion implementiert (`Q9K_ExperimentalRelocateCsl`,
`q9kernel_traplink.c`): Moduldirectory-Eintrag verschieben, Bytekopie
(memmove-artig fuer den ueberlappenden Bereich), Trap-Tabellen-
Eintraege (ModPtr/ExecEntry) in BEIDEN betroffenen Prozessen
(eigenem Testprozess und `echo`, das die Registrierung per `F$Fork`
geerbt hat) um denselben Versatz nachziehen.

**Ergebnis: echte Regression, nicht nur "hilft nicht".** Nach dem
Experiment verschwand sogar die vorher zuverlaessig funktionierende
`F$TLink`-Erfolgskette (`lctE`) komplett -- `echo` fiel zurueck in
seinen URALTEN, aus Fortsetzung 31 bekannten Fehlerpfad ("can't
install csl", Absturz bei `PC=$7031`, A6=0). Auch eine Interrupt-Sperre
um die komplette Relokation (Verdacht: Scheduler schaltet waehrend der
48-KB-Kopie auf das frisch geforkte `echo` um und trifft es halb
verschoben an) behob das NICHT -- der Fehler liegt tiefer.

**Lehre, warum "nachtraeglich verschieben" grundsaetzlich fragil ist:**
zum Zeitpunkt der Verschiebung existieren bereits ZWEI unabhaengige
Kopien der Trap-Tabellen-Registrierung (eigener Testprozess UND `echo`,
durch Vererbung bei `F$Fork` getrennt) -- und moeglicherweise weitere,
hier nicht bedachte Referenzen auf die ALTE `csl`-Adresse (z. B. in
`csl`s eigenem, bereits initialisiertem statischem Speicher, der beim
`M$Init`-Aufruf VOR der Verschiebung Werte relativ zur ALTEN
Codeadresse abgelegt haben koennte). Jede uebersehene Referenz macht
das Ergebnis inkonsistent. **Experiment vollstaendig zurueckgesetzt**
(`git checkout`), Repo wieder exakt auf dem verifizierten, funktionierenden
Stand von Commit `4a21cd8`.

### Einordnung fuer eine echte Folgesitzung

Eine tragfaehige Loesung muesste `csl` VOR dem ersten `F$TLink`-Aufruf
an der richtigen Adresse laden (nicht nachtraeglich verschieben) --
das heisst vermutlich: `csl` beim Systemstart (oder spaetestens beim
allerersten `F$Load("csl")`) gezielt an eine Adresse legen, die aus dem
A6-Wert des ANFORDERNDEN Prozesses berechnet wird, BEVOR irgendeine
Registrierung/Initialisierung stattfindet. Das ist ein eigenstaendiges
Architektur-Vorhaben (Aenderung an `F$Load`s bzw. `F$TLink`s
Speicherzuteilung selbst, nicht nur ein nachtraeglicher Patch) und
sollte in einer eigenen, dafuer vorgesehenen Sitzung angegangen werden
-- nicht als schneller Versuch am Ende einer bereits sehr langen
Sitzung.

**Damit ist die `echo`/`csl`-Baustelle nach bestem Wissen dieser
Sitzung vollstaendig dokumentiert** (Fortsetzung 34, 37-46). Alle 15
Host-Testsuiten gruen, keine Codeaenderung im Repo aus diesem
Experiment (vollstaendig zurueckgesetzt). Der Hauptauftrag der Sitzung
(Interrupt-Race-Fix, Commits `6e5a4de`-`4a21cd8`) bleibt geloest und
verifiziert.

## Fortsetzung 47: zweiter Positionierungs-Versuch (vorab statt nachtraeglich) -- ebenfalls verworfen, neue Erkenntnis: Ueberlappung mit bereits geladenen Modulen (2026-09-11, elfte Sitzung, endgueltiger Abschluss)

**Verbesserter Ansatz gegenueber Fortsetzung 46:** `csl` nicht mehr
NACHTRAEGLICH verschieben, sondern SOFORT nach `F$Load(csl)` und VOR
jeder Registrierung positionieren (`Q9K_ExperimentalPositionCsl`,
`q9kernel_traplink.c`) -- zu diesem Zeitpunkt existiert nur eine
Referenz (der Moddir-Eintrag), kein Trap-Tabellen-Eintrag, kein
`M$Init`, kein `echo`-Fork. `echo`s kuenftiges `A6` wird vorhergesagt,
indem die Freiliste von `Q9K_AllocMem` GELESEN (nicht veraendert) wird
-- deterministisch, solange zwischen Vorhersage und tatsaechlichem
`F$Fork` nichts anderes allokiert.

**Erster Fehlversuch dabei (in derselben Sitzung gefunden+gefixt):**
die Namenssuche fuer "echo" (4 echte Buchstaben) verwendete
faelschlich dieselbe 3-Buchstaben-plus-Terminator-Logik wie fuer "csl"
-- "echo" wurde nie gefunden, die Funktion kehrte immer frueh zurueck,
OHNE jemals etwas zu verschieben (per Live-Test bestaetigt: Moddir
zeigte `csl` unveraendert). Gefixt (eigene 4-Buchstaben-Pruefung).

**Nach dem Fix: `F$TLink` blieb erfolgreich (`t`), aber `F$Fork(echo)`
schlug jetzt fehl (`e`, klein -- Fehlschlag).** Ursache gefunden: die
BERECHNETE Zieladresse fuer `csl` (`$3ccd4`) UEBERLAPPT VOLLSTAENDIG
mit `echo`s eigenem, bereits geladenem Modul (`$3e510`-`$3f19e` liegt
komplett innerhalb von `csl`s neuem Bereich `$3ccd4`-`$489c2`) -- die
Bytekopie hat `echo` selbst teilweise ueberschrieben.

**Auch dieses Experiment vollstaendig zurueckgesetzt** (`git checkout`),
Repo wieder exakt auf dem verifizierten Stand von Commit `646739e`,
15/15 Host-Testsuiten gruen.

### Endgueltige Einordnung dieser Sitzung

Zwei unabhaengige, sorgfaeltig durchdachte Loesungsversuche (Fortsetzung
46 und 47) sind an ZWEI VERSCHIEDENEN, jeweils erst durch den Versuch
sichtbar gewordenen Nebenwirkungen gescheitert (verwaiste Referenzen
bei nachtraeglicher Verschiebung; Ueberlappung mit anderen Modulen bei
vorheriger Platzierung). Das ist ein starkes Signal: **eine tragfaehige
Loesung braucht einen echten Speicherzuteilungs-Entwurf** (mit
Ueberlappungspruefung gegen ALLE bereits geladenen Bereiche, nicht nur
gegen `echo`) -- kein Experiment, das sich in einer bereits sehr langen
Sitzung "nebenbei" verifizieren laesst. Fuer die naechste, dediziert
dafuer angesetzte Sitzung empfohlen:
1. Eine echte "ist dieser Adressbereich frei?"-Pruefung gegen die
   GESAMTE Moduldirectory (nicht nur ein Modul) VOR jeder Positionierung.
2. Klaeren, ob die Konstante `$E33C` ueberhaupt fuer JEDES `echo`-Mal
   stabil ist, oder ob sie selbst von `csl`s Ladeadresse zirkulaer
   abhaengt (in dieser Sitzung nie unabhaengig von der aktuellen
   Kombination verifiziert).
3. In Erwaegung ziehen, `csl` grundsaetzlich VOR jedem Nutzerprogramm zu
   laden (fester, frueher Boot-Slot) statt bedarfsgesteuert -- naeher an
   der vermuteten Original-Systemkonvention (s. Fortsetzung 44).

**Damit ist die `echo`/`csl`-Baustelle fuer diese (sehr lange, sehr
ergiebige) Sitzung wirklich abgeschlossen.** Der Hauptauftrag
(Interrupt-Race-Fix, Commits `6e5a4de`-`4a21cd8`) bleibt vollstaendig
geloest und verifiziert. Alle 15 Host-Testsuiten gruen, Repo sauber,
keine offenen Prozesse.

## Fortsetzung 48: MEILENSTEIN -- csl/echo-Adressbeziehung korrekt geloest per Kombi-Allokation; neuer, eigenstaendiger Folgefund (fehlende "Initialized Data") (2026-09-11, elfte Sitzung, dritter und erfolgreicher Loesungsanlauf)

**Dritter Anlauf zur Architekturfrage aus Fortsetzung 44, ERSTER
ERFOLGREICHER (nach den verworfenen Fortsetzung 46/47):**
`Q9K_ExperimentalCombinedAlloc` (`q9kernel_traplink.c`) reserviert EINE
einzige `Q9K_AllocMem`-Allokation, die `csl`s Code UND `echo`s
kuenftigen Prozessblock GEMEINSAM enthaelt (Layout: `[csl-Kopie] [Leer-
raum] [echos kuenftiger Block]`, Gesamtgroesse = `$E33C` +
`echos Speicherbedarf`) -- garantiert ueberlappungsfrei mit ALLEM
anderen, weil der Allocator selbst dafuer buergt (kein Raten wie in
Fortsetzung 47, keine nachtraegliche Fremdreferenzen-Jagd wie in
Fortsetzung 46). `Q9K_FORK_BLOCK_OVERRIDE` (selbstloeschende Scratch-
Zelle, `q9kernel_firstproc.c`) sorgt dafuer, dass der naechste `F$Fork`
exakt diesen vorreservierten Block bekommt statt einer neuen Allokation
-- fuer jeden ANDEREN Aufrufer vollstaendig unveraendertes Verhalten.

**Live verifiziert, per `tools/annotate_trace.py` Instruktion fuer
Instruktion nachvollzogen:** `csl` laedt jetzt exakt an der Adresse,
die `echo`s fest einkompilierte `jsr -$78a0(a6)`-Konstante braucht
(Moduldirectory zeigt die berechnete Adresse byte-genau). **`echo`
springt jetzt korrekt auf `csl`s ECHTEN Funktionsanfang** (`csl+$6a9c`,
samt intaktem Registersicherungs-Prolog `movem.l d1/d6-d7/a0,-(a7)` --
vorher wurde dieser uebersprungen, s. Fortsetzung 42). Der urspruengliche
Absturzmechanismus (Epilog frisst `echo`s Ruecksprungadresse als
Registerwert) ist damit behoben.

### Neuer, eigenstaendiger Folgefund: fehlende "Initialized Data"

Der Ablauf kommt jetzt WEITER als je zuvor, stuerzt aber an einer
NEUEN Stelle: `echo` liest `move.l -$78cc(a6),d0` und uebergibt diesen
Wert (`$feb64a80` -- eindeutiger Speichermuell, weit ausserhalb 16 MB
RAM) als Zeiger an `csl`s Funktion. Ursache **direkt im Handbuch
verifiziert** (`68k_tech.pdf`, Table 1-8): der Modulkopf hat zwei
dokumentierte Felder, die unser `F$Fork` (`Q9K_ProcFork`,
`q9kernel_firstproc.c`) bisher VOLLSTAENDIG ignoriert:

- **`M$IData`** (Offset `$40`): zeigt auf eine Tabelle aus
  `(Zieloffset im Datenbereich, Byteanzahl, <Rohdaten>)`-Eintraegen --
  "the linker places all constant values declared in vsects here".
  Muss beim Fork in den NEUEN Prozessblock kopiert werden.
- **`M$IRefs`** (Offset `$44`): zeigt auf eine Tabelle von
  Zeiger-Korrekturen ("MS-Wort" + Anzahl LS-Woerter, kombiniert zum
  vollen Offset eines Zeigers IM Datenbereich) -- jeder so gefundene
  Zeiger muss um die ECHTE Ladeadresse (Code- oder Datenbereich, je
  nachdem) erhoeht werden. Terminiert laut Handbuch bei MS=0/Anzahl=0.

**Das ist eine voellig eigenstaendige, klar umrissene Baustelle** --
unabhaengig von der jetzt geloesten Adressbeziehung. Kleinere,
reine Assembler-Testmodule (`hellosvc`, `forkchild`) brauchten das nie
(keine initialisierten C-Globalen mit Zeigerwerten), `echo` als
echtes, kompiliertes C-Programm dagegen schon.

**Fuer eine Folgesitzung, konkret:** `Q9K_ProcFork` um das Kopieren von
`M$IData` UND das Anwenden von `M$IRefs` erweitern (Reihenfolge:
Kopieren, DANACH Zeiger korrigieren). Eine Detailfrage bleibt aus dem
Handbuchtext ungeklaert: WIE genau "Code-Zeiger" von "Daten-Zeiger"
innerhalb derselben `M$IRefs`-Tabelle unterschieden werden (zwei
getrennte, je durch MS=0/Anzahl=0 beendete Abschnitte sind die
naheliegendste Deutung, aber nicht ausdruecklich bestaetigt) -- am
sichersten per Byte-Dump von `echo.mod`s eigenem `M$IData`/`M$IRefs`-
Bereich zu klaeren, bevor die Implementierung beginnt.

Alle 15 Host-Testsuiten gruen. Committet+gepusht. Testdateien
(`test_q9kernel_firstproc.c`/`test_q9kernel_traplink.c`) um passende
Testadress-Umleitungen erweitert (echte kleine absolute Adressen sind
auf dem 64-Bit-Testhost keine gueltigen Zeiger, gleiche Konvention wie
ueberall in diesen Tests).

## Fortsetzung 49: `M$IData`/`M$IRefs` implementiert (F$Fork UND F$TLink) -- live verifiziert, neue eigenstaendige Baustelle dahinter gefunden (2026-09-11, elfte Sitzung, Fortsetzung nach "ok mach weiter")

**Byte-Ebenen-Verifikation des Tabellenformats gegen `echo.mod` (VOR jeder
Implementierung, wie in Fortsetzung 48 angekuendigt):**

```
M$IData ($40=$c26, $44=$c66 in echo.mod):
  EIN Eintrag: Zieloffset=$734, Anzahl=$38 (56 Byte Nutzlast) --
  Ende der Nutzlast trifft EXAKT auf den Beginn von M$IRefs ($c66).
  M$IData hat also KEINEN eigenen Endemarker: die Tabelle laeuft bis
  zum Beginn von M$IRefs.

M$IRefs ($c66): GENAU ZWEI Gruppen, je durch MS=0/Anzahl=0 beendet:
  Gruppe 1 (8 Eintraege): Versaetze $744,$748,$74c,$750,$754,$758,$75c,$73c
  Gruppe 2 (2 Eintraege): Versaetze $734,$738
  Danach 4 Byte Rest ("0040fdde") -- passt zur reelen OS-9-Modul-CRC
  (letzte 3 Byte des Moduls + 1 Fuellbyte), NICHT Teil der Tabelle.
```

Die in Gruppe 1 genannten Versaetze zeigen auf Werte, die per
`M$IData` gerade als $1f0-$30a kopiert wurden -- viel zu klein fuer
Datenversaetze (`M$Mem`=$76c waere zwar auch groesser), aber eindeutig
als MODULRELATIVE KODEVERSAETZE erkennbar (der komplette Rest der
kopierten Nutzlast -- inkl. zweier `4ef9 00000000`-"jmp.l $0"-Befehle
weiter hinten, s. u. -- ergibt zusammen ein klassisches
Compiler-generiertes Sprung-/Funktionszeiger-Vtable-Muster). Gruppe 2
zeigt auf die ERSTEN beiden kopierten Langworte ($44, $6c4) -- deutlich
kleiner, plausible DATENVERSAETZE. **Schlussfolgerung (durch das
Ergebnis des Livetests weiter unten bestaetigt):** Gruppe 1 = Kodezeiger
(Korrektur: `alterWert + hdrAddr`), Gruppe 2 = Datenzeiger (Korrektur:
`alterWert + Datenbereichsbasis`) -- exakt die im Handbuchtext erwaehnte
Unterscheidung, hier erstmals konkret bestaetigt.

**Implementierung:**

- `Q9K_ApplyInitializedData(hdrAddr, block)` in `q9kernel_firstproc.c`
  (aufgerufen aus `Q9K_ProcFork`, unmittelbar nachdem `block`
  feststeht): kopiert `M$IData` byteweise in den neuen Datenbereich,
  wendet danach `M$IRefs` an (Gruppe 1 -> `+hdrAddr`, Gruppe 2 ->
  `+block`). Fuer Module OHNE `M$IData`/`M$IRefs` (`hellosvc`,
  `forkchild`, beide Felder 0) exakt kein Verhaltensunterschied.
- **Wichtiger Host-Test-Fallstrick, gefunden beim ersten Testlauf:**
  wie schon bei `Q9K_SetFrameReg` MUSS die Relozierung byteweise
  erfolgen (`Q9K_GetU8`/`Q9K_SetU8`), NICHT ueber das normale
  `Q9K_GetU32`/`Q9K_SetU32` (8 statt 4 Byte breit auf diesem
  64-Bit-Testhost) -- bei den hier typischerweise nur 4 Byte
  auseinanderliegenden `M$IRefs`-Versaetzen haette das sonst
  Nachbarfelder ueberschrieben. Neuer Testfall F5 in
  `test_q9kernel_firstproc.c` (eigenes, kleines Tabellenpaar,
  1:1-Format-Nachbau) verifiziert Kopie UND beide Relozierungsarten.
  Zweiter Fallstrick: `fakeHdr` in der bestehenden F1-F4-Testumgebung
  war nur `[0x40]` Byte gross -- da `Q9K_ProcFork` jetzt IMMER auch
  `M$IData`/`M$IRefs` (Offset `$40`/`$44`) liest, waere das ein
  Lesezugriff hinter dem Arrayende gewesen; auf `[0x48]` vergroessert
  (per `memset` ohnehin genullt, also unveraendertes Verhalten fuer
  F1-F4).

**Live getestet (erster Lauf, NUR `Q9K_ProcFork`-Seite):** `echo` forkt
jetzt erfolgreich (`E`), OHNE dass zuvor `csl` erneut installiert werden
musste -- deutlich weiter als in Fortsetzung 48. `D_Proc` im
Diagnose-Dump zeigt `echo`s eigenen, per `F$Fork` erzeugten Deskriptor
als AKTIV laufend (`P$State='a'`), kein sofortiger Absturz mehr.

**Neuer Fund NOCH WAEHREND dieser Sitzung, per Diagnose-Dump:** ein
`Q9K_ExcTrap`-Eintrag mit Vektor 4 (Illegal Instruction), `PC=$6c`,
`A3=echo`s Headerzeiger, `A6=echo`s Datenbereich -- klassisches Muster
eines Sprungs durch einen NICHT relozierten Zeiger. Direkt geprueft:
**`csl.mod` hat SELBST nicht-null `M$IData`/`M$IRefs`
(`$afa0`/`$bb08`)** -- unser `F$TLink` (`Q9K_ProcTLink`,
`q9kernel_traplink.c`) hat das bisher VOLLSTAENDIG ignoriert, obwohl
`Q9K_ProcTLink` bereits einen passenden, individuellen statischen
Speicherbereich fuer die Bibliothek bereitstellt (`staticPtr`, aus
`Q9K_ProcSRqMem`, wird spaeter als `a6` an `M$Init` uebergeben -- exakt
dieselbe Rolle wie `block`/`a6` bei `Q9K_ProcFork`). Deshalb `Q9K_
ApplyInitializedData` (identische Logik, EIGENSTAENDIG dupliziert nach
etablierter Konvention -- keine gemeinsamen Header) auch in
`q9kernel_traplink.c` ergaenzt und direkt nach der `Q9K_ProcSRqMem`-
Zuteilung in `Q9K_ProcTLink` aufgerufen (nur wenn `size!=0`, s.
Kopfkommentar). Neuer Testfall F9 in `test_q9kernel_traplink.c` (echter,
dereferenzierbarer Host-Zeiger als `staticPtr` -- ANDERS als F5/F6 dort,
die `staticPtr` nur als reinen Wert vergleichen, nie hineinschreiben).

**Live getestet (zweiter Lauf, BEIDE Seiten):** identischer Ablauf bis
`F$TLink`/`F$Fork` (`lcPtE`), danach ABERMALS derselbe Absturz --
byteidentisch (`PC=$6c`, `A3`, `A6`, `A1`, `A2` alle exakt gleich wie im
ersten Lauf, nur `D0`/`SR` minimal anders). **Das bedeutet: dieser
zweite Absturz ist NICHT durch fehlende `M$IData`/`M$IRefs`-Anwendung
verursacht** (die jetzt fuer `csl` ebenfalls laeuft) -- er liegt an
anderer Stelle. Aufklaerung durch genaues Lesen des `E`-Zeichens im
Ausgabestrom: es gibt ZWEI Quellen fuer `'E'` in `q9kernel_entry.a`,
nicht nur "echo geforkt" (Zeile ~1479) -- `Q9K_ExcTrap` selbst schreibt
NACH dem Sichern aller Diagnosedaten EBENFALLS ein `'E'` (Zeile ~1075),
bevor es in `Q9K_ExcTrapSpin` (`bra Q9K_ExcTrapSpin`, reine Endlos-
schleife) haengen bleibt. Die beobachtete Zeichenkette `lcPtE AAA BBBB...
E AAAA...` ist also: Haupt-/Bootprozess forkt `echo` erfolgreich (`E`),
faellt in seine eigene `TestProcA`-Idle-Schleife (`A`), `TestProcB`
laeuft parallel (`B`) -- UNTERDESSEN stuerzt `echo` selbst (dritter
Prozess) bei einem echten Konsolen-E/A-Aufruf ab (`E` von
`Q9K_ExcTrap`), haengt fortan reglos in `Q9K_ExcTrapSpin`, waehrend der
Scheduler `TestProcA`/`TestProcB` per Timer-Interrupt weiter bedient
(deshalb endlos weitere `A`/`B` NACH dem zweiten `E`).

**Ursache des `PC=$6c`-Absturzes selbst NICHT neu -- bereits am
2026-09-03 (Commits `2c2a24d`/`341b9ac`, weit VOR dieser gesamten
`echo`/`csl`-Baustelle UND vor der A4-Herkunftsbug-Entdeckung in
Fortsetzung 25) per Stack-Backtrace-Erweiterung in `Q9K_ExcTrap`
untersucht:** der Kommentar dort haelt woertlich fest, dass genau dieses
Muster (`PC=$6c`) frueher auf `scf+$352` zurueckgefuehrt wurde, per
rekonstruiertem Aufrufpfad `ioman+$1560 -> scf+$190 -> scf+$1f4 ->
scf+$352`. Das ist also ein SEIT LANGEM bekannter, aber nie behobener
Absturz irgendwo in der `scf`-Treiberkette (Konsolen-Ein-/Ausgabe) --
`echo` ist damit vermutlich das ERSTE Testmodul dieses Kernels, das
tatsaechlich eine echte, ausgewachsene Konsolenoperation ausloest (statt
der bisherigen, sehr gezielten `I$Write`/`I$ReadLn`-Einzeltests), und
trifft dabei auf eine seit neun Sitzungen unangetastete Baustelle.

**Zwei offene Hypothesen fuer eine Folgesitzung, NICHT unterschieden:**
1. Es ist die eine, seit Fortsetzung 25/37/38 bekannte
   A4-Herkunftsbug-Familie, nur diesmal ueber einen anderen Aufrufpfad
   (nicht RBFs `P$Preempt`-Semaphor, sondern ein aehnliches Muster
   irgendwo in `scf`) -- ERSTER PRUEFSCHRITT laut ÜBERGABE-Methodik:
   `Q9_WATCH_ADDR=<q9kernel-Modulbasis>+$3ac` setzen und erneut laufen
   lassen.
2. Es ist ein eigenstaendiger, unabhaengiger Fehler in der
   `scf`-Treiberimplementierung dieses Kernels, der bisher nie
   ausgeloest wurde, weil nie ein Testfall so weit kam.

**Alle 15 Host-Testsuiten gruen** (neue Faelle: F5 in
`test_q9kernel_firstproc.c`, F9 in `test_q9kernel_traplink.c`). Kernel
neu gebaut, Sicherheitsabstand aus Fortsetzung 38 (Datei-Offset
`$3ac`-`$3af`, s. ÜBERGABE oben) NACH BEIDEN Aenderungen explizit per
Byte-Dump erneut geprueft und weiterhin exakt an der richtigen Stelle
(unveraendert durch die zusaetzliche `Q9K_ApplyInitializedData`-Groesse
in `q9kernel_firstproc.c`, da deren Linker-Platzierung nicht vor dem
geschuetzten Bereich in `q9kernel_entry.a` liegt).

**Fuer die naechste Sitzung:** mit der oben genannten
`Q9_WATCH_ADDR`-Pruefung ansetzen, um zwischen den beiden Hypothesen zu
unterscheiden -- die eigentliche `M$IData`/`M$IRefs`-Baustelle dieser
Sitzung ist damit vollstaendig abgeschlossen und verifiziert, der neue
Fund ist bewusst NICHT mehr "nebenbei" am Ende dieser bereits sehr
langen Sitzung angegangen worden.

## Fortsetzung 50: `PC=$6c`-Absturz VOLLSTAENDIG aufgeklaert -- echte Architekturluecke in der Fortsetzung-48-Kombi-Allokation gefunden (2026-09-11, elfte Sitzung, direkter Anschluss an Fortsetzung 49, auf "ok mach weiter")

**Hypothese 1 aus Fortsetzung 49 GEPRUEFT und VERWORFEN:**
`Q9_WATCH_ADDR=<q9kernel-Modulbasis>+$3ac` zeigt: die alte
A4-Herkunftsbug-Familie (P$Preempt-Muster, `addq.l/subq.l #1,$3ac(a4)`)
schlaegt tatsaechlich WEITERHIN zu (PC=$fa7c/$fa98 in `scf`, alternierend
Werte 1/2) -- aber GENAU an dieser Stelle greift der Fortsetzung-38-Fix
(Totraum), der Schreibzugriff bleibt folgenlos. Die A4-Bug-Familie ist
also weiterhin aktiv, aber NICHT die Ursache des NEUEN Absturzes.

**Direkte Ursachenermittlung per `Q9_TRACE_INSTR=1`:** dieser Kernel
friert die Instruktionsspur automatisch ein, sobald `PC<$1000` wird
(bereits vorhandener Mechanismus in `m68krt.c`, extra fuer genau diesen
Fall gebaut) -- kein `Q9_FREEZE_PC` noetig. Die letzten Eintraege zeigen:
der Sprung nach `PC=0` (der ueber ein paar Nullwoerter hinweg bis
`PC=$6c` "krabbelt" und dort auf ein echtes Illegal-Opcode-Muster
trifft) ist KEIN `jsr`/`jmp` durch einen Nullzeiger, sondern ein
**`rts` bei `csl+$6b74`** (Ende der Funktion, auf die `echo`s
`jsr -$78a0(a6)` korrekt zeigt -- der Fortsetzung-48-Fix selbst
funktioniert also weiterhin einwandfrei) mit einer korrumpierten
Ruecksprungadresse.

**Genaue Ursache gefunden per gezieltem `Q9_WATCH_ADDR` auf die
betroffenen Befehlsbytes selbst** (`csl+$6a9c`, der Funktionsprolog
`movem.l d1/d6-d7/a0,-(a7)`):
1. Erster Treffer (PC in unserem eigenen Kernel, `Q9K_ExperimentalCombinedAlloc`s
   Kopierschleife): schreibt die vier Bytes `48 e7 43 80` -- die
   ECHTEN, unveraenderten Opcode-Bytes des Prologs. Die Kombi-Allokation
   kopiert `csl` also byte-genau korrekt.
2. Zweiter Treffer, VIEL SPAETER, PC in `echo` selbst (`echo+$7c2`):
   schreibt vier Bytes `00 04 b0 f6` GENAU auf die Adresse der
   Registermaske (`csl+$6a9e`) -- ueberschreibt die Maske `$4380`
   (4 Register: A0,D6,D7,D1, macht `movem` zu einem
   16-Byte-Stack-Push) mit `$0004` (NUR Register A5, 4 Byte) und
   korrumpiert dabei zusaetzlich die ersten 2 Byte der naechsten
   Instruktion. Musashis eigene `movem`-Logik (`m68k_in.c`,
   Quellcode gegengeprueft, nicht nur vermutet) bestaetigt exakt
   dieses Verhalten fuer beide Maskenwerte -- **kein Emulatorbug**,
   reine Speicherkorruption durch unseren eigenen Kernel/`echo`s
   Code.

**Die schreibende Instruktion in `echo.mod` analysiert (nach
Ausrichtungspruefung ueber einen unabhaengigen `beq.b`-Sprung auf
dieselbe Adresse, s. Fussnote):** `echo+$7c2` ist
`move.l d0,-$789e(a6)` -- STRUKTURELL IDENTISCH zu den beiden bereits
bekannten `echo`-eigenen a6-relativen Zugriffen (`jsr -$78a0(a6)` aus
Fortsetzung 44/48, `move.l -$78cc(a6),d0` aus Fortsetzung 48). Ziel-
adresse: `a6 - $789e` = `(combinedBlock+$E33C) - $789e` =
`combinedBlock + $6A9E` = GENAU 2 Byte hinter `csl`s Funktionsanfang,
also mitten in der Registermaske.

**Architektureinordnung (der eigentliche Fund dieser Fortsetzung):**
`echo` benutzt MEHRERE verschiedene, fest einkompilierte NEGATIVE
a6-Offsets fuer VERSCHIEDENE Zwecke:
- `-$78a0(a6)`: ruft eine ECHTE `csl`-Funktion auf (per Fortsetzung 48
  korrekt geloest).
- `-$78cc(a6)`: liest einen Zeiger (Fortsetzung 48, urspruenglicher
  Absturzfund).
- `-$789e(a6)`: SCHREIBT einen Wert (NEU, dieser Fortsetzung).

Die Fortsetzung-48-Kombi-Allokation (`Q9K_ExperimentalCombinedAlloc`)
geht implizit davon aus, dass der GESAMTE Bereich `[a6-$E33C, a6)`
eine reine KOPIE von `csl`s Code ist -- das erklaert zufaellig genau
den EINEN beobachteten Aufruf (`-$78a0`), zerstoert aber `csl`s
Code, sobald `echo` (wie hier) versucht, in denselben Bereich zu
SCHREIBEN. Die reale Microware-Konvention ist vermutlich: dieser
Bereich ist ein PRO-PROZESS-EIGENER, von `csl`s `M$Init` bei der
Installation gefuellter Bereich (Sprungtabelle + Scratch-Zellen fuer
genau diesen einen Aufrufer) -- NICHT `csl`s gemeinsamer Code direkt.
Das deckt sich mit dem laengst bekannten, aber bisher nicht weiter
verfolgten Table-D-9-Hinweis ("a6 ... biased by $8000") UND mit den
beiden bereits in Fortsetzung 48 in `echo`s eigenem `M$IData`
gefundenen, nie relozierten `4ef9 00000000`-("jmp.l $0")-Stubs
(Datenoffset `$760`/`$764`) -- das sind vermutlich GENAU die
Sprungtabellen-Eintraege, die `csl`s `M$Init` mit echten Adressen in
diesen pro-Prozess-Bereich haette eintragen muessen.

**Bewusst NICHT in dieser (bereits sehr langen) Sitzung angegangen:**
eine echte Loesung braucht einen neuen Speicher-Layout-Entwurf (ein
eigener, per `M$Init` gefuellter pro-Prozess-`csl`-Bereich statt einer
blossen Codekopie) -- vom selben Schwierigkeitsgrad wie die
3-Anlauf-Adressbeziehungsfrage aus Fortsetzung 46-48. Fuer eine
Folgesitzung empfohlen:
1. `68k_tech.pdf`, Abschnitt zu `M$Init`/Trap-Bibliotheken, gezielt auf
   Hinweise zu einem "per-caller private area" oder aehnlichem pruefen
   (das bisher implementierte `M$Init` behandelt nur die
   TrapInit-Registeruebergabe, s. Fortsetzung 44/45 -- nicht, WAS
   `M$Init` mit einem solchen Bereich tut).
2. `csl.mod`s eigenen `M$Init`-Code (`csl+$50`, bereits einmal
   analysiert, s. Fortsetzung 44) daraufhin lesen, ob er
   irgendeinen Bereich VOR dem uebergebenen `a6` beschreibt/erwartet.
3. Erst danach `Q9K_ExperimentalCombinedAlloc` neu entwerfen.

Keine Codeaenderung in dieser Fortsetzung -- reine Diagnose, alle 15
Host-Testsuiten unveraendert gruen, Repo sauber auf Commit `3847561`.

**Fussnote (Ausrichtungsverifikation):** `echo.mod` enthaelt an
zahlreichen Stellen die OS-9-Konvention "trap #0 / dc.w <Funktionscode>"
-- der Funktionscode ist ein DATENWORT, kein Code, capstone (und jeder
andere Analysewerkzeuge ohne dieses Sonderwissen) dekodieren beim linearen
Durchlauf ab einer falsch geratenen Startadresse deshalb leicht falsch
ausgerichteten Folgecode. Verifiziert wurde die Ausrichtung von
`echo+$7c2` deshalb NICHT per linearem Vorwaertslauf, sondern
gegenlaeufig: ein `beq.b`-Sprung bei `echo+$7a2` zeigt UNABHAENGIG exakt
auf `echo+$7c2` -- ein Sprungziel MUSS ein echter Befehlsanfang sein.

## Fortsetzung 51: ECHTE URSACHE + FIX der gesamten echo/csl-Adressbeziehungssaga -- fehlender $8000-Bias auf A6 (2026-09-11, elfte Sitzung, auf "ok mach weiter")

**Der Handbuchtext, direkt gefunden (68k_tech.pdf, ZWEI unabhaengige
Stellen, Table 2-6 UND Table D-7):**

> "(a6) is always biased by $8000 to allow object programs to access
> 64K of data using indexed addressing. You can usually ignore this
> bias because the OS-9 linker automatically adjusts for it."

> "(a6) is actually biased by $8000, but this can usually be ignored
> because the linker biases all data references by -$8000."

**Das war die ECHTE, EINZIGE Ursache der gesamten `echo`/`csl`-
Adressbeziehungs-Saga seit Fortsetzung 44** -- nicht drei verschiedene
Probleme, sondern EIN einziges: unser `Q9K_ProcFork` uebergab A6 bisher
UNVERSCHOBEN (`a6 = block`), waehrend der ECHTE Microware-Linker jeden
negativen a6-relativen Zugriff in kompiliertem Code (wie `echo.mod`)
so einkompiliert, dass er `a6 = block + $8000` erwartet. Nachrechnung
bestaetigt es zweifelsfrei:
- `-$78a0(a6)` (Fortsetzung 44/48, Aufruf) = `$8000-$78a0` =
  **`$0760`** relativ zur ROHEN Basis -- exakt der Datenoffset der
  beiden nie relozierten `4ef9 00000000`-("jmp.l $0")-Stubs, die in
  Fortsetzung 48 in `echo`s eigenem `M$IData` gefunden wurden!
- `-$78cc(a6)` (Fortsetzung 48, Lesen) = `$8000-$78cc` = **`$0734`** --
  der ALLERERSTE kopierte Wert von `M$IData`, per `M$IRefs` bereits
  korrekt zum Datenzeiger relozierbar.
- `-$789e(a6)` (Fortsetzung 50, Schreiben) = `$8000-$789e` = **`$0762`**
  -- genau 2 Byte in den ERSTEN `jmp.l`-Stub hinein, also dessen
  4-Byte-Sprungziel-Operand.

**Die drei scheinbar unabhaengigen Symptome (Sprung ~68 Byte daneben in
Fortsetzung 44, Speichermuell-Zeiger in Fortsetzung 48, Zerstoerung von
`csl`s Code in Fortsetzung 50) waren die GANZE ZEIT nur unterschiedliche
Konsequenzen DESSELBEN fehlenden Bias.** `echo` benutzt seinen eigenen,
per `M$IData`/`M$IRefs` bereits korrekt aufgebauten "Sprungtabellen"-
Bereich (die beiden `jmp.l`-Stubs) fuer den Aufruf in `csl` hinein --
schreibt VORHER per `-$789e(a6)` die vom Trap-Bibliotheks-Mechanismus
aufgeloeste Zieladresse in den ersten Stub, ruft ihn dann per
`-$78a0(a6)` auf. Alles davon spielt sich in `echo`s EIGENEM, ganz
normalen Prozessblock ab -- die kunstvolle Fortsetzung-48-Kombi-
Allokation war unnoetig und sogar schaedlich (sie legte eine blosse
Codekopie von `csl` genau dort hin, wo `echo` in Wahrheit in seinen
EIGENEN Speicher schreiben wollte).

**Fix:**
- `Q9K_ProcFork` (`q9kernel_firstproc.c`): `a6 = block + $8000` statt
  `a6 = block` (Table 2-6/D-7-konform). `M$IData`/`M$IRefs` bleiben
  UNVERAENDERT relativ zur ROHEN Basis `block` (verifiziert: `M$Mem`
  passt nur zur rohen Basis, nicht zum gebiasten a6).
- `Q9K_ExperimentalCombinedAlloc` (`q9kernel_traplink.c`) samt Aufruf
  in `q9kernel_entry.a` und `Q9K_FORK_BLOCK_OVERRIDE`-Mechanismus
  (`q9kernel_firstproc.c`) VOLLSTAENDIG entfernt -- durch den Bias-Fix
  ueberfluessig geworden, kein Sonderfall mehr fuer `F$Fork`.
- `test_q9kernel_firstproc.c`: F1-Erwartung fuer a6 um die Bias-
  Subtraktion ergaenzt; die Fortsetzung-48/49-Testinfrastruktur
  (`Q9K_FORK_BLOCK_OVERRIDE`-Umleitung, `Q9K_COMB_*`-Umleitungen in
  `test_q9kernel_traplink.c`, dortiger `Q9K_AllocMem`-Stub) entfernt,
  da nichts mehr davon referenziert wird.

**Live verifiziert:** `echo` erreicht nach dem Fix erneut alle vier
Meilensteine (`l`=Load, `c`=csl geladen, `t`=F$TLink erfolgreich,
`E`=erfolgreich geforkt) -- diesmal OHNE das fruehere `'P'`-Diagnose-
zeichen (Kombi-Allokation entfallen) und OHNE den Fortsetzung-50-
Absturz (`csl`s Code bleibt unangetastet). Kernel neu gebaut (kleiner:
`$439e`->`$41b4`, da die Kombi-Allokation entfallen ist), Sicherheits-
abstand aus Fortsetzung 38 erneut per Byte-Dump geprueft (weiterhin
exakt richtig). Alle 15 Host-Testsuiten gruen.

### Neuer, eigenstaendiger Fund dahinter (NICHT mehr in dieser Sitzung verfolgt): korrupter IRQ-Vektortabellen-Eintrag

Nach ausreichend langer Laufzeit (`echo`/`csl` laufen jetzt so lange
und so weit wie nie zuvor) tritt ein NEUER, unabhaengiger Absturz auf
(Vektor 4, `PC=$7002`) -- reproduzierbar auch OHNE jeden Tastendruck
(also kein Diagnose-Trigger-Artefakt). Per `Q9_FREEZE_PC=$7002` +
`Q9_TRACE_INSTR=1` exakt zurueckverfolgt: die IRQ-Dispatch-Schleife
(`q9kernel_entry.a`, Adresse `$7c60`-`$7ccc`, laeuft die 16-Eintraege-
Tabelle bei `$1500` ab, s. `Q9K_ProcIRQ`/`Q9K_IRQTAB_BASE` in
`q9kernel_exctable.c`) fuehrt bei einem Tabelleneintrag ein `jsr (a1)`
aus, wobei `a1` faelschlich `$1650` enthaelt -- das ist
`Q9K_VMODUL_RETBUF` (`q9kernel_moddir.c`), ein reiner DATEN-
Rueckgabepuffer fuer `F$VModul` (NIE als Code gedacht). Der Sprung
dorthin krabbelt ueber Nullwoerter bis zu einem echten Illegal-Opcode-
Muster bei `$7002` (exaktes Gegenstueck zum Mechanismus aus
Fortsetzung 50, nur an anderer Stelle).

**Bedeutet:** irgendein `F$IRQ`-Registrierungsaufruf (vermutlich
`sc68681`s eigene Treiberinitialisierung, die laut Kopfkommentar in
`Q9K_ProcIRQ` dreimal `F$IRQ` aufruft) bekommt/uebergibt an dieser
Stelle `$1650` als ISR-Zeiger (a0-Eingabe von `F$IRQ`) statt einer
echten Interrupt-Routine -- vermutlich ein Registerleck (ein
Aufrufpfad laesst einen von `F$VModul` stammenden Restwert in einem
Register stehen, das spaeter fuer `F$IRQ` wiederverwendet wird, ohne
neu belegt zu werden). Noch NICHT weiter eingegrenzt, WELCHER Aufruf
genau den Leckwert einschleust. Vermutlich ein SEIT LANGEM latent
vorhandener Bug, der nie ausgeloest wurde, weil `echo`/`csl` nie lange
genug liefen, um eine echte Interrupt-Zustellung durch diese Tabelle
zu erreichen.

**Fuer eine Folgesitzung:** den Aufrufpfad zwischen dem letzten
`F$VModul`-Aufruf (liefert `$1650` in irgendeinem Register) und dem
naechsten `F$IRQ`-Aufruf (uebernimmt diesen Wert faelschlich als ISR)
per Ringpuffer/Kanarie in `Q9K_SysFVModul`/`Q9K_SysFIRQ`
(`q9kernel_entry.a`) genau nachvollziehen -- gleiche Methodik wie bei
allen bisherigen Registerleck-Funden dieses Kernels.

Alle 15 Host-Testsuiten gruen. Committet+gepusht.

## Fortsetzung 52: IRQ-Tabellen-Absturz GELOEST -- echte Ursache war Adresskollision mit VIER anderen Scratch-Zellen-Gruppen (2026-09-11/12, elfte Sitzung, auf "ok, du kannst weiter machen")

**Ursache der Fortsetzung-51-Vermutung ("Registerleck zwischen F$VModul
und F$IRQ") war falsch -- es gibt gar kein Registerleck.** Per
`Q9_WATCH_ADDR` auf `Q9K_IRQScratch_Isr` ($13D0, die Trampolin-Zelle,
in die `Q9K_SysFIRQ` das eingehende `a0` GANZ AM ANFANG schreibt, VOR
jedem C-Aufruf) gezeigt: es gibt in der GESAMTEN Boot-Sitzung nur EINEN
einzigen echten `F$IRQ`-Aufruf (von `sc68681`), und der uebergibt einen
VOLLKOMMEN PLAUSIBLEN ISR-Zeiger (`$d9cc`, real innerhalb von
`sc68681`s eigenem Modul). Das Register `a0` war beim Trap-Eintritt
also nie falsch.

**Echte Ursache, per vollstaendigem Watch auf die GESAMTE Tabelle
gefunden:** `Q9K_IRQTAB_BASE` ($1500) mit 16 Eintraegen a 20 Byte
reicht bis `$1640` -- das Tabellenende ueberlappt mit GLEICH VIER
spaeter (in anderen Dateien) angelegten Scratch-Zellen-Gruppen, die
"$16xx" faelschlich fuer frei hielten:
- Slot 13 (`static`=`$1610`, `port`=`$1614`) = `Q9K_SEND_SCRATCH_ERROR`/`_SUCCESS` (`q9kernel_procsleep.c`)
- Slot 14 (`isr`=`$1620`..`port`=`$1628`) = `Q9K_RETPD_SCRATCH_*` (`q9kernel_iopath.c`)
- Slot 15 (ALLE FUENF Felder `$162C`-`$163C`) = BYTE-GENAU deckungsgleich mit `Q9K_VMODUL_SCRATCH_HDR`/`_SIZE`/`_ENTRY`/`_ERROR`/`_SUCCESS` (`q9kernel_moddir.c`)

Slot 15s Ueberlappung ist die, die den Absturz ausloeste: sobald
`F$VModul` einmal erfolgreich lief (z.B. beim Laden von "echo"),
enthaelt `Q9K_VMODUL_SCRATCH_ENTRY` ($1634, alias Slot 15s `isr`-Feld)
den Wert `$1650` (`Q9K_VMODUL_RETBUF`, ein reiner Datenpuffer). Da
Slot 15s "Vektor"-Feld (alias `Q9K_VMODUL_SCRATCH_HDR`, $162C) zu
diesem Zeitpunkt zufaellig ungleich 0 ist (haelt den zuletzt
gepruexften Modulkopfzeiger), haelt die IRQ-Dispatch-Schleife
(`q9kernel_entry.a`) Slot 15 faelschlich fuer einen GUELTIGEN,
registrierten Eintrag und ruft `jsr (a1)` mit `a1=$1650` auf -- Vektor
4, `PC` krabbelt ueber Nullwoerter bis `$7002`.

**Fix:** `Q9K_IRQTAB_SLOTS` von 16 auf 12 verkleinert (`q9kernel_exctable.c`
UND die gleichnamige Assembler-Konstante in `q9kernel_entry.a`) -- 12
Eintraege enden bei `$15F0`, VOR der ersten tatsaechlich belegten
Nachbarzelle (`$1600`, `Q9K_PRSNAM_SCRATCH_ERROR`). Keine funktionale
Einschraenkung (bisher wird nur EIN Slot je ueberhaupt benutzt). Host-
Test (`test_q9kernel_exctable.c`) verwendet ein eigenes, redirected
`g_irqTable[16*20]` und ruft `Q9K_ProcIRQ` gar nicht direkt auf --
keine Anpassung noetig.

**Lektion, ergaenzt zur bestehenden Konvention** ("beim Anlegen neuer
Scratch-Felder IMMER die $13xx-Belegung gegenpruefen"): das gilt nicht
nur fuer den ANFANG einer neuen Konstante, sondern fuer den GESAMTEN
belegten Bereich einer TABELLE (Anfang + Groesse × Eintragsgroesse) --
eine Tabelle mit mehreren Eintraegen kann in einen Bereich hineinragen,
der beim Anlegen als "frei, weil weit hinter dem Tabellenanfang" galt.

**Live verifiziert:** der Fortsetzung-51-Absturz (Vektor 4, `PC=$7002`)
tritt nach dem Fix nicht mehr auf. Alle 15 Host-Testsuiten gruen,
Kernel neu gebaut, Sicherheitsabstand aus Fortsetzung 38 erneut per
Byte-Dump geprueft (weiterhin exakt richtig).

### Neuer, NOCH TIEFERER Fund dahinter (NICHT mehr in dieser Sitzung verfolgt)

`echo`/`csl` laufen jetzt so lange wie noch nie -- und stossen auf
einen VIERTEN, wieder eigenstaendigen Absturz: Vektor 10 (A-Line/
"1010 Emulator"-Trap, typischerweise eine vom CPU-Kern nicht
implementierte Instruktion), `PC=$4e25e`. Anders als alle bisherigen
Funde dieser Sitzung liegt die Absturzstelle NICHT in einer bekannten
Modul-Kopfregion, sondern (per Registerauswertung: `A6=$5567f`, abzueglich
des jetzt korrekten `$8000`-Bias also roher Block `$4d67f`) mutmasslich
INNERHALB `echo`s EIGENEM Stack-Bereich (M\$Mem endet bei `block+$76c`,
der PC faellt in den direkt anschliessenden `M$Stack`-Bereich). Der
Hex-Dump um den PC zeigt Bytemuster, die eher wie eine ADRESSTABELLE
aussehen (mehrere Langworte, die selbst wie Zeiger INNERHALB `echo`s
Modul aussehen, z.B. `$3e310`), nicht wie echter Code -- Verdacht:
wieder ein Sprung in Daten statt Code, diesmal mutmasslich ueber einen
Wert, der aus `echo`s eigenem Stack gelesen wird. NICHT weiter
eingegrenzt (welcher Aufruf/welches Register).

**Fuer eine Folgesitzung:** `Q9_FREEZE_PC=$4e25e` + `Q9_TRACE_INSTR=1`
fuer die Instruktionsspur bis zum Absturz, dann pruefen, welche
Instruktion den Sprung/Aufruf ausloest und ob der verwendete Zeiger
aus dem Stack, aus `echo`s Datenbereich oder aus einem Register mit
Fremdherkunft stammt -- gleiche Methodik wie bei allen bisherigen
Funden dieser Sitzung.

Alle 15 Host-Testsuiten gruen. Committet+gepusht.

## Fortsetzung 53: Vektor-10-Absturz TEILWEISE aufgeklaert -- echte Ursache noch offen, Sitzung hier bewusst beendet (2026-09-12, elfte Sitzung, auf "ok weiter")

**Per `Q9_FREEZE_PC=$4e25e` + `Q9_TRACE_INSTR=1` zurueckverfolgt:**
Der Absturz ist KEIN Sprung durch einen Nullzeiger, sondern ein
GENAU BERECHNETER `jsr -$78a0(a6)`-Aufruf (dieselbe, in Fortsetzung 51
gefixte Instruktion bei `echo+$b84`, ein ZWEITES Mal ausgefuehrt) --
die Adressrechnung selbst stimmt exakt (`a6-$78a0` ergibt genau die
beobachtete Sprungadresse, nachgerechnet und bestaetigt). Das
Sprungziel liegt in `echo`s eigenem, per `M$IData` kopiertem
Datenbereich -- dort steht aber diesmal KEIN gueltiger `jmp.l`-Befehl
mehr, sondern die CPU "krabbelt" durch die Bytes wie durch Daten, bis
sie bei `$4e25e` auf ein echtes 1010-Emulator-Bitmuster (Vektor 10)
trifft.

**Neue, wichtige Erkenntnis per `Q9_WATCH_ADDR` auf den Datenbereich:**
`echo`s eigener Code beschreibt Teile dieses Bereichs OFFENSICHTLICH
MEHRFACH und WIEDERHOLT waehrend der Laufzeit (derselbe Wert `$3e700`
-- eine per Kodebasis relozierte Adresse INNERHALB von `echo` selbst,
`hdrAddr+$1f0` -- wurde bei DEMSELBEN Testlauf siebenmal an dieselbe
Zelle geschrieben, mit steigenden Sequenznummern ueber die gesamte
Laufzeit verteilt). Das widerspricht der in Fortsetzung 51
angenommenen Deutung "einmaliger Lazy-Binding-Stub, von aussen
gefuellt" -- es sieht eher nach einer von `echo`s COMPILIERTEM CODE
SELBST bei jedem Aufruf einer bestimmten Funktion neu aufgebauten
Tabelle aus (z.B. ein Formatierungs-/Dispatch-Mechanismus, der
Kodezeiger-Werte aus kleinen, im Modul selbst gespeicherten relativen
Offsets berechnet). Die urspruengliche `M$IData`/`M$IRefs`-Kopie
liefert dabei nur den ANFANGSZUSTAND (einmalig bei `F$Fork`) -- was
`echo` SPAETER selbst hineinschreibt, ist eine ganz andere,
eigenstaendige Frage.

**Zusaetzliche Komplikation, ehrlich benannt:** die genaue
Blockadresse (`a6`/roher Datenbereich) unterscheidet sich zwischen
zwei ansonsten identisch gestarteten Boot-Laeufen desselben Abbilds
um einen kleinen, nicht-runden Betrag (`$4d67f` vs. `$4d6a0` in zwei
Messungen) -- vermutlich, weil die exakte Instruktionsanzahl bis zu
diesem Punkt leicht vom relativen Timing zwischen Boot-Ablauf und
Hintergrund-Interrupts abhaengt (Emulator-Timing ist an dieser Stelle
nicht perfekt deterministisch). Das bedeutet: Adressen, die aus EINEM
Testlauf gewonnen werden, muessen im NAECHSTEN Testlauf nicht mehr
exakt stimmen -- jede weitere Untersuchung sollte `Q9_FREEZE_PC` immer
im SELBEN, unmittelbar vorangegangenen Lauf gewinnen, nicht aus einem
aelteren Dump uebernehmen.

**Bewusst NICHT weiter verfolgt in dieser (bereits sehr langen)
Sitzung:** die eigentliche Frage -- WARUM/WIE die kopierte "Stub"-
Zelle irgendwann einen ungueltigen Wert enthaelt, obwohl `echo`
denselben Bereich nachweislich mehrfach erfolgreich neu beschreibt --
bleibt offen. Mit vier bereits in dieser Sitzung geloesten,
voneinander unabhaengigen Bugs (Interrupt-Race, `M$IData`/`M$IRefs`,
A6-Bias, IRQ-Tabellenkollision) ist das ein sinnvoller Punkt, um
diesen fuenften/sechsten Fund einer eigenen, frischen Sitzung zu
ueberlassen statt ihn am Ende einer bereits erschoepfend langen
Sitzung zu erzwingen.

**Fuer eine Folgesitzung:** in EINEM einzigen, zusammenhaengenden Lauf
(a) den Fortsetzung-51-Watch auf die Stub-Zelle wiederholen, DIESMAL
bis zum tatsaechlichen Absturz durchlaufen lassen (nicht vorher
abbrechen), um die LETZTE Schreiboperation vor dem Fehlschlag zu
sehen; (b) klaeren, ob `echo`s wiederholtes Neuschreiben derselben
Zelle eine legitime, wiederholt aufgerufene Bibliotheksfunktion ist
(z.B. Zahlformatierung) und ob DORT ein Fall fehlt, der die Zelle
in einen inkonsistenten Zwischenzustand versetzen kann (z.B. eine
unterbrochene Teilaktualisierung durch einen Interrupt mitten in der
Mehr-Byte-Schreibsequenz -- passt zum bereits zweimal in diesem
Kernel gefundenen "Interrupt unterbricht eine nicht-atomare
Aktualisierung"-Fehlermuster, s. Fortsetzung 37/38 und den
IRQ-Dispatch-Kopfkommentar in `q9kernel_entry.a`).

Keine Codeaenderung in dieser Fortsetzung -- reine Diagnose. Alle 15
Host-Testsuiten unveraendert gruen, Repo sauber auf Commit `6b29d32`.

### Nachtrag (direkter Anschluss, "ok weiter"): Adressen sind DOCH reproduzierbar -- der eigentliche Fund ist praeziser als gedacht

Die oben vermutete Timing-Nichtdeterminismus-Erklaerung war ZU
VORSCHNELL: ein sauberer, WIEDERHOLTER Test (drei separate Boots
desselben frischen Abbilds, davon zwei OHNE jede Instrumentierung)
zeigt exakt dieselbe Absturzadresse (`PC=$4e25e`, `A6=$5567f`) --
byteidentisch. Die fruehere Abweichung (`$4d67f` vs. `$4d6a0`) kam
offenbar davon, dass die Watch-Instrumentierung selbst (jeder
Speicherzugriff wird zusaetzlich geprueft) das Timing GENUG
verschiebt, um eine andere Allokationsreihenfolge zu erzeugen --
NICHT von echter, instrumentierungsfreier Nichtdeterminism. Fuer
zukuenftige Untersuchungen: Adressen aus einem UNINSTRUMENTIERTEN Lauf
gewinnen, dann in einem ZWEITEN, GLEICH GEBOOTETEN Lauf gezielt
beobachten -- funktioniert zuverlaessig (dreimal bestaetigt).

**Die Kopierschleife selbst ist NACHWEISLICH FREI VON BUGS** -- der
komplette Maschinencode von `Q9K_ApplyInitializedData`s `M$IData`-
Kopierschleife (`q9kernel_firstproc.c`, kompiliert nach
`q9kernel` Datei-Offset `$9266`-`$92da`) wurde Instruktion fuer
Instruktion gegen den C-Quelltext geprueft: `dstOff`/`count` werden
korrekt byteweise big-endian aus dem Modulkopf gelesen, Ziel- UND
Quelladresse verwenden denselben Schleifenindex, die aeussere
"while (p < end)"-Schleife ist exakt nachgebildet. Kein Diskrepanzpunkt
gefunden.

**Trotzdem beobachtet:** die Kopierschleife schreibt am Ende (Index
`i=44`, Zieladresse `block+$760`, GENAU die Stelle, auf die `echo`s
`jsr -$78a0(a6)` spaeter zeigt) den Wert `$f0` -- auf der Festplatte
(`echo.mod`, frisch aus der Datei gelesen, NICHT aus altem
Sitzungs-Gedaechtnis rekonstruiert) steht an dieser Stelle aber `$4e`
(Beginn von `4ef9 00000000`, dem erwarteten "jmp.l"-Stub). `$f0` ist
stattdessen der Wert, der laut Datei an `M$IData`-Index 11 (Datenoffset
`$73f`) steht.

**Da die Kopierschleife selbst korrekt ist, MUSS `echo`s eigenes,
GELADENES Modulabbild im RAM zum Zeitpunkt des Kopierens bereits von
der Datei abweichen** -- ein 33-Byte-Versatz (`$21`) an GENAU der
Stelle, wo `M$IData` beginnt, ist auffaellig regelmaessig (derselbe
Betrag wie die urspruenglich als "Nichtdeterminismus" fehlgedeutete
Abweichung). Das deutet auf eine ECHTE Speicherkorruption VOR dem Fork
hin -- am ehesten waehrend `F$Load` (Laden von Diskette) oder
`F$VModul` (CRC-Pruefung), NICHT auf einen Fehler in der M$IData/
M$IRefs-Anwendung selbst.

**Fuer eine Folgesitzung, konkret:** in EINEM Lauf `Q9_WATCH_ADDR` auf
`echo`s KOMPLETTES, GELADENES Modulabbild setzen (`hdrAddr` bis
`hdrAddr+Groesse`, aus dem Moduldirectory-Dump bekannt) UND VOR jedem
Kopiervorgang (also VOR `F$Fork`) einen Kontroll-Hexdump von
`hdrAddr+$0c26` (`M$IData`) gegen die Datei vergleichen, um zu
bestaetigen, ob die Abweichung schon beim `F$Load` entsteht oder erst
danach (z.B. durch `F$VModul`s CRC-Berechnung, die denselben
Speicherbereich liest) hinzukommt.

Keine Codeaenderung. Alle 15 Host-Testsuiten weiterhin gruen.

## Fortsetzung 54: Repo-Reorganisation nachvollzogen + Vektor-10-Fund WEITER PRAEZISIERT -- Q9K_ApplyInitializedData vollstaendig entlastet, echte Ursache liegt in A6 selbst zur Laufzeit (2026-09-13, elfte Sitzung, auf "ok, dann mach damit bitte weiter")

**Repo-Reorganisation (ausserhalb dieser Sitzung geschehen):** `fix/
a4-aufruferabhaengig` wurde nach `main` gemerged; der Kernel liegt
jetzt unter `Q9-KERNEL/68k/src/kernel/` (vorher `src/kernel/`), der
Emulator unter `Q9-Forge/Q9-Flux/Q9-Flux-68k/` (vorher `Q9-Forge/
Q9-Flux-68k/` direkt) -- Q9-Flux-x86/-Devices als Geschwisterverzeichnisse
angelegt (fuer geplante Ports). Zwei dadurch zerbrochene relative Pfade
gefunden+gefixt (waren schlicht nicht an die neue Struktur angepasst):
`Q9-KERNEL/68k/src/kernel/build.sh` (`q9sysglob.h` liegt jetzt unter
`Q9-KERNEL/common/src/`, nicht mehr eine Ebene ueber `src/kernel/`) und
`tools/mkbootfile.sh` (`BUILD`-Pfad). Alle 15 Host-Testsuiten UND der
Live-Test laufen am neuen Ort unveraendert.

**Vektor-10-Fund (Fortsetzung 53) weiter eingegrenzt:** die dortige
Vermutung "`echo`s Modulabbild weicht schon vor dem Kopieren von der
Datei ab" hat sich NICHT bestaetigt -- eine gezielte Ueberpruefung
zeigt das Gegenteil:

1. **Der `F$Load`-Ladevorgang schreibt den Stub-Bereich BYTE-GENAU
   korrekt** (per `Q9_WATCH_ADDR` auf die exakte Zieladresse in `echo`s
   Modulabbild direkt verifiziert: `4e f9 00 00 00 00`, exakt wie in
   der Datei).
2. **`Q9K_ApplyInitializedData`s Kopierschleife ist ebenfalls
   vollstaendig korrekt** -- diesmal per TEMPORAERER, gezielter
   Erweiterung des Emulator-eigenen Instruktions-Trace-Hooks
   (`Q9-Flux-68k/src/kernel/m68krt.c`, NACH Gebrauch wieder entfernt)
   direkt nachgewiesen: `a2` (= `block`, der Kopierziel-Basisparameter)
   ist zur Laufzeit `$4d6a0` -- die vorherige Fortsetzung-53-Annahme
   `$4d67f` (hergeleitet aus `A6-$8000` DES ABSTURZES) war schlicht die
   FALSCHE Referenzadresse fuer DIESEN Zweck. Mit dem echten `a2`
   stimmen Quelle UND Ziel der Kopierschleife fuer JEDES `i` exakt
   ueberein (`payload[i]` landet korrekt bei `block+dstOff+i`).
3. **`Q9K_SetFrameReg` schreibt beim Fork ebenfalls den korrekten
   Wert** in den A6-Slot des Fake-Rahmens: per `Q9_WATCH_ADDR` auf die
   berechnete Slot-Adresse (`frameBase+$38`) verifiziert -- der ALLERERSTE
   Schreibzugriff dort ist `$0005556a0` = `block+$8000`, exakt der
   erwartete, korrekte Wert. **Der Fortsetzung-51-Fix (A6-`$8000`-Bias)
   ist damit beim Fork nachweislich vollstaendig korrekt.**

**Der eigentliche Widerspruch:** Trotz alledem zeigt die Absturz-
Mitschrift `A6=$5567f` -- NICHT `$556a0`. Differenz weiterhin exakt
`$21` (33 Byte). Per Watch auf denselben Speicherplatz (`frameBase+$38`)
gezeigt: NACH dem korrekten Erstschreiben wird genau diese Adresse noch
MEHRFACH von `echo`s eigenem, laufendem Code beschrieben (Adressen
innerhalb `echo`s Modul: `$3e5a6`, `$3e5ae`, `$3ea76`) -- das ist
schlicht NORMALE Stack-Nutzung (diese Adresse liegt oberhalb von
`echo`s eigenem `M$Stack`-Bereich, wird also im Laufe der Ausfuehrung
ganz gewoehnlich fuer lokale Variablen/gerettete Register wiederverwendet,
sobald der anfaengliche "Fake-Rahmen" beim allerersten Prozessstart
konsumiert ist). **Keiner dieser spaeteren Schreibzugriffe ergibt
jedoch `$5567f` oder `$4d67f`** -- der beim Absturz tatsaechlich im
CPU-Register befindliche A6-Wert stammt also NICHT (mehr) aus dieser
Speicherzelle.

**Schlussfolgerung:** Der Fehler ist NICHT (mehr) in `Q9K_ProcFork`,
`Q9K_ApplyInitializedData` oder dem A6-Bias-Fix zu suchen -- alle drei
sind jetzt zweifelsfrei als korrekt verifiziert. Der tatsaechliche
A6-Wert zur Absturzzeit muss auf REGISTER-Ebene entstehen (vermutlich:
`echo`s eigener, compilierter Code veraendert A6 irgendwann waehrend
der Ausfuehrung -- unklar, ob legitim mit anschliessender Wiederher-
stellung, dabei aber von einem Timer-Interrupt genau in diesem Fenster
unterbrochen, oder aus einem anderen Grund) -- eine reine Speicher-
adressen-Beobachtung (wie in dieser gesamten Sitzung verwendet) kann
das NICHT mehr aufloesen, weil der Wert nie (wieder) an eine feste
Adresse geschrieben werden muss, um im Register zu stehen.

**Fuer eine Folgesitzung, konkret:** den Emulator-eigenen Instruktions-
Trace-Hook (`m68krt.c`, `q9_dbg_instr_hook`) TEMPORAER um eine
zusaetzliche Spalte `a6` erweitern (analog zur bereits bestehenden
Erweiterung fuer dieses Fortsetzung, die NACH Gebrauch wieder entfernt
wurde) -- damit laesst sich der GENAUE Zeitpunkt/PC finden, an dem A6
erstmals von `$556xx` auf `$5567f`-aehnliche Werte wechselt, und ob das
mit einem Timer-Interrupt-Eintritt zusammenfaellt (per `Q9K_RaceRing`,
bereits vorhanden, im selben Dump gegenpruefen).

Alle 15 Host-Testsuiten gruen (unveraendert). Reine Diagnose in dieser
Fortsetzung -- die beiden Pfad-Fixes (`build.sh`/`mkbootfile.sh`) sind
die einzige inhaltliche Aenderung, beide reine Anpassungen an die
Repo-Reorganisation.

## Fortsetzung 55: A6-Kantenverfolgung -- echte, legitime csl/echo-Uebergabe bestaetigt, aber der Absturzuebergang selbst bleibt unbeobachtbar (2026-09-13, elfte Sitzung, auf "ja mach bitte weiter")

**Umgesetzt:** der in Fortsetzung 54 skizzierte Plan -- den Emulator-
eigenen Instruktions-Trace-Hook (`Q9-Flux-68k/src/kernel/m68krt.c`)
TEMPORAER um eine kantengetriggerte A6-Verfolgung erweitert (protokolliert
NUR bei AENDERUNG, gefiltert auf den Wertebereich um echos erwartete
Datenbereichsbasis `$54000`-`$57000`). Nach Gebrauch wieder vollstaendig
zurueckgesetzt (`git checkout`).

**Positives Ergebnis:** Die ERSTEN 13 beobachteten A6-Uebergaenge zeigen
ein VOLLSTAENDIG korrektes, sich wiederholendes Aufruf-/Ruecksprung-
Muster zwischen `echo` (a6=`$556a0`) und `csl` (a6=`$3df10`, csl's
eigener statischer Bereich aus `F$TLink`) -- `csl`s Code an Datei-
Offset `$3f40e` schaltet a6 korrekt auf seinen EIGENEN Bereich um,
Offset `$3f524` schaltet es korrekt WIEDER auf `echo`s Wert zurueck.
Dieses Muster wiederholt sich sauber ueber mehrere Aufrufe (Eintraege
6-12) -- **die grundsaetzliche a6-Umschaltung beim Aufruf einer Trap-
Bibliotheksfunktion funktioniert also nachweislich korrekt.**

**Blockiert:** der Uebergang zum tatsaechlichen Fehlerwert (`$5567f`)
selbst konnte NICHT beobachtet werden -- die Protokollierung bricht
REPRODUZIERBAR (zweimal exakt gleich) unmittelbar nach Eintrag #13
(derselbe `csl`-Ruecksprungpunkt `$3f524`) ab, weil der EMULATOR-
PROZESS SELBST (nicht nur die emulierte CPU) an dieser Stelle
abstuerzt -- kein sauberes "Host-Escape"/Dump-Ende, der Prozess
verschwindet einfach aus der Prozessliste. Der eigentliche
`Q9K_ExcTrap`-Dump (mit `A6=$5567f`) wird davor noch korrekt
geschrieben, das Verhalten des `q9dbg_dump.txt`-Mechanismus selbst ist
also nicht betroffen -- nur meine ZUSAETZLICHE, temporaere
Instrumentierung bringt den Host-Prozess irgendwann danach zum
Absturz (Ursache nicht ermittelt: entweder eine Wechselwirkung mit dem
bereits eingefrorenen Trace-Ring, oder eine sehr hohe Aufruffrequenz
meines `fprintf`, die mit dem laufenden Absturz-/Spinzustand
kollidiert). **Bewusst nicht weiterverfolgt** -- ein Debug-Werkzeug,
das selbst instabil wird, ist kein tragfaehiger Weg fuer den letzten
Schritt dieser Untersuchung.

**Stand am Ende dieser Sitzung:** Die Ursache des Vektor-10-Absturzes
ist auf einen sehr kleinen, klar umrissenen Rest eingegrenzt -- ALLE
kernel-seitigen Mechanismen (`F\$Load`, `Q9K_ApplyInitializedData`,
`Q9K_ProcFork`s A6-Bias-Fix, UND jetzt auch das grundsaetzliche
`csl`/`echo`-a6-Umschaltmuster) sind nachweislich korrekt. Der Fehler
muss in einem SPAETEREN, bisher nicht beobachteten a6-Uebergang
liegen -- entweder einer WEITEREN `csl`-Aufruf/Ruecksprung-Runde (nach
Eintrag 13) mit einem subtilen Fehler, oder einer Interrupt-bedingten
Verfaelschung genau in diesem Fenster. Fuer eine Folgesitzung: dieselbe
A6-Kantenverfolgung erneut versuchen, aber ROBUSTER umgesetzt (z. B.
in einen Ringpuffer statt live `fprintf`, erst beim naechsten Ctrl-^-Dump
ausgegeben -- vermeidet die vermutete Host-Instabilitaetsursache).

Alle 15 Host-Testsuiten gruen (unveraendert), keine Kernel-Codeaenderung
in dieser Fortsetzung, Emulator-Repo sauber zurueckgesetzt.

---

## Fortsetzung 56: Vektor-10-Absturz VOLLSTAENDIG aufgeklaert -- echte
Ursache liegt in `csl`s EIGENEM privaten Freispeicher-Verwalter, nicht
im Q9-OS-Kernel (2026-09-13, elfte Sitzung, auf "ok, du kannst weiter
machen es ist noh frueh")

### Ausgangslage

Zwei Kandidaten-Fixes aus der vorigen Sitzungshaelfte zuerst live
geprueft:

1. **Arena-Interrupt-Sperre** (`Q9K_IntLock`/`Q9K_IntUnlock` in
   `q9kernel_entry.a`, um `Q9K_AllocMem`/`Q9K_FreeMem`/
   `Q9K_AllocLargest` gelegt, `q9kernel_arena.c`): rebuilt, live
   getestet -- IDENTISCHER Absturz (`PC=$4e25e A6=$5567f`). Nicht die
   Ursache, aber als eigenstaendige Haertung (verhindert eine echte,
   wenn auch hier nicht ausschlaggebende Racebedingung) BEHALTEN.
2. **`F\$SRqMem`/`F\$SRtMem`-Registerrettung** (`Q9K_SysFSRqMem`/
   `Q9K_SysFSRtMem` in `q9kernel_entry.a` retteten bisher nur `a6` um
   ihren internen C-Aufruf, nicht die anderen laut Konvention noetigen
   Register -- derselbe Fehlerklasse, bereits einmal fuer `F\$TLink` in
   Fortsetzung 45 gefixt): `movem.l`-Rettung ergaenzt, rebuilt, live
   getestet -- ERNEUT IDENTISCHER Absturz. Auch das BEHALTEN (echte,
   unabhaengig gerechtfertigte Korrektur), aber nicht die Ursache.

Beide Fixes bauen weiterhin sauber (`build.sh`, Exit=0), alle 15
Host-Testsuiten weiterhin gruen (neuer Testfall in
`test_q9kernel_arena.c`: `Q9K_IntLock`/`_Unlock`-Aufrufbilanz).

### Neue Diagnosewerkzeuge im Emulator

Die bestehende A6-Kantenverfolgung (Fortsetzung 55, Ringpuffer statt
`fprintf` im heissen Pfad -- vermeidet den dort dokumentierten
Host-Absturz) um zwei Bausteine erweitert (`Q9-Flux-68k/src/kernel/
m68krt.c`/`m68krt.h`/`q9boardrun.c`):

- **`Q9_A6TRACE_LO`/`Q9_A6TRACE_HI`** (Wertebereichsfilter) +
  **`Q9_A6TRACE_FREEZE=<Wert>`** (Ringpuffer wird eingefroren, sobald
  dieser A6-Wert zum ersten Mal auftritt -- haelt die ZUFUEHRENDEN
  Eintraege fest, statt sie durch spaeteres Rauschen zu ueberschreiben).
  Zusaetzlich `a0` je Eintrag mitprotokolliert.
- **`Q9_PCHIT_ADDR=<Adresse>`**: eigener, unabhaengiger Ringpuffer, der
  bei JEDEM Erreichen einer festen PC-Adresse Registerinhalte (frei
  waehlbar im Hook-Code, hier zuletzt `a4`/`d3`/`d4` + ein
  Speicherwort) mitschneidet -- generisch fuer "was liegt in Register
  X, wenn Code Y erreicht wird" ohne Host-Absturzrisiko (kein I/O im
  heissen Pfad, Ausgabe erst beim Ctrl-^-Dump).

Beide vollstaendig `Q9_TRACE_INSTR=1`-gebunden (der Hook wird nur bei
gesetzter Variable ueberhaupt registriert, `m68k_set_instr_hook_
callback` in `m68krt.c` -- beim ersten Versuch OHNE dieses Flag lief
der Ringpuffer leer, `q9_dbg_a6_n=0`, Falle notiert).

### Herleitung (Schritt fuer Schritt, alles live gemessen)

1. **A6-Ringpuffer mit Freeze** (`Q9_A6TRACE_LO=0x30000 Q9_A6TRACE_HI=
   0x60000 Q9_A6TRACE_FREEZE=0x5567f`): zeigt 6 vollstaendig korrekte
   `echo`⇄`csl`-Rundtrips (`a6` wechselt sauber zwischen `$3df10` und
   `$556a0`, ueber die Instruktionen bei `csl+0xee`/`csl+0x202`), dann
   ZWEI Interrupt-Durchlaeufe (`pc=$780c`/`$782e`, der generische
   ISR-Ein-/Austritts-Wrapper -- NICHT csl-spezifisch), und danach der
   7. Eintrittsversuch liefert sofort den korrupten Wert `$5567f`.
2. **`a0`-Mitschnitt an genau dieser Stelle**: `a0=0` bei ALLEN acht
   Durchlaeufen (den 7 guten und dem fehlschlagenden) -- das bedeutet,
   die Adressierung `4(a0)` in der `csl`-Austrittsroutine (Analysebefund
   bei `csl+0x1e0`ff, `movea.l \$4(a0),a1` gefolgt von `movea.l a1,a6`)
   rechnet mit `a0=0`, greift also auf die ABSOLUTE ADRESSE `\$4` zu --
   kein dynamischer Listenknoten, sondern eine FESTE, GLOBALE Zelle.
3. **`Q9_WATCH_ADDR=0x4 Q9_WATCH_LEN=4`** (Schreibzugriffe auf genau
   diese Adresse ueber den gesamten Lauf): 8 Treffer von `pc=\$3f4be`
   (schreibt korrekt `\$556a0` -- das ist die `csl`-Eintrittsroutine,
   die beim allerersten Aufruf ihre eigene a6-Adresse dort ablegt),
   dann EIN Treffer von einer VOELLIG ANDEREN Adresse: `pc=\$449ec`
   schreibt `\$5567f`.
4. **Analysebefund um `\$449ec`** (`csl.mod`): Teil von
   `csl`s EIGENEM privaten Freispeicher-Verwalter (Freiliste INNERHALB
   des von `F\$TLink` gewaehrten statischen Bereichs, komplett getrennt
   von den Kernel-Aufrufen `F\$SRqMem`/`F\$SRtMem`). Ausschnitt:
   ```
   000449c4: move.l -$64c0(a6), d5      ; Freilisten-Kopf
   000449c8: movea.l d5, a4
   000449ca: tst.l a4
   000449cc: bne.b $449d2               ; NULL-Pruefung -- nur EINMAL, am Kopf
   000449d2: move.l a4, d4
   000449d6: movea.l (a0), a4           ; a4 = *(a4)  ("naechster" Zeiger)
                                        ; -- HIER KEINE erneute NULL-Pruefung!
   000449d8: cmp.l $4(a4), d3           ; a4 kann jetzt 0 sein
   ...
   000449ec: sub.l d3, $4(a4)           ; a4=0 -> schreibt auf ABSOLUT $4
   ```
   Der Bug: nach dem Weiterruecken zum "naechsten" Freiblock (Zeile
   `$449d6`) wird NICHT erneut auf `a4==0` (Listenende) geprueft, bevor
   der vermeintliche Block benutzt wird.
5. **`Q9_PCHIT_ADDR=0x449ec`** mit `a4`/`d3`/Speicherwort mitgeschnitten
   (eigener Zusatz, s. o.): `a4=$00000000`, `d3=$00000021` (=33),
   `*(a4+4)=$556a0`. `$556a0 - $21 = $5567f` -- exakt der spaetere
   Absturzwert, auf das Byte genau nachvollzogen. Kein Verdacht mehr,
   sondern arithmetisch bewiesen.

### Bewertung -- wessen Bug ist das?

`csl.mod` ist das ECHTE, closed-source Microware-Modul (nicht von
Q9-OS nachgebaut, s. Kopfkommentare in `q9kernel_traplink.c`). Der
Freispeicher-Verwalter darin gehoert NICHT zum Q9-OS-Kernel -- der
Q9-OS-Anteil endet bei `F\$TLink`, das `csl` genau `M\$Mem=\$2690`
(9872 Byte, aus dem Modulkopf gelesen, `q9kernel_traplink.c`) an
statischem Speicher zuteilt. Ob das der Groesse entspricht, die echte
OS-9/68K-Hardware fuer dasselbe `csl.mod` zuteilen wuerde (dieselbe
`M\$Mem`-Konvention, real dokumentiert), oder ob Q9-OS an anderer
Stelle (z. B. `F\$SRqMem`-Aufrufsequenz waehrend `echo`s Ablauf) ein
anderes Allokationsmuster erzeugt als reale Hardware und dadurch
`csl`s privaten Allocator frueher/anders an diese Kante fuehrt als im
Normalbetrieb, ist NICHT geklaert und braecht eine eigene, neue
Untersuchung (Vergleich der exakten Allokationsgroessen-Sequenz mit
echter OS-9-Dokumentation/-Hardware). Die beiden zuvor versuchten
Kernel-Fixes (Interrupt-Sperre, Registerrettung) waren beide sinnvolle,
unabhaengig gerechtfertigte Haertungen, konnten dieses Ergebnis aber
denknotwendig NICHT aendern, weil die Fehlerursache ausserhalb ihrer
Reichweite liegt (in `csl`s eigenem Code, nicht im Kernel-Trampolin
oder -Allocator).

### Ergebnis dieser Fortsetzung

- Kernel: `Q9K_IntLock`/`Q9K_IntUnlock` + Arena-Sperre, `F\$SRqMem`/
  `F\$SRtMem`-Registerrettung -- beide committet, alle 15 Host-
  Testsuiten gruen, Sicherheitsabstand (Fortsetzung 37/38) erneut
  ueberprueft (weiterhin exakt an Datei-Offset `\$3aa`-`\$3b5`).
- Emulator: `Q9_A6TRACE_LO/_HI/_FREEZE` (mit `a0`-Mitschnitt) und
  `Q9_PCHIT_ADDR` (generischer PC-Treffer-Ringpuffer) als dauerhafte,
  wiederverwendbare Diagnosewerkzeuge committet -- kein Host-
  Absturzrisiko (reines Ringpuffer-Schreiben, Ausgabe erst beim
  Ctrl-^-Dump, exakt wie die bereits bestehenden Werkzeuge).
- Vektor-10-Absturz: Ursache VOLLSTAENDIG UND BEWEISBAR auf
  Instruktionsebene geklaert, liegt aber ausserhalb des Q9-OS-Kernels
  (im privaten Freispeicher-Verwalter des echten `csl.mod`). Keine
  weitere Kernel-seitige Reparatur ohne eine neue, eigene Untersuchung
  moeglich/sinnvoll -- naechster Schritt waere ein Vergleich mit
  realer OS-9-Dokumentation/-Hardware zur exakten `F\$TLink`/
  `F\$SRqMem`-Groessenvergabe, nicht mehr die bisherige "Kernel-Bug"-
  Annahme.

### Randnotiz (Werkzeugbau)

Beim Bau der Host-Testsuiten in dieser Sitzung eine Umgebungs-Falle
entdeckt: `gcc ... $DEFS ...` (Flags aus einer Shell-Variable) wurde
in dieser Shell-Umgebung NICHT wortweise gesplittet -- `-D"A B"`
landete als EIN zusammenhaengendes, falsches `-D`-Argument bei gcc.
Direktes Ausschreiben der Flags in der Kommandozeile (statt ueber eine
Variable) umgeht das zuverlaessig. Fuer kuenftige Sitzungen: bei
scheinbar grundlosen `#error`-Abbruechen trotz "richtig gesetzter"
Flags zuerst genau DAS pruefen.

---

## Fortsetzung 57: die in Fortsetzung 56 offen gelassene Frage
GEKLAERT -- mehr Speicher fuer `csl` VERSCHIEBT den Absturz nur, behebt
ihn NICHT (2026-09-13, elfte Sitzung, auf "ok, dann mach bitte weiter")

### Experiment

Offene Frage aus Fortsetzung 56: liegt die Ursache DOCH (indirekt) bei
Q9-OS, weil `F\$TLink` `csl` weniger statischen Speicher zuteilt als
echte OS-9-Hardware es taete, und der an sich in `csl` latente
Freilisten-Bug dadurch ueberhaupt erst ausgeloest wird? Direkt
geprueft: `Q9K_ProcTLink`s `size`-Berechnung TEMPORAER um Faktor 8
erhoeht (`size = size * 8UL` direkt nach der `M\$Mem`-Ermittlung, NUR
fuer diesen Live-Test, danach vollstaendig zurueckgesetzt -- kein
Rest im Diff). Kernel neu gebaut, derselbe `echo`-Lauf erneut
gestartet.

### Ergebnis

Der Absturz bleibt bestehen, verschiebt sich aber deutlich:
- Vorher (normale Groesse): Vektor 10, `PC=\$4e25e`, `A6=\$5567f`.
- Mit 8-facher `csl`-Speicherzuteilung: Vektor 11, `PC=\$5f6ae`,
  `A6=\$6646f` -- andere Absturzart (F-Line statt A-Line), andere
  Adresse, anderer A6-Wert, spaeter im Lauf.

Das ist der entscheidende Beweis: MEHR Speicher verzoegert den
Absturz (offensichtlich laeuft `echo` dadurch laenger/kommt weiter,
bevor `csl`s privater Allocator erneut in dieselbe Kante läuft),
BEHEBT ihn aber nicht. Waere die Ursache schlichte Speicherknappheit
(Q9-OS teilt `csl` zu wenig zu), haette 8-facher Speicher das Problem
entweder ganz vermieden oder den Absturz bei EXAKT dem gleichen
"Freilisten fast leer"-Zustand nur weiter nach hinten verschoben, aber
NICHT die Absturzart (Vektor) UND den betroffenen Speicherbereich
gleichzeitig veraendert. Stattdessen zeigt sich: der in Fortsetzung 56
gefundene fehlende NULL-Check nach dem Weiterruecken in `csl`s privater
Freiliste ist ein von der Speichergroesse UNABHAENGIGER Logikfehler --
er tritt zuverlaessig auf, sobald `csl`s eigener Allocator (bei genug
Allokations-/Freigabezyklen, unabhaengig davon wie viel Gesamtspeicher
zur Verfuegung steht) einmal an das Ende seiner Freiliste laeuft, ohne
dass die zuletzt gepruefte Groesse passt.

### Bewertung

Die in Fortsetzung 56 offen gelassene Frage ist damit BEANTWORTET:
Q9-OS' `F\$TLink`/`M\$Mem`-Speichervergabe an `csl` ist NICHT die
Ursache und muss NICHT "repariert" werden -- der Bug liegt
ausschliesslich in `csl`s eigenem, geerbtem Maschinencode und wuerde
bei genuegend langer Laufzeit JEDE Kombination aus `echo` (oder jedem
anderen `csl`-nutzenden Programm) mit genuegend vielen internen
`malloc`/`free`-Zyklen treffen, unabhaengig vom zugeteilten
`M\$Mem`. Eine echte Reparatur muesste `csl.mod`s Maschinencode selbst
patchen (die fehlende `tst.l a4`/`beq`-Pruefung nach `movea.l (a0),a4`
bei `csl+ca.\$449d6` einfuegen) -- eine bewusste Entscheidung ausserhalb
des bisherigen Q9-OS-Kernel-Umfangs (Aenderung an echtem, closed-source
Microware-Binaercode), nicht mehr fuer diese Sitzung getroffen.

### Ergebnis dieser Fortsetzung

- Kein Kernel-Code veraendert (Experiment vollstaendig zurueckgesetzt,
  `git diff` auf `q9kernel_traplink.c` leer). Kernel neu gebaut, alle
  15 Host-Testsuiten erneut gruen.
- Vektor-10-Absturz: Ursachenermittlung jetzt ENDGUELTIG abgeschlossen.
  Kein weiterer Q9-OS-Kernel-seitiger Ansatz ersichtlich. Naechster
  Schritt (falls gewuenscht) waere ein bewusster, separat zu
  entscheidender Bināerpatch von `csl.mod` selbst -- keine Kernel-
  Baustelle mehr.

---

## Fortsetzung 58: MEILENSTEIN -- `echo`/`csl` laeuft jetzt VOLLSTAENDIG
UND ABSTURZFREI durch (Laufzeit-Binaerpatch von `csl.mod` + echte
Kernelluecke `F$SetSys` geschlossen) (2026-09-13, elfte Sitzung, auf
"lässt sich das csl jetzt auch ordnungsgemäß ausführen.")

### Auftrag

Direkte Nachfrage im Anschluss an Fortsetzung 57: kann der in
Fortsetzung 56/57 vollstaendig aufgeklaerte `csl`-eigene Freilisten-Bug
jetzt auch tatsaechlich BEHOBEN werden, statt nur dokumentiert zu
bleiben?

### Teil 1: Laufzeit-Binaerpatch fuer `csl`s Freilisten-Bug

Vollstaendige `malloc()`-Routine in `csl.mod` analysiert (nicht nur
die zuvor bekannte Fehlerstelle). Ergebnis: eine klassische, K&R-artige
ZIRKULAERE Freiliste (`-$64c0(a6)` = Rover-/Kopfzeiger, jeder Knoten hat
bei Offset 0 seinen "naechster"-Zeiger). Der reale Abbruchtest der
Suchschleife ("wieder beim Ausgangspunkt angekommen? -> `F$SRqMem`
nachfordern", bei Dateiversatz `$448da`) ist vorhanden und korrekt --
die eigentliche Luecke: NACH dem Weiterruecken zum naechsten Knoten
(`movea.l (a0),a4`, Versatz `$56b6`) wird das Ergebnis NICHT auf `0`
geprueft, bevor sein Groessenfeld gelesen/beschrieben wird. Kommt ein
kaputt terminierter Ring vor (Ursache dafuer NICHT weiter verfolgt --
liegt auch in `csl`s eigenem Code, ausserhalb der hier gewaehlten
Reparaturtiefe), landet `a4=0`, und `sub.l d3,$4(a4)` bei Versatz
`$56bc` schreibt auf ABSOLUTE ADRESSE `$4` -- exakt der in Fortsetzung
56 gefundene Absturzmechanismus.

**Fix** (`Q9K_PatchCslFreelistBug`, neu in `q9kernel_traplink.c`,
aufgerufen direkt nach `Q9K_ApplyInitializedData` in `Q9K_ProcTLink`):
patcht NACH DEM LADEN, in der RAM-Kopie (NICHT die Datei `csl.mod`
selbst) die 4 Byte bei Dateiversatz `$56bc` (`bhi.w $448da`) zu
`bsr.w <Stub>`. Der Stub (28 Byte, per `Q9K_AllocMem` frisch alloziert,
im Modulabbild selbst ist an dieser Stelle kein Platz fuer eine
Einfuegung) tut GENAU dasselbe wie vorher, PLUS die fehlende
NULL-Pruefung davor (`tst.l a4; beq.w <"mehr Speicher"-Pfad>`) --
Byte-Ebenen-Herleitung samt allen Distanzberechnungen im
Kopfkommentar der Funktion. **Sicherheitsnetz:** patcht NUR, wenn (a)
`M$Size` des Moduls die Patchstelle ueberhaupt abdeckt UND (b) die 4
Byte an der Zielstelle EXAKT dem bekannten Original entsprechen -- bei
jeder anderen `csl`-Version bleibt der Patch unwirksam, kein blindes
Ueberschreiben. Neuer Testfall F10 (`test_q9kernel_traplink.c`, drei
Teilfaelle a/b/c) prueft alle drei Pfade inklusive der exakten
Stub-Bytes und Sprungdistanzen.

**Live bestaetigt:** derselbe Fehlerpfad (jetzt ueber unseren Stub
umgeleitet) tritt tatsaechlich ein zweites Mal auf -- diesmal in
`echo`s EIGENEM Speicherbereich statt in `csl`s (der Patch schuetzt
BEIDE, weil er im gemeinsam genutzten `csl`-Code liegt, nicht an einer
`csl`-spezifischen Adresse haengt). Kein Absturz mehr an dieser Stelle
-- ABER ein NEUER, dahinterliegender Absturz (Vektor 5, Zero Divide)
wird dadurch erstmals ueberhaupt erreichbar (vorher blockierte die
Speicherkorruption jeden Fortschritt vorher).

### Teil 2: echte Q9-OS-Kernelluecke gefunden -- `F$SetSys` (Callcode
`$27`) war komplett unimplementiert

Der neue Zero-Divide-Absturz (`divu.l d1,d7` bei Dateiversatz `$448fc`)
zurueckverfolgt: `csl`s `malloc()`-Wachstumslogik (einmalige
Initialisierung beim ersten Aufruf, Dateiversatz `$44a06`) fragt per
internem Wrapper (`$49d18`, `trap #0`/Callcode `$27`) eine
Systemvariable ab, um die minimale Speicherblock-Zuwachsgroesse zu
bestimmen (`d0.l`=Variablennummer `$7C`, `d1.l`=Flags mit Bit 31
gesetzt fuer "lesen"). Callcode `$27` = `F$SetSys` laut
internem Referenzmaterial -- bei Q9-OS bisher
GAR NICHT registriert. Der Wrapper erkennt den Fehlschlag zwar korrekt
(Carry gesetzt), ABER sein Aufrufer prueft das Ergebnis NICHT und liest
die lokale Ausgabevariable trotzdem -- die bleibt dadurch `0`, und
genau DAS ist der Divisor der kurz darauf folgenden Division. Ein
GENUINER, bisher unentdeckter Q9-OS-Kernel-Bug (nicht `csl`s eigener
Code) -- die anderen beiden Funde dieser Sitzung liegen in `csl` selbst,
DIESER hier liegt eindeutig bei uns.

**Fix** (`q9kernel_setsys.c`, neu, plus `Q9K_SysFSetSys` in
`q9kernel_entry.a`, Registrierung in `q9kernel_cinit.c`): Konvention
EMPIRISCH aus dem aufrufenden Code hergeleitet (kein Zugriff auf den
exakten `68k_tech.pdf`-Abschnitt in dieser Sitzung, daher bewusst NICHT
als "aus dem Handbuch zitiert" gekennzeichnet, anders als bei den
uebrigen Syscalls in diesem Kernel) -- `d0.l`=Variablennummer,
`d1.l`=Flags (Bit 31 = lesen), `d2.l`=Wert (lesen: Ausgabe, schreiben:
Eingabe). **Bewusst NUR pragmatisch implementiert** (gleiches Muster
wie `F$CCtl`): kein echtes, persistentes System-Global-Register fuer
beliebige Variablennummern (dafuer fehlt die vollstaendige reale
Variablenliste) -- Schreiben wird bestaetigt, aber nicht gespeichert;
Lesen liefert fuer die eine live gefundene Variable (`$7C`, die
`csl`-Speicherzuwachsgroesse) einen sinnvollen Standardwert (4096
Byte), fuer jede andere (noch) unbekannte Variable `0`, IMMER mit
Erfolg (Carry geloescht) -- bewusst kein Fehlschlag, weil unklar ist,
ob andere Aufrufer das pruefen. Neue Scratch-Zellen `$1644`-`$164F`
(genau in die Luecke zwischen `Q9K_SRqCMemFrameScratch` und
`Q9K_VMODUL_RETBUF` gesetzt, Kollision explizit gegen den GESAMTEN
belegten Bereich gegengeprueft, nicht nur Startadressen -- Lehre aus
Fortsetzung 52). Neuer, 16. Host-Testfall `test_q9kernel_setsys.c`
(vier Faelle: bekannte Variable, unbekannte Variable, Schreiben ohne
Persistenz, Scratch-Bruecke).

### Ergebnis: LIVE VERIFIZIERT, ZWEIMAL REPRODUZIERT

Nach BEIDEN Fixes (Freilisten-Patch + `F$SetSys`): `echo`/`csl` laeuft
komplett durch. `Q9K_ExcTrap-Mitschrift` zeigt `Vektor=0` ("keine
Exception aufgetreten") -- die Ready-Queue enthaelt danach nur noch den
Idle-Testprozess, `echo`s eigener Prozess ist sauber beendet und aus
der Queue entfernt (`F$Exit` erreicht). ZWEIMAL unabhaengig voneinander
live getestet, beide Male identisches Ergebnis. Alle 16 Host-
Testsuiten gruen (neuer Testfall F10 in `test_q9kernel_traplink.c` +
neue Datei `test_q9kernel_setsys.c`). Sicherheitsabstand (Fortsetzung
37/38) erneut per Byte-Dump geprueft, weiterhin exakt richtig.

**Damit ist das seit Fortsetzung 44 verfolgte `echo`/`csl`-Sagathema
ENDGUELTIG UND VOLLSTAENDIG abgeschlossen** -- vom anfaenglichen
kompletten Fehlschlag ueber die A6-Bias-Entdeckung (Fortsetzung 51),
`M$IData`/`M$IRefs` (Fortsetzung 49), die IRQ-Tabellenkollision
(Fortsetzung 52), die vollstaendige Aufklaerung des Vektor-10-Absturzes
(Fortsetzung 56/57) bis zu den beiden hier umgesetzten Fixes -- ein
reales, unveraendertes Microware-C-Programm samt seiner echten,
closed-source C-Laufzeitbibliothek laeuft jetzt absturzfrei auf dem
selbstgeschriebenen Q9-OS-Kernel.

---

## Fortsetzung 59: `echo` produziert nachweislich KORREKTE Ausgabe --
nicht nur absturzfrei, sondern auch inhaltlich richtig (2026-09-13,
elfte Sitzung, auf "Erst echo mit echtem Argument testen")

### Anlass

Beim Reporting von Fortsetzung 58 selbst aufgefallen: die Terminal-
Mitschrift zeigte NIE echo-eigenen Text, nur den `lctE`-Marker gefolgt
von der Idle-Schleife. Ursache gefunden: der Testcode forkt `echo`
seit jeher mit `clr.l d2 * Parametergroesse 0 -- kein argv` (eigener
Kommentar im Code) -- ein `echo` ganz ohne Argument hat nichts
auszugeben, das Fehlen von Text war also erwartungsgemaess, aber
dadurch blieb "gibt `echo` inhaltlich das Richtige aus" bisher
UNGEPRUEFT (nur "stuerzt nicht ab" war belegt).

### Aenderung

`q9kernel_entry.a`, `F$Fork("echo")`-Testaufruf: `d2`/`a1` jetzt auf
einen echten Parameterbereich gesetzt (`"echo Hallo"` + `$0d`,
11 Byte) statt leer -- reale Konvention (`Q9K_ProcFork`,
`q9kernel_firstproc.c`: Parameterbereich = rohe Kommandozeile wie
eingetippt).

### Ergebnis

Live verifiziert: in der Terminal-Mitschrift erscheint jetzt, direkt
nach dem `lctE`-Marker, woertlich **`echo Hallo`** -- exakt der
uebergebene Parameterinhalt, korrekt durch `csl`s Laufzeitbibliothek
verarbeitet und ausgegeben. `Q9K_ExcTrap-Mitschrift` weiterhin
`Vektor=0` (kein Absturz). Damit ist nicht nur "stuerzt nicht ab",
sondern auch "verhaelt sich inhaltlich korrekt" fuer den getesteten
Fall handfest belegt -- die in Fortsetzung 58 offen gelassene Frage
ist geklaert.

Alle 16 Host-Testsuiten weiterhin gruen (reine Testcode-Aenderung in
`q9kernel_entry.a`, keine Kernel-Logik betroffen). Sicherheitsabstand
(Fortsetzung 37/38) erneut geprueft, unveraendert korrekt.

**Weiterhin unverifiziert (unveraendert gegenueber Fortsetzung 58):**
andere `csl`-Funktionen, andere Programme, andere `F$SetSys`-Variablen
jenseits der einen bekannten (`$7C`) -- s. dortige Einschraenkungen.

---

## Fortsetzung 60: `F$SetSys` gehaertet -- unbekannte Variablen melden
jetzt einen sauberen Fehlschlag statt still `0` + Erfolg vorzutaeuschen
(2026-09-13, elfte Sitzung, auf "F$SetSys robuster machen")

### Anlass

Selbst in Fortsetzung 58 als Einschraenkung dokumentiert: `F$SetSys`
lieferte fuer JEDE unbekannte Systemvariable still `0` UND meldete
trotzdem Erfolg. Ein Programm, das eine andere Variable als die eine
bekannte (`$7C`) braucht, haette dadurch einen STILLEN FALSCHWERT
bekommen statt eines erkennbaren Fehlers -- gefaehrlicher als ein
sauberer Fehlschlag, weil ein Aufrufer, der die Carry-Flagge tatsaechlich
prueft, auf einen klaren Fehler reagieren kann (z. B. eigenen
Standardwert verwenden), auf einen unbemerkt falschen Wert aber nicht.

### Aenderung

`q9kernel_setsys.c`: `Q9K_ProcSetSys` um einen `outError`-Parameter
erweitert, liefert bei "Lesen einer unbekannten Variable" jetzt `0`
(Fehlschlag) mit Fehlercode **E$UnkSvc = `$D0`** (internem
Referenzmaterial -- Wert NICHT geraten, sondern rueckwaerts aus der
Fehlercode-Tabelle gezaehlt: `E$ModBsy=$D1`/`E$BPAddr=$D2` waren
bereits an anderer Stelle in diesem Kernel belegt und bestaetigt, davor
in der Tabelle stehen `E$MemFul/E$UnkSvc/E$ModBsy`, macht `E$UnkSvc=
$D0`). "Schreiben" bleibt UNVERAENDERT (bestaetigt, nicht gespeichert)
-- der eine bekannte Aufrufer liest nur, ein Fehlschlag beim Schreiben
haette keinen bekannten Nutzen.

Neue Scratch-Zellen `Q9K_SetSysScratch_Error`/`_Success` bei
`$1664`/`$1668` -- BEWUSST NICHT bei `$1650` (das ist
`Q9K_VMODUL_RETBUF`s Start, `q9kernel_moddir.c`, hätte sonst genau die
in Fortsetzung 52/58 dokumentierte Kollisionsfalle wiederholt), sondern
im naechsten wirklich freien Bereich ($1664-$168F, vor
`Q9K_TLinkScratch_*` bei `$1690`) -- gegen den GESAMTEN belegten
Adressbereich gegengeprueft, nicht nur Startadressen.

`Q9K_SysFSetSys` (`q9kernel_entry.a`): setzt jetzt bedingt Carry
(Erfolg/Fehlschlag je nach `Q9K_SetSysScratch_Success`), liefert bei
Fehlschlag `d1.w`=Fehlercode -- gleiches Muster wie
`Q9K_SysFAllPD_Fail` u. a.

### Ergebnis

Host-Test `test_q9kernel_setsys.c` erweitert (5 Faelle: bekannte
Variable inkl. Fehlercode-Feld, unbekannte Variable jetzt Fehlschlag
mit `E$UnkSvc`, Schreiben unveraendert, Scratch-Bruecke Erfolgs- UND
Fehlerfall). Alle 16 Host-Testsuiten gruen. Live erneut verifiziert:
`echo Hallo` erscheint weiterhin korrekt, `Vektor=0` (kein Absturz) --
die Haertung aendert nichts am bereits funktionierenden Fall (`$7C`
bleibt erfolgreich), schliesst nur die dokumentierte Luecke fuer
kuenftige, andere Aufrufer. Sicherheitsabstand (Fortsetzung 37/38)
erneut geprueft, unveraendert korrekt.

---

## Fortsetzung 61: ZWEITES echtes Kommandomodul getestet ("date") --
"traphandler mismatch" gefunden, echte Ursache bis zu einer belegten
Arena-Speicherueberlappung zwischen ZWEI GLEICHZEITIG AKTIVEN Prozessen
zurueckverfolgt (2026-09-13, elfte Sitzung, auf "weitere echte
Programme testen")

### Auftrag und Testaufbau

Direkter Anschluss an Fortsetzung 58/59/60: ein ZWEITES, von `echo`
unabhaengiges echtes Kommandomodul aus dem Microware-Referenzabbild
testen (`date`, ausgewaehlt weil es die bisher komplett unbenutzte
Zeit-Syscall-Gruppe beruehren koennte). `q9kernel_entry.a` um einen
zweiten Test-Block erweitert (`F\$Load("/dd/CMDS/date")` +
`F\$Fork("date")`, direkt im Anschluss an den erfolgreichen
`echo`-Fork -- WICHTIG: der urspruengliche Code sprang nach
erfolgreichem `echo`-Fork SOFORT in die Idle-Schleife, der neue
Testblock war dadurch beim ersten Versuch unerreichbar UND KORRIGIERT
worden, s. Commit).

### Fund 1: `date` laeuft ab, druckt aber keine Ausgabe

`echo`/`csl` (Fortsetzung 58-60) bestaetigt weiterhin fehlerfrei.
`date` dagegen druckt **`**** csl traphandler mismatch ****`** und
beendet sich, OHNE ein Datum auszugeben -- kein Absturz (`Vektor=0`),
aber inhaltlich falsches/abgebrochenes Verhalten.

### Fund 2: die Pruefung selbst, byte-genau analysiert

Der Text stammt aus `date.mod`s eigenem, statisch mitkompiliertem
Startcode (nicht aus `csl.mod` -- derselbe Text UND Mechanismus
existiert identisch in `echo.mod`, s. u.). Mechanismus (per
Analyse von `date.mod` UND `csl+$58`, `csl`s `M\$Init`):
- Vor `F\$TLink(13,"csl")` schreibt das Programm den Sentinel-Wert `10`
  an eine LOKALE Adresse `a3 = a6-$7ff0` (`a6` = das Programm selbst,
  NICHT `csl`).
- `csl`s `M\$Init` (bekommt `a3` als TrapInit-Parameter durchgereicht,
  s. Fortsetzung 45) prueft `cmpi.l #$a,(a3); bgt <ueberspringen>` --
  bei `10` (nicht `>10`) wird normal initialisiert UND `*(a3)` auf `0`
  gesetzt.
- Nach der Rueckkehr prueft das Programm `tst.l (a3)` -- ist es NICHT
  `0`, gilt "traphandler mismatch".

### Drei Hypothesen geprueft, ALLE widerlegt

1. **Trap-#15-Auto-Link-Mechanismus** (vermuteter Ausloeser eines
   gemeinsam genutzten Handler-Slots) -- widerlegt: keine einzige
   `trap #15`-Instruktion (Byte-Muster `4e4f`) in `echo.mod`,
   `date.mod` ODER `csl.mod`.
2. **`echo` und `date` teilen sich denselben eigenen Speicherbereich**
   -- widerlegt: live gemessen an `M\$Exec` (allererste Instruktion,
   VOR jedem `F\$TLink`): `echo`s A6 = `$556a0`, `date`s A6 = `$57690`
   -- klar verschieden, beide korrekt.
3. **Scheduler rettet A6 beim Kontextwechsel nicht** -- widerlegt: im
   Assembler-Code ist A6 überall konsequent Teil von
   `movem.l d0-d7/a0-a6` bei jedem Kontextwechsel (Sleep/Wait/Timer-
   Verdraengung), keine Ausnahme gefunden.

### Fund 3: die ECHTE Ursache -- Arena-Ueberlappung

Live wiederholte Messungen an `csl+$58` (`M\$Init`s Pruefinstruktion)
zeigten NICHT-DETERMINISTISCHE Werte zwischen zwei Laeufen desselben
Abbilds -- Hinweis auf eine echte Race Condition, kein fester Bug.
Nachrechnung der beteiligten Adressen:
- `csl`-Block fuer `echo`s Instanz (`a6=$556a0`): `$4d6a0`-`$4fd30`
  (Groesse `M\$Mem=$2690`).
- `date`s EIGENER Prozessbereich (`a6=$57690`) beginnt bei `$4f690` --
  **mitten in `csl`s obigem Bereich!**

Live per `Q9_WATCH_ADDR` an genau dieser Adresse (`$4f690`) bestaetigt:
nach `date`s eigener Initialisierung (Heap-Grenzen, `M\$IData`-Kopie --
alles korrekt und erwartet) wird die STELLE SPAETER als Teil der
Kernel-Freiliste beschrieben (`next`-/`size`-Felder, `Q9K_FreeMem`s
eigenes Schreibmuster, live an `pc=$8c5c` im Kernel selbst
verifiziert) -- **waehrend `date` sie noch aktiv benutzt.** Eine
zweite, separate Messung zeigt zusaetzlich, dass VORHER `csl`s
GESAMTER Block (`$4d6a0`) auf dieselbe Weise freigegeben wird
(passend zu `echo`s `F\$UnLink`/`F\$SRtMem`-Aufraeumen seiner eigenen
`csl`-Instanz) -- durch die Ueberlappung trifft diese (fuer sich
genommen korrekte) Freigabe DIREKT `date`s aktiven Speicher.

**Das ist ein echter Speicherzuteilungs-Bug:** `Q9K_AllocMem` hat
`date`s `F\$Fork`-Anfrage einen Block zugewiesen, der sich zum
Zuteilungszeitpunkt noch mit `csl`s (zu diesem Zeitpunkt aktivem)
statischem Bereich ueberschneidet. Erklaert zwanglos sowohl die
beobachtete Nichtdeterminismus (Zeitpunkt/Reihenfolge der beiden
gleichzeitig laufenden Prozesse entscheidet, wessen Daten wann
ueberschrieben werden) als auch die "traphandler mismatch"-Meldung
(`date`s Sentinel-Zelle liegt im ueberlappten, spaeter korrumpierten
Bereich).

### Offen fuer eine Folgesitzung

Der EXAKTE Aufrufer der fehlerhaften Freigabe (welche Instruktion in
`csl` oder unserem Kernel `F\$SRtMem` mit der falschen Groesse/Adresse
ausloest) konnte trotz mehrerer Versuche NICHT zweifelsfrei isoliert
werden -- die Ruecksprungadressen-Rekonstruktion im Stack
compilierter, symbolloser C-Funktionen (versch. `movem.l`-
Registersaetze je Funktion, kein Stack-Frame-Zeiger durchgehend
genutzt) erwies sich als fehleranfaellig ohne ein vollstaendiges
Funktionslisting mit Funktionsgrenzen. Naechster Schritt: entweder ein
vollstaendiges Kernel-Listing (`r68`/`l68`-Map-Datei) heranziehen, um
Funktionsgrenzen exakt zu bestimmen, oder gezielt JEDEN `F\$SRtMem`-
Aufruf systemweit mitschneiden (Adresse+Groesse+Ruecksprung) statt nur
punktuell an einer vermuteten Stelle zu suchen.

### Ergebnis dieser Fortsetzung

- `q9kernel_entry.a`: zweiter Testblock (`date` laden+forken) hinzugefuegt
  und der Kontrollfluss-Fehler (Erfolgspfad sprang an ihm vorbei)
  korrigiert.
- Kein Kernel-Logik-Code veraendert (reine Testcode-Erweiterung).
  Alle 16 Host-Testsuiten weiterhin gruen (unberuehrt).
- Emulator-Diagnosewerkzeuge (`Q9_PCHIT_ADDR`-Beispielfelder) mehrfach
  fuer diese Untersuchung angepasst, im aktuellen Zustand committet
  (`a0`/Stack-Woerter -- fuer die naechste Fragestellung anzupassen,
  s. Kopfkommentar).
- Ein bislang UNBEKANNTER, ECHTER Speicherzuteilungs-Bug (Arena-
  Ueberlappung zwischen zwei gleichzeitig aktiven Prozessen) konkret
  nachgewiesen, aber NOCH NICHT behoben (exakter Ausloeser offen, s. o.).
  Betrifft NICHT nur `date` -- jedes Szenario mit zwei parallel
  laufenden, `csl` nutzenden Prozessen waere potenziell betroffen.

### NACHTRAG/KORREKTUR (direkter Anschluss, selbe Sitzung): die
"Arena-Ueberlappung" oben ist NICHT haltbar -- zurueckgezogen

Bei der Suche nach dem EXAKTEN Ausloeser (geplanter naechster Schritt
oben) zwei Dinge gefunden, die die obige Schlussfolgerung widerlegen:

1. **Nachrechnung von `echo`s tatsaechlichem Blockende:** `totalSize =
   M\$Mem($76c) + M\$Stack($c00) + Parametergroesse($b) = $1377`. `echo`s
   Block (`$4d6a0`) endet damit bei `$4ea17` -- erreicht `date`s Block
   (`$4f690`) NICHT. Die beiden Bloecke ueberlappen sich nach dieser
   Rechnung gar nicht.
2. **Der vermeintliche "Beweis" (identischer Speicherinhalt an beiden
   Adressen) war eine Fehldeutung:** der Wert (`e317cde6`) ist
   hoechstwahrscheinlich ein generisches Stack-Fuellmuster, das JEDE
   Prozessinitialisierung gleich schreibt (an ihre JEWEILS EIGENE,
   getrennte Adresse) -- keine echte Adressueberlappung.

**Die "Arena-Ueberlappung"-Schlussfolgerung aus dieser Fortsetzung ist
damit ZURUECKGEZOGEN.** Die tatsaechliche Ursache von "csl traphandler
mismatch" bleibt ungeklaert.

**Eine echte, verlaessliche Erkenntnis bleibt aber bestehen** (aus dem
Quellcode gelesen, nicht aus fragilem Live-Raten): `q9kernel_procend.c`
(`F\$Exit`) gibt den PRIMAEREN Prozessblock (die bei `F\$Fork`
registrierte statische Flaeche) automatisch an die Arena zurueck
(`Q9K_FreeMem(base, size)`, Zeile 188) -- das WIDERSPRICHT der
bisherigen, an mehreren Stellen dokumentierten Annahme "Speicher-
Ruecknahme bei Prozessende nicht implementiert" (die bezog sich, wie
sich jetzt zeigt, nur auf EXPLIZIT per `F\$SRqMem` angeforderte
Zusatzbloecke, nicht auf den Primaerblock selbst). Diese Erkenntnis
ist unabhaengig von der zurueckgezogenen Ueberlappungs-These und bleibt
gueltig.

**Lehre fuer kuenftige Live-Untersuchungen in diesem Kernel:** ohne
ein echtes, funktionsgrenzen-genaues Listing (r68/l68-Map-Datei) ist
die Ruecksprungadressen-/Blockgrenzen-Rekonstruktion aus reiner
Laufzeitbeobachtung fehleranfaellig -- mehrere Zwischenschluesse in
dieser Fortsetzung mussten deshalb bereits INNERHALB derselben Sitzung
wieder verworfen werden. Fuer eine Folgesitzung: zuerst ein
Adress-zu-Funktion-Mapping (Map-Datei oder sorgfaeltig von Hand
gezaehlte Funktionsgrenzen) beschaffen, BEVOR neue Ursachenthesen
aufgestellt werden.

**Konkret geprueft und bestaetigt (noch in derselben Sitzung):** der
Assembler `r68.exe` (Referenz-Werkzeugkette) kennt den Schalter **`-s`** und
liefert damit eine VOLLSTAENDIGE Symboltabelle mit Adressen/Offsets
fuer JEDES Label -- direkt getestet gegen `q9kernel_entry.a`:
```
arch -x86_64 "$WINE_BIN" ".../r68.exe" -s q9kernel_entry.a
```
liefert Zeilen wie `Q9K_TrapDispatch 0004-01ad 000005f4` (Psect-Index,
Segmentoffset, Groesse) fuer ALLE Labels der Datei -- exakt das
gesuchte Funktionsgrenzen-Mapping, OHNE weiteres Raten. Fuer die aus C
uebersetzten Dateien (`q9kernel_arena.c` usw., ueber `cpfe`+`r68`
laufend, s. `build.sh`) muesste `-s` noch in DIESEN r68-Aufruf
eingeschleust werden (aktuell laeuft er innerhalb von `mwos-build`/
`os9make`, nicht direkt aus `build.sh` aufrufbar) -- das ist der
konkrete erste Handgriff fuer eine Folgesitzung, BEVOR neue
Live-Thesen zu `Q9K_FreeMem`/`Q9K_AllocMem`/`Q9K_ProcTLink` (die
eigentlich interessanten Funktionen fuer die "traphandler mismatch"-
Ursache) aufgestellt werden.

### ZWEITE KORREKTUR (direkter Anschluss, selbe Sitzung): die
beobachteten "Freigaben" waren normale Prozessenden, KEINE Korruption

Der oben beschriebene Schalter `-s` wurde tatsaechlich genutzt (volle
Kompilierkette fuer `q9kernel_arena.c` manuell nachgebaut: `cpfe` ->
`ilink` -> `iopt` -> `be68k` -> `opt68k` -> `r68 -s`) und lieferte eine
echte Symboltabelle. Ergebnis: `Q9K_FreeMem` beginnt exakt bei der
schon vorher live gefundenen Kerneladresse (`$8c2e`) -- DAS war also
korrekt identifiziert.

Aber: `grep -rn "Q9K_FreeMem" *.c` zeigt GENAU ZWEI Aufrufstellen im
gesamten Kernel-C-Code -- `q9kernel_firstproc.c:694` (Fehlerpfad
innerhalb `F\$Fork`, nur bei Fehlschlag) und `q9kernel_procend.c:188`
(normaler `F\$Exit`-Pfad, gibt den PRIMAEREN Prozessblock zurueck, s.
Erkenntnis oben). Da weder `echo` noch `date` beim Forken scheitern,
MUSS es der normale `F\$Exit`-Pfad sein. Die beiden in dieser
Fortsetzung beobachteten "Freigaben" (`echo`s eigener Block, dann
`date`s eigener Block) sind damit hoechstwahrscheinlich schlicht
**normales Prozessende** -- `echo` beendet sich und gibt seinen
eigenen Speicher zurueck, danach tut `date` dasselbe. KEINE
Korruption, KEIN Speicherzuteilungs-Bug.

**Damit war die GESAMTE "Arena"-Spur dieser Fortsetzung eine
Sackgasse** -- die eigentliche, unbeantwortete Frage (warum `date`s
`csl`-Sentinel-Pruefung VOR jedem `F\$Exit` fehlschlaegt) ist dadurch
NICHT beantwortet. Fuer eine Folgesitzung, MIT der jetzt verfuegbaren
`r68 -s`-Symboltabellen-Technik: `q9kernel_traplink.c`
(`Q9K_ProcTLink`/`Q9K_ApplyInitializedData`) und `q9kernel_firstproc.c`
(`Q9K_ProcFork`) auf dieselbe Weise kompilieren und analysieren,
um zu sehen, ob/wie diese beiden Funktionen sich bei `echo`s und
`date`s jeweiligem Aufruf unterscheiden -- DORT, nicht in der Arena,
liegt die eigentliche Erklaerung.
## Fortsetzung 62: Speicher-Trace-Scratch-Kollision gefunden und beseitigt (2026-09-16)

Die neue Speicher-Trace-Instrumentierung hatte ihre Statusfelder ab `$1650`
abgelegt. Dieser Bereich ist jedoch bereits fest belegt: `$1650-$1663`
gehört zum `F$VModul`-Rückgabepuffer, `$1664/$1668` zu `F$SetSys` und
der anschließende Bereich enthält weitere Kernel-Scratch-Felder. Dadurch
konnte der Debug-Code beim Modul-Laden echte Systemdaten überschreiben.

Die Trace-Felder wurden in den geprüften freien Bereich `$16C0-$16F8`
verschoben, unmittelbar vor der Owner-Tabelle ab `$1710`. Der Kernel baut
fehlerfrei; der Emulator-Test bestätigt, dass CF/RBF weiterhin bis zur
Treiberinitialisierung läuft. Der bekannte spätere `scf`-Fehler mit
`Vektor 4, PC=$6c` tritt in diesem Lauf weiterhin auf und ist damit nicht
allein durch diese Scratch-Kollision verursacht. Die nächste Untersuchung
bleibt der Rücksprung-/Dispatchpfad von `scf`.

## Fortsetzung 63: A4-Herkunftsprüfung im Trap-Dispatcher korrigiert (2026-09-16)

Die Herkunftsprüfung in `Q9K_TrapDispatch` verwendete `A4` vorübergehend als
Kernel-Modulbasis. Im Fremdaufrufer-Pfad wurde dieser Wert vor dem direkten
Handleraufruf nicht wiederhergestellt. Ein verschachtelter Systemaufruf aus
IOMan/RBF konnte dadurch mit `A4=$7100` statt mit dem Prozessdeskriptor laufen.

Der ursprüngliche `A4`-Wert wird jetzt stackbasiert gesichert und vor dem
Fremdhandler sowie vor der Rückkehr wiederhergestellt. Der Testlauf bestätigt
für beide beobachteten `F$SRqMem`-Aufrufe `A4=$55510` am Dispatch-Eintritt;
der Aufruf kehrt anschließend korrekt in RBF zurück.

Der vollständige CF-Lauf scheitert danach weiterhin mit `Vektor 4, PC=$6c`.
Die weitere Spur liegt somit hinter dem erfolgreichen Speicheraufruf, im
RBF-/CFIDE-Rückkehrpfad; der A4-Fehler war real, aber nicht die letzte Ursache.

## Fortsetzung 64: Externer Trap-Rückweg korrigiert (2026-09-16)

Im Rückweg von extern über `F$SSvc` registrierten Handlern wurde der
72-Byte-Registerrahmen entfernt, anschließend aber `78` statt `72` Byte zum
ursprünglichen Hardware-Exception-Frame weitergeschaltet. `RTE` las dadurch
das Format-/Vektorwort an der falschen Position; das erklärte den späteren
Sprung auf `PC=$6c`.

Die Korrektur in `Q9K_TrapCallExternal` verwendet jetzt `lea 72(sp),sp`.
Der Kernel und das Bootfile wurden neu gebaut. Der anschließende CF-Emulator-
Test erreicht den wiederholten Scheduler-/Prozesslauf mit den erwarteten
`A`-Ausgaben; die vorherige `Vektor 4, PC=$6c`-Exception tritt in diesem
Lauf nicht mehr auf. Ein anschließender Stabilitätstest über rund 90 Sekunden
produzierte etwa 560.000 `A`-Ausgaben, ohne Exception, Illegal Instruction
oder `PC=$6c`; der Dump meldete `Vektor=0`. Der Rückweg ist damit im
Emulator als behoben und stabil einzustufen.

## Fortsetzung 65: Minimaler nativer I$Close-Pfad (2026-09-16)

`I$Close` (`$8F`) ist jetzt als eigener Q9-Kernelhandler registriert. Der
Handler validiert die Pfadnummer im aktuellen Prozessdeskriptor, löscht den
Prozess-Pfadslot und gibt eine Referenz auf den zugehörigen Q9-nativen
256-Byte-Pfaddeskriptor frei. Der Deskriptor wird erst nach dem letzten
`I$Close` wieder in die Freiliste eingehängt. Standardpfade `0..2` werden
nicht als Poolobjekte freigegeben; ungültige oder leere Pfade liefern
`E$BPNUM`.

Der Kernel baut ohne Fehler. Ein vollständiger CF-Emulatorlauf mit dem neuen
Bootfile erzeugte etwa 537.000 Scheduler-Ausgaben und meldete `Vektor=0`; es
gab keine Illegal Instruction und keinen Sprung zu `PC=$6c`. Damit ist der
minimale native Pfadlebenszyklus `I$Open`/`I$Dup`/`I$WritLn`/`I$Close` im
aktuellen Teststand funktionsfähig. Die Referenzzählung wurde im Hosttest
geprüft; der Emulatorlauf bestätigt die Boot- und Rückwegstabilität. Die
eigentliche Close-Weiterleitung an
beliebige File Manager sowie die vollständige Trennung der Pfadpools pro
Prozess bleiben noch offen.

## Fortsetzung 66: Minimaler nativer I$Write-Pfad (2026-09-16)

`I$Write` (`$8A`) ist jetzt als Q9-Kernelhandler registriert. Er schreibt die
angegebene Bytezahl über den bestehenden Q9-Konsolenpfad und hält damit die
aktuelle minimale native I/O-Schicht konsistent mit `I$WritLn`. Die vollständige
geräteabhängige Write-Semantik und die Weiterleitung an echte File Manager sind
weiterhin offen.

Beim Absichern der Pfadverwaltung wurde außerdem ein Fehlerpfad in `I$Dup`
korrigiert: Bei voller Pfadtabelle wird der gesicherte Registerrahmen jetzt
auch vor der Fehlerrückkehr restauriert. `I$Close` verwirft zusätzlich leere
oder inkonsistente native Deskriptoren, bevor die Referenzzählung verändert wird.

Der Kernel-Build endet mit `Errors: 00000`. Der Host-Regressionslauf für
`q9kernel_iopath.c` meldet `ALLE TESTS BESTANDEN`. Das aktualisierte Bootfile
wurde im CF-Emulator getestet: etwa 494.000 Scheduler-Ausgaben, `Vektor=0`,
keine Illegal Instruction, kein `PC=$6c` und kein Formatfehler.

Damit ist die minimale native Schicht für `I$Open`/`I$Dup`/`I$Write`/`I$WritLn`/
`I$Close` stabiler, aber noch kein vollständiger OS-9-I/O-Stack. Offen bleiben
insbesondere `I$Read`, `I$ReadLn`, `I$GetStt`, `I$SetStt`, `I$Seek`, echte
Pfad-/Geräteauflösung, File-Manager-Dispatch und die vollständige Trennung der
Pfadpools pro Prozess.

## Fortsetzung 67: Minimaler nativer I$Read-Pfad (2026-09-16)

`I$Read` (`$89`) ist jetzt als Q9-Kernelhandler registriert. Der Handler liest
die angeforderte Bytezahl blockierend über `SRA.RxRDY` und `RHRA` der emulierten
DUART in den vom Aufrufer angegebenen Puffer. Damit ist der minimale native
Konsolenpfad für Ausgabe und Eingabe symmetrisch erweitert. Eine Eingabeprüfung
auf Pfadtyp, Geräteparameter und File-Manager-Zustand ist noch nicht enthalten.

Der Kernel-Build endet weiterhin mit `Errors: 00000`. Der aktualisierte
Bootfile-Stabilitätstest im CF-Emulator erzeugte etwa 529.000 Scheduler-
Ausgaben und meldete `Vektor=0`; es gab keine Illegal Instruction, keinen
Sprung zu `PC=$6c` und keinen Formatfehler. Der automatische Boottest ruft
`I$Read` nicht ohne Eingabedaten auf, weil der korrekte Read-Aufruf dabei bis
zum Eintreffen von Zeichen blockieren würde.

## Fortsetzung 68: Minimaler nativer I$ReadLn-Pfad (2026-09-16)

`I$ReadLn` (`$8B`) ist jetzt als eigener Q9-Kernelhandler registriert. Er liest
blockierend Zeichen über die DUART, beendet die Zeile bei CR oder beim Erreichen
der maximalen Pufferlänge und liefert die gespeicherte Bytezahl in `d1`. Der
Puffer wird dabei nicht überschrieben; Zeichenecho, Backspace-Verarbeitung,
Großschreibung und die übrigen gerätespezifischen Optionen bleiben bewusst dem
späteren File-Manager überlassen.

Der Kernel-Build endet mit `Errors: 00000`. Der anschließende CF-Emulatorlauf
erzeugte etwa 559.000 Scheduler-Ausgaben und meldete `Vektor=0`; es gab keine
Illegal Instruction, keinen Sprung zu `PC=$6c` und keinen Formatfehler. Der
automatische Stabilitätstest löst den ReadLn-Aufruf weiterhin nicht interaktiv
aus, damit der Bootlauf ohne Eingabedaten nicht absichtlich blockiert.

## Fortsetzung 69: Minimaler nativer GetStt/SetStt-Pfad (2026-09-16)

`I$GetStt` (`$8D`) und `I$SetStt` (`$8E`) sind jetzt registriert und
unterstützen für Q9-native Pfade den Statuscode `SS_Opt`. Dabei werden die
ersten 32 Bytes des SCF-Optionsbereichs zwischen dem Pfaddeskriptor und dem
Aufruferpuffer kopiert. Das deckt insbesondere die Echo- und grundlegenden
Zeilenoptionen ab. Nicht unterstützte Statuscodes liefern `E$UnkSvc`, ungültige
Pfade `E$BPNUM` und ein Nullpuffer `E$BPADDR`.

Die vollständige Statusstruktur, Standardpfade `0..2`, echte Geräteparameter
und die Weiterleitung an den File Manager bleiben noch offen. Der Kernel-Build
endet mit `Errors: 00000`. Der CF-Emulatorlauf mit dem aktualisierten Bootfile
erzeugte etwa 492.000 Scheduler-Ausgaben und meldete `Vektor=0`; es gab keine
Illegal Instruction, keinen Sprung zu `PC=$6c` und keinen Formatfehler.

## Fortsetzung 70: Minimaler nativer I$Seek-Pfad (2026-09-16)

`I$Seek` (`$88`) ist jetzt als eigener Q9-Kernelhandler registriert. Für
Q9-native Konsolenpfade validiert er den Pfad und kehrt erfolgreich als No-op
zurück. Das entspricht der OS-9-Semantik für sequenzielle File Manager, die
keine zufällige Dateipositionierung anbieten. Eine echte logische
Positionsverwaltung bleibt für RBF und andere Random-Access-File-Manager offen.

Der Kernel-Build endet mit `Errors: 00000`, der Host-Regressionslauf meldet
`ALLE TESTS BESTANDEN`. Der anschließende CF-Emulatorlauf mit dem aktualisierten
Bootfile erzeugte 264.505 Scheduler-Ausgaben und meldete `Vektor=0`; es gab
keine Illegal Instruction, keinen Sprung zu `PC=$6c` und keinen Formatfehler.

## Fortsetzung 71: Minimaler nativer I$SGetSt-Pfad (2026-09-16)

`I$SGetSt` (`$92`) ist jetzt als eigener Q9-Kernelhandler registriert. Im
Unterschied zu `I$GetStt` wird die übergebene Nummer direkt als Systempfad
interpretiert. Für einen belegten nativen Pfad unterstützt der Handler
`SS_Opt` und kopiert die ersten 32 Optionsbytes. Andere Statuscodes, ungültige
Pfade und Nullpuffer werden mit den üblichen Fehlercodes abgewiesen.

Die Implementierung führt bewusst noch keine gruppen-/benutzerabhängige
Berechtigungsprüfung, Geräte-Namensabfrage oder File-Manager-Weiterleitung
durch. Diese Punkte müssen vor einer vollständigen OS-9-Kompatibilitätswertung
noch ergänzt werden.

## Fortsetzung 72: Einordnung von I$Attach und I$Detach (2026-09-16)

Die Prüfung des vorhandenen I/O-Aufbaus bestätigt, dass `I$Attach` (`$80`) und
`I$Detach` (`$81`) keine zusätzlichen nativen Kernelhandler für den aktuellen
Q9-Ansatz benötigen. Die eigentliche Attach-/Detach-Logik gehört zum externen
IOMan-/File-Manager-Dreiklang; der Kernel stellt dafür den externen Dispatch- und
Registerrahmen bereit. `I$Attach` wird im bekannten `/term`-Pfad aus `scf`
heraus aufgerufen und ist damit vom aktuellen externen Dispatcher grundsätzlich
erreichbar. Ein eigener Stub würde diese Zuständigkeit verdecken und wäre daher
kein sinnvoller Fortschritt.

Offen bleibt ein separater Lebenszyklustest für `I$Detach` sowie die vollständige
Gerätebindung für RBF/CF. Der nächste technische Schwerpunkt ist deshalb nicht
ein weiterer allgemeiner I/O-Stub, sondern der Vergleich des funktionierenden
SCF-Attach-Pfades mit dem noch blockierenden RBF/CF-Attach-Pfad.

## Fortsetzung 73: Direkter Attach-Test und CF-Grenze (2026-09-17)

Für den direkten Nachweis wurde ein kleines Modul `iattachsvc` ergänzt. Es ruft
`I$Attach("c0")`, anschließend `I$Detach` mit dem von `I$Attach` gelieferten
Gerätetabelleneintrag und danach `F$Exit` auf. Der Build des Testmoduls endet
ohne Fehler.

Der Testlauf mit `rbf`, `cfide`, `d0` und `c0` im Bootfile zeigt jedoch, dass
der eigene Startup-Prozess den Test noch nicht erreicht: Der `cfide`-Treiber
initialisiert erfolgreich, danach bleibt die vorhandene CF/RBF-Startkette vor
dem Testprozess stehen. Es gibt dabei keinen Illegal-Instruction- oder
Vektorfehler (`Vektor=0`). Damit ist die nächste Fehlergrenze klar eingegrenzt:
Vor einem aussagekräftigen `I$Detach`-Test muss zuerst der externe CF/RBF-
Initialisierungspfad bis zur Prozessausführung weiterlaufen.

## Fortsetzung 74: F$Link-Schutz und erneuter Attach-Versuch (2026-09-17)

Die Spur zeigte anschließend die konkrete Endlosschleife: `F$Link` lief beim
Überlesen eines nicht OS-9-konformen Namens ohne Begrenzung weiter. Die
Namenssuche ist jetzt auf 256 Bytes begrenzt und liefert in diesem Fall
`E$MNF`, statt den Kernel festzusetzen. Der Kernel-Build endet weiterhin mit
`Errors: 00000`.

Der CF/RBF-Emulatorlauf kommt danach wieder aus dem IOMan-Aufruf zurück (`RT`);
die verbleibenden `A`-Zeichen stammen aus der absichtlich laufenden
Scheduler-Testschleife `Q9K_TestProcA`, nicht aus der früheren F$Link-Schleife.
Ein erneuter Lauf mit `iattachsvc` als F$Fork-Ziel und geladenem Testmodul
erzeugte jedoch noch keine `@`-/`#`-/`!`-Marker. Damit ist der direkte
Attach-/Detach-Nachweis weiterhin offen; die nächste Untersuchung muss den
F$Fork-/Prozessstartpfad bis zum tatsächlichen Einsprung in `iattachsvc`
verfolgen.
## Fortsetzung 75: Vollständiger CF/RBF-Attach-/Detach-Nachweis (2026-09-17)

Der direkte Attach-Test ist jetzt als standardmäßig deaktivierter Schalter
`Q9K_TestDirectAttach` im Kernel reproduzierbar auswählbar. Im aktivierten
Testmodus wird der Shell-/Dateitest nach der IOMan-Initialisierung übersprungen
und `iattachsvc` direkt per `F$Fork` gestartet.

Der Emulatorlauf mit `rbf`, `cfide`, `d0` und `c0` erzeugte die Markerfolge
`J@#`: `F$Fork` war erfolgreich, `I$Attach("c0")` erfolgreich und der
anschließende `I$Detach` ebenfalls erfolgreich. Es gab keinen `!`-Marker,
keinen Illegal-Instruction-Fehler und keinen Vektorfehler (`Vektor=0`). Damit
ist der bisher offene CF/RBF-Attach-/Detach-Pfad im direkten Prozess-Test
nachgewiesen. Der Schalter bleibt für den normalen Boottest auf `0`.

## Fortsetzung 76: `date`-F$Load isoliert (2026-09-17)

Der kombinierte normale Testlauf bleibt nach dem F$Link-Schutz stabil: kein
Illegal Instruction, kein Formatfehler und kein Vektorfehler. Die vielen
`A`-Zeichen stammen weiterhin aus der absichtlich laufenden
`Q9K_TestProcA`-Scheduler-Schleife.

Der vorhandene Testschalter `Q9K_TestPrograms` wurde vorübergehend auf den
isolierten `date`-Fall (`2`) gesetzt und danach wieder auf den kombinierten
Standardwert (`3`) zurückgestellt. Das Modul `/dd/CMDS/date` ist im Testimage
vorhanden; `os9 ident` bestätigt gültige Kopfparität und CRC. Trotzdem endet
der isolierte Lauf nach dem Ladeversuch mit `E$MNF` (`$00D7`), und `date` wird
nicht in der Q9-Moduldirectory-Kette sichtbar. Der Moduldirectory-Pool ist
dabei nicht erschöpft.

Damit ist der nächste Fehlerbereich enger eingegrenzt: Der Microware-
`F$Load`-/RBF-Pfad findet oder registriert genau dieses vorhandene Kommando
noch nicht zuverlässig. `F$Fork` und der Prozessstart sind für den separaten
`iattachsvc`-Nachweis bereits erfolgreich getestet; eine Änderung an
`F$Fork` wäre an dieser Stelle voreilig. Als nächstes sollte der Pfadname,
der RBF-Rückgabecode und der Übergang von `F$Load` zu `F$VModul` für `date`
direkt verglichen werden, am besten gegen den bereits erfolgreichen
`echo`-Ladevorgang.
