# Q9-IOMAN / Manager / Treiber – Protokollspezifikation (Entwurf)

**Status:** Diskussionsentwurf, noch kein festgeschriebener ABI-Vertrag

**Zweck:** Architektur und offene Entscheidungen so festhalten, dass Manager,
Treiber, IOMan und später der Hardware-Simulator dieselbe Schnittstelle
implementieren können. Dies ist Q9-eigener Entwurfscode/-vertrag und keine
Microware-ABI-Spezifikation.

## 1. Festgehaltene Leitlinien

- Der IOMan vermittelt Kernel-I/O-Aufrufe an den zuständigen File-Manager;
  Dateisystem- und gerätespezifische Semantik bleibt beim Manager bzw. Treiber.
- Die Manager-Schnittstelle umfasst 13 Aufrufe (Tabelle unten).
- Ein Manageraufruf hat je nach Kommando 1 bis 5 Parameter. Die genaue
  Belegung, Breite und Richtung ist noch pro Kommando festzulegen.
- Für einen Aufruf wird ein kleiner, aufruflokaler Kommando-Deskriptor
  verwendet. Kein global geteilter, während der Bearbeitung veränderlicher
  Kommando-Puffer: parallele/reentrante Aufrufe dürfen sich nicht
  überschreiben.
- Der Deskriptor enthält Kommando und skalare Parameter sowie bei Bedarf
  Zeiger und Längen. Datei-/Pfadnamen und Nutzdaten werden nicht in IOMan-
  Buffern dupliziert; ihre Buffer kommen vom Aufrufer (Manager/Treiber) und
  werden als Zeiger weitergereicht.
- Vorläufig gilt synchrone Bearbeitung: der Aufruf kehrt erst zurück, wenn
  Empfänger/Simulator die referenzierten Daten nicht mehr benötigt. Buffer
  und Deskriptor müssen bis dahin gültig bleiben; Adressen dürfen danach
  nicht gespeichert oder erneut verwendet werden.
- Bufferzeiger werden – wo der konkrete Aufrufvertrag es vorsieht – in
  Registern wie A0/A1 übergeben. Welche Register für welchen Aufruf gelten,
  ist noch genau zu spezifizieren.
- Kommandoabhängige Längen werden nur für die jeweils verwendeten Buffer
  gesetzt und geprüft. Es gibt keine pauschale feste Datenbufferlänge.
- Der Descriptor enthält mindestens Basispfad und Descriptor-/Gerätenamen;
  Remote-Adresse und Port sind optionale Descriptorfelder. Beim Init werden
  daraus Geräte-/Laufwerksname und Basispfad für die nachfolgende Auflösung
  eingerichtet.

## 2. Kommando-Deskriptor (Vorschlag, ABI offen)

Als gemeinsame logische Form wird zunächst vorgeschlagen:

```c
struct Q9IO_Command {
    LONG command;       /* Kommando-ID */
    LONG param_count;   /* 1..5; genaue Regel pro Kommando */
    LONG param[5];      /* Skalare oder 32-bit Adressen, kommandoabhängig */
};
```

Dies ist ein Diskussionsmodell, **keine bereits beschlossene C-Struktur**.
Noch zu entscheiden sind: 16-/32-bit Breiten, Vorzeichen, Alignment,
Byteordnung, tatsächliche Übergabe (Register oder Zeiger auf den Block),
Ergebnis-/Fehlerfelder und ob `param_count` überflüssig ist, wenn jede
Kommando-ID ihre feste Form hat. Ein kleiner fester Maximalblock vereinfacht
die ABI; unbenutzte Parameter müssen dann ignoriert werden. Alternativ kann
ein kompakter kommandoabhängiger Block mit expliziter Größe verwendet werden.

Ein Parameter, der auf einen Namen- oder Datenbuffer zeigt, ist nur eine
Adresse; der IOMan kopiert den referenzierten Inhalt nicht. Pro Kommando muss
die Spezifikation Richtung (IN/OUT/INOUT), Länge, Terminierung und Gültigkeit
des Zeigers definieren. Bei 32-bit Adressen muss außerdem feststehen, dass
der Empfänger dieselbe Adressdomäne/Abbildung sieht und wie ungültige oder
nicht zugängliche Adressen erkannt werden.

## 3. Manager-Kommandos

Die Kernelregister der eingehenden 13 Systemaufrufe sind eine getrennte
Schnittstelle. Der aktuelle Q9-Implementierungsstand ist in
[`KERNEL_IO_ABI.md`](KERNEL_IO_ABI.md) aufgelistet. Diese Inventur belegt
einige Register- und Bufferwerte, aber sie definiert nicht die 1–5 Parameter
des neuen Managerkommandos.

Die 13 Einträge entsprechen dem Q9-IOMAN-Aufgabeninventar. Die Anzahl
„1–5“ stammt aus der bisherigen Schnittstellenplanung; die genaue Zahl und
Belegung je Kommando ist offen und wird nicht aus OS-9-Ähnlichkeiten
abgeleitet.

| Entwurfs-ID | Manageraufruf | Kernel-I$-Callcode | Parameterzahl | Buffer/Ergebnis (vorläufig) | Noch festzulegen |
|---:|---|---:|---:|---|---|
| 0 | Create | `$83` | 1–5 | Pfadname und ggf. Create-Attribute; Rückgabe/Handle | Namenformat, Modus, Attribute, Pfadslot und Ergebnis |
| 1 | Open | `$84` | 1–5 | Pfadname; ggf. Modus/Attribute; Rückgabe Managerpfad | Pfadauflösung, Rechte, lokaler Slot und Handle |
| 2 | MakDir | `$85` | 1–5 | Verzeichnisname und ggf. Attribute | Pfad-/Attributformat und Fehlerfälle |
| 3 | ChgDir | `$86` | 1–5 | Verzeichnisname; ggf. Prozess-/Pfadkontext | Arbeitsverzeichnisbesitz und Rückgabe |
| 4 | Delete | `$87` | 1–5 | Zielname; ggf. Typ/Flags | Datei vs. Verzeichnis, Rechte, Flags |
| 5 | Seek | `$88` | 1–5 | offener Pfad und Position/Modus | Positionsbreite, Ursprung, Ergebnis |
| 6 | Read | `$89` | 1–5 | Zielbuffer-Zeiger und angeforderte Länge | Register/Parameterzuordnung, EOF/Kurzread, gelesene Länge |
| 7 | Write | `$8A` | 1–5 | Quellbuffer-Zeiger und angeforderte Länge | Register/Parameterzuordnung, Kurzwrite, geschriebene Länge |
| 8 | ReadLn | `$8B` | 1–5 | Zielbuffer und Kapazität | Terminierung, Zeilenende, EOF und Rückgabelänge |
| 9 | WritLn | `$8C` | 1–5 | Quellbuffer und Länge | Zeilenabschluss/CR-Regel und Rückgabelänge |
| 10 | GetStt | `$8D` | 1–5 | Statuscode und ggf. Ausgabezeiger/-größe | Statuscode-Tabelle, Ausgabeformat und Länge |
| 11 | SetStt | `$8E` | 1–5 | Statuscode und ggf. Eingabezeiger/-größe | Statuscode-Tabelle, Eingabeformat und Rechte |
| 12 | Close | `$8F` | 1–5 | Managerpfad/Handle | Dup-/Referenzsemantik, Fehler und Freigabe |

Die IDs 0–12 sind **Entwurfs-IDs für den neuen Kommando-Deskriptor**. Die
Spalte „Kernel-I$-Callcode“ ist nur eine Referenz zur Zuordnung des
ursprünglichen Systemaufrufs. Weder diese Callcodes noch die Entwurfs-IDs
legen bereits die Sprungtabellen-Slots des Managers fest. Der Protokollcode
kann später absichtlich dem Managervektor-Slot entsprechen; das muss aber
explizit beschlossen und darf nicht mit `$83`–`$8F` verwechselt werden.

## 4. Initialisierung aus Descriptor und Lebenszyklus

### 4.1 Descriptor-Eingaben

Vorgesehene Konfigurationswerte:

| Feld | Zweck | Status |
|---|---|---|
| Descriptor-/Gerätename | Identität und Lookup | vorgesehen; Format offen |
| Basispfad | Präfix für Namen-/Pfadauflösung | vorgesehen; Separator-/Normalisierungsregeln offen |
| Laufwerksname | vom Init einzurichtender Name/Alias | vorgesehen; Verhältnis zum Basispfad offen |
| Remote-Adresse | optionales Ziel eines entfernten Geräts | optional vorgesehen; Kodierung/Protokoll offen |
| Remote-Port | optionaler Remote-Endpunkt | optional vorgesehen; Breite/Byteordnung offen |

Der Descriptor ist Konfiguration, nicht der Manager-Kommandoblock. Der IOMan
liest und validiert ihn beim Attach, richtet die Namens-/Basispfadzuordnung
ein und startet danach die festgelegte Init-Sequenz.

### 4.2 Init-/Destroy-Reihenfolge – offen

Manager und Treiber haben beide `Init` und `Destroy`/`Term`. Es ist noch zu
prüfen, wer die Treiberfunktionen aufruft und welche Reihenfolge die
Schnittstellen voraussetzen. Bis zur Prüfung gilt nur folgende
Rollback-Anforderung:

1. Descriptor lesen und vollständig validieren.
2. Benötigte Module/Objekte linken und ihre Vektoren prüfen.
3. Manager-/Treiber-Init in der später bestätigten Reihenfolge ausführen.
4. Erst nach vollständigem Erfolg Gerät/Route sichtbar veröffentlichen.
5. Bei Fehler nur bereits erfolgreich initialisierte Komponenten in
   umgekehrter Reihenfolge zerstören und Links freigeben.
6. Beim Detach keine neuen Anfragen zulassen, offene Pfade und laufende I/O
   abwarten/abbrechen, dann bestätigte Destroy-Reihenfolge ausführen und
   Referenzen lösen.

**Offen:** ob IOMan beide Ebenen direkt startet/stoppt oder ob der Manager
seinen Treiber selbst verwaltet; ob `Destroy` immer mit `Init` korrespondiert;
Fehler-/Wiederholungssemantik; Mehrfach-Attach und Referenzzählung.

## 5. Kommandoauslösung: SetStat oder Write?

Vorläufige Empfehlung: **Steuer-/Kontrollkommandos über SetStat**, Datenstrom
über Write/Read. SetStat passt semantisch zu Init, Konfiguration und
Statusänderungen; Write bleibt für Nutzdaten. Die Kommando-ID könnte dabei
den SetStat-Unterbefehl bezeichnen und der Parameterblock seine Argumente
enthalten.

Das ist noch nicht entschieden. Insbesondere ist `I$SetStt` im aktuellen
Q9-Kernel nicht als IOMan-Manager-Schatten verdrahtet; der vorhandene native
Pfad deckt nur einen begrenzten Statusfall ab. Daher muss geklärt werden, ob
der Protokolltransport ein interner Manager-/Treiber-Aufruf ist oder ob die
Kernel-Weiterleitung für `I$SetStt` erweitert werden soll. `I$Write` als
Kontrolltransport bleibt eine mögliche Alternative, müsste aber einen
eigenen Kommando-Kanal klar von normalen Schreibdaten unterscheiden.

## 6. Fehler, Parallelität und Datenlebensdauer

- Jeder Befehl liefert einen eindeutigen Erfolgs-/Fehlerstatus; bei Read/
  Write zusätzlich die tatsächlich übertragene Länge.
- Bei Fehler muss feststehen, ob teilweise bearbeitete Buffer/Objekte gültig
  bleiben und ob ein Retry erlaubt ist.
- Kein globales veränderliches Requestobjekt. Aufruflokaler Deskriptor oder
  äquivalenter reentranzsicherer Kontext.
- Solange Requests synchron sind, muss jeder Aufruferstack bis zur Rückkehr
  erhalten bleiben. Wird später asynchron gearbeitet, braucht es explizite
  Request-Lebensdauer, Completion, Abbruch sowie Schutz/Pinning der Buffer;
  Stackzeiger dürfen dann nicht einfach gespeichert werden.
- Länge und Adressbereich vor Nutzung prüfen; Addition von Adresse+Länge darf
  nicht überlaufen. Der konkrete Mechanismus hängt von Q9-Speicherschutz und
  Emulator-Adressmodell ab.
- Semantik von `I$Dup`, Prozessende-Cleanup, Sperren und parallelen Close-/I/O-
  Operationen ist vor produktivem Kerneladapter zu lösen.

## 7. Offene Entscheidungen / nächste Prüfungen

1. Vorhandene Manager-/Treiber-Vektoren und Descriptorformat untersuchen:
   Aufrufsignaturen, Register, Parameterrichtung, Längen und Init/Destroy-
   Eigentümer nachweisen.
2. Die 1–5 Parameter je der 13 Manageroperationen tabellarisch finalisieren.
3. Festlegen, ob Kommando-ID ein SetStat-Code, Managervektor-Slot oder eigener
   Q9-Protokollcode ist; IDs nicht implizit mit OS-9-Callcodes gleichsetzen.
4. Request-ABI wählen: feste 7-LONG-Struktur, kompakter variabler Block oder
   Registerparameter. Version/Größe, Ergebnis und Fehler darin festlegen.
5. Exakte A0/A1- und D0–D3-Belegung sowie Adressdomäne/Validierung definieren.
6. Descriptorfelder für Basispfad, Gerätename, Laufwerksname und optionalen
   Remote-Endpunkt samt Encoding/Normalisierung festlegen.
7. Init-/Destroy-Verantwortung, Reihenfolge, Rollback, Attach-Referenzen und
   Detach bei offenen Pfaden prüfen.
8. SetStat-Steuertransport gegen direkten Manager-/Treiberaufruf und
   I$Write-Alternative abwägen; bei I$SetStt Kernel-Schattenintegration planen.
9. Synchrone Completion als MVP verbindlich bestätigen oder asynchronen
   Lebenszyklus samt Queue/Completion definieren.
10. Fehlercode- und Teiltransfervertrag pro Kommando ergänzen.

## 8. Geltungsgrenze zum aktuellen IOMan-Code

Diese Spezifikation ist noch nicht im `qioman.h`-API oder in den aktiven
`I$Open/Read/Close`-Handlern umgesetzt. Der aktuelle Host-Router, Backend-
Präfixregister und Kernel-Registerrahmenhelfer sind Vorarbeiten; sie belegen
weder dieses Protokoll noch eine lauffähige Manager-/Treiber-/Emulatorkette.
Die derzeitigen registrierten Kernelhandler bleiben sichere `E$UnkSvc`-
Stubs, bis Pfadslots, `I$Dup`, Prozess-Cleanup und der endgültige Request-
Vertrag abgestimmt sind.
