# Eigener Q9-Kernel — Stand und offene Punkte

Fortlaufender Status des in `src/kernel/` neu geschriebenen, OS-9/68K-
kompatiblen Kernels. Ergänzt [`OWN_KERNEL_INIT_PLAN.md`](OWN_KERNEL_INIT_PLAN.md)
(dem Plan) um das, was davon **real läuft** — nachgewiesen im
Q9-Flux-Emulator, nicht bloß implementiert.

**Stand: 2026-09-04**, Branch `fix/ccr-error-signaling-flink-funlink` (PR #7).

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

Maßgeblich ist `MWOS/OS9/SRC/DEFS/process.a`. Angeglichen wurde alles, was
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

Das Statiklayout stammt aus `MWOS/OS9/SRC/DEFS/iodev.a`: `V_PORT $00`,
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
(`MWOS/OS9/SRC/DEFS/funcs.a`, `org 0`: S$Kill 0, **S$Wake 1**, S$Abort 2).

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

**Nächster Ansatzpunkt:** `ioman+$11f8` disassemblieren (capstone) und
nachsehen, was IOMan dort vor und nach dem `F$Sleep` erwartet — insbesondere,
welche Bedingung es prüft, bevor es den Lesevorgang mit Carry und `d1 = 0`
abbricht.

### IOMan-Analyse (2026-09-05)

IOMan aus dem Bootfile extrahiert (Laufzeitbasis `$a7a0`, Größe `$161c`) und
mit capstone disassembliert. Gesucht war, warum `I$ReadLn` mit Carry
zurückkehrt, ohne den Puffer zu füllen.

**Alle Syscall-Aufrufe von IOMan aufgelistet.** Das Trampolin-Muster ist im
Code eindeutig erkennbar (`pea <ret>(pc)` / `move.l $XX(a3),-(a7)` /
`movea.l $4XX(a3),a3` / `rts`), der Slot-Offset geteilt durch 4 ergibt den
Callcode. IOMan fordert damit **vier bei uns nicht belegte Dienste** an —
Namen per `MWOS/OS9/SRC/DEFS/funcs.a` ausgezählt:

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
`MWOS/OS9/SRC/DEFS/funcs.a` ab dem bekannten `E$PthFul = $C8`; dieselbe
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
`MWOS/OS9/SRC/DEFS/process.a`). Jedes Altern eines Prozesses schrieb damit
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

*(Nebenbei aus der Disassemblierung: scfs Open holt den Pfadnamen mit
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
System anhängt. Der Code ist per `MWOS/OS9/SRC/DEFS/funcs.a` bestätigt
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
    REF_TAIL_START=0x33d6 REF_TAIL_SPLIT=0x35d2 \
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
Moduloffset `$32e0`). Disassembliert ist er bemerkenswert kurz:

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
Modulbasen im Testimage. Eine Disassemblierung mit der GESTERN gültigen
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
vermutlich die Disassemblierung von scfs internem Attach-Buchführungscode
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
- **`F$ChkMem` meldet immer Erfolg.** Der Kernel hat keinen Speicherschutz
  (keine MMU-Nutzung, keine getrennten Adressräume) — eine ehrliche Prüfung
  hätte keine Datenbasis. Muss mitwachsen, sobald es Adressräume gibt.
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

`MWOS/OS9/68030/PORTS/common/RBF/cfide/cfide_v42.a` liegt vor (passt zur
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
`MWOS/OS9/SRC/DEFS/funcs.a` ein Gerät anhängt, Treiber-Statikspeicher
alloziert und `V_PORT` — Offset `$00`, „Required by kernel in static storage
of all devices", `MWOS/OS9/SRC/DEFS/iodev.a` — aus dem Deskriptor einträgt)
fehlt bei uns (`$64` nicht in der `q9kernel_cinit.c`-Tabelle, fällt auf
`Q9K_SysUnimplemented`). **Das ist aber eine Sackgasse:** Laut
`docs/REVERSE_ENGINEERING.md` (Zeile 2169) ist `$64`–`$70` auch im **echten,
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
(kein Quellcode im MWOS-Baum gefunden, anders als die Gerätetreiber), das
wäre die nächste Disassemblierungsrunde.

**Stand:** Kein Fixversuch. `F$DAttach` NICHT implementieren (Sackgasse,
s. o.). Konkreter nächster Schritt: entweder RBFs eigenen Attach-Code
disassemblieren, oder sc68681s (funktionierenden!) Attach-Weg mit cfides
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
Trampolin-Aufruf für `F$GProcP` disassemblieren (Rücksprungadresse aus dem
Stack unter dem Exception-Frame — `Ret0`/`Ret1` waren beim ersten Versuch
nicht brauchbar, da aus dem inkonsistenten Build gelesen; mit dem jetzt
etablierten Ein-Build-Verfahren neu messen) und mit IOMans bekanntem
Trampolin-Muster vergleichen, um den 2-Byte-Versatz zu erklären.

## RBFs Aufrufstelle disassembliert: echter TRAP #0, plausible Eingabe

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

## Compilierten Code direkt disassembliert — sauber, aber Ursache weiterhin offen

Da der eingebaute Instruktions-Hook in diesem Emulator-Build nachweislich
nicht feuert (früherer Fund, s. o.), kein echtes Einzelschritt-Tracing
möglich. Stattdessen `Q9K_SysGProcPImpl` und `Q9K_ProcLookup` direkt im
kompilierten Kernel-Binary lokalisiert (über die eindeutigen Scratch-
Adressen `$1384`/`$1388`) und von Hand disassembliert.

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
mehrfach heute: Zwischen der Watch-Messung und der RBF-Disassemblierung
hatte sich die Kernelgröße erneut leicht verschoben (14278 → 14222 Byte),
wodurch die Aufrufstelle im RBF-Modul falsch berechnet wurde
(`RBF+$530` statt korrekt `RBF+$588`). Mit frisch ermittelten
Modulgrenzen für **exakt** dieses Testimage neu disassembliert.

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

Statische Disassemblierung (capstone) des unveränderten `rbf.mod` klärt
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
Disassemblierung (capstone, `rbf.mod` ab einem willkürlich gewählten
Byte-Offset `$ea0`) ergab plausibel aussehenden, aber tatsächlich
FALSCH ausgerichteten Code — bestätigt per `Q9_COUNT_PC` an sechs so
gewonnenen Kandidatenadressen: vier der sechs (`$1250`, `$64c`, `$f1c`,
`$f24`) wurden beim echten Boot NIE erreicht (0 Treffer), nur die aus
dem unmittelbaren Kontext übernommene Adresse `$eb2` traf tatsächlich
(2 Treffer, je einmal pro `I$Open`). Capstone synchronisiert sich beim
Start mitten in einer `bsr.w`-Verschiebungskonstante nicht von selbst
auf echte Befehlsgrenzen — jede weitere Adresse aus so einem Lauf ist
erst durch eine LIVE-Messung (Freeze/Count) zu vertrauen, nicht durch
bloßes Ablesen der Disassemblierung.

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
echten Funktionsgrenzen disassemblieren (z. B. beginnend an der
M$Exec-Einsprungadresse aus dem Modulkopf, linear, ohne Bereichslücken)
und JEDE daraus abgeleitete Adresse per `Q9_COUNT_PC`/`Q9_FREEZE_PC`
gegenmessen, bevor sie als gesichert gilt, oder (b) gezielt die
GetStat-Kommunikation zwischen RBF und unserem Treiber/Deskriptor beim
`I$Attach` von `/dd`/`/term` mitschneiden (der eigentliche "Dreiklang"-
Einstieg) und dort direkt nach Feldern suchen, die unverändert in zwei
verschiedene Zielorte kopiert werden.

## DURCHBRUCH: wahre Ursache von $D8 gefunden — kein Bug in RBF, sondern ein Verzeichnis-Lesefehler (2026-09-08)

Nach dem oben dokumentierten Methodenfehler (unausgerichtete
Disassemblierung) folgte eine **vollständige, sauber ausgerichtete**
Disassemblierung von `rbf.mod`, verankert am echten Modulkopf-Feld
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
   `$A9E`/`$D70`/`$1116` bzw. `$1268`/`$1326`) die Disassemblierung ab
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
LIB MWOS README SYS **startup** reinstall.old reinstall.ultra OldBoot
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
zurückgesetzt; die vollständige, ausgerichtete RBF-Disassemblierung
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
verwendet wird. Die vorherige Disassemblierungs-Ausrichtung im Bereich
`$100`-`$200` (I$Attach-Fortsetzung) selbst erwies sich bei einer
Live-Gegenprobe (`Q9_FREEZE_PC` bei `$e22a`/Bittest) ebenfalls als NICHT
durchgängig verlässlich (ein direkt anschließender Sprung passte nicht
zur linear disassemblierten Nachbarschaft) — vermutlich ein weiterer,
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
jedes Ausrichtungsrisiko einer eigenständigen Disassemblierung — die
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
bestätigten PCs, kein Disassemblierungsrisiko):

- Die Bedingung vor der `$32(a1)`-Aktualisierungs-Instruktion (echte
  Adresse `pc=$e25e`, per Probieren mehrerer Startoffsets sauber
  ausgerichtet): `d1 = d1 AND $70(a1)` (Maske `$1FF`, passt zur
  512-Byte-Puffergröße); ist das Ergebnis **Null** (Sektorgrenze
  erreicht), wird `bsr.w $efa8` aufgerufen, sonst wird die Grenze
  übersprungen.
- `$efa8` wird während des Testlaufs nachweislich 7x erreicht
  (`Q9_COUNT_PC`) — der Mechanismus feuert also.
- **Korrektur:** `$efa8` selbst ist bei genauer Live-Disassemblierung
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

**Nächster Schritt:** `e522` live disassemblieren (Ground-Truth-Bytes,
wie oben) -- das ist der Aufruf, der bei JEDER Scan-Iteration
(unabhängig von der Sektorgrenze) erfolgt, und damit ein besserer
Kandidat für den eigentlichen "Eintrag lesen/vergleichen"-Kern als
`efa8`.
