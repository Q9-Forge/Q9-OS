# Kernel-Arbeitsplan

Stand: 07.10.2026, Grundlage `main` `2ea3c3c` (Fortsetzung 122).

Dieser Plan zerlegt alle offenen Arbeiten am Q9-Kernel in Schritte, die
jeweils **in einer Session** erledigt werden können. Er ersetzt die Roadmap
in `KERNEL_NEXT_SESSION_de.md` (Stand 14.09.2026, überholt).

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
| A | Kernel-Fundament | 0 / 8 | ⬜ |
| B | Technische Schulden | 0 / 4 | ⬜ |
| C | Teilweise Syscalls abschließen | 0 / 8 | ⬜ |
| D | Eigener IOMan | 0 / 8 | ⬜ |
| E | Eigene Filemanager | 0 / 7 | ⬜ |
| F | Eigene Treiber, Statusumstellung | 0 / 3 | ⬜ |
| G | Speicherschutz (MMU) | 0 / 5 | ⛔ |
| H | Werkzeuge und Pflege | 0 / 4 | ⬜ |
| **Gesamt** | | **0 / 47** | |

| Meilenstein | Status |
|---|---|
| M1 Stabil mit Microware-I/O | ⬜ |
| M2 Eigener IOMan | ⬜ |
| M3 Eigene I/O-Kette | ⬜ |
| M4 Speicherschutz | ⛔ |

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

## Meilensteine

| Meilenstein | Erreicht nach | Grobe Schätzung |
|---|---|---|
| M1 Stabil mit Microware-I/O | Phasen A, B, C | ~20 Sessions |
| M2 Eigener IOMan (mit Microware-Filemanagern) | Phase D | ~8 Sessions |
| M3 Eigene I/O-Kette | Phasen E, F | ~10 Sessions |
| M4 Speicherschutz | Phase G | offen, wartet auf Q9-Flux |

Die Schätzungen sind grob. Besonders D2, E3 und E4 können sich teilen, wenn
unterwegs Fehler auftauchen.

## Phase A — Kernel-Fundament

Zuerst, weil sonst alles andere blockiert: ohne Regressionslauf ist kein
späterer Schritt nachweisbar, und ohne FARCALL stößt jede größere Änderung
an die `bsr`-Reichweite (±32 KB bei 62 KB Kernel).

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| A1 | ⬜ | **Ein-Kommando-Regressionslauf**: frischer Build → Image-Klon → Login → `echo`/`dir`/`pd`/`date` mit Ausgabevergleich | Ein Skript meldet grün/rot für beide Varianten; vorhandene Einzelskripte (`startup_shell_test.sh`, `mgrpath_probe_test.sh`) eingebunden | — | |
| A2 | ⬜ | **FARCALL-Bisektion**: Stellen 1–21 mit `fcbisect.sh` weiter halbieren | Schuldige Stelle(n) und Ursache benannt. Zuerst prüfen, ob dort `a0` beim Aufruf noch gebraucht wird — `Q9K_FARCALL` überschreibt `a0` | A1 | |
| A3 | ⬜ | **FARCALL ausrollen**: alle 42 `bsr` auf C-Funktionen in `q9kernel_entry.a` umstellen; Reichweitenprüfung im Build | Login grün in beiden Varianten; `Q9K_SRCDEBUG` mit mehreren Dateien baut und bootet | A2 | |
| A4 | ⬜ | **`build.sh` ehrlich machen**: `mwos-build`-Fehler werden heute ignoriert, alte Objekte täuschen Erfolg vor | Ein absichtlich eingebauter Fehler bricht den Build ab | — | |
| A5 | ⬜ | **Register-Erhaltungsprüfung** im Developer-Kernel (Debug-Paket 3, `DEBUG_KONZEPT_de.md`) | Login plus Befehlsfolge ohne Meldung, oder eine Fundliste als neue Zeilen in Phase A | A1 | |
| A6 | ⬜ | **Stresstest verschachtelter externer Traps**: mehrere Prozesse mit gleichzeitigem I/O über IOMan, Filemanager und Treiber bei laufendem Timer | N Durchläufe ohne Exception; Szenario eingecheckt und schaltbar wie `Q9K_TestNestedTrapStress` | A1 | |
| A7 | ⬜ | **IRQ während Queue- und Kontextübergabe**: kritische Abschnitte inventarisieren, gezielt Interrupts injizieren | Jeder Abschnitt ist abgesichert oder seine Sicherheit schriftlich begründet | A6 | |
| A8 | ⬜ | **Langlauf und Ressourcenbilanz**: ~1000 Zyklen `F$Fork`/`F$Exit`/`F$TLink` | Freispeicher, Modul-Linkzähler und Trap-Slots kehren auf den Ausgangswert zurück | A1 | |

## Phase B — Technische Schulden

Aus `KERNEL_NEXT_SESSION_de.md`, Abschnitt „Technische Schulden".

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| B1 | ⬜ | **`Q9K_ApplyInitializedData`**: Grenzprüfungen für Modul und Zielspeicher; das verworfene IRefs-Gruppenwort klären; unterstützten Relokationsumfang festlegen | Hosttests mit kaputten Modulen und Grenzfällen grün | — | |
| B2 | ⬜ | **Fehlerpfade von `F$TLink`**: erworbene Modulreferenzen, Static-Speicher und Trap-Slots zurückgeben | Negativtest ohne Ressourcenverlust | A8 | |
| B3 | ⬜ | **Scratch-Adressen und IRQ-Sperre im Trap-Pfad**: zentrale Belegungstabelle, Regel für Verschachtelung und Kontextwechsel | Keine Überlappung; Prüfskript läuft im Build mit (vgl. behobener `$3F0`/`$3F8`-Konflikt aus Fortsetzung 122) | — | |
| B4 | ⬜ | **`Q9K_PatchCslFreelistBug`** prüfen: greift er überhaupt (erwartetes Bytemuster fehlte am 16.09.)? | Entscheidung belegt: als Kompatibilitätsmaßnahme gekennzeichnet oder entfernt | A1 | |

## Phase C — Teilweise Syscalls abschließen

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| C1 | ⬜ | **`date` und `F$VModul`**: Status-Hinweis „`date` erreicht `F$VModul` nie" ist vermutlich seit dem `F$Load`-Fix (Fortsetzung 108) überholt | Neu gemessen, Status korrigiert | A1 | |
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
| D8 | ⬜ | **`F$Load`, `F$PErr`, Bitmap-Dienste** im eigenen IOMan; native Kernel-Bitmaps (`F$SchBit`/`AllBit`/`DelBit`) nutzen | **M2:** Login komplett mit eigenem IOMan plus Microware-Filemanagern | D7 | |

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
| F3 | ⬜ | **Statusumstellung**: `I$`-Aufrufe und `F$PErr`/`F$IOQu`/`F$IODel` von 🔷 auf ✅, wo die Kette eigen ist | **M3:** `Q9-KERNEL/STATUS.md` ohne 🔷, jede Zeile mit Beleg | F1, F2 | |

## Phase G — Speicherschutz (MMU/SSM)

**Blockiert:** Q9-Flux hat kein 68030-PMMU-Modell (`PFLUSH` wird als
unbehandelt gemeldet). Siehe „MMU/SSM audit" in `Q9-KERNEL/STATUS.md`.

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| G0 | ⛔ | **PMMU-Modell in Q9-Flux** (eigenes Projekt, mehrere Sessions) | `PMOVE`/`PFLUSH`/Tablewalk im Emulator getestet | — | |
| G1 | ⛔ | **Adressraumstruktur im Prozessdeskriptor** | Kontextwechsel lädt den Adressraum | G0 | |
| G2 | ⛔ | **Seitentabellen und Fault-Behandlung** | Zugriffsfehler beendet den Prozess, nicht den Kernel | G1 | |
| G3 | ⛔ | **`F$Permit`/`F$Protect`/`F$ChkMem` echt** | Verbotener Zugriff wird abgewiesen | G2 | |
| G4 | ⛔ | **`F$AllTsk`/`F$DelTsk`/`F$GSPUMp` echt** | **M4:** Werte stimmen gegen das Handbuch | G2 | |

## Phase H — Werkzeuge und Pflege

Kann jederzeit eingeschoben werden, wenn eine Fehlersuche es verlangt.

| Nr | Status | Session-Ziel | Fertig, wenn | Vorauss. | Beleg |
|---|---|---|---|---|---|
| H1 | ⬜ | **Emulator-Debugger mit Symbolen** aus Linkkarte und Moduldirectory (`DEBUG_KONZEPT_de.md`, Abschnitt 5) | PC wird als `Funktion+Offset` angezeigt | — | |
| H2 | ⬜ | **Quelltextzeilen** aus den entschlüsselten `xcc -g`-Sätzen (Abschnitt 5.3) | PC wird als `datei.c:zeile` angezeigt | H1, A3 | |
| H3 | ⬜ | **GDB-Stub** (Debug-Paket 5) | Breakpoint und Einzelschritt aus gdb | H1 | |
| H4 | ⬜ | **Doku-Hygiene**: `OWN_KERNEL_STATUS.md` (über 11.000 Zeilen) archivieren und kürzen; `KERNEL_NEXT_SESSION_de.md` ins Archiv | Kurzer aktueller Status plus Archiv | — | |

## Nicht in diesem Plan

- **Userland:** sysgo, tsmon, login, mshell, csl und math sind ebenfalls
  Microware-Module. Soll Q9-OS auch dort unabhängig werden, braucht es einen
  eigenen Plan.
- **`F$MBuf`:** bleibt mangels belegter ABI offen (siehe Phase C).

## Quellen

- `Q9-KERNEL/STATUS.md` — Kopf (Stand Fortsetzung 122), Syscall-Tabellen,
  „Remaining kernel work outside the SysCalls"
- `docs/KERNEL_NEXT_SESSION_de.md` — „Technische Schulden" (Stand 14.09.2026)
- `docs/OWN_KERNEL_STATUS.md` — Fortsetzung 122 (FARCALL-Bisektion)
- `docs/DEBUG_KONZEPT_de.md` — Abschnitte 5–7
- `Q9-IOMAN/STATUS.md` — Abschnitte 5, 7 und 8
