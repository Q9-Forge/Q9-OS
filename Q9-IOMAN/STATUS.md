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

**Wichtig:** Die vorhandene Host-Routerlogik ist kein laufendes OS-9-IOMan-
Modul. Trap-Adapter, Modullader, Attach, SCF/RBF und Emulatorintegration
fehlen weiterhin.

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
| K-ICREATE | `I$Create` | Datei/Objekt anlegen und öffnen | 🔴 | Manager auswählen, Create-Semantik, Pfadslot und Rechte |
| K-IOPEN | `I$Open` | Pfad öffnen und lokale Pfadnummer vergeben | 🟡 | Hostkern kann mit vorgewähltem Backend öffnen und lokalen Slot vergeben; Kernel-Trap, Namens-/Deviceauflösung, Attach und Rechteprüfung fehlen |
| K-IMAKDIR | `I$MakDir` | Verzeichnis anlegen | 🔴 | Pfad-/Managerauflösung, Backend-Aufruf und Cleanup |
| K-ICHGDIR | `I$ChgDir` | Prozess-Arbeitsverzeichnis ändern | 🔴 | Pfadtyp prüfen, Prozessdescriptor aktualisieren und Fehlervertrag |
| K-IDEL | `I$Delete` | Datei/Verzeichnis löschen | 🔴 | Zielauflösung, Rechte/Typ, Manageroperation und Fehlerfälle |
| K-ISEEK | `I$Seek` | Position eines offenen Pfades ändern | 🟡 | generischer Backend-Dispatch vorhanden; OS-9-Parameter, Position/Range und Trap-ABI fehlen |
| K-IREAD | `I$Read` | Bytes lesen | 🟡 | generischer Backend-Dispatch vorhanden; Kernelpufferprüfung/-kopie, Kurzread, EOF und Fehlervertrag fehlen |
| K-IWRITE | `I$Write` | Bytes schreiben | 🟡 | generischer Backend-Dispatch vorhanden; Pufferprüfung/-kopie, Rechte, Kurzwrite und Fehlervertrag fehlen |
| K-IREADLN | `I$ReadLn` | zeilenorientiert lesen | 🟡 | generischer Backend-Dispatch vorhanden; Zeilen-/Puffersemantik, SCF und Grenzfälle fehlen |
| K-IWRITLN | `I$WritLn` | zeilenorientiert schreiben | 🟡 | generischer Backend-Dispatch vorhanden; Terminator-/Puffersemantik und SCF fehlen |
| K-IGETSTT | `I$GetStt` | Manager-/Gerätestatus abfragen | 🟡 | generischer Backend-Dispatch vorhanden; Statuscode-ABI, Ausgabevalidierung und Backendabdeckung fehlen |
| K-ISETSTT | `I$SetStt` | Manager-/Gerätestatus setzen | 🟡 | generischer Backend-Dispatch vorhanden; Eingabevalidierung, Rechte und Statuscodevertrag fehlen |
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
| Modulstart/Systemzustand | 🟡 | Q9-Systemmodul-Entry startet den idempotenten C-Zustand; Fehlerpfad vorhanden, Boot-/Image-Integration und Laufzeittest fehlen |
| Kernel-Service-Registrierung | 🟡 | Systemmodul namens `ioman` initialisiert C-Zustand und registriert `I$Open/Read/Close` per F$SSvc; Q9-Kernel-Schattenpfad ist unit-getestet, Emulator-/Bootintegration fehlt |
| Register-/Pfadadapter für I$-Handler | 🟡 | 72-Byte-Framezugriff und Decoder für die registrierten `I$Open/Read/Close`-Layouts hostgetestet und Q9-toolchain-kompiliert; Handlerdispatch, Pointer-Adressraum, Ergebnisregister, Fehler-/Carry-Vertrag und Pfadslotabbildung fehlen |
| Zielübersetzung und Modulbuild | 🟢 | Q9-eigene Kette erzeugt aus C+68K-Glue das `qioman`-OS-9-Modul; noch kein Emulator-/Kernel-Integrationstest |
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
| Kernel-Request-Decoder für Schatten-I/O | 🟢 | Hosttests prüfen Open-Modus/Pathpointer, Read-Pfad/Länge/Buffer, Close-Pfad, Nullargumente und unbekannten Callcode; keine Dereferenzierung und kein Trap-Dispatch |
| Managerstatus → Q9-Kernel-Fehler | 🟡 | Grundzuordnung für Parameter, Pfadnummer, falschen Modus, volle Tabelle, nicht gefunden und nicht unterstützt implementiert/getestet; pro I$-Aufruf und Backend noch semantisch zu bestätigen |
| Manager-/Treiber-/Emulator-Kommunikationsspezifikation | 🟡 | Entwurf in `docs/IO_PROTOCOL_SPEC.md`; 13 Managerkommandos ihren Kernel-Callcodes `$83`–`$8F` zugeordnet, aber getrennt von Entwurfs-IDs und Manager-Vektorslots; Vorschlag für aufruflokalen, kommandoabhängig langen Block ohne unnötige Parameter; genaue Arity, Register-/SetStat-Transport sowie Init-/Destroy-Eigentümer offen |
| Kernel-I/O-Registerinventur | 🟡 | Eingabe-/Rückgaberegister und aktuelle Implementierungsabdeckung für alle 13 I$-Callcodes aus Q9-Kernelcode inventarisiert (`docs/KERNEL_IO_ABI.md`); nicht implementierte Calls und fehlende Voll-ABI bleiben offen |
| Residenten Systemzustand initialisieren | 🟢 | Hosttests prüfen Zugriff vor Start, Tabellenkapazität und wiederholten Start |
| Backend-Open und lokale Slotvergabe | 🟡 | Hosttest erfolgreich; nur bei bereits ausgewähltem Backend, keine Deviceauflösung |
| Open-Rollback und synchrone Callback-Reentranz | 🟢 | Backendfehler gibt reservierten Slot frei; reentrant Open bekommt anderen Slot; Detach bleibt während OPENING gesperrt; parallele präemptive Aufrufe sind nicht abgedeckt |
| Pfadzugriffsmodus prüfen | 🟡 | Q9-Open-Modus wird gespeichert; READ/READLN und WRITE/WRITLN geprüft, Modus 0 als Read+Write getestet; weitere Modusbits und OS-Fehlervertrag offen |
| generische Operation an Backendpfad weiterleiten | 🟡 | Hosttest prüft READ und Argumente; kein Trap-/68K-Dispatch |
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
