# Kernel: Bauen, Testen, Diagnosewerkzeuge

Operatives Handwerkszeug für die Arbeit am eigenen Q9-Kernel: wie gebaut
und im Emulator getestet wird, welche Diagnosetechniken sich bewährt
haben, und welche Fallen wiederholt Zeit gekostet haben.

Diese Datei sammelt Wissen, das bis 2026-09-20 nur in privaten Notizen
stand und dadurch für andere nicht auffindbar war. Der Stand der
einzelnen Systemaufrufe steht in `STATUS.md`, der Arbeitsstand in
`docs/OWN_KERNEL_STATUS.md`.

> **Pfadhinweis:** Einige Angaben stammen aus älteren Sitzungen. Das Repo
> wurde seither mehrfach umgebaut — der Kernel liegt unter
> `Q9-KERNEL/68k/src/kernel/`, der Emulator unter
> `Q9-Forge/Q9-Flux/Q9-Flux-68k/`. Im Zweifel den Pfad prüfen, nicht die
> Angabe hier glauben.

## Bauen

`./build.sh [ausgabeverzeichnis]` im Kernelverzeichnis (Wine/xcc-Toolchain,
`-O7`). Ergebnis: `q9kernel` plus die Testmodule.

**Neue C-Module gehören ans ENDE der Link-Liste** — an allen fünf Stellen
in `build.sh` (Kopierliste, `all:`-Ziel, Abhängigkeitsregel, Prüfschleife,
Link-Zeile). Grund: der Kernel hat die **16-Bit-Reichweite von `bsr`**
erreicht. Eine neue Funktion in einem früh gelinkten Modul verschiebt alle
später gelinkten, und dann reißt irgendwo ein bestehendes `bsr`. `l68`
meldet das nur als **„operand size error" ohne die Stelle**. Aufrufe aus
`q9kernel_entry.a` in die letzten Module laufen über Zeigerzellen (Muster
`Q9K_StrapImplPtr`, von `q9kernel_cinit.c` beim Booten gefüllt).

**Neuer Assemblercode in `q9kernel_entry.a`** gehört **hinter** die
Syscall-Handler — nie zwischen `__multiply` und `Q9K_TestProcA`. Dort
liegen die Laufzeithelfer (`__multiply`, `__udivide`, `__umodulo`;
Konvention: `d0` = links, `d1` = rechts, Ergebnis in `d0`). Sie fehlten
lange, weil alle C-Dateien nur durch Zweierpotenzen teilten und der
Compiler daraus Shifts machte; neuer C-Code mit echter 32-Bit-Division
durch eine Nicht-Zweierpotenz braucht sie.

**Einzelne Datei prüfen**, ohne den ganzen Kernel zu bauen: Quelldatei und
`q9kernel_config.h` in ein leeres Verzeichnis kopieren, ein `makefile` mit
denselben `CFLAGS` wie `build.sh` erzeugen, `mwos-build . all` aufrufen.
Der Abbruch mit `can't open file, all.r` am Ende ist erwartet (os9make
versucht danach ein Programm `all` zu linken) — entscheidend ist, dass die
`.r`-Datei entstanden ist.

**Compiler-Flags beim Testbau immer ausschreiben**, nicht über eine
Shell-Variable übergeben: in dieser Umgebung wurden sie aus einer Variablen
heraus nicht wortweise gesplittet.

**Linkage-Map:** `l68.exe -s=<datei>.txt` erzeugt eine echte Map mit allen
aufgelösten Symboladressen. PSECT-Adressen entsprechen bei diesem Kernel
direkt den Datei-Byte-Offsets — **außer für den vom Linker erzeugten
Modulkopf** (`$3C` Byte). Das war lange eine Fehlerquelle: jedes am
Modulanfang orientierte PC-relative Label zeigt hinter den Kopf, nicht auf
Offset 0.

**Symboltabelle mit Funktionsgrenzen:** `r68 -s`, auch für aus C übersetzte
Dateien (manuelle Kette: `cpfe` → `ilink` → `iopt` → `be68k` → `opt68k` →
`r68 -s`).

## Hosttests

```
gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
    -o test_q9kernel_<x> test_q9kernel_<x>.c && ./test_q9kernel_<x>
```

**Die beiden `-D`-Schalter sind nicht optional.** `q9kernel_config.h`
verlangt die Variantenwahl per `#error`; ohne sie brechen die meisten
Tests schon beim Übersetzen ab (gemessen: 24 von 27). Wer den Fehlschlag
für ein Testergebnis hält, sucht den Fehler an der falschen Stelle.

Die Tests `#include`n die Kernelquelle direkt und lenken die
Scratch-Adressen per `#define` **vor** dem `#include` auf ein Fake-Array um.

**Falle — 64 Bit gegen 32 Bit:** auf dem Host ist `unsigned long` 64 Bit,
im Kernel 32. Deskriptorfelder, die nur vier Byte auseinanderliegen (etwa
`$1B0`/`$1B4`), überlappen sich dadurch beim Schreiben im Test, und der Test
schlägt fehl, obwohl die Implementierung stimmt. Lösung: die Offsets im
Hosttest umdefinieren (s. `test_q9kernel_chain.c`, `test_q9kernel_mem.c`).
Dasselbe gilt für Relozierungen: byteweise (`Q9K_GetU8`/`Q9K_SetU8`) statt
`Q9K_GetU32`/`Q9K_SetU32`.

**Falle — der Suite glauben:** die Hosttestsuite war einmal über Tage
kaputt, während Doku und Commits durchgehend „alle grün" behaupteten
(Kernel-Dateien bekamen neue Abhängigkeiten, ohne dass die Test-Stubs
nachgezogen wurden; zwei Tests griffen auf echte Kerneladressen zu →
sofortiger SIGSEGV, leeres Log). **Vor jeder Syscall-Arbeit die Suite
einmal komplett laufen lassen, statt der Doku zu glauben.**

## Im Emulator testen

1. Emulator bauen, Testabbild **niemals** das Master-Abbild: eigene
   APFS-Kopie je Sitzung (`cp -c`, bzw. `tools/q9img.py new <tag>`).
2. Bootkette schreiben (`tools/mkbootfile.sh`). **Nie per `os9 copy`** —
   das fragmentiert, und der Bootloader liest linear.
3. **Der Emulator MUSS aus seinem eigenen Verzeichnis gestartet werden.**
   Der Debug-Dump-Pfad (`local_images/q9dbg_dump.txt`) ist relativ zum
   Prozess-CWD, nicht zum Abbildpfad. `cd` und Start müssen im **selben**
   Aufruf stehen, weil das Shell-CWD zwischen Werkzeugaufrufen zurückgesetzt
   wird.
4. Nachweis eines sauberen Laufs ist `Vektor=0` im Dump.
5. **Den Emulator nach jedem Lauf explizit beenden** — er beendet sich bei
   stdin-EOF nicht zuverlässig selbst, und verwaiste Prozesse sammeln sich
   an.

**Werkzeugwarnung:** `os9 gen -b=` bricht mit *„is fragmented"* ab. Die
Bootkette immer zuerst auf ein frisch (ohne `-e`) formatiertes Abbild
schreiben, danach erst Dateien kopieren.

**Werkzeugwarnung:** `os9 copy -r` setzt das Owner-Execute-Bit nicht;
`F$Load` verweigert Treibermodule dann ohne Fehlertext. Danach `os9 attr -e`.

## Messen — die wichtigste Falle zuerst

> **„A short window onto a noisy channel is not a measurement."**

Am 2026-09-19 wurden **vier Befunde auf einmal** zurückgezogen (der
Zeitgeber ticke nicht, `F$Sleep` kehre nie zurück, ein unterbrochener
Prozess laufe nie wieder, der Kontext werde über einen Wechsel nicht
wiederhergestellt). Nichts davon stimmte; alles stammte aus **einem**
Messfehler: die Konsole wird von `Q9K_TestProcA`s Diagnoseschleife
geflutet, und jede Ablesung erfolgte in einem ~40-Zeichen-Fenster hinter
der Markierung, in dem die Flut alles Nachfolgende begraben hatte.

**Also: immer das vollständige Protokoll sichern und gezielt filtern**
(etwa die `A`-Flut und Speicherspur-Zeilen `^M [A-Z] r=` herausfiltern),
nie ein kurzes Fenster hinter einem Marker ablesen.

**Zweite Ausprägung derselben Falle:** ein normal endender Prozess sieht in
Spuren aus wie ein Fehler. Sowohl die „leere Ready-Queue"/"verschwundener
Prozess"-Deutung als auch die zweimal behauptete und zweimal zurückgezogene
Arena-Überlappungs-These gingen darauf zurück: beobachtete
Speicherfreigaben waren normales Prozessende, keine Korruption.

**Dritte Falle:** ein Marker-Lauf, der den Emulator kurz nach dem Marker
beendet, schneidet den Dump **vor** dem Exception-Abschnitt ab — ein
`Vektor=14` blieb so unsichtbar. Für echte Nachweise mit unerreichbarem
Marker und Timeout laufen lassen.

### Gemessen wurde etwas anderes, als man denkt

Drei Varianten derselben Sache, alle real aufgetreten. Gemeinsames
Merkmal: das Werkzeug **meldet** den Fehler, aber an einer Stelle, auf
die niemand schaut.

**Der Dump stammt aus einem früheren Lauf.** Wird der Emulator mit einem
anderen CWD gestartet, kann er `local_images/q9dbg_dump.txt` nicht
schreiben. Er sagt das ins Konsolenprotokoll („konnte … nicht zum
Schreiben oeffnen") — aber das `Vektor=0`, das man danach aus der
liegengebliebenen Datei liest, beschreibt einen alten Lauf. **Prüfen:**
Zeitstempel des Dumps gegen die Laufzeit, und das Protokoll nach dieser
Meldung durchsuchen. Verlässlicher ist, die Datei vor dem Lauf zu
löschen.

**Das Abbild enthält den neuen Code gar nicht.** Scheitert der Bau
(typisch: `operand size error`), bootet der Emulator die zuvor
geschriebene Bootkette weiter — jede Messung beschreibt dann die alte
Fassung. Ein Befund über den Zweig, in dem eine frisch eingebaute
Diagnose stumm blieb, war auf diese Weise falsch. **Prüfen:** nach jedem
Bau auf `Errors:` und `operand size` sehen und den Zeitstempel des
Artefakts vergleichen, bevor gemessen wird.

**Der Filter verfälscht die Daten.** Die `A`-Flut aus `Q9K_TestProcA`
wird beim Auswerten gern per `tr -d 'A'` entfernt — das löscht aber auch
die Ziffer `A` aus jeder Hexzahl. Aus `0005A840` wird `0005840`, eine
plausibel aussehende, falsche Adresse. **Nur außerhalb der Hex-Bereiche
filtern**, und Marker so wählen, dass sie keine Hexziffern sind (`%`,
`&` etwa statt `A`–`F`).

### Ein fehlender Marker beweist nur eines

Nämlich: *dieser Code lief nicht*. Warum, ist damit offen. Die
Diagnoseausgabe wartet auf TXRDY, und eine solche Warteschleife kann
selbst hängen — in `chaintgt` kam gar keine Ausgabe, bis ein
ungeschütztes `move.b` davorgesetzt wurde. Die frühere Behauptung, ohne
TXRDY-Prüfung gingen Zeichen verloren, wurde deshalb zurückgenommen:
belegt ist keine der beiden Richtungen.

## Diagnosewerkzeuge

**`Q9_BOARD_DEBUG=1`** — periodische Ausgabe des echten PC, funktioniert
auch bei hängender oder durchdrehender CPU.

**Kanarien-Bisektion** — an mehreren aufeinanderfolgenden Stellen je einen
eindeutigen Wert an eine **weit entfernte** Adresse schreiben und per
Dump auslesen. Damit wurde die Arena/Stack-Kollision gefunden.

**Minimal-invasiver Ringpuffer (dem PC-Sampling vorzuziehen)** — eine
kleine Kanarie direkt in den Assemblercode der interessanten Stelle
einbauen, die bei jedem Durchlauf ein paar Werte in einen RAM-Ringpuffer
schreibt. Läuft mit voller Geschwindigkeit, deshalb **kein
Heisenbug-Risiko** — anders als PC-Sampling mit verkleinerter Slice-Größe,
das das Timing verzerrt und beobachtete Symptome verändern kann. Regeln
dabei:

* Aufruferkonvention einhalten: bei Syscall-Handlern sind meist nur
  bestimmte Register frei; alle anderen vorher sichern und exakt
  wiederherstellen, sonst erzeugt die Diagnose neue, sehr verwirrende
  Fehler.
* Bit-Operationen (`andi`, `asl`, …) gehen **nur** auf Datenregistern,
  nie auf Adressregistern.
* Rückgaberegister nie als Rechenregister missbrauchen, auch nicht kurz.
* Ringpuffer-Adressbereiche sauber auseinanderhalten — Kollisionen liefern
  plausibel aussehende Falschdaten.
* Stack-Balance prüfen: SP am Anfang und am Ende der eigenen Routine in je
  eine feste Adresse schreiben; Differenz 0 schließt die eigene Routine aus.

**Modul im RAM finden:** RAM-weite Suche nach dem Modulnamen ist
zuverlässiger, als die Ladeadresse aus Annahmen abzuleiten. Ergänzend die
volle `$4AFC`-Sync-Wort-Suche mit Validierung jedes Treffers gegen
`M$Name`/`M$Size`.

**Fremde Module disassemblieren:** capstone. Für Einstiegspunkte des
Originalkernels siehe `Q9-OS-Research/kernel-68k/tools/dis68k.py` — nötig,
weil **Ghidra Syscall-Einstiege als Daten führt**: sie sind über keinen
Kontrollfluss erreichbar, sondern werden ausschließlich über die
Dispatchtabelle betreten.

**Kernelwachstum als Fehlerursache:** wächst der Kernel, verschieben sich
alle Bootmodule. Bei unerklärlichen Abstürzen nach Wachstum **zuerst den
Padding-Test**: unveränderten Kernel plus etwas reines Padding booten.
Stürzt der auch ab, ist es ein reiner Größeneffekt und nicht der neue Code.

## Dauerhafte Merksätze

**Registersicherung.** Jeder Syscall-Handler, den *fremder* Code aus einer
**Interruptroutine** heraus aufrufen kann, muss den **vollen Registersatz**
erhalten — nicht nur `a6`. Jeder Handler, der C-Code aufruft und (auch)
über das externe PEA+RTS-Trampolin erreichbar ist, muss sein eingehendes
`a6` sichern und auf `Q9K_CRuntimeData` umschalten.

**Rahmenerkennung.** `a5 == sp+8` allein **beweist keinen Registerrahmen**.
Bei `F$Sleep` war `a5` nur zufällig `sp+8`; das Schreiben in den
vermeintlichen Rahmen zerstörte den Stack des Aufrufers. Wer diese
Erkennung übernimmt, muss zusätzlich prüfen, woher der Aufruf kommt.

**Globale Zustandsflags im Trap-Pfad sind falsch**, sobald verschachtelt
aufgerufen werden kann.

**Wenn ein Einzeltest klappt, der echte Aufrufer aber scheitert**, ist fast
immer das *Argument* anders als gedacht. Ein Protokoll der tatsächlichen
Anfrage klärt das in einem Lauf — besser als jede Codelektüre.

**Wenn ein technisch korrekter Fix eine andere Funktion bricht**, ist die
alte, fehlerhafte Situation vermutlich irgendwo zur Voraussetzung geworden.
Nicht weiter Varianten probieren, sondern messen, worauf sich der fremde
Code tatsächlich verlässt.

**Bei einer „das ist aus einem Aufruf unmöglich"-Schlussfolgerung** erst
nach echtem Quelltext der Fremdkomponente suchen, bevor man sie als
endgültige Sackgasse dokumentiert. Eine in sich stimmige Schlussfolgerung
kann auf einer falschen Prämisse stehen.

**Systemglobal-Adressen mit „[HANDBUCH]"- oder „[PLATZHALTER]"-Kennzeichnung
nie ohne Live-Gegenprobe benutzen.** Ein Versuch mit `Q9_D_MINPTY` (`$55E`)
ließ das System beim Booten stehenbleiben — an dieser Adresse steht hier
etwas anderes.

**Scratch-Zellen-Kollisionen** prüfen immer den **gesamten** belegten
Bereich einer Tabelle (Anfang + Anzahl × Eintragsgröße), nicht nur die
Anfangsadresse. Es gibt keine automatische Kollisionsprüfung.

**ABI nie raten.** Fehlercodes aus `MWOS/SRC/DEFS/errno.h` belegen,
Strukturoffsets aus den DEFS, Aufrufkonventionen aus
`68k_tech.pdf` — und wo das Handbuch schweigt, aus dem Originalkernel
zurückgewinnen (erprobt mit `F$Sema`, `F$FModul`, `F$Mem`).
