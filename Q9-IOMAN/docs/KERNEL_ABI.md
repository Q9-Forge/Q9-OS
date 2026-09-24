# Kerneladapter und QCC-Syscalls

Der Q9-IOMAN-Kern soll keine Trap-Instruktionen verstreut in der
Managerlogik enthalten. Ein späterer `src/`-Adapter kapselt die Dienste, die
für Modul-Linking, Speicher, Pfad-/Prozessverwaltung und gegebenenfalls
Service-Registrierung erforderlich sind.

QCC dokumentiert die Deklarationsform:

```c
extern modul syscall(0xNN, CALL_DA) int32_t q9_kernel_call(...);
```

Diese Form ist zum jetzigen Stand Spezifikation, nicht bestätigte
Compilerfunktion: `modul syscall` steht in Q9-QCC Phase 7 auf offen. Daher
keine solche Deklaration in produktivem Q9-IOMAN-Code verwenden, bis Frontend,
IR, 68K-Backend und ein erzeugtes/ausgeführtes Modul durch Regressionstests
bestätigt sind.

Wenn verfügbar, soll Q9-IOMAN direkte Compiler-Syscalls für passende
Register-Archetypen nutzen. Das reduziert Wrapper-Boilerplate und kann einen
zusätzlichen Wrapper-Aufruf einsparen; es beschleunigt nicht den eigentlichen
Kernel-Dispatcher oder `TRAP #0`. Dienste mit mehreren Rückgaberegistern,
Sonderrahmen oder nicht abbildbarer Registerbelegung brauchen weiterhin
einen schmalen ABI-Adapter bzw. eine gezielte Assemblerbrücke.

Vor der Auswahl jedes Callcodes müssen Eingaberegister, Rückgaberegister,
Carry-/Fehlerkonvention, Prozess-/Supervisor-Kontext und Seiteneffekte aus
dem Q9-Kernelvertrag dokumentiert und durch Tests abgesichert sein.

`Q9IOMAN_u32` ist als `unsigned int` festgelegt: auf dem 32-bit-Q9-Ziel und
dem Host-Testsystem bildet der Typ damit einen Registerwert ab. `unsigned long`
wäre auf LP64-Hosts 64 Bit breit und darf deshalb nicht als vermeintlich
portabler 32-bit-ABI-Typ verwendet werden. Der Hosttest prüft die Größe
explizit; der Q9-Build prüft die Typdeklaration im Zielcompiler.

## Aktueller 68K-Registrierungseinstieg

`68k/qioman_entry.a` stellt `M$Exec` eines Q9-Systemmoduls namens `ioman`
bereit. Der Q9-Bootpfad linkt `ioman`, setzt für den Manageraufruf `A4` auf
den aktuellen Prozessdescriptor und `A6` auf null und ruft anschließend
`M$Exec` auf. Der Einstieg initialisiert den residenten C-Managerzustand und
ruft dann `F$SSvc` mit einer Tabelle für `I$Open` (`$84`), `I$Read` (`$89`)
und `I$Close` (`$8F`) auf. Pro Tabellenzeile ist der Routineoffset relativ
zum Codewort und um vier Byte korrigiert; `A3` zeigt auf die per-Service-
Datenbasis. Das entspricht dem implementierten Q9-`F$SSvc`-ABI.

Der Kernel behandelt diese drei Callcodes besonders: Er behält seine nativen
Dispatch-Einträge und speichert die IOMan-Routinen als Manager-Schatten.
`I$Open` wird für nicht-native Pfade an den Manager delegiert; `I$Read` und
`I$Close` nur für Pfade, die ein erfolgreicher Manager-Open markiert hat.
Bis die Registerrahmen-Adapter fertig sind, zeigen alle drei Einträge auf
einen sicheren Stub, der `E$UnkSvc` (`$D0`) mit gesetztem Carry liefert.
Damit kann der unfertige Manager keine Pfade übernehmen. Nächster Schritt:
Die Registerrahmen-Offsets und Big-Endian-Zugriffe sind nun in
`qioman_kernel.h`/`qioman_kernel.c` gekapselt und auf dem Host getestet.
Das ist noch kein syscall-spezifischer Handler: insbesondere fehlen die
validierte Prozess-Pfadslot-Abbildung, Zugriffsschutz und Carry-/Fehler-
Rückgabe.

`q9ioman_status_to_os9_error()` stellt eine erste gemeinsame Übersetzung
dieser internen Statuswerte bereit: ungültiger Parameter→`E$Param`, ungültiger
Pfadslot→`E$BPNum`, falscher Read-/Write-Modus→`E$BMODE`, erschöpfte
Pfad-/Registrierungsslots→`E$PthFul`, kein
Präfixtreffer→`E$MNF`, nicht unterstützte Operation→`E$UnkSvc`. Die Zuordnung
ist host- und Q9-toolchain-getestet, aber noch kein Ersatz für die spätere
syscall-spezifische Fehlersemantik; Backendfehler brauchen eine explizite
Weitergabe/Übersetzung. Der interne Busy-Status besitzt noch kein bestätigtes
Q9-Systemaufruf-Mapping.

## Backend-Auflösung und offene Lebensdauerfrage

Der Managerkern kann Backends unter Pfadpräfixen registrieren und wählt den
längsten Treffer, sofern das Präfix an einem Pfadtrenner endet (`/dd` passt
auf `/dd/file`, nicht auf `/ddx/file`). Die Registrierung speichert geliehene
Zeiger; Prefix und Ops-Tabelle müssen resident bleiben. Das ersetzt weder
Descriptor-/Modullinking noch Attach.

Noch keine Handler auf diese Routerlogik schalten: Kernel-`I$Dup` läuft
aktuell nativ und benachrichtigt den Manager nicht; außerdem ist kein
Prozessende-Cleanup eingebunden. Ohne diese Lebensdauerpfade kann der Manager
Backendhandles zu früh oder gar nicht freigeben.
