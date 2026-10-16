# Kernel- und Userland-Arbeitsplan

Stand: 15.10.2026, Branch `kernel/phase-a-fundament` (A1, A4, B3 erledigt), Grundlage `main` `7653580` (Fortsetzung 144).

Dieser Plan zerlegt alle offenen Arbeiten am Q9-System in Schritte, die
jeweils **in einer Session** erledigt werden können:

- **Teil 1 — Kernel** (Phasen A–H): Kernel, eigener IOMan, Filemanager,
  Treiber, Speicherschutz.
- **Teil 2 — Userland** (Phasen I–O): Systemstart und Anmeldung, Shell,
  Laufzeitbibliotheken, Entwicklungswerkzeuge und alle übrigen
  Systemprogramme. Der Code dafür liegt überwiegend im Repo `Q9-Tools`.

Er ersetzt die Roadmap in `KERNEL_NEXT_SESSION_de.md`; deren Analyse liegt im Archiv (Stand 14.09.2026).

## So wird der Plan benutzt

- **Eine Session, ein Schritt.** Zu Beginn den obersten offenen Schritt
  nehmen, dessen Voraussetzungen erfüllt sind.
- **„Fertig, wenn" ist das Abnahmekriterium.** Ein Schritt gilt erst als
  erledigt, wenn genau dieser Nachweis vorliegt — nicht schon, wenn der Code
  geschrieben ist.
- **Beim Abschluss** die Status-Spalte auf ✅ setzen, die Übersicht
  „Fortschritt auf einen Blick" nachziehen und in der Spalte
  „Beleg" Commit und Testlauf eintragen (Evidenzregel aus
  `Q9-KERNEL/STATUS.md`). Ein Schritt, der sich als größer erweist, wird
  hier in a/b geteilt, statt halb fertig liegen zu bleiben.
- **Neue Funde** (Fehler, die unterwegs auftauchen) als eigene Zeile in die
  passende Phase eintragen, nicht stillschweigend mitlösen.
- Ist ein Schritt durch Prüfung **überflüssig** geworden, mit ⏭️ und
  Begründung markieren, nicht löschen.

| Status | Bedeutung |
|---|---|
| ⬜ | offen |
| 🔄 | in Arbeit |
| ✅ | erledigt, Beleg eingetragen |
| ⛔ | blockiert (Grund in der Zeile) |
| ⏭️ | entfallen (Begründung in der Zeile) |

## Fortschritt auf einen Blick

Bei jedem abgeschlossenen Schritt hier die Zählung und das Phasensymbol
mitziehen: ⬜ nichts begonnen, 🔄 teilweise erledigt, ✅ Phase komplett,
⛔ blockiert.

| Phase | Inhalt | Erledigt | Status |
|---|---|---|---|
| A | Kernel-Fundament | 16 / 16 | ✅ |
| B | Technische Schulden | 4 / 5 | 🔄 |
| C | Teilweise Syscalls abschließen | 1 / 8 | 🔄 |
| D | Eigener IOMan | 0 / 8 | ⬜ |
| E | Eigene Filemanager | 0 / 7 | ⬜ |
| F | Eigene Treiber, Statusumstellung | 0 / 3 | ⬜ |
| G | Speicherschutz (MMU) | 0 / 5 | ⛔ |
| H | Werkzeuge und Pflege | 0 / 4 | ⬜ |
| **Teil 1 Kernel** | | **21 / 56** | |
| I | Userland-Grundlagen | 0 / 5 | ⬜ |
| J | Systemstart, Anmeldung, Shell | 0 / 8 | ⬜ |
| K | Laufzeit und Bibliotheken | 0 / 7 | ⬜ |
| L | Entwicklungswerkzeuge auf dem Ziel | 0 / 5 | ⬜ |
| M | Datenträger und Archive | 0 / 5 | ⬜ |
| N | Netzwerk | 0 / 7 | ⬜ |
| O | Grafik und übrige Programme | 0 / 6 | ⬜ |
| **Teil 2 Userland** | | **0 / 43** | |
| **Gesamt** | | **21 / 91** | |

| Meilenstein | Status |
|---|---|
| MS1 Stabil mit Microware-I/O | ⬜ |
| MS2 Eigener IOMan | ⬜ |
| MS3 Eigene I/O-Kette | ⬜ |
| MS4 Speicherschutz | ⛔ |
| MS5 Eigener Systemstart bis zur Shell | ⬜ |
| MS6 Alle eigenen Programme mit eigener Werkzeugkette gebaut | ⬜ |
| MS7 Userland vollständig eigen | ⬜ |

## Bereits erledigt (vor diesem Plan)

Zur Orientierung, damit nichts davon erneut angegangen wird.

| Status | Arbeit | Beleg |
|---|---|---|
| ✅ | Echter Boot über sysgo in beiden Kernelvarianten (Atom/Developer) | `c9c0c41` |
| ✅ | Login über tsmon, neun Kernelfehler auf dem Weg behoben | `c356bd6`, Fortsetzung 113 |
| ✅ | Login mit Shell, `echo`/`dir`/`pd` laufen | `e09de94`, Fortsetzung 120 |
| ✅ | Trap-Rahmen-Korruption im Nebenläufigkeits-Stresstest (Ursache: A4 auf dem Stack) | Fortsetzung 106 |
| ✅ | Scheduler-Deadlock bei leerer Ready-Queue (Idle-Prozess) | `2b74e30` |
| ✅ | `F$Wait` löscht die Signalmaske und kehrt bei anstehendem Signal sofort zurück | `8644059` |
| ✅ | Debug-Paket 1: Bauschalter, Atom-Kernel baut und bootet | `c9c0c41` |
| ✅ | Debug-Paket 2: Ringpuffer, Trace-Satzformat, Filter, `F$Q9Dbg`, Trap-Haken | `2f14c5f`, `1bb3581` |
| ✅ | Syscall-Trace live verifiziert | `62a0e74` |
| ✅ | Debug-Paket 4: `trace`-Kommando und Host-Dekoder | `cd20612` |
| ✅ | Debug-Abschnitt von `xcc -g` entschlüsselt | `2ea3c3c` |
| ✅ | Negativtests mit beschädigten Bootmodulen | `tools/malformed_boot_test.sh` |
| ✅ | 52 von 101 Syscalls voll umgesetzt | `Q9-KERNEL/STATUS.md` |
| ✅ | 67 der 198 Systemprogramme aus `CMDS` als eigene Q9-Tools vorhanden, dazu 22 weitere eigene; gebaut mit dem Microware-Compiler, geprüft unter dem Microware-Kernel | `Q9-Tools/System/PORT_STATUS.md`, `Q9-Tools/Network/NETWORK_STATUS.md` |
| ✅ | Gesamtbau aller eigenen Programme (88) mit SDK-Werkzeugen nach `Q9SYS/CMDS_XCC` | 27.09.2026 |
| ✅ | Eigene Werkzeugkette `qcpp`/`qcir`/`qir68k`/`qr68k`/`ql68k` läuft auf dem Ziel, byteidentisch zum Host | Q9-QCC, 27.09.2026 |
| ✅ | qclib: alle 140 öffentlichen clib-Symbole vorhanden, 79 verifiziert, 61 mit dokumentierter Abweichung | `Q9-QCC/Q9-BACKEND-68K/q9-qclib/STATUS.md` |
| ✅ | bash 1.12 portiert (läuft bis 16 MB RAM) | `Q9-Tools/System/bash_v1.12.10` |

## Ausgangslage

- **Echter Boot läuft** in beiden Varianten (Atom und Developer):
  sysgo → startup → tsmon → login → mshell; `echo`, `dir`, `pd` laufen.
- **Syscalls** (101 Aufrufe laut `Q9-KERNEL/STATUS.md`): 52 voll umgesetzt,
  3 nur hostgetestet, 17 teilweise, 24 nur über den Microware-IOMan,
  1 fehlt (`F$MBuf`), 4 bewusst nicht (in OS-9/68K selbst zurückgezogen).
- **I/O hängt vollständig an Microware.** Alle 16 `I$`-Aufrufe sowie
  `F$PErr`, `F$IOQu`, `F$IODel` laufen über den Microware-IOMan.
- **Eigener IOMan** (`Q9-IOMAN`): Modul baut, Start und `F$SSvc`-Registrierung
  laufen im Emulator; von den 24 I/O-Eingängen ist **keiner vollständig**
  fertig. Letzte Arbeit 02.10.2026.
- **Eigene Filemanager** (`Q9-Manager/Q9-SCF`, `Q9-RBF`, `Q9-PIPE`,
  `Q9-SBF`): nur Verzeichnisgerüste, **noch kein Code**.
- **Microware-Module im laufenden Boot:** ioman, scf, rbf, sc68681, cfide,
  sysgo, tsmon, login, mshell, csl, math. Eigen sind der Kernel und der
  DHF-Treiber (`dhfmgr`, `dhfdrv`).
- **Userland:** `Q9SYS/CMDS` enthält ohne eigene Testprogramme 198
  Programme. **67 davon gibt es schon als eigene Q9-Tools, 131 kommen noch
  von Microware.** Die eigenen Tools werden bisher mit dem
  Microware-Compiler gegen Microware-Bibliotheken gebaut und unter dem
  Microware-Kernel getestet.

## Meilensteine

| Meilenstein | Erreicht nach | Grobe Schätzung |
|---|---|---|
| MS1 Stabil mit Microware-I/O | Phasen A, B, C | ~20 Sessions |
| MS2 Eigener IOMan (mit Microware-Filemanagern) | Phase D | ~8 Sessions |
| MS3 Eigene I/O-Kette | Phasen E, F | ~10 Sessions |
| MS4 Speicherschutz | Phase G | offen, wartet auf Q9-Flux |
| MS5 Eigener Systemstart bis zur Shell | Phase J (setzt MS1 voraus) | ~8 Sessions |
| MS6 Eigene Werkzeugkette für alle Programme | Phase K | ~7 Sessions |
| MS7 Userland vollständig eigen | Phasen I, L–O | ~28 Sessions, genauer nach I1 |

Die Schätzungen sind grob. Besonders D2, E3 und E4 können sich teilen, wenn
unterwegs Fehler auftauchen.

# Teil 1 — Kernel

## Phase A — Kernel-Fundament

Zuerst, weil sonst alles andere blockiert: ohne Regressionslauf ist kein
späterer Schritt nachweisbar, und ohne FARCALL stößt jede größere Änderung
an die `bsr`-Reichweite (±32 KB bei 62 KB Kernel).

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| A1 | ✅ | **Ein-Kommando-Regressionslauf**: frischer Build → Image-Klon → Login → `echo`/`dir`/`pd`/`date` mit Ausgabevergleich | Ein Skript meldet grün/rot für beide Varianten; vorhandene Einzelskripte (`startup_shell_test.sh`, `mgrpath_probe_test.sh`) eingebunden | — | `0c5addd`: `tools/kernel_regress.sh` 5 Stufen grün (Hosttests 30/30, Login-Test development 53 s + atomic 52 s, startup_shell_test, mgrpath_probe_test), 15.10.2026; fand zwei rote Suiten (moddir, dbg_live ohne `-DQ9K_DEBUG`) |
| A2 | ✅ | **FARCALL-Bisektion**: Stellen 1–21 mit `fcbisect.sh` weiter halbieren | Schuldige Stelle(n) und Ursache benannt. Zuerst prüfen, ob dort `a0` beim Aufruf noch gebraucht wird — `Q9K_FARCALL` überschreibt `a0` | A1 | `fcddmin.py`: einzige schuldige Stelle Nr. 7 = `Q9K_SysWaitImpl` in `Q9K_SysFWait` (Boot bricht mit Vektor 4, PC im Datenbereich); dieselbe Stelle als `bsr`+4 NOPs grün, also kein Layout-Effekt; **Ursache: das alte FARCALL zerstört `a0`, das in den Trap-Pfaden noch dem Aufrufer gehört** (F$Wait sichert es als Wiederaufnahme-Rahmen). Registerneutrale Fassung an allen 43 Stellen grün, 15.10.2026 |
| A3 | ✅ | **FARCALL ausrollen**: alle 42 `bsr` auf C-Funktionen in `q9kernel_entry.a` umstellen; Reichweitenprüfung im Build | Login grün in beiden Varianten; `Q9K_SRCDEBUG` mit mehreren Dateien baut und bootet | A2 | alle 43 Stellen auf registerneutrales `Q9K_FARCALL`; `kernel_regress.sh` 5/5 grün (development 65.298 Byte, atomic 61.084 Byte); `Q9K_SRCDEBUG="q9kernel_date.c q9kernel_sched.c q9kernel_clock.c"` baut (67.368 Byte) und bootet; `q9_farcall_check.py` im Build, Negativprobe schlägt an, 15.10.2026 |
| A4 | ✅ | **`build.sh` ehrlich machen**: `mwos-build`-Fehler werden heute ignoriert, alte Objekte täuschen Erfolg vor | Ein absichtlich eingebauter Fehler bricht den Build ab | — | `4eeebe1`: `tools/build_honesty_test.sh` 11/11 grün (C-, Assembler-, Linkfehler); gegen altes `build.sh` 5 von 10 rot (C-Fehler lieferte Exit 0, alter `q9kernel` blieb liegen) |
| A5 | ✅ | **Register-Erhaltungsprüfung** im Developer-Kernel (Debug-Paket 3, `DEBUG_KONZEPT_de.md`) | Login plus Befehlsfolge ohne Meldung, oder eine Fundliste als neue Zeilen in Phase A | A1 | Prüfung `Q9K_RegChkReturn` (nur Q9K_DEBUG): Schnappschuss d0–d7/a0–a6 beim Handlereintritt, Vergleich von d2–d7/a2–a6 vor dem `rte`, Ausgabe `!R c=<Code> <Reg> <vorher> <nachher>`. Erstlauf: 26 Meldungen in 5 Callcodes. Belegte Ausgaben (a2 bei F$Link/F$TLink/F$SRqMem/F$SRqCMem, d2 bei F$SetSys und I$GetStt/SS.Size) stehen mit Beleg in `Q9K_RegChkExempt`; **echter Fund: F$TLink hinterließ a4 = M$Init-Einsprung und ein verändertes d2 im Aufrufer** — behoben (d2–d7/a3–a5 um M$Init gesichert). Danach Login-Test beider Varianten ohne Meldung (`login_shell_test.sh` prüft `!R`), `kernel_regress.sh` 5/5, 15.10.2026. Grenze: verschachtelte Traps überschreiben den Schnappschuss, der Schlüssel `sp` unterdrückt dann die Aussage (keine Fehlalarme, aber Lücken bei I/O über IOMan) |
| A6 | ✅ | **Stresstest verschachtelter externer Traps**: mehrere Prozesse mit gleichzeitigem I/O über IOMan, Filemanager und Treiber bei laufendem Timer | N Durchläufe ohne Exception; Szenario eingecheckt und schaltbar wie `Q9K_TestNestedTrapStress` | A1 | `tools/leak_test.sh` mit `Q9K_LEAK_CMDS='dir /dd/CMDS&dir /dd/SYS&date&pd&echo a&wait;'` und `Q9K_LEAK_PER=2`: **fünf gleichzeitige Prozesse je Zyklus** (zwei `dir` mit Dateizugriff über IOMan, RBF und Treiber, dazu `date`, `pd`, `echo`; Hintergrundjobs mit `&` und `wait`) bei laufendem Timer. **96 Zyklen = 518 Prozessenden in 5 min, keine Exception, alle 192 Verzeichnisausgaben vollständig, Speicherbilanz 0 Byte.** Eine 24-Zyklen-Fassung (120 Forks, ~1,7 min) ist als Stufe „Parallellauf“ in `kernel_regress.sh` (7/7 grün, 15.10.2026). Hinweis: ein erster Lauf scheiterte nur, weil im Testabbild `dir` fehlte (`mshell: can't execute "dir"`) |
| A7 | ✅ | **IRQ während Queue- und Kontextübergabe**: kritische Abschnitte inventarisieren, gezielt Interrupts injizieren | Jeder Abschnitt ist abgesichert oder seine Sicherheit schriftlich begründet | A6 | Inventur in `docs/KERNEL_IRQ_SPERRE_de.md` (Abschnitt „A7“): jeder Kontext, der Queues, Deskriptoren oder die Freiliste anfasst, ist abgesichert oder begründet (Trap-Dispatch mit Maske `$0700`, Timer als höchste Stufe, Arena unter `Q9K_IntLock`, Boot ohne Interrupts). **Zwei Lücken gefunden und geschlossen:** (1) `Q9K_TimerIRQHandler` schaltete auch dann um, wenn er eine Interrupt-Behandlung niedrigerer Stufe (UART-ISR, Level 3) unterbrach — die halb ausgeführte ISR wäre auf dem Stack des alten Prozesses liegen geblieben, bei wieder freiem Level; jetzt trägt er `SR & $0700` in `Q9K_SchedNoSwitch` (`$2488`) ein, und `Q9K_SchedReschedule` macht dann nur die Tick-Buchführung (Hosttest `test_q9kernel_sched` Fall 4b). (2) `Q9K_TraceHandler` (Vektor 9, `F$DExec`-Einzelschritt) fasste Queues ohne Sperre an; jetzt `ori.w #$0700,sr` am Eintritt. `kernel_regress.sh` 7/7 grün, 15.10.2026. **Nicht belegt:** das gezielte Einspielen von Interrupts — die Wirksamkeit stützt sich auf die Lasttests mit dem 10-ms-Takt (A6, A8, A12); eigene Zeile A15 |
| A8 | ✅ | **Langlauf und Ressourcenbilanz**: ~1000 Zyklen `F$Fork`/`F$Exit`/`F$TLink` | Freispeicher, Modul-Linkzähler und Trap-Slots kehren auf den Ausgangswert zurück | A1 | `tools/leak_test.sh [Zyklen] [Warmup]` (+ `run_leak_test.exp`; wertet `M A`/`M F` zwischen zwei Marken aus). **Freispeicher gemessen: 333 Zyklen = 1001 Prozessenden (echo/date/pd gemischt, 6 min), Zuwachs 0 Byte (4 Blöcke, 65.504 Byte an beiden Marken)**; als Stufe „Langlauf 192 Forks“ in `kernel_regress.sh`. Der Langlauf deckte zuvor A12 auf. Auswertungshinweis: bei `M F` ist `e=` die Nachbaradresse, bei `M A` der Fehlercode (immer 0, auch bei `a=0` = Fehlschlag) **Linkzähler und Trap-Slots (16.10.2026):** der Developer-Kernel gibt bei jedem Prozessende eine Zeile `M R r=<Summe der Link-Zähler> a=<Einträge im Modulverzeichnis> s=<belegte Trap-Slots aller Prozesse>` aus (`Q9K_ProcResAudit` in `q9kernel_procend.c`, `Q9K_ModDirResTotals` in `q9kernel_moddir.c`, Op 8 im Konsolen-Backend); `leak_test.sh` vergleicht die letzte Zeile vor jeder Marke (`Q9K_LEAK_MAX_LINKS`, Standard 0). **1001 Prozessenden (333 Zyklen, 6 min): Linkzähler-Summe 4 → 4, Modulverzeichnis 36 → 36 Einträge, Trap-Slots 2 → 2, Freispeicher +0 Byte.** Negativprobe (Unlink des Hauptmoduls und Trap-Freigabe beim Prozessende abgeschaltet): Linkzähler +1, Trap-Slots +1, Test rot. |
| A9 | ✅ | **Absturz bei schnell aufeinanderfolgenden Eingaben** (Fund aus A1) | Ursache benannt und behoben; deterministischer Test `icptsleep` | A1 | **Ursache: `F$Sleep` mit bereits anstehendem Signal.** `Q9K_ProcSleep` stellt es dem Aufrufer selbst zu; `Q9K_IcptDeliver` baut den 68-Byte-Rahmen bei `P$SavedSP-68`, genau dort läuft aber die C-Aufrufkette des Sleep-Aufrufs — der Rahmen überschrieb ihre Rücksprungadresse (Instruktionsspur: `Q9K_IcptDeliver+0xe8` kehrt nach `PC=1` zurück, danach Nullspeicher bis zum ersten ungültigen Opcode `$128c`; daher immer derselbe Absturz-PC). Auslöser in der Praxis: ein Tastendruck zwischen `SS_SSig` und `F$Sleep` von mshell, deshalb zeitabhängig. **Korrektur:** `Q9K_SysFSleep_Block` hält vor dem C-Aufruf 68 Byte unter `P$SavedSP` frei (`Q9K_IcptFrameGap`). **Beleg:** neues Testprogramm `icptsleep.a` (Intercept anmelden, Signal an sich selbst, `F$Sleep`; erwartet `intercept ok`/`sleep done`) — ohne Korrektur rot mit genau dem A9-Absturz (Vektor 4, SR `$2708`, PC `$128c`), mit Korrektur grün; Teil von `login_shell_test.sh`, `kernel_regress.sh` 5/5 in beiden Varianten, 15.10.2026 |
| A10 | ✅ | **Gleiche Fehlerklasse prüfen: Rahmen über dem C-Stack.** Stellen, die einen Prozessrahmen bei `P$SavedSP` schreiben, während der Kernel auf demselben Stack läuft | Jede Stelle benannt, abgesichert oder als unkritisch begründet; je ein Test | A9 | Drei Stellen rufen `Q9K_IcptDeliver`: (1) `Q9K_ProcSleep` an den Aufrufer = A9, behoben. (2) **`Q9K_IcptReturn` (F$RTE) bei weiterem anstehendem Signal: gleicher Fehler** — der neue Rahmen entstand bei `P$SavedSP-68`, wo die Intercept-Routine samt C-Aufrufkette lief; Instruktionsspur: `Q9K_IcptDeliver` läuft durch, danach kehrt `Q9K_IcptReturn` nach `PC=0` zurück. Behoben: der Kernelpfad räumt nur ab (`Q9K_IcptPop`), `Q9K_SysFRTE` wechselt auf den Stack des Hauptprogramms, lässt 68 Byte Luft und stellt erst dann zu (`Q9K_SysRTERedeliverImpl`); `Q9K_IcptReturn` bleibt für die Host-Tests unverändert. (3) `Q9K_ProcSend` stellt nur an einen NICHT laufenden Empfänger zu — unkritisch. `F$SigMask` schreibt keinen Rahmen (nur `Q9K_SchedWake`), `F$Wait` stellt nicht zu — unkritisch. **Beleg:** `icptrte.a` (Signal 32, die Routine sendet sich Signal 33, `F$RTE`; erwartet `h32`/`h33`/`main done`) — vor der Korrektur rot (Kernel hält mit Vektor 4, PC `$48`), danach grün; Hosttests 30/30, `kernel_regress.sh` 5/5, 15.10.2026 |
| A11 | ✅ | **`F$Wait` mit anstehendem Signal**: laut Handbuch kehrt `F$Wait` (wie `F$Sleep`) sofort zur Intercept-Routine zurück, wenn ein Signal ansteht; `Q9K_SysFWait` stellt nicht zu und blockiert | Test wie `icptsleep` für `F$Wait`; Zustellung nach dem Muster von A9/A10 (Lücke unter `P$SavedSP`, nie auf dem laufenden C-Stack) | A10 | **Handbuch (`68k_tech.pdf` S. 535):** „If a signal is received by a process waiting for children to terminate, it is activated. In this case, d0.w contains zero.“ Der Kernel kehrte bei anstehendem Signal schon mit `d0=0` zurück, **stellte das Signal aber nie an die Intercept-Routine zu** (im Quelltext als „OFFEN“ markiert), und es blieb anstehen — ein zweites `F$Wait` kehrte ebenfalls sofort mit `d0=0` zurück statt das Kind abzuwarten. **Korrektur:** `Q9K_SysWaitImpl` meldet bei anstehendem Signal und eingetragener Routine Ausgang 3; `Q9K_SysFWait_Signal` sichert den Registersatz, lässt wie bei `F$Sleep` (A9) 68 Byte Luft unter `P$SavedSP`, ruft `Q9K_ProcWaitDeliver` (Zustellung, nächster Prozess) und kehrt nach der Routine mit `d0=0` zurück. Außerdem steht `d0`/`d1` im gesicherten Rahmen des blockierenden Pfads jetzt auf 0, damit ein Prozess, den ein Signal weckt, `d0.w=0` bekommt. **Beleg:** `icptwait.a` + `waitkid.a` (Signal an sich, Kind starten, `F$Wait` → `w intercept`, `wait signal ok`; zweites `F$Wait` → `child reaped`) — mit dem alten Kernel rot (`child fehlt`, kein `w intercept`), mit der Korrektur grün; Teil von `login_shell_test.sh`; `kernel_regress.sh` 6/7 Stufen grün plus Hosttests nach dem Stub-Fix 30/30, 15.10.2026 |
| A12 | ✅ | **Absturz im Fork-Langlauf** (Fund aus A8) | Ursache benannt und behoben; `leak_test.sh 333 8` (≈1000 Forks) läuft durch | A9 | **Ursache: die Arena überdeckte feste Diagnoseadressen.** Die Zellen `Q9K_ExcInfo_*` (`$144020`…), der `Q9K_RaceRing` des Trap-Dispatchers (`$1440C0`, 8192×8 Byte, Indexzelle `$1540C0`) und die Trace-Zellen bis `$1541BB` liegen an festen Adressen im oberen RAM, die Arena reichte aber bis zum RAM-Ende. Sobald der Heap `$144000` erreichte (rund 25 Fork-Zyklen), überschrieb jeder Trap (zwei Schreibzugriffe, `pc=$8CAC/$8CAE`) Freilistenknoten und Prozessspeicher: Knoten mit Müll (`next=$41/$5404`), Allocator ohne Block (`M A … a=0`), `date` sprang neben seinen Stack, die CPU lief durch Nullspeicher. Das Zeitverhalten (nur manche Läufe) kam vom Heap-Layout. **Wie gefunden:** Instruktionsspur (Sprung ins Leere) → Freilisten-Prüfung `Q9K_ArenaAudit` im Developer-Kernel (hält beim ersten beschädigten Knoten an) → `Q9_WATCH_ADDR` auf den Knoten nannte den Schreiber. **Korrektur:** `Q9K_ArenaInitSkipRing` in `q9kernel_cinit.c` spart `[$144000, $1541C0)` aus (zwei Freiblöcke). **Beleg:** vor der Korrektur Absturz in 60–80 % der Läufe bei 72 Forks; danach 3×192 Forks und 1×1001 Prozessenden ohne Absturz, Bilanz 0 Byte; `kernel_regress.sh` 6/6 in beiden Varianten. **Nebenfunde:** (a) eine erste, zu kleine Aussparung (bis `$1540D0`) ließ die atomare Variante schon im Boot sterben — die Diagnosezellen bis `$1541BB` wurden von der ersten Belegung überschrieben; (b) `Q9K_RaceRing` ist laut eigenem Kommentar „TEMPORÄR“, läuft aber in jedem Kernel — hinter `Q9K_DEBUG` stellen oder ganz entfernen (Zeitverhalten ändert sich, danach A9/A10-Tests wiederholen) |
| A13 | ✅ | **`mshell <datei` scheitert**: das Kind meldet endlos „error getting path options!“ — `I$GetStt`/`I$ReadLn` auf Pfad 0 liefern `E$BPNUM` | `mshell <skript` führt das Skript aus (so wie der Startup-Pfad es kann) | A1 | **Ursache:** Die Shell öffnet `loop` (IOMan, Pfad 0), forkt `mshell` und schließt danach sofort ihren Pfad 0. `F$Fork` kopiert IOMan-eigene `P$Path`-Einträge (Bit 15) ohne IOMans Zähler zu erhöhen (im Quelltext als „noch nicht“ vermerkt), der Close des Erzeugers gibt den Deskriptor also frei, und das Kind liest `E$BPNUM` (in der Prozessliste: Kind `path=8001 …`). **Korrektur:** Ein Überschuss-Zähler je IOMan-Pfad (`Q9K_MgrRefs`, `$2500`, 64 Byte; `refs = Pfadeinträge − IOMans Zählung`): `F$Fork` erhöht ihn für kopierte Einträge (`q9kernel_firstproc.c`), `Q9K_TrapTryManagerPathIO` lässt ein `I$Close` bei `refs > 0` nur herunterzählen (Eintrag räumen, Erfolg, IOMan sieht es nicht), das Prozessende zählt ebenso (`q9kernel_procend.c`). Erst der letzte Mitbenutzer schließt wirklich. **Beleg:** Hosttests (procend Fall R2) und Login-Test `mshell <loop` (Skript mit zwei `echo`-Zeilen) — mit dem alten Kernel hängt der Test an dieser Stelle (kein Prompt), mit der Korrektur grün in beiden Varianten; `kernel_regress.sh` 7/7, 15.10.2026. **Rest:** IOMan-Pfade, die ein Prozess bei seinem Ende noch offen hat und die kein Mitbenutzer hält, werden weiterhin nicht geschlossen (Leck des PD, wie vorher; eigener Schritt) |
| A14 | ✅ | **`F$SRtMem` gibt Blöcke teils ungerundet zurück**: gesehen `M F r=00000101` beim Boot (257 Byte) — der Allocator rundet beim Belegen auf 16, die Freiliste bekommt aber die ungerundete Größe zurück (kleiner Verlust je Freigabe, ungerade Knotengrößen). Außerdem verliert `Q9K_FreeMem` Blöcke unter 16 Byte ganz | Größe beim Freigeben wie beim Belegen aufrunden (in `Q9K_SysSRtMemImpl`/`Q9K_FreeMem`); Hosttest dazu | A12 | `Q9K_FreeMem` rundet jetzt wie das Belegen auf 16 Byte auf (alle Aufrufer geben die Größe unverändert zurück, die sie angefordert haben); Hosttest `test_q9kernel_arena` Fall 11 — vor der Korrektur rot (257-Byte- und 4-Byte-Block), danach grün; `kernel_regress.sh` 6/6, 15.10.2026. Die Prüfung `Q9K_ArenaAudit` erlaubt ungerade Größen weiterhin, schadet aber nicht |
| A15 | ✅ | **Interrupt-Injektion**: Emulator-Lauf mit stark verkürzter Tickzeit (oder zusätzlichen künstlichen Level-3-Interrupts), damit die IRQ-Fenster aus A7 deutlich häufiger getroffen werden | `kernel_regress.sh`-Stufe mit verkürztem Takt läuft durch (Langlauf, Parallellauf) | A7 | **Umsetzung (16.10.2026):** Q9-Flux `src/devices/timer_irq/timer_irq.c` liest die Umgebungsvariable `Q9_TIMER_PERIOD_MS` (1–1000, Standard 10); `docs/BOARD.md` dort dokumentiert sie. Gemessen: bei `1` rückt die Q9-Uhr im selben Testlauf 55 s statt 5 s vor (zehnfacher Takt). **Beleg:** bei 1 ms laufen Langlauf (176 Prozessenden), Parallellauf (122) und der Login-Test beider Varianten (mit `icptsleep`/`icptrte`/`icptwait`, also den A9-/A10-/A11-Fällen) ohne Exception, Speicherbilanz 0 Byte, Linkzähler und Trap-Slots unverändert; drei neue Stufen „Interrupt-Injektion“ in `kernel_regress.sh` (11/11 Stufen grün, 16.10.2026). **Hinweis:** die Q9-Flux-Änderung liegt dort noch uncommittet im Arbeitsbaum (zusammen mit fremden Änderungen); ein älterer `q9.exe` ignoriert die Variable, die Stufen laufen dann nur mit 10 ms. Grenze: es sind Timer-Interrupts (Level 6) in höherer Dichte, keine zusätzlichen Level-3-Interrupts mitten in einem bestimmten Fenster. |
| A16 | ✅ | **IOMan-Pfade beim Prozessende schließen**: `Q9K_ProcReleasePaths` räumte IOMan-eigene Einträge (Bit 15) nur aus der Tabelle und rief kein `I$Close` — ein Programm, das beendet wird, ohne seine Dateien zu schließen, ließ den Pfaddeskriptor in IOMan stehen | Beim Prozessende ein `I$Close` über IOMan je offenem IOMan-Pfad (ohne Mitbenutzer); Test: Programm öffnet eine Datei und beendet sich ohne Close, danach ist die Pfadtabelle von IOMan wieder frei | A13 | **Korrektur:** `Q9K_SysFExit` ruft vor dem Aufräumen `Q9K_ExitCloseMgrPaths` (`q9kernel_entry.a`): je P$Path-Eintrag mit Bit 15 ein gewöhnliches `trap #0` / `I$Close` aus dem F$Exit-Handler, der Dispatcher legt einen neuen Exception-Frame darüber und kehrt per RTE zurück. `Q9K_TrapTryManagerPathIO` behandelt dabei den Mitbenutzer-Fall aus A13 (`Q9K_MgrRefs > 0`: nur herunterzählen). Der Schleifenzähler liegt auf dem Stack (externe Handler dürfen d2–d7 ändern); nicht im Trampolin-Pfad (`Q9K_InTrapPath = 0`, IOMan selbst ruft F$Exit). **Beleg:** neues Testprogramm `openleak.a` (öffnet `/dd/SYS/motd`, endet ohne Close) und `leak_test.sh` mit `Q9K_LEAK_CMDS='openleak;openleak;'`: ohne Korrektur scheitert schon der zweite Open mit `$D6` (**23 von 24 Opens misslungen**, die offene Datei bleibt belegt), mit Korrektur kein einziger; Stufe „Prozessende mit offenem IOMan-Pfad“ in `kernel_regress.sh` (8/8 Stufen grün, beide Varianten, 16.10.2026) |

## Phase B — Technische Schulden

Aus `KERNEL_NEXT_SESSION_de.md`, Abschnitt „Technische Schulden".

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| B1 | ✅ | **`Q9K_ApplyInitializedData`**: Grenzprüfungen für Modul und Zielspeicher; das verworfene IRefs-Gruppenwort klären; unterstützten Relokationsumfang festlegen | Hosttests mit kaputten Modulen und Grenzfällen grün | — | **Umgesetzt (16.10.2026):** `Q9K_ApplyInitializedData` (zwei Kopien: `q9kernel_firstproc.c` für `F$Fork`/`F$Chain`, `q9kernel_traplink.c` für `F$TLink`) hat jetzt den Parameter `blockSize` und liefert `E$BMID` ($CD) bei jedem Verstoß. **Geprüft:** M$IData/M$IRefs liegen im Modul (M$Size), M$IData vor M$IRefs; jede IData-Nutzlast liegt vor M$IRefs; jedes Kopierziel und jedes relozierte 32-Bit-Feld liegt im Zielblock `[block, block+blockSize)` (auch gegen Überlauf bei Offsets nahe 2^32); jede Zeigertabelle ist im Modul abgeschlossen, kein Lauf ragt über das Modul. Aufrufer geben bei einem Fehler Block/statischen Speicher und Modul-Link zurück (`F$Fork`, `F$Chain` und `F$TLink`; beim Chain bleibt der Aufrufer unversehrt). **Festgelegt (steht im Kopfkommentar der Funktion):** unterstützt ist M$IData (Einträge bis M$IRefs; ohne M$IRefs wird wie bisher nichts kopiert; Rest unter 8 Byte vor M$IRefs = Füllbytes) und M$IRefs mit genau zwei Tabellen (Kodezeiger + Modulanfang, Datenzeiger + Datenblock), Läufe `[MS, Anzahl, LS…]`, Ende bei MS=0 und Anzahl=0; keine weiteren Relokationsarten. **Das „verworfene IRefs-Gruppenwort“ ist geklärt:** MS sind die oberen 16 Bit des Offsets (Offset = MS<<16 | LS, 68k_tech.pdf Table 1-8), seit Fortsetzung 112 so verarbeitet, der Kopfkommentar sagte noch „verworfen“. **Beleg:** Hosttests F5d (a–g: Nutzlast über M$IRefs, Ziel am/hinter dem Blockende, Offset nahe 2^32, Zeigerfeld über das Blockende, MS-Wort über 64 KByte, Tabelle nicht abgeschlossen, Lauf länger als das Modul, Tabellen außerhalb, M$Size=0, Füllbytes), F5e (`F$Fork` gibt alles zurück), F9b (`F$TLink`: Speicher und Link zurück, E$BMID), Chain-Test (Block und Link zurück, Aufrufer unversehrt), jeweils mit Prüfung, dass außerhalb des Blocks nichts verändert wird; 30/30 Hosttests, 12 Stufen grün, kein echtes Modul abgewiesen (echo, mshell, csl, sysgo, login, tsmon, dir, date, 16.10.2026). |
| B2 | ✅ | **Fehlerpfade von `F$TLink`**: erworbene Modulreferenzen, Static-Speicher und Trap-Slots zurückgeben | Negativtest ohne Ressourcenverlust | A8 | **Zwei Fehler gefunden und behoben (16.10.2026):** (1) `Q9K_ProcTLink` gab den Modul-Link nicht zurück, wenn die Anforderung des statischen Speichers scheiterte (`Q9K_ModDirUnlinkByHeader(hdr)` fehlte); (2) `Q9K_SysFTLink` (`q9kernel_entry.a`) löschte nach `M$Init` das Carry-Bit (`andi #$fe,ccr`) und meldete immer Erfolg, obwohl das Handbuch (68k_tech.pdf S. 172) für `TrapInit` „carry set, d1.w = error code“ vorsieht. Jetzt: Carry-Test als erste Instruktion nach `M$Init`, bei Fehler Rückbau wie bei `F$TLink` mit Namenszeiger 0 (Link, statischer Speicher, Slot; die Trap-Nummer liegt unter dem `M$Init`-Rahmen auf dem Stack) und Fehlercode an den Aufrufer. Der Rückbau ist unsere Entscheidung, das Handbuch regelt nur die Rückgabe von `M$Init`. **Beleg:** Hosttest `test_q9kernel_traplink` (F7: ein Unlink auf den Modulkopf, rot ohne Korrektur); Live: `trapfail.a` (TrapLib, `M$Init` meldet $A5) und `tlinkerr.a` (drei Runden mit frischem `F$Load`: A5, A5, CF). Mit `leak_test.sh` und `Q9K_LEAK_CMDS=\'tlinkerr;tlinkerr;\'` bleiben Linkzähler (4 → 4), Modulverzeichnis (36 → 36), Trap-Slots (2 → 2) und Freispeicher gleich. **Negativproben:** ohne C-Korrektur Linkzähler +20 und +20 Verzeichniseinträge in 24 Läufen; ohne Assembler-Korrektur meldet `F$TLink` `err=00`. Stufe „F$TLink-Fehlerpfade“ in `kernel_regress.sh` und Block 9 im Login-Test (12 Stufen grün, 16.10.2026). |
| B3 | ✅ | **Scratch-Adressen und IRQ-Sperre im Trap-Pfad**: zentrale Belegungstabelle, Regel für Verschachtelung und Kontextwechsel | Keine Überlappung; Prüfskript läuft im Build mit (vgl. behobener `$3F0`/`$3F8`-Konflikt aus Fortsetzung 122) | — | `5b16faa`: 450 Zellen geprüft, drei Doppelbelegungen behoben (F$FModul zerstörte Alarm 0; TRAP-#1-15-Zellen auf dem Speicherprotokoll; Callcode-Wort von I$Open überschrieben); Prüfung läuft in `build.sh`; `docs/KERNEL_SPEICHERBELEGUNG_de.md`, `docs/KERNEL_IRQ_SPERRE_de.md` |
| B4 | ✅ | **`Q9K_PatchCslFreelistBug`** prüfen: greift er überhaupt (erwartetes Bytemuster fehlte am 16.09.)? | Entscheidung belegt: als Kompatibilitätsmaßnahme gekennzeichnet oder entfernt | A1 | **Entscheidung: entfernt (16.10.2026).** Gemessen im Login-Test mit einer vorübergehenden Diagnosezeile im Developer-Kernel (`M C r=<Ausgang> a=<Modulkopf> s=<M$Size>`): bei **jedem** `F$TLink` von `csl` (Modulgröße `$BCEE`, Kopf `$366A0`) stimmte das erwartete Bytemuster an Dateiversatz `$56BC`, der Patch endete aber mit Ausgang 5 = „Sprung außer Reichweite“: der Stub lag im Arena-Speicher weit von `csl` entfernt, die Reichweitenprüfung aus Fortsetzung 109 verwarf ihn. Der Patch war also seit Fortsetzung 109 nie wirksam, und alle bisherigen Läufe (1001 Prozessenden, Parallellauf, Takt 1 ms) liefen ohne ihn. Die damals beobachtete zerstörte Verkettung hatte andere Ursachen (fremd relozierter Speicher, Fortsetzung 112; Arena über festen Diagnoseadressen, A12). Entfernt: `Q9K_PatchCslFreelistBug`, sein Aufruf in `Q9K_ProcTLink`, die Hosttest-Fälle F10a–F10d samt Stubs; im Quelltext steht ein Hinweis mit Fundort in der Git-Historie. 12 Stufen grün, 30/30 Hosttests. |
| B5 | ⬜ | **`F$Link`/`F$UnLink` zählen nicht symmetrisch**: `F$Link` ruft die nebenwirkungsfreie Suche `Q9K_SysFLinkSearchImpl` (`Q9K_ModDirFindSlotByName`) und erhöht den Link-Zähler nicht, `F$UnLink` senkt ihn (bis 0), `F$Load` hinterlässt 0 (IOMans Wrapper zählt in einem eigenen Puffer, `Q9K_VMODUL_RETBUF`). Ein geladenes Modul verschwindet dadurch nach dem ersten Link/Unlink-Paar aus dem Verzeichnis, auch nach einem kernelinternen `F$TLink`/`F$Fork`, und ein `F$Link` aus einem Programm hält das Modul nicht fest | `F$Link` erhöht den Zähler, `F$UnLink` senkt ihn, `F$Load` hinterlässt 1 (wie Microware) — oder die Abweichung ist mit Beleg als bewusst begründet; IOMans Treiberanschluss (`F$Link` aus dem Trampolin) bleibt funktionsfähig; Hosttest und Login-Test grün | A8, B2 | Fund aus B2 (`tlinkerr.a`: nach Load + Link + `F$TLink` + Rückbau fehlte das Modul, gemessen per `F$Link`-Probe, 16.10.2026). Vorsicht: IOMan ruft `F$Link` auch direkt über das Trampolin; ein erhöhter Zähler muss dort wieder zurückgenommen werden, sonst wächst er mit jedem Treiberanschluss |

## Phase C — Teilweise Syscalls abschließen

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| C1 | ✅ | **`date` und `F$VModul`**: Status-Hinweis „`date` erreicht `F$VModul` nie" ist vermutlich seit dem `F$Load`-Fix (Fortsetzung 108) überholt | Neu gemessen, Status korrigiert | A1 | `date` allein aus `/dd/CMDS` gebootet: `F$VModul`-Zellen im Q9-Flux-Dump hdr=`$ab3f0` size=`$c70` success=1 (Modul `$c64`, 16 Byte aufgerundet; `echo` ergibt `$c90`), 15.10.2026; `STATUS.md` korrigiert |
| C2 | ⬜ | **Debugger-Aufrufe live**: `F$DFork`/`F$DExec`/`F$DExit` mit Einzelschritt-Testprogramm | Im Emulator verifiziert, 🟢 → ✅ | A1 | |
| C3 | ⬜ | **`F$Sema` P live** mit zwei Prozessen; **`F$Alarm`-Matrix** live | Beide ✅ | A1 | |
| C4 | ⬜ | **`M$Extens`-Kaltstart-Scan** (OS9P2-Erweiterungen); damit wird `F$UAcct` möglich | Ein Testmodul installiert sich beim Boot | A1 | |
| C5 | ⬜ | **`F$SetSys`-Restvariablen**, **`F$Panic`**-Feinheiten | Jede Variable klassifiziert: umgesetzt oder begründet offen | — | |
| C6 | ⬜ | **`F$IRQ`/`F$FIRQ` mit echten Gerätequellen** (DUART, Timer); minimaler FIRQ-Prolog | Live-Zustellung belegt | A7 | |
| C7 | ⬜ | **`F$SysDbg`/RomBug-Rückkehr** samt `F$PwrMan`-Vorlauf | Einstieg und Rückkehr live verifiziert | A1 | |
| C8 | ⬜ | **`F$SRqCMem`-Farbspeicher** | Farbauswahl funktioniert oder begründet auf einen Bereich reduziert | — | |

`F$MBuf` bleibt bewusst offen: es gibt keine belegte ABI, und die Regel des
Projekts lautet, keine ABI zu raten.

## Phase D — Eigener IOMan

Reihenfolge nach `Q9-IOMAN/STATUS.md`, Abschnitt 7. **Strategie:** der eigene
IOMan läuft zuerst mit den **Microware-Filemanagern** als Prüfstein; danach
wird eine Schicht nach der anderen ersetzt — nie zwei auf einmal.

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| D1 | ⬜ | **Aufruf- und Rückgabevertrag** Kernel ↔ IOMan festschreiben (Carry, `d1`, Registerrahmen, QCC-Aufrufweg) | Spezifiziert; Hosttest und erzeugter 68k-Code stimmen überein | A3 | |
| D2 | ⬜ | **`E$BPNum` beim Read** finden (Slot wird beim Read nicht mehr als offen erkannt) | Rundlauf Open → Read → Close mit Testbackend im Emulator | D1 | |
| D3 | ⬜ | **Device-Descriptor** finden, linken, validieren; Treiber und Filemanager linken; Funktionsvektoren auflösen | Microware-`scf`/`sc68681` werden korrekt gebunden | D2 | |
| D4 | ⬜ | **Attach/Detach-Transaktion** mit Rollback und echter Device-Tabelle | Mehrfach-Attach, Detach und Fehlerfälle getestet | D3 | |
| D5 | ⬜ | **Namensauflösung** `/gerät/rest` → Manager; `I$ChgDir` | Boot bis zur Konsole mit eigenem IOMan | D4 | |
| D6 | ⬜ | **Fehlermapping und Cleanup bei Prozessende** (offene Pfade, Locks, Queue-Elemente, Modulreferenzen) | Offene Pfade werden bei `F$Exit` freigegeben | D5 | |
| D7 | ⬜ | **Pfad-Locks, Wait/Wake, I/O-Queue** (`F$IOQu`/`F$IODel`) | Konkurrierende Zugriffe serialisiert und getestet | D6 | |
| D8 | ⬜ | **`F$Load`, `F$PErr`, Bitmap-Dienste** im eigenen IOMan; native Kernel-Bitmaps (`F$SchBit`/`AllBit`/`DelBit`) nutzen | **MS2:** Login komplett mit eigenem IOMan plus Microware-Filemanagern | D7 | |

## Phase E — Eigene Filemanager

Die Verzeichnisse unter `Q9-Manager/` sind derzeit leer.

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| E1 | ⬜ | **Q9-SCF minimal**: Open/Read/Write/ReadLn/WritLn/Close auf der Konsole | Eine Zeile ein, eine Zeile aus | D8 | |
| E2 | ⬜ | **Q9-SCF vollständig**: Echo, Zeileneditor, `SS_Opt` | tsmon und login laufen über Q9-SCF | E1 | |
| E3 | ⬜ | **Q9-RBF lesend**: Open/Read/Seek/Close, Verzeichnisse lesen, auf dem CF-Image | `dir` und Programmstart von CF | D8 | |
| E4 | ⬜ | **Q9-RBF schreibend**: Create/Write/Delete/MakDir, Belegungsbitmap | Create/Write-Round-Trip- und FAT16-mkdir/delete-Regressionen grün | E3 | |
| E5 | ⬜ | **Q9-RBF robust**: Satzsperren, mehrere Prozesse, Fehler- und Volllauffälle | Stresstest grün | E4, A6 | |
| E6 | ⬜ | **Q9-PIPE** | Pipes in der Shell funktionieren | E2 | |
| E7 | ⬜ | **Q9-SBF**: Bedarf klären (sequentielle Geräte) | Entschieden; sonst ⏭️ | — | |

## Phase F — Eigene Treiber und Statusumstellung

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| F1 | ⬜ | **Konsolentreiber** für den 68681-DUART (ersetzt `sc68681`) | Login über eigenen Treiber | E2 | |
| F2 | ⬜ | **CF/IDE-Treiber** (ersetzt `cfide`) | Boot von CF über eigene Kette | E3 | |
| F3 | ⬜ | **Statusumstellung**: `I$`-Aufrufe und `F$PErr`/`F$IOQu`/`F$IODel` von 🔷 auf ✅, wo die Kette eigen ist | **MS3:** `Q9-KERNEL/STATUS.md` ohne 🔷, jede Zeile mit Beleg | F1, F2 | |

## Phase G — Speicherschutz (MMU/SSM)

**Blockiert:** Q9-Flux hat kein 68030-PMMU-Modell (`PFLUSH` wird als
unbehandelt gemeldet). Siehe „MMU/SSM audit" in `Q9-KERNEL/STATUS.md`.

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| G0 | ⛔ | **PMMU-Modell in Q9-Flux** (eigenes Projekt, mehrere Sessions) | `PMOVE`/`PFLUSH`/Tablewalk im Emulator getestet | — | |
| G1 | ⛔ | **Adressraumstruktur im Prozessdeskriptor** | Kontextwechsel lädt den Adressraum | G0 | |
| G2 | ⛔ | **Seitentabellen und Fault-Behandlung** | Zugriffsfehler beendet den Prozess, nicht den Kernel | G1 | |
| G3 | ⛔ | **`F$Permit`/`F$Protect`/`F$ChkMem` echt** | Verbotener Zugriff wird abgewiesen | G2 | |
| G4 | ⛔ | **`F$AllTsk`/`F$DelTsk`/`F$GSPUMp` echt** | **MS4:** Werte stimmen gegen das Handbuch | G2 | |

## Phase H — Werkzeuge und Pflege

Kann jederzeit eingeschoben werden, wenn eine Fehlersuche es verlangt.

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| H1 | ✅ | **Emulator-Debugger mit Symbolen** aus Linkkarte und Moduldirectory (`DEBUG_KONZEPT_de.md`, Abschnitt 5) | PC wird als `Funktion+Offset` angezeigt | — | Q9-Flux: Modul-Header-Hinweis aus A3/PD; Live-Breakpoints in `echo` und dynamischem `trace` zeigen `echo+0x4e` bzw. `TrStart+0x0`. `trace.map` wird beim Kernel-Build erzeugt; Q9-Flux `6271936`: Microware-`.stb`-Code-Symbole sind Fallback hinter `.map`; CRC-Laufzeitabgleich in Q9-Flux `5ebf5b5` ergänzt. |
| H2 | ✅ | **Quelltextzeilen** aus den entschlüsselten `xcc -g`-Sätzen (Abschnitt 5.3) | PC wird als `datei.c:zeile` angezeigt | H1, A3 | Echter `xcc -g`-C-Befehl als dynamische private Prozesskopie live geprüft: `PC=00083916 main+0x1e [h2probe.c:6:4]`. Endmarken verhindern falsche Zeilen in nicht instrumentiertem Laufzeitcode; Q9K-Quelltext-Debug-Build plus Startup-Emulatorregression bestanden. |
| H3 | ✅ | **GDB-Stub** (Debug-Paket 5) | RSP-Breakpoint und Einzelschritt | H1 | Q9-Flux `8790ca6`; `make test-gdbstub`/`make test` mit RSP-Breakpoint und Einzelschritt bestanden; OS-9-Live-Attach mit Register-/Speicherzugriff und Detach bestanden. GDB-RSP `Z0`-Breakpoint in dynamischem `h2probe` bei `0x83916` live getroffen; PC/A3=`00083916`/`000836e0`, gueltiger Modulheader. `vCont;s` stoppt danach bei `0x8391a` innerhalb derselben instrumentierten C-Zeile. Host-Dump zeigt `main+0x1e [h2probe.c:6:4]`. |
| H4 | ✅ | **Doku-Hygiene**: lange Statuschronik und alte Übergabe archivieren; kurze Einstiegspunkte behalten | Historie bleibt unverändert auffindbar; aktuelle Statusquellen sind verlinkt | — | Chronik bis Fortsetzung 144 und Übergabe vom 14.09.2026 unter `docs/archive/`; Kurzseiten verweisen auf `Q9-KERNEL/STATUS.md` und diesen Plan. |

# Teil 2 — Userland

Grundlage ist der Abgleich vom 07.10.2026: `Q9SYS/CMDS` gegen die
Verzeichnisse in `Q9-Tools/System`, `Network` und `Language`. Jedes der 131
noch von Microware stammenden Programme steht unten in genau einer Zeile.
Programme, deren Zweck nicht sicher feststeht, sind **nicht geraten
einsortiert**, sondern stehen in Phase O, bis Schritt I1 sie einordnet.

## Phase I — Userland-Grundlagen

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| I1 | ⬜ | **Einordnung aller 131 Microware-Programme**: Zweck (Handbuch, `ident`), Abhängigkeiten, Priorität; Phase-O-Blöcke danach neu schneiden | Tabelle in `Q9-Tools/docs`, jedes Programm einer Phase zugeordnet | — | |
| I2 | ⬜ | **Regressionslauf für Werkzeuge**: jedes Q9-Tool mit Beispielaufruf im Emulator starten, Ausgabe vergleichen | Ein Skript, lauffähig unter Microware- und unter Q9-Kernel | A1 | |
| I3 | ⬜ | **Alle Q9-Tools auf dem Q9-Kernel** laufen lassen | Ergebnisliste; Kernelfehler als neue Zeilen in Phase A/C | I2 | |
| I4 | ⬜ | **`mdir` und `devs` vollständig** über `F$CpyMem` (im Q9-Kernel inzwischen ✅) | Ausgabe entspricht dem Original | I2 | |
| I5 | ⬜ | **Offene Funktionen aus `PORT_STATUS.md`**: `touch` (Setstat), `del -e`, `del -f`, zentrale `-x`-Auflösung | Alle vier Punkte erledigt oder begründet offen | I2 | |

## Phase J — Systemstart, Anmeldung, Shell

Ohne diese Phase gibt es kein eigenes System: heute laufen sysgo, tsmon,
login und mshell aus dem Microware-Bestand.

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| J1 | ⬜ | **`sysgo`** eigen | Boot startet über eigenes sysgo bis zum startup | MS1 | |
| J2 | ⬜ | **init-Modul aus eigener Quelle** (heute aus dem CB030-Port über `mk_init.sh`) | Byteidentisch zum bisherigen oder Abweichungen begründet | — | |
| J3 | ⬜ | **`tsmon`** eigen | Anmeldeaufforderung über eigenes tsmon | J1 | |
| J4 | ⬜ | **`login`** eigen, samt Passwortdatei | Anmeldung mit Benutzer und Passwort | J3 | |
| J5 | ⬜ | **Shell-Entscheidung** (ersetzt `shell` und `mshell`): eigene mshell-kompatible Shell oder bash als Standard (bash 1.12 läuft nur bis 16 MB RAM) | Entscheidung dokumentiert | — | |
| J6 | ⬜ | **Shell Grundfunktionen**: Befehle, Umleitung, Pipes, Hintergrund, Variablen | Interaktiver Betrieb ohne mshell | J5 | |
| J7 | ⬜ | **Shell-Skripte**: `startup` und vorhandene Skripte laufen unverändert | Boot-startup und eine Skriptsammlung grün | J6 | |
| J8 | ⬜ | **`su`, `tmode`, `xmode`** | Alle drei eigen und geprüft | J4 | |

## Phase K — Laufzeit und Bibliotheken

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| K1 | ⬜ | **qclib-Abweichungen ordnen**: die 61 🟡-Funktionen nach Gruppe und Korpus-Häufigkeit sortieren | Reihenfolge für K2–K4 festgelegt | — | |
| K2 | ⬜ | **qclib: Zeitfunktionen** | Gruppe auf ✅ | K1 | |
| K3 | ⬜ | **qclib: Signale** | Gruppe auf ✅ | K1 | |
| K4 | ⬜ | **qclib: Mathematik (Transzendente)** | Gruppe auf ✅ | K1 | |
| K5 | ⬜ | **Trapmodule `csl`, `math`, `cio`**: werden sie für eigene Programme noch gebraucht, oder nur für Fremdprogramme? Bei Bedarf eigene Fassung | Entscheidung belegt; ggf. eigenes Modul im Boot | K4 | |
| K6 | ⬜ | **Q9-Tools mit eigener Kette bauen**, erster Block von 10 (`qcc` + qclib statt xcc + `clib.l`) | Die 10 laufen gleich wie die xcc-Fassung | K1 | |
| K7 | ⬜ | **Alle Q9-Tools mit eigener Kette** | **MS6:** kein Programm braucht mehr den Microware-Compiler | K6 | |

## Phase L — Entwicklungswerkzeuge auf dem Ziel

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| L1 | ⬜ | **`cc`, `c68`, `o68`, `cpp`, `r68`, `l68`** durch die QCC-Kette abdecken: Aufrufnamen, Standardpfade (`qcc.conf`, `/dd/LIB/Q9`, `/dd/DEFS`) | Ein C-Programm lässt sich auf dem Ziel mit den gewohnten Befehlen bauen | K6 | |
| L2 | ⬜ | **`make`** | Ein Q9-Tools-Makefile läuft auf dem Ziel | L1 | |
| L3 | ⬜ | **Modul- und ROF-Werkzeuge**: `rdump`, `mkdatmod`, `editmod` | Ausgabe stimmt mit dem Original überein | L1 | |
| L4 | ⬜ | **`debug`** (Debugger) | Breakpoint und Einzelschritt in einem Programm | C2 | |
| L5 | ⬜ | **Editoren `umacs` und `edt`** | Datei öffnen, ändern, speichern | — | |

## Phase M — Datenträger und Archive

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| M1 | ⬜ | **`format` und `os9gen`** | Neues Abbild formatiert und bootfähig gemacht | — | |
| M2 | ⬜ | **`backup`, `dsave`, `fsave`, `frestore`** | Sichern und Zurückspielen eines Verzeichnisbaums | — | |
| M3 | ⬜ | **`tar`, `compress`, `unzip`** | Archive des Originals werden gelesen und erzeugt | — | |
| M4 | ⬜ | **`undel`, `rcopy`, `mount`, `park`, `diskcache`** | Alle fünf geprüft | — | |
| M5 | ⬜ | **`partition`, `partdgen`, `pcformat`, `mbinstall`, `mbdump`** | Alle fünf geprüft | — | |

## Phase N — Netzwerk

Abgleich mit `Q9-Tools/Network/NETWORK_STATUS.md` zu Beginn von N1.

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| N1 | ⬜ | **Grundkonfiguration**: `ipstart`, `dhcp`, `bootpd`, `arp`, `routed` | Netzwerk startet über eigene Programme | — | |
| N2 | ⬜ | **Dienste**: `telnetd`, `telnetdc`, `ftpd`, `ftpdc`, `tftpd`, `tftpdc` | Anmeldung per telnet, Dateiübertragung per ftp/tftp | N1 | |
| N3 | ⬜ | **RPC-Grundlage**: `portmap`, `rpcgen`, `rpcdbgen`, `rpcdump`, `rpchost` | RPC-Dienst registrierbar und abfragbar | N1 | |
| N4 | ⬜ | **RPC-Dienste**: `rstatd`, `rup`, `rusers`, `rusersd`, `spray`, `sprayd`, `rexd`, `rexdc`, `rpr` | Alle geprüft | N3 | |
| N5 | ⬜ | **NFS**: `nfsc`, `nfsd`, `mountd`, `exportfs`, `showmount`, `nfsstat`, `pcnfsd`, `nfs.map` | Freigabe einbinden, lesen, schreiben | N3 | |
| N6 | ⬜ | **SMB**: `smbdrv`, `smbmount`, `smbsetime`, `smbstat`, `smbview` | Freigabe einbinden | N1 | |
| N7 | ⬜ | **PPP und seriell**: `pppd`, `pppauth`, `chat`, `kermit` | Verbindung aufgebaut, Datei übertragen | N1 | |

## Phase O — Grafik und übrige Programme

Die Blöcke O2–O6 sind vorläufig alphabetisch geschnitten. Schritt I1
ordnet ihren Inhalt ein und schneidet sie danach neu.

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| O1 | ⬜ | **Grafik (MAUI)**: `maui`, `graphic`, `setfont`, `fkeys` | Bedarf entschieden; wenn gebraucht, eigen und geprüft | I1 | |
| O2 | ⬜ | Block: `BOOTOBJ`, `bfed`, `cfp`, `code`, `com`, `cslot`, `cudo`, `dird`, `dpsplit` | Eingeordnet und eigen | I1 | |
| O3 | ⬜ | Block: `exbin`, `expand`, `idbdump`, `idbgen`, `initslot`, `irqs`, `lmm`, `maps`, `merge` | Eingeordnet und eigen | I1 | |
| O4 | ⬜ | Block: `moded`, `msgd`, `ndbmod`, `ndpio`, `on`, `p2init`, `padrom`, `probe`, `pwrstat` | Eingeordnet und eigen | I1 | |
| O5 | ⬜ | Block: `rldd`, `rload`, `romsplit`, `sortd`, `spfndpd`, `spfndpdc`, `spfnppd`, `spfnppdc` | Eingeordnet und eigen | I1 | |
| O6 | ⬜ | Block: `tape`, `tapegen`, `tapestart`, `target`, `undpd`, `undpdc`, `x`, `y` | **MS7:** Eingeordnet und eigen; `CMDS` enthält kein Microware-Programm mehr | I1 | |

## Hinweise zum Userland
- **Reihenfolge:** Phase J setzt einen stabilen Kernel voraus (MS1). Phasen
  I, K, L und M können parallel zu Teil 1 laufen, weil sie zunächst unter
  dem Microware-Kernel testbar sind.
- **Ein Programm gilt als eigen,** wenn es aus eigener Quelle gebaut ist und
  der Regressionslauf aus I2 es unter dem Q9-Kernel grün meldet.
- **`F$MBuf`** bleibt mangels belegter ABI offen (siehe Phase C).

## Quellen

- `Q9-KERNEL/STATUS.md` — Kopf (Stand Fortsetzung 144), Syscall-Tabellen,
  „Remaining kernel work outside the SysCalls"
- `docs/archive/KERNEL_NEXT_SESSION_de_2026-09-14.md` — „Technische Schulden" (Stand 14.09.2026)
- `docs/archive/OWN_KERNEL_STATUS_chronik_2026-10-12.md` — Fortsetzungen 1–144
- `docs/DEBUG_KONZEPT_de.md` — Abschnitte 5–7
- `Q9-IOMAN/STATUS.md` — Abschnitte 5, 7 und 8
- `Q9-Tools/System/PORT_STATUS.md`, `Q9-Tools/Network/NETWORK_STATUS.md`
- `Q9-QCC/Q9-BACKEND-68K/q9-qclib/STATUS.md`
- Abgleich `Q9-Images/cf_images/Q9SYS/CMDS` gegen `Q9-Tools` (07.10.2026)
