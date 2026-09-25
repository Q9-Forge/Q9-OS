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
Die drei Einträge rufen jetzt über 68K-Stubs `tc_q9ioman_target_dispatch`
auf. `A5` wird als 72-Byte-Serviceframe übergeben; der jeweilige Callcode ist
im Stub festgelegt. Nach dem C-Aufruf werden D0/D1/A0 aus dem Frame
wiederhergestellt und Carry passend gesetzt. `I$Open` nutzt einen begrenzten
Pfadresolver (max. 256 Byte); in Q9s flachem Adressraum kann er ungültige oder
nicht gemappte Speicherbereiche nicht abfangen. Die Aufrufe von den
Assembler-Stubs in die QCC-C-Psects müssen PC-relativ (`BSR`) erfolgen: ein
absolutes `JSR` sprang im relocierbaren Modul auf den unverschobenen Psect-
Offset statt auf die geladene Moduladresse. Der Emulator bestätigt den
IOMan-Start, die drei `F$SSvc`-Schattenadressen und einen reentranten
nicht-nativen `I$Open`-Aufruf bis zur Handler-Rückkehr. Ein echter Backend-
Open/Read/Close-Pfad und dessen konkrete Fehlerregister sind noch nicht
end-to-end verifiziert.

**QCC-68K-Stackkonvention für Assemblerbrücken:** Bei einem C-Aufruf mit
`(callcode, frame)` erwartet der QCC-Einstieg den letzten Parameter `frame`
bei `8(a5)` und den ersten Parameter `callcode` bei `12(a5)`. Der Assembler
muss daher erst den ersten und danach den letzten Parameter pushen. Der
Resolver-Callback `(context, address, path, length)` liest seine Argumente
entsprechend bei `20/16/12/8(a5)`. Abweichende Reihenfolge vertauscht
Argumente, obwohl C-Code und Modul-Link sauber aussehen. Die IOMan-Brücken
wurden nach einem Emulatorlauf entsprechend korrigiert.

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

Die Kernel-Schattenhandler verwenden inzwischen diese Routerlogik. Die
residenten Pfadslots sind derzeit jedoch global statt pro Prozess; Kernel-
`I$Dup` läuft nativ und benachrichtigt den Manager nicht, und Prozessende-
Cleanup ist nicht eingebunden. Vor Mehrprozess-/Dup-Nutzung muss die
Ownership-/Referenzabbildung geklärt werden, sonst könnten Handles falsch
zugeordnet oder zu früh freigegeben werden.
