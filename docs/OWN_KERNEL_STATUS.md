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

**Nächster Schritt:** Verfolgen, was zwischen dem zweiten und dem dritten
Wrapper-Aufruf geschieht — also im `I$Open`-Ablauf oberhalb des Wrappers.
Ring-Freeze auf den Wrapper-Einstieg beim **dritten** Treffer (ohne Push)
zeigt, welcher Weg dorthin führt; mit Push fehlt genau dieser.

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
