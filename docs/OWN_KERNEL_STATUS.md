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

## Offene Punkte

### Exception nach dem ersten `I$Write` (nächster Arbeitspunkt)
Der Text wird vollständig ausgegeben, `I$Write` meldet die korrekte
Byte-Zahl zurück — kurz danach löst der DUART-Interrupt eine Exception aus
(**Vektor 4, Illegal Instruction, PC=`$6C`**). Prozess A bleibt danach
stehen, B läuft weiter.

**Was gemessen und damit ausgeschlossen ist:**
- **Kein Interrupt-Sturm.** Vektor 80 kam genau **17 mal** — exakt die 17
  gesendeten Bytes. Der Treiber sendet zeichenweise per Interrupt, das ist
  korrektes Verhalten. (Die frühere Deutung „Sturm" war falsch.)
- **Kein liegengebliebenes Empfangszeichen.** RX-FIFO leer
  (`count=0`, kein Overflow), IMR=`$02`.
- **`D_DevTbl` ist korrekt gefüllt** (Eintrag 0 verweist auf Treiber,
  Deskriptor und File-Manager).
- **Nicht das TxRDY-Zeitverhalten** der Emulation.
- **Nicht Verschachtelung** — der Dispatcher sperrt seit `5467083` die
  Interrupts für den Tabellendurchlauf; die Exception bleibt.

**Ebenfalls ausgeschlossen** (Stand 2026-09-04, jeweils getestet):
- **Nicht der Autovektor-Zweig.** Abgeschaltet — die Exception bleibt.
- **Nicht die fehlende Reentranz des Trap-Rückwegs.** Zwei echte Lücken
  dieser Art wurden dabei gefunden und behoben (`5f00f74`: Epilog auf den
  Stack statt globaler Ablagen, Interruptsperre für den Rückweg) — die
  Exception bleibt trotzdem.

**Zwei Beobachtungen, die den nächsten Ansatz bestimmen:**

1. **Der Erfolgsmarker `O` fehlt in der Ausgabe.** Die Exception schlägt
   also bereits beim *Rücksprung* aus `I$Write` zu, nicht im Testcode
   danach. Prozess B läuft weiter — es stirbt nur A, das in
   `Q9K_ExcTrap` festhängt.
2. **`PC=$6C` ist nur der Sterbeort, nicht die Ursache.** Ein Sprung nach 0
   lässt die CPU durch die Systemglobals laufen (dort stehen Daten, kein
   Code), bis sie bei `$6C` auf etwas Illegales trifft.

**Das Race ist extrem schmal:** Schon *eine einzelne* zusätzliche Instruktion
irgendwo im Kernel entscheidet, ob der Fehler auftritt — mehrfach beobachtet.
Deshalb ist die Instruktionsspur im Emulator (`Q9_TRACE_INSTR=1`) das einzige
brauchbare Werkzeug; jede Diagnose im Kernel verschiebt das Fenster.

**Die Mechanik ist inzwischen lückenlos vermessen** (Instruktionsspur,
24576 Einträge):

- **16 von 17 Dispatcher-Eintritten** erfolgen unmittelbar nach dem `rte`
  des vorherigen Durchlaufs. Der DUART hält seinen Interrupt also
  durchgehend: sobald die Sperre mit dem `rte` fällt, feuert er sofort
  wieder. Der unterbrochene Code kommt während der ganzen Sendephase (17
  Zeichen) nicht ein einziges Mal zum Zug.
- Der `rte` landet dabei auf **`moveq #$f,d2`** — also *nach* der
  `a0`-Initialisierung, aber *vor* dem Zähler. Die Schleife startet dadurch
  mit frischem `d2=15`, aber altem, schon fortgeschrittenem `a0` und wandert
  bei jeder Runde weiter aus der Tabelle heraus, bis Stack und Rahmen nicht
  mehr stimmen und das `rte` nach 0 springt.

Zwei Fixes sind daraus entstanden und bleiben (beide für sich richtig,
keiner beseitigt das Symptom): der Dispatcher sperrt die Interrupts für den
Tabellendurchlauf (`5467083`) und stellt diese Sperre nach dem ISR-Aufruf
wieder her (`4740cce`) — die ISR senkt die Maske, weil sie es muss.

**Nächster Schritt:** Den *geretteten PC im Exception-Frame* bei jedem
Eintritt messen (Instruktionsspur um `a7` erweitern, dann im Dump den Frame
bei `a7+2` lesen). Damit lässt sich klären, warum der Frame-PC `$798e`
lautet, obwohl der äußere Durchlauf an dieser Stelle bereits gesperrt hat —
das ist der letzte offene Widerspruch.

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
