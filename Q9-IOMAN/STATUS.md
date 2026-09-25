# Q9-IOMAN – Implementierungsstatus

Stand: **2026-09-24**
Ziel: ein eigenständiger Q9-I/O-Manager mit klarer Kernel-, File-Manager-
und Treibergrenze. Dieses Inventar ist eine Q9-Anforderungsliste, keine
Behauptung vollständiger Microware-Kompatibilität.

## Legende

| Status | Bedeutung |
|---|---|
| 🔴 | Nicht fertig, nicht angefangen |
| 🟡 | Teilweise implementiert; der Tabellenhinweis grenzt den fertigen Teil ab |
| 🟢 | Für den ausdrücklich genannten Umfang komplett fertig und getestet |
| 🔵 | Schnittstelle, Zuständigkeit oder ABI noch zu klären; Implementierung wartet darauf |
| ⚪ | Bewusst nach dem ersten lauffähigen System zurückgestellt / optional |

**Wichtig:** Der IOMan ist als OS-9-Modul baubar und die drei Schattenhandler
sind an den Router gebunden; produktive Backends/Attach fehlen noch. Die
Prozess-/Dup-Lebensdauerintegration ist offen. Der isolierte Emulatorlauf
kommt inzwischen ohne Illegal Instruction über den IOMan-Einstieg hinaus;
die tatsächliche Weiterleitung eines I/O-Aufrufs an einen Schattenhandler
ist noch nicht end-to-end nachgewiesen (siehe „Laufzeittest“).

## Laufzeittest

| Test | Ergebnis | Aussage |
|---|---|---|
| Erstlauf mit `JSR absolut` zu QCC-Funktionen | ❌ Illegal Instruction, Vektor 4, `PC=$7031` | Instruktionsspur zeigte den Sprung vom Modul bei `$18D20` auf `$290C` statt auf `Modulbasis+$290C`. Ursache: absoluter Aufruf war für das relocierbare Modul ungeeignet. |
| Kontrollklon mit gleichem frisch gebautem Kernel und denselben Diskmodulen, aber originalem IOMan (5.660 Byte, gültige CRC/Parität) | ✅ Bootstrap läuft weiter; CompactFlash-Treiber und normale Programmtestausgaben erscheinen; Exception-Mitschrift bleibt leer | Derselbe Kernel-/Emulatorlauf funktioniert mit dem originalen IOMan. Das grenzt den Fehler auf den neuen IOMan-Einstieg oder dessen Integration ein. |
| Neuer Build mit PC-relativen `BSR`-Aufrufen zu QCC-Funktionen | 🟡 Einstieg und C-Initialisierung laufen ohne Exception; Watchpoint bestätigt `F$SSvc`-Einträge `Open=$18D64`, `Read=$18DA8`, `Close=$18DEC`; Instruktionsspur erreicht `Q9IOMAN_OpenEntry` bei `$18D64` | Relokationsfehler behoben und `I$Open`-Dispatch bis zum Handler-Eintritt live nachgewiesen. Handler-Rückgabe mit fehlendem Backend, `Read`/`Close`, Attach und Dateisystem-Backends sind noch offen. |
| Kontrollierter, temporärer nicht-nativer `I$Open` aus `M$Exec` ohne registriertes Backend | 🟡 Kernel-Trace: `$84`-Trap → externer Handler (`X`) → Rückkehr (`A`); keine Exception | Der reentrante Aufruf läuft durch den registrierten Open-Einstieg und kehrt zum Aufrufer zurück. Der Emulatorlauf sicherte `D1`/Carry nicht; der Hosttest bestätigt separat `E$MNF` plus Carry. Der Test-Hook ist aus dem Produktionsbuild entfernt. |
| Temporärer Emulator-Backendtest `Open → Read → Close` mit Testmodus | 🟡 `I$Open` erreicht den Backendpfad und liefert Handle `1`; `I$Read` wird mit Handle `1`, Buffer und Länge `4` aufgerufen, kehrt aber mit `E$BPNum` (`D1=$00C9`, Carry gesetzt) zurück; Close wird nicht erreicht | Der Test fand zwei QCC-Stack-ABI-Fehler: Assembler-Stubs pushten `(frame, callcode)` statt `(callcode, frame)`, und der handgeschriebene Pfadresolver las Callbackargumente in falscher Reihenfolge. Beide Korrekturen sind umgesetzt. Open und Read-Dispatch laufen nun bis zum Pfadslotzugriff; warum der Slot beim Read nicht mehr als offen erkannt wird, ist noch offen. Test-Hooks wurden danach aus dem Produktionsbuild entfernt. |
| Temporärer Zustandstest mit registriertem `/q9diag`-Backend | 🟡 Trace bestätigt `F$SSvc` und Eintritt in den externen `I$Open`-Handler für `/q9diag/file`; der Handler liefert in diesem Testlauf keine sichtbare Rückkehr und `I$Read` wird nicht erreicht | Dieser Testaufbau bleibt daher nicht als Beleg für den ursprünglichen Read-Fehler verwendbar. Instruktionsspur endet in einer Kernel-Warteschleife; Diagnose-Backend und Selbsttest wurden aus dem Produktionsbuild entfernt. |

Beide Läufe verwenden separate `cp -c`-Klone; das Master-Image wurde nicht
verändert. Die Kontrollausgabe enthält lange `A`-Folgen aus der vorhandenen
Kernel-/Emulatordiagnostik; sie sind kein IOMan-Erfolgskriterium. Nächster
Schritt: die Ursache für `E$BPNum` beim Read finden; danach `Close` und den
erfolgreichen Open/Read/Close-Rundlauf nachweisen. Die Emulator-Dumpzähler für
externe `F$SSvc`-Registrierungen belegen die Manager-Schattenhandler nicht:
`I$Open`/`I$Read`/`I$Close` sind Kernel-eigene Dienste und werden separat
gehalten. Die Testklone liegen unter `/private/tmp/q9ioman-bridge-test-full.hda` und
`/private/tmp/q9ioman-control-original-full.hda`. Keine Aussage über
produktive Dateisystemfunktionalität ableiten.
## 1. Kernel → IOMan: empfangene Systemaufrufe

Die 68K-Referenzinventur führt 7 `F$`- und 17 `I$`-Aufrufe auf, insgesamt
24 Eingänge. `I$`-Aufrufe, deren eigentliche Arbeit an File-Manager geht,
brauchen einen IOMan-Einstieg und Dispatch; das Dateisystem selbst gehört
nicht in diese Tabelle. Die Übersicht zählt damit die Callcodes, nicht alle
internen Funktionen.

### `F$`-Eingänge (7)

| ID | Aufruf | Aufgabe im Q9-IOMAN | Status | Noch nötig |
|---|---|---|---|---|
| K-FLOAD | `F$Load` | Programm/Modul über den I/O-Namensraum suchen und laden | 🔴 | Suchreihenfolge, Modulpfad, Übergabe an Kernel-Modullader, Fehler-/Link-Handling |
| K-FPERR | `F$PErr` | OS-9-Fehlertext ausgeben | 🔴 | Fehlercode→Text, Ausgabeziel und Rekursion/Fehlerfälle definieren |
| K-FSCHBIT | `F$SchBit` | Bit in einer IOMan-eigenen Bitmap suchen | 🔴 | Bitmapvertrag, Bereichsprüfung, atomare Reservierung |
| K-FALLBIT | `F$AllBit` | Bits in der Bitmap reservieren | 🔴 | Start-/Längenparameter, Grenzfälle und Konkurrenztests |
| K-FDELBIT | `F$DelBit` | reservierte Bits freigeben | 🔴 | Besitz-/Bereichsprüfung und Fehlersemantik |
| K-FIOQU | `F$IOQu` | I/O-Anfrage in eine Queue einordnen | 🔴 | Queue-Element, Reihenfolge, blockierend/nichtblockierend, Wake-up-Vertrag |
| K-FIODEL | `F$IODel` | I/O-Anfrage aus Queue entfernen | 🔴 | Abbruch, bereits laufende Anfrage und Prozessbeendigung behandeln |

### `I$`-Eingänge (17)

| ID | Aufruf | Aufgabe im Q9-IOMAN | Status | Noch nötig |
|---|---|---|---|---|
| K-IATTACH | `I$Attach` | Descriptor/Device/Manager anbinden und Gerätetabelleneintrag liefern | 🔴 | Namen auflösen, Module link(en), Typen prüfen, Init ausführen, atomar veröffentlichen, Rollback |
| K-IDETACH | `I$Detach` | Attach-Referenz lösen und Gerät ggf. terminieren | 🔴 | Referenz-/Busy-Regeln, Term, Tabelle bereinigen, Links freigeben |
| K-IDUP | `I$Dup` | offenen lokalen Pfad duplizieren | 🔴 | Pfad- und Backend-Referenzzähler, Fehlerrückabwicklung |
| K-ICREATE | `I$Create` | Datei/Objekt anlegen und öffnen | 🟡 | Hostdispatcher plus Backend-Create-Callback und lokaler Pfadslot; Kernel-Schattenhandler und Emulatornachweis fehlen |
| K-IOPEN | `I$Open` | Pfad öffnen und lokale Pfadnummer vergeben | 🟡 | Hostkern kann mit vorgewähltem Backend öffnen und lokalen Slot vergeben; Kernel-Trap, Namens-/Deviceauflösung, Attach und Rechteprüfung fehlen |
| K-IMAKDIR | `I$MakDir` | Verzeichnis anlegen | 🟡 | Hostdispatcher plus Backend-Namensoperation; Kernel-Schattenhandler und Emulatornachweis fehlen |
| K-ICHGDIR | `I$ChgDir` | Prozess-Arbeitsverzeichnis ändern | 🔴 | Pfadtyp prüfen, Prozessdescriptor aktualisieren und Fehlervertrag |
| K-IDEL | `I$Delete` | Datei/Verzeichnis löschen | 🟡 | Hostdispatcher plus Backend-Namensoperation; Kernel-Schattenhandler, Rechte-/Typprüfung und Emulatornachweis fehlen |
| K-ISEEK | `I$Seek` | Position eines offenen Pfades ändern | 🟡 | Registerdecoder/Hostdispatcher mit 32-bit-Position; Kernel-Schattenhandler und Emulatornachweis fehlen |
| K-IREAD | `I$Read` | Bytes lesen | 🟡 | Buffer-/Range- und Transferlängenprüfung im Hostdispatcher; Emulatorlauf endet noch mit `E$BPNum` |
| K-IWRITE | `I$Write` | Bytes schreiben | 🟡 | Registerdecoder/Hostdispatcher mit Zugriffsprüfung; Kernel-Schattenhandler und Emulatornachweis fehlen |
| K-IREADLN | `I$ReadLn` | zeilenorientiert lesen | 🟡 | Registerdecoder/Hostdispatcher; Zeilenregeln, SCF und Emulatornachweis fehlen |
| K-IWRITLN | `I$WritLn` | zeilenorientiert schreiben | 🟡 | Registerdecoder/Hostdispatcher; Terminatorregeln, SCF und Emulatornachweis fehlen |
| K-IGETSTT | `I$GetStt` | Manager-/Gerätestatus abfragen | 🟡 | Registerdecoder/Hostdispatcher mit Statuscode und Ausgabezeiger; Statusformat und Backendimplementierung fehlen |
| K-ISETSTT | `I$SetStt` | Manager-/Gerätestatus setzen | 🟡 | Registerdecoder/Hostdispatcher mit Statuscode und Eingabezeiger; Rechte- und Statusvertrag fehlen |
| K-ICLOSE | `I$Close` | Pfad schließen und lokale Nummer freigeben | 🟡 | Hostkern schließt über Backend und behält den Slot bei Close-Fehler; Trap-/Prozessintegration und endgültige OS-9-Fehlersemantik fehlen |
| K-ISGETST | `I$SGetSt` | systemweiten Status abfragen | 🔴 | Ziel/Anwendungsfälle, Berechtigungen und Kernel-/Managervertrag klären |

## 2. IOMan → File-Manager: weitergereichte I/O-Aufrufe

Das sind die 13 Operationen der File-Manager-Tabelle ab `I$Create` (`0x83`)
bis `I$Close` (`0x8f`). Q9-IOMAN soll prüfen, sperren und weiterleiten; RBF,
SCF oder ein anderer Q9-Manager besitzt die jeweilige Datei-/Gerätesemantik.
Diese Kernel-Callcodes sind Referenzen auf die eingehenden Systemaufrufe;
sie legen weder die Manager-Vektorslots noch die neuen Protokoll-Kommandocodes
fest. Der Entwurf und seine Trennung sind in `docs/IO_PROTOCOL_SPEC.md`
dokumentiert.

| Slot | Weitergabe | Status | Noch nötig |
|---:|---|---|---|
| 0 | `I$Create` → Manager `Create` | 🔴 | callcode→Slot, Argument-/Rückgabekonvention und Fehlerweitergabe |
| 1 | `I$Open` → Manager `Open` | 🟡 | abstraktes Backend-Open existiert; Modul-Dispatch, Namensauflösung und echte Manager-ABI fehlen |
| 2 | `I$MakDir` → Manager `MakDir` | 🔴 | Dispatch und Vertrag |
| 3 | `I$ChgDir` → Manager `ChgDir` | 🔴 | Dispatch und Vertrag |
| 4 | `I$Delete` → Manager `Delete` | 🔴 | Dispatch und Vertrag |
| 5 | `I$Seek` → Manager `Seek` | 🟡 | abstrakte Operation kann weitergereicht werden; ABI und Datenpfad fehlen |
| 6 | `I$Read` → Manager `Read` | 🟡 | abstrakte Operation kann weitergereicht werden; Datenkopie und Fehlersemantik fehlen |
| 7 | `I$Write` → Manager `Write` | 🟡 | abstrakte Operation kann weitergereicht werden; Datenkopie und Fehlersemantik fehlen |
| 8 | `I$ReadLn` → Manager `ReadLn` | 🟡 | abstrakte Operation kann weitergereicht werden; Line-I/O-Regeln fehlen |
| 9 | `I$WritLn` → Manager `WritLn` | 🟡 | abstrakte Operation kann weitergereicht werden; Line-I/O-Regeln fehlen |
| 10 | `I$GetStt` → Manager `GetStt` | 🟡 | abstrakte Operation kann weitergereicht werden; Status-ABI fehlt |
| 11 | `I$SetStt` → Manager `SetStt` | 🟡 | abstrakte Operation kann weitergereicht werden; Status-ABI fehlt |
| 12 | `I$Close` → Manager `Close` | 🟡 | abstraktes Schließen existiert; Modul-Dispatch und endgültige Fehler-/Freigaberegel fehlen |

Der aktuelle Hostcode transportiert eine Q9-eigene Operations-ID und drei
Werte an einen Funktionszeiger. Das ist **noch nicht** die verifizierte
68K-Manager-Sprungtabellen-ABI.

## 3. File-Manager → Treiber: Treibervector

Ein File-Manager wie RBF oder SCF nutzt den Geräte-Treiber für Hardware-
Operationen. Der genaue Aufrufer einzelner Eintrittspunkte (IOMan direkt
oder Manager) und das Q9-ABI müssen für jede Operation vertraglich
festgeschrieben werden. Die Tabelle führt alle Standardfunktionen, die Q9-
Treiber bereitstellen können.

| Slot | Treiberfunktion | Status | Noch nötig |
|---:|---|---|---|
| 0 | `Init` | 🔴 | Geräteinstanz initialisieren, Ressourcen/Storage-Vertrag und Fehler-Rollback |
| 1 | `Read` | 🔴 | Block-/Zeichen-Lesevertrag, Puffer, Länge, Fehler und Kurztransfer |
| 2 | `Write` | 🔴 | Schreibvertrag, Puffer, Länge, Schreibschutz und Fehler |
| 3 | `GetStat` | 🔴 | Statuscodes, Ausgabeformat und Puffergröße |
| 4 | `SetStat` | 🔴 | Statuscodes, Eingabeformat und Rechte |
| 5 | `Term` | 🔴 | offene Operationen stoppen und Ressourcen sicher freigeben |
| 6 | `Trap` | 🔴 | optionaler/geräteabhängiger Zusatzslot; genaue Q9-Semantik festlegen |

## 4. IOMan → Kernel: benötigte Kernel-Dienste

Diese Tabelle ist vom Abschnitt 1 zu unterscheiden: Dort **empfängt** IOMan
Kernel-Dispatches; hier ruft der IOMan selbst Kernel-Dienste auf. Die ersten
vier Dienste sind im bisherigen Referenz-Disassembly beobachtet. Weitere
Lifecycle-Dienste sind Implementierungsbedarf bzw. ABI-Prüfpunkte und müssen
vor dem Codegen gegen Q9 verifiziert werden.

| Kernel-Dienst | Zweck | Status | Nachweis / offener Punkt |
|---|---|---|---|
| `F$Link` | Descriptor, Treiber und File-Manager laden/verlinken | 🔴 | im Referenz-IOMan beobachtet; Q9-Aufrufadapter und Typ-/Register-ABI fehlen |
| `F$UnLink` | Teilweise oder vollständig gelinkte Module zurückgeben | 🔵 | für Rollback/Detach zu klären; konkrete Aufrufpfade und Q9-Semantik verifizieren |
| `F$SRqMem` | Tabellen, Queue-Elemente und ggf. I/O-Puffer anfordern | 🔴 | im Referenz-IOMan beobachtet; Größen-/Owner-/Lebensdauervertrag offen |
| `F$SRtMem` | Speicher wieder freigeben | 🔵 | abhängig von Allokationsmodell; Aufrufpfad/Größenvertrag bestätigen |
| `F$GProcP` | aktuellen Prozessdescriptor für Pfad-/Wait-Verwaltung holen | 🔴 | im Referenzpfad beobachtet; Q9-ABI-/Kontextprüfung fehlt |
| `F$Send` | wartende/konkurrierende Pfadoperationen wecken/signalisieren | 🔴 | im Referenzpfad beobachtet; Signal- und Retry-Protokoll implementieren |
| `F$SSvc` | Q9-seitige Registrierung externer Manager-Handler | 🔵 | Integrationsvertrag zwischen Boot/Kernel/IOMan festlegen; nicht als sicherer IOMan-Eigenaufruf voraussetzen |
| `F$FindPD` / Pfad-API | Prozesspfade auflösen bzw. Tabellen anbinden | 🔵 | entscheiden, ob IOMan Kernelservice oder eigene Tabelle verwendet; Registervertrag prüfen |

### QCC-Aufrufweg

| Arbeit | Status | Hinweis |
|---|---|---|
| `modul syscall(callcode, archetype)` für passende Aufrufe einsetzen | 🔵 | QCC-Syntax/Design liegt vor, aber Codegen und End-to-End-Verifikation sind noch offen; aktuell nicht in Q9-IOMAN verwendbar |
| mehrere Register-Rückgaben / Sonder-ABI abbilden | 🔴 | C-Rückgabewert und `CALL_D`/`CALL_DA` reichen nicht automatisch für alle Dienste; Adaptervertrag nötig |
| Trap-Kontext, Carry und `d1`-Fehler korrekt transportieren | 🔴 | pro Dienst festlegen und mit generiertem 68K-Code testen |

## 5. Weitere zwingende IOMan-Subsysteme

| Subsystem/Funktion | Status | Fertigstellungskriterien |
|---|---|---|
| Modulstart/Systemzustand | 🟢 | Q9-Systemmodul-Entry startet den idempotenten C-Zustand; Fehlerpfad, Image-Integration und exceptionfreier Emulatorstart geprüft; temporärer reentrant Open-Selbsttest kehrte zurück |
| Kernel-Service-Registrierung | 🟢 | Watchpoint bestätigt `F$SSvc`-Schattenadressen für `I$Open/Read/Close`; kontrollierter nicht-nativer `I$Open` erreicht den externen Handler und kehrt zurück; `D1`-/Carry-Fehlerwert noch separat auszulesen |
| Register-/Pfadadapter für I$-Handler | 🟡 | Open/Read/Close-Dispatcher und Assembly-Schatteneinträge sind Q9-toolchain-kompiliert/verlinkt; Hosttests prüfen Ergebnis-/Carry-Mapping, Open→`E$MNF`, fehlenden Manager, `I$Read`-Kurzlesung, übergroße Transfermeldung und Null-/umlaufende Bufferbereiche; Emulator belegt Open-Aufruf/Rückkehr, Read/Close-Runtime und Read-Buffer-Speicherschutz fehlen; Pfadresolver begrenzt Scan auf 256 Byte, kann ungültige Zeiger nicht abfangen |
| Zielübersetzung und Modulbuild | 🟢 | Q9-eigene Kette erzeugt ein gültiges `qioman`-OS-9-Modul; CRC/Parität, Bootkettenaufnahme und exceptionfreier Emulatorlauf verifiziert |
| Device-Descriptor lesen/parsen/validieren | 🔴 | Name, Typ, Treiber-/Managerreferenzen und Größen/Attribute sicher validiert |
| Device-Descriptor finden, linken und validieren | 🔴 | Name, Typfilter, Edition, Größe und Datenfelder prüfen |
| Im Descriptor referenzierten Treiber finden/linken | 🔴 | Modulname/Typ verifizieren, Linklebensdauer und Rollback testen |
| Im Descriptor referenzierten File-Manager finden/linken | 🔴 | Modulname/Typ verifizieren, Linklebensdauer und Rollback testen |
| Treiber-/Manager-Funktionsvektoren auflösen und eintragen | 🔴 | Einsprungbasis, Slotreihenfolge, relative Offsets, Nullslots und Register-ABI prüfen |
| Attach-Transaktion und Rückabwicklung | 🟡 | Zustandsmodell/Atomaritäts- und Rollbackinvarianten in `docs/IO_PROTOCOL_SPEC.md` entworfen; Link-/Init-Sequenz und Fehlerpfade noch nicht implementiert |
| Device-Tabelle und Attach-Referenzen | 🟡 | `DETACHED/PREPARING/READY/QUIESCING`-Modell dokumentiert; echte Device-Tabelle, parallele Übergänge, Mehrfach-Attach, Referenzen und Detach noch offen |
| Gerätename/Pfadprefix parsen und Manager auswählen | 🟡 | Backendregister und längster Präfixtreffer mit Trennergrenze hostgetestet; Descriptor-/Attach-Auflösung und Pfadrestübergabe fehlen |
| lokale Pfadnummern und Backendpfade verwalten | 🟡 | caller-owned Tabelle und lokale→Backend-Pfadbindung vorhanden; Prozessdescriptor-/Kernelintegration und Konkurrenzschutz fehlen |
| `Dup`-/Close-Referenzlebenszyklus | 🔴 | Duplikate und Backendfreigabe korrekt bis zum letzten Nutzer; Kernel-`I$Dup`-Benachrichtigung und Prozessende-Cleanup fehlen |
| Read-/Write-Modus und Zugriffsrechte | 🟡 | Q9-I$Open-Datenbits werden beim Open gespeichert; READ/READLN und WRITE/WRITLN werden vor Backenddispatch geprüft; Status-/Seek-Rechte und endgültige syscall-Fehlersemantik offen |
| Pfad-Lock, Wait/Wake und Wiederaufnahme | 🔴 | konkurrierende Zugriffe serialisieren, Prozessende/Signal/Fehler sicher behandeln |
| I/O-Queue und asynchrone Anfragen | 🔴 | Einreihen, Abbrechen, Abschluss, Wake-up und Ressourcenbesitz spezifizieren |
| Aufruflokaler Kommando-Deskriptor | 🟡 | Kommando und nur die benötigten Parameter liegen auf dem Stack des Managers/Treibers; genaue Struktur und Kommando-Arity gemäß `docs/IO_PROTOCOL_SPEC.md` noch festzulegen; der IOMan besitzt keinen geteilten Kommando-Puffer |
| Bufferzeiger- und Längenvertrag | 🟡 | Zero-copy-Grundsatz festgelegt: Namen-/Pfad- und Datenbuffer gehören dem Aufrufer und werden vom IOMan nur als Zeiger weitergereicht; Kommando-Längen, Adressraum-/Reichweitenprüfung und asynchrone Lebensdauer sind noch zu spezifizieren; keine IOMan-eigenen Datenbuffer/Kopien |
| `ReadLn`/`WritLn`-Puffer und Zeilenregeln | 🔴 | Terminator, Pufferende, Blocking und Teilzeilen definieren |
| Fehler-/Rückgabemapping | 🔴 | Carry, Fehlercode, Rückgaberegister und partielle Transfers vereinheitlichen |
| Prozessende-/Fehler-Cleanup | 🔴 | offene Pfade, Locks, Queueelemente, Buffer und Modulreferenzen freigeben |
| `F$Load`-Suchpfad und Storage-Fallback | 🔴 | resident modules, Pfadlisten und Plattenzugriff in festgelegter Reihenfolge |
| `F$PErr` Fehlerausgabe | 🔴 | robust auch ohne verfügbaren Standardpfad/Terminal |
| Bitmap-/IO-Queue-F$-Services | 🔴 | `F$SchBit/AllBit/DelBit/IOQu/IODel` als eigener Kernel-Dispatchvertrag |
| SCF-Integration | 🔴 | Terminal-Open/Close/Read/Write/Status end-to-end |
| RBF-Integration | 🔴 | CF-Open/Read/Seek/Close; danach Verzeichnis, Schreiben und Mutationen |
| Reentranz-/Parallelitätstests | 🟡 | Hosttests decken verschachtelten Open-Callback, Close während aktivem Backendaufruf und rekursives Close ab; echte präemptive Parallelität/Kernel-Locks und Mehrprozesspfade fehlen |
| Emulator- und Image-Tests | 🔴 | boot, Shell-I/O, reale Dateioperationen, Fehler-/Rollbackfälle reproduzierbar |

## 6. Bereits implementiert (eng abgegrenzt)

| Teil | Status | Testbeleg |
|---|---|---|
| Caller-owned Pfadtabelle initialisieren | 🟢 | Hosttest prüft Nullinitialisierung/Managerbindung im lokalen Scope |
| Backendpräfixe registrieren und auflösen | 🟢 | Hosttest prüft Registrierung, Duplikat, längsten Treffer und `/dd` vs. `/ddx`; keine Modul-/Descriptorbindung |
| Backendpräfix sicher lösen (Detach-Grundlage) | 🟢 | Hosttest prüft Busy bei aktivem Pfad, erfolgreiches Lösen nach Close und anschließendes Not-Found; noch keine Descriptor-/Modulreferenzfreigabe |
| Kernel-Rahmen-Feldzugriffe | 🟢 | Hosttests prüfen D0/A0 big-endian 32-bit sowie SR/PC 16-bit; keine syscall-spezifische Adapterlogik |
| Kernel-Request-Decoder für I/O | 🟡 | Hosttests prüfen Registerfelder aller 13 Callcodes `$83`–`$8F`; `ChgDir` wird decodiert, aber nicht an ein Dateisystembackend delegiert |
| Managerstatus → Q9-Kernel-Fehler | 🟡 | Grundzuordnung für Parameter, Pfadnummer, falschen Modus, volle Tabelle, nicht gefunden und nicht unterstützt implementiert/getestet; pro I$-Aufruf und Backend noch semantisch zu bestätigen |
| Manager-/Treiber-/Emulator-Kommunikationsspezifikation | 🟡 | Entwurf in `docs/IO_PROTOCOL_SPEC.md`; 13 Managerkommandos ihren Kernel-Callcodes `$83`–`$8F` zugeordnet, aber getrennt von Entwurfs-IDs und Manager-Vektorslots; Vorschlag für aufruflokalen, kommandoabhängig langen Block ohne unnötige Parameter; genaue Arity, Register-/SetStat-Transport sowie Init-/Destroy-Eigentümer offen |
| Kernel-I/O-Registerinventur | 🟡 | Eingabe-/Rückgaberegister und aktuelle Implementierungsabdeckung für alle 13 I$-Callcodes aus Q9-Kernelcode inventarisiert (`docs/KERNEL_IO_ABI.md`); nicht implementierte Calls und fehlende Voll-ABI bleiben offen |
| Residenten Systemzustand initialisieren | 🟢 | Hosttests prüfen Zugriff vor Start, Tabellenkapazität und wiederholten Start |
| Backend-Open und lokale Slotvergabe | 🟡 | Hosttest erfolgreich; nur bei bereits ausgewähltem Backend, keine Deviceauflösung |
| Open-Rollback und synchrone Callback-Reentranz | 🟢 | Backendfehler gibt reservierten Slot frei; reentrant Open bekommt anderen Slot; Detach bleibt während OPENING gesperrt; parallele präemptive Aufrufe sind nicht abgedeckt |
| Pfadzugriffsmodus prüfen | 🟡 | Q9-Open-Modus wird gespeichert; READ/READLN und WRITE/WRITLN geprüft, Modus 0 als Read+Write getestet; weitere Modusbits und OS-Fehlervertrag offen |
| generische Operation an Backendpfad weiterleiten | 🟡 | Hosttest prüft READ und Argumente; kein Trap-/68K-Dispatch |
| Create- und namensbasierte Backendaufrufe | 🟡 | `Create`-Callback reserviert lokalen Pfad; `MakDir`/`Delete` werden über längsten Backendpräfixtreffer geroutet; keine Managervektorbindung |
| Backend-Close und lokales Freigeben | 🟡 | Hosttest prüft Erfolg sowie Erhalt des Pfads bei Backendfehler; OS-9-Semantik noch zu bestätigen |
| Host-Regressionstest-Suite | 🟢 | `make test` besteht; deckt ausschließlich den aktuellen Q9-eigenen Routerkern ab |

## 7. Reihenfolge zum lauffähigen System

1. Kernel-/QCC-ABI für Aufruf und Rückgabe festlegen; direkte QCC-Syscalls
   erst nutzen, wenn Codegen verifiziert ist.
2. IOMan-Systemstart, Device-Descriptor, Attach/Detach und Managerbindung.
3. Minimaler Kernel-`I$Open/Read/Write/Close`-Adapter plus Q9-SCF-Terminal.
4. RBF-/CF-Pfad `Open → Read → Seek → Close` mit echtem Image testen.
5. Pfadduplikate, Status, Line-I/O, Locks/Queues, `Create/Delete/MakDir`.
6. `F$Load`, PErr, alle Bitmap-/Queue-Dienste und erweiterte Fehler-/Cleanup-
   Fälle.

## 8. Zählstand

- Kernel-Eingänge: **24** (7 `F$` + 17 `I$`).
- File-Manager-Dispatchslots: **13**.
- Treibervector-Slots: **7**.
- Aktuell für Q9 implementierte OS-9-Eingänge: **0 vollständig**.
- Teilweise vorhandener, host-getesteter Managerkern: lokale Pfadslots,
  Backendregistrierung/Präfixauflösung, Open/Dispatch/Close und
  Registerrahmen-Zugriffshilfen.
