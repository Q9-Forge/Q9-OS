# IRQ-Sperre im Trap-Pfad des Q9-Kernels (Arbeitsplan B3)

Stand 15.10.2026. Jede Aussage ist an `Q9-KERNEL/68k/src/kernel/q9kernel_entry.a` belegt
(Zeilennummern = Stand dieses Commits; bei Änderungen mit `grep -n 'ori.w *#\$0700,sr'` neu ermitteln).
Die zugehörige Belegungstabelle der Scratch-Zellen steht in `KERNEL_SPEICHERBELEGUNG_de.md`
(erzeugt von `tools/q9_scratch_map.py`, geprüft von `build.sh`).

## Regel

Der Trap-Pfad benutzt **feste Scratch-Zellen** (`Q9K_TrapCallerPC`, `Q9K_InTrapPath`,
Callcode `$1370`, `Q9K_*Scratch_*` …), keine Stackrahmen pro Aufruf. Solche Zellen sind nur dann
sicher, wenn zwischen Schreiben und Lesen **kein zweiter Trap-Pfad-Durchlauf** (etwa aus einer ISR,
die selbst einen Syscall absetzt) und kein Kontextwechsel dazwischenkommt. Deshalb gilt:

1. **Der Trap-Dispatch-Einstieg sperrt Interrupts sofort** (`ori.w #$0700,sr`, Zeile 3426),
   noch vor dem Retten der Register und vor dem ersten Schreiben in eine Scratch-Zelle.
2. **Der Dispatch-Pfad nimmt die Sperre nicht selbst zurück, sie endet mit dem `RTE`**: maßgeblich ist
   danach das im Exception-Frame gerettete SR, das unangetastet bleibt (Kommentar Zeilen 3416–3425).
   (Andere Stellen der Tabelle unten geben Interrupts gezielt frei, z. B. Zeile 2716.)
3. **Der Rückweg wird erneut gesperrt** (Zeile 3824), weil ein externer Handler (Treiber-ISR-Pfad)
   die Sperre absichtlich aufhebt, um senden zu können; ohne die zweite Sperre liefe der Epilog mit
   offenen Interrupts und könnte verschachtelt durchlaufen werden.
4. **Neue Scratch-Zellen** gehören in die Tabelle (`tools/q9_scratch_tables.txt` bzw. als `equ`),
   `build.sh` bricht bei Doppelbelegung ab. Wer eine Zelle bewusst mehrfach nutzt, trägt sie mit
   Begründung in `tools/q9_scratch_shared.txt` ein.

## Alle Sperr-/Freigabestellen in `q9kernel_entry.a`

| Zeile | Befehl | Zweck |
|---:|---|---|
| 1275 | `move.w #$2700,sr` | Frühstart: Interrupts aus, bis der Kernel steht |
| 2692 | `ori.w #$0700,sr` | Start des Testprozess-Pfads (Maskierung) |
| 2716 | `andi.w #$f8ff,sr` | Freigabe dort: F$Fork und `hellosvc` brauchen IRQs (Konsolenausgabe über I$WritLn) |
| 3426 | `ori.w #$0700,sr` | **Trap-Dispatch-Einstieg** (siehe Regel 1/2) |
| 3483 / 3490 | `ori.w` / `move.w (sp)+,sr` | gesperrter Abschnitt mit gesichertem SR (Zweck nicht einzeln geprüft) |
| 3824 | `ori.w #$0700,sr` | **Rückweg gegen Verschachtelung** (Regel 3) |
| 4037 / 4044 | `ori.w` / `move.w (sp)+,sr` | gesperrter Abschnitt mit gesichertem SR (Zweck nicht einzeln geprüft) |
| 5217 | `ori.w #$0700,sr` | „ZUERST sperren“ (Begründung im Kommentar dort) |
| 5356 | `ori.w #$0700,sr` | Sperre nach einem Handleraufruf wiederherstellen |
| 8389 / 8393 | `Q9K_IntLock` / `Q9K_IntUnlock` | Aufrufbar aus C: sperrt, liefert altes SR; Unlock setzt es zurück |

## Befund: Zeitpunkt des IRQ entscheidet (nachgewiesen im Quelltext-Kommentar)

14 Byte reine NOPs im Trap-Pfad zerlegten den Kernel reproduzierbar, dieselben 14 Byte als toter
Code nicht – es war also nie das Layout, sondern der Zeitpunkt des IRQ. Die Sperre am Einstieg
(Regel 1) beseitigt diese Abhängigkeit.

## Offener Befund (15.10.2026, Login-Test)

Bei **schnell aufeinanderfolgenden Eingaben** (Test mit `Q9K_LOGIN_PAUSE=0`) stürzt der
Entwickler-Kernel nach einigen Forks ab: Vektor 4 (Illegal Instruction), PC im Kerneldatenbereich
(`$128c`, Warteschlangen bei `$1240`–`$12f0`), Rücksprungadressen in `mshell`; `Q9K_ExcTrap` hält
dann mit einem einzelnen `E` an (`Q9K_ExcTrapSpin`, Zeile 1901). Mit einer Sekunde Pause vor jeder
Eingabe läuft derselbe Test in beiden Varianten durch. Ursache **nicht geklärt**; verdächtig sind
das SR-Fenster zwischen Handler-Rückkehr und Epilog (Regel 3) und Eingabe-Interrupts, die während
eines Forks einschlagen. Eigener Arbeitsschritt (Trap-Stresstest), kein Teil von B3.

## A7 — Inventur der Queue-Zugriffe und IRQ-Fenster (15.10.2026)

Schutzregel: **Queues, Prozessdeskriptoren und die Freiliste ändert nur Code, den kein anderer Interrupt unterbrechen
kann.** Wer sie anfasst, läuft entweder (a) im Trap-Dispatch mit Maske `$0700` (Einstieg Zeile 3426, endet mit dem `rte`),
(b) im Timer-Interrupt (Level 6, höher als jede Geräte-ISR), (c) im Boot vor dem ersten Interrupt oder (d) unter
`Q9K_IntLock` (Arena).

| Kontext | Wer fasst Queues/Listen an | Schutz |
|---|---|---|
| **Trap-Dispatch** (`TRAP #0`) | alle `Q9K_Sys*Impl` über `Q9K_FARCALL`: `Q9K_SchedInsert`, `Q9K_WaitQInsert`, `Q9K_SleepQInsert`, `Q9K_SchedWake`, `Q9K_ProcSend`, `Q9K_ProcExit`, `Q9K_SchedCommitPreempt` (aus `F$AProc`) | Maske `$0700` ab Einstieg. Ausnahme: **externe Handler** (IOMan, Treiber) öffnen sie absichtlich; ein Systemaufruf aus ihnen maskiert am eigenen Einstieg erneut. Der Rückweg wird erneut gesperrt (Zeile 3824). |
| **Timer-IRQ** (Level 6, 10 ms) | `Q9K_SchedReschedule`: Ready-Queue, Sleep-Queue (`Q9K_SleepQDecrementAll`), Alarme, Uhr | Kein anderer Interrupt (außer NMI) kann ihn unterbrechen. **Neu:** schaltet nicht um, wenn der Unterbrochene selbst eine Interrupt-Maske hatte (`Q9K_SchedNoSwitch`, s. unten). |
| **Geräte-ISR** (Level 3, `sc68681`) | keine direkt; Weckpfade (`F$Send`) gehen über den Trap-Dispatch | Der Timer darf sie unterbrechen. **Vorher:** er konnte dabei umschalten und die halb ausgeführte ISR auf dem Stack des alten Prozesses liegen lassen, während Level 3 wieder frei war (Wiedereintritt möglich). **Jetzt:** kein Wechsel, nur Tick-Buchführung. |
| **Trace-Ausnahme** (Vektor 9, `F$DExec`-Einzelschritt) | `Q9K_ProcDebugTrace` | **Vorher ungeschützt** (Maske des Unterbrochenen, meist 0). **Jetzt:** `ori.w #$0700,sr` am Eintritt, endet mit dem `rte`. |
| **Arena** (`Q9K_AllocMem`/`Q9K_FreeMem`/`Q9K_AllocLargest`) | Freiliste | `Q9K_IntLock`/`Q9K_IntUnlock` um jede Funktion; im Developer-Kernel zusätzlich `Q9K_ArenaAudit`. |
| **Boot** (`Q9K_CInit`) | Pools, Queues anlegen | Interrupts aus, bis der Start fertig ist. |

**Änderungen aus dieser Inventur:** (1) `Q9K_TimerIRQHandler` trägt `SR & $0700` des unterbrochenen Codes in `Q9K_SchedNoSwitch`
(`$2488`) ein; `Q9K_SchedReschedule` gibt dann nach der Tick-Buchführung 0 zurück, die Zeitscheibe bleibt. Hosttest
`test_q9kernel_sched` Fall 4b. (2) `Q9K_TraceHandler` sperrt am Eintritt.

**Nicht getan / offen:** Das gezielte Einspielen von Interrupts (Injektion) fehlt: belegt ist die Wirksamkeit nur durch die
Lasttests mit dem normalen 10-ms-Takt (`leak_test.sh`, A6, A12). Ein Test mit stark verkürzter Tickzeit im Emulator würde
die Fenster häufiger treffen. Außerdem unverändert: `Q9K_RaceRing`-Marker werden in jedem Trap geschrieben und sind
kein Schutzmechanismus.

