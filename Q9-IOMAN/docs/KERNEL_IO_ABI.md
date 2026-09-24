# Q9-Kernel I/O-Aufrufinventur für Q9-IOMAN

**Zweck:** Festhalten, was der aktuelle Q9-Kernel für die 13 I/O-Callcodes
implementiert. Dies beschreibt den lokalen Kernelstand, nicht automatisch eine
vollständige OS-9-Referenz-ABI und nicht den neuen Manager→Treiber→Simulator-
Kommandovertrag.

## Register- und Dispatchübersicht

Notation: `D0`–`D7` sind Datenregister, `A0`–`A6` Adressregister. „Bekannt“
bedeutet im aktuellen Q9-Assemblercode ausdrücklich kommentiert oder direkt
benutzt. „Nicht implementiert“ bedeutet, dass in der untersuchten
Kernelinitialisierung kein nativer Handler für den Callcode installiert ist;
es behauptet nicht, dass ein späterer externer Handler unmöglich wäre.

| Callcode | Kernelaufruf | Im Q9-Kernel feststellbare Eingabe | Feststellbare Ausgabe / Verhalten | Aktueller Stand |
|---:|---|---|---|---|
| `$83` | I$Create | Nicht im aktuellen nativen Handlerbestand belegt | Nicht belegt | Kein nativer Handler in der untersuchten Initialisierung gefunden |
| `$84` | I$Open | `D0.b` Zugriffsmodus, `A0` Pfadnamenzeiger | Erfolg: `D0.w` Pfadnummer, `A0` hinter den Namen; Fehler: Carry + `D1.w` Fehler | Q9-Kernel akzeptiert Modusbits gemäß Maske `$D7`; Bit 0 Lesen, Bit 1 Schreiben, Modus 0 wird auf Lesen+Schreiben normalisiert. Nativ für Q9-Pfade; zusätzlich selektiver IOMan-Schattenpfad für nicht-native Pfade, momentan Stub |
| `$85` | I$MakDir | Nicht belegt | Nicht belegt | Kein nativer Handler in der untersuchten Initialisierung gefunden |
| `$86` | I$ChgDir | `D0.w` Selektor (`1` Execution-, `3` Data-Directory), `A0` NUL-terminierter Name | Speichert den Namen im Prozessdescriptor; Fehler über Carry/`D1.w` | Native, begrenzte Prozessverzeichnisfunktion; kein vollständiger Managerdispatch |
| `$87` | I$Delete | Nicht belegt | Nicht belegt | Kein nativer Handler in der untersuchten Initialisierung gefunden |
| `$88` | I$Seek | `D0.w` lokaler Pfad, `D1.l` absolute Byteposition | Native Konsolenpfade speichern logische Position; Fehler Carry + `D1.w` | Begrenzte native Pfadbuchführung; kein IOMan-Schattenhandler |
| `$89` | I$Read | `D0.w` lokaler Pfad, `D1.l` angeforderte Bytezahl, `A0` Zielbuffer | Erfolg gibt `D1` als gelesene/angeforderte Länge zurück; natives Terminal kann blockieren; Fehler Carry + `D1.w` | Native Minimalpfade plus Manager-Schattenroute nur für markierte IOMan-Pfade, derzeit Stub |
| `$8A` | I$Write | `D0.w` lokaler Pfad, `D1.l` Bytezahl, `A0` Quellbuffer | Native Konsolenausgabe schreibt die angegebene Länge; Fehler Carry + `D1.w` | Native Minimalfunktion; kein IOMan-Schattenhandler |
| `$8B` | I$ReadLn | `D0.w` lokaler Pfad, `D1.l` Bufferkapazität, `A0` Zielbuffer | `D1` Anzahl gespeicherter Bytes inkl. CR; stoppt bei CR oder Kapazitätsende | Native Minimalfunktion; kein IOMan-Schattenhandler |
| `$8C` | I$WritLn | `D0.w` lokaler Pfad, `D1.l` Länge, `A0` Quellbuffer | `D1` geschriebene Bytes inkl. CR; stoppt bei CR oder Längenende | Native Minimalfunktion; kein IOMan-Schattenhandler |
| `$8D` | I$GetStt | `D0.w` Pfad, `D1.w` Statuscode, `A0` Ausgabezeiger | Implementierte native Fälle: Optionsdaten (32 Byte), Position (Long) und Ready-Abfrage | Begrenzte native Statusfälle; kein IOMan-Schattenhandler |
| `$8E` | I$SetStt | `D0.w` Pfad, `D1.w` Statuscode, `A0` Eingabezeiger | Implementiert ist derzeit SS_Opt mit 32 Byte; unbekannter Status liefert Fehler | Begrenzte native Statusfunktion; kein IOMan-Schattenhandler |
| `$8F` | I$Close | `D0.w` lokaler Pfad | Entfernt nativen Prozesspfad/Referenz; Fehler Carry + `D1.w` | Native Q9-Pfade plus Manager-Schattenroute für markierte IOMan-Pfade, derzeit Stub |

## Wichtige Abgrenzungen

1. Die Callcodes `$83`–`$8F` sind Kernel-I/O-Aufrufe. Der Kernel gibt daraus
   nicht automatisch die Parameter des neuen IOMan-Managerkommandos vor.
2. Die 1–5 Parameter je Managerkommando und deren Richtung werden separat
   spezifiziert. Eine Übersetzung Kernelregister→Managerkommando muss je
   Aufruf explizit erfolgen.
3. Die Parameterangaben oben beschreiben die aktuellen Q9-Handler. Bei nicht
   implementierten Calls oder Registerwerten, die der Code nicht prüft,
   bleibt der ABI-Teil offen.
4. Kernel-Schattenregistrierung ist im aktuellen Q9-Code nur für `I$Open`,
   `I$Read` und `I$Close` vorgesehen. `I$SetStt` ist aktuell kein IOMan-
   Schattenhandler; der native Pfad deckt nur den begrenzten `SS_Opt`-Fall ab.
5. Ein Pointer in `A0` ist ein Registerwert, kein Buffer-Copy. Dass ein
   Emulator-/Hardware-Simulator eine Stackadresse des Managers/Treibers
   dereferenzieren kann und dies synchron tut, muss im darunterliegenden
   Kommandovertrag bestätigt werden.
6. Die tatsächlichen Kernelhandler behalten teils Eingaberegister oder
   verwenden `D1` sowohl als Eingabe als auch als Fehler-/Längenrückgabe. Der
   IOManadapter muss daher vor Änderung eines Registers die syscall-spezifische
   Rückgabesemantik beachten.

## Lokale Q9-Quellen

Diese Inventur basiert auf dem Q9-Repository, insbesondere:

- `Q9-KERNEL/68k/src/kernel/q9kernel_entry.a`: Handler und kommentierte
  Registerverträge; relevante Abschnitte `Q9K_SysFIWritLn` bis
  `Q9K_SysFISetStt`, danach `Q9K_SysFISeek`, `Q9K_SysFIDup`,
  `Q9K_SysFIClose`, `Q9K_SysFIChgDir` und `Q9K_SysFIOpen`.
- `Q9-KERNEL/68k/src/kernel/q9kernel_cinit.c`: Callcode→nativer Handler
  Installation.
- `Q9-KERNEL/68k/src/kernel/q9kernel_ssvc.c`: Kernel-eigene Callcodes und
  die drei gesondert gespeicherten IOMan-Manager-Schattenhandler.

Die Dateien werden hier nur als lokale Q9-ABI-Quelle benannt. Diese
Dokumentation erweitert oder ändert sie nicht.
