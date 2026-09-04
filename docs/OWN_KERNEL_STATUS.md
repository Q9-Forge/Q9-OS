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
