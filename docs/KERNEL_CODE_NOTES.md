# Q9-Kernel – Code- und Ablaufnotizen (Original-Firmware)

Laufende Sammlung von Fakten über das Verhalten des originalen,
gewachsenen OS-9/68K-Kernelmoduls (`dker030s`, Development/Standard-
Allocator-Variante – lädt die reale CB030-Bootkonfiguration), gewonnen
durch Instruktions- und Kontrollflussanalyse des Modul-Binaries sowie
Beobachtung am laufenden Emulator. Ziel ist ein byte-exakt
nachvollziehbares Bild der Kernel-internen Abläufe – nicht die
Dekompilierung nach C als Selbstzweck (das war ein früherer, verworfener
Ansatz), sondern Verständnis einzelner Funktionen und ihrer Aufrufer,
damit sich eigene Boards/Anpassungen verlässlich darauf aufbauen
lassen.

Diese Datei ist der Nachfolger einer älteren, gleichnamigen Notiz-Serie
(vor der Repo-Umstrukturierung vom 2026-09-13 unter einem anderen Pfad
geführt); der historische Stand liegt als Sicherung vor, wird hier aber
nicht 1:1 übernommen, sondern nur bei Bedarf nachgezogen.

## Adressumrechnung

Das Kernelmodul wird laut Laufzeitbeobachtung nach der Relokation bei
RAM-Adresse `0x7100` geladen. Für Adressen aus der statischen
Modul-Analyse gilt also:

```
Live-RAM-Adresse = Modul-Offset + 0x7100
```

Verifiziert an zwei unabhängigen Ankerpunkten: der TRAP-#0-Dispatcher
(Modul-Offset `0x488`) liegt live bei `0x7588`; die
Speicherfreigabe-Funktion (Modul-Offset `0x5a22`) liegt live bei
`0xcb22` – beide stimmen exakt mit unabhängig beobachtetem
Laufzeitverhalten überein.

## Bereits bekannte Funktionsadressen (Modul-Offset)

| Offset | Funktion | Bemerkung |
|---|---|---|
| `0x1004`/`0x1034`/... | diverse Trap-Rückkehr-Helfer | teils noch unbenannt |
| `0x183a` | Ready-Queue-Insert (Priority-Aging) | 10 bekannte Aufrufer |
| `0x1c74`/`0x1e18` | kleine Wrapper | `0x1e18` ruft nur `0x3370` |
| `0x24d8`–`0x258f` | **Prozess-Exit-Routine** | s. Abschnitt unten |
| `0x2590`, `0x2cee`, `0x4518` | von der Exit-Routine gerufen | noch nicht gelesen |
| `0x3140`–`0x32bf` | **Scheduler-Dispatch-Kern** | s. Abschnitt unten |
| `0x3370`–`0x33c5` | Queue-Slot-Freigabe (Tabellen-Index-basiert) | ruft ggf. `0x131c` |
| `0x131c`–`0x132f` | Wrapper um `0x5a22` (Freigeben, Argument 1) | |
| `0x554`–`0xbbf` | großer, noch nicht vollständig gelesener Block | Trap-Rückkehr-/Reschedule-Logik, s. u. |
| `0x5440`–`0x5557` | Speicher-Allozieren (First-Fit, Split von hinten) | |
| `0x5a22`–`0x5bab` | Speicher-Freigeben (Boundary-Tag-Coalescing) | |
| `0x1a70`–`0x1ab1` | geteilter Fehler-/Abbruch-Nachlauf, s. Abschnitt unten | über Sprungtabelle erreicht, keine direkten Aufrufer auffindbar |

Ein früherer Sitzungsstand enthält eine vollständige Liste der
öffentlichen Einstiegspunkte (Live-Adressen, nicht Modul-Offsets) mit
ihren Namen aus dem offiziellen Technical Reference Manual. Damit
lassen sich einige der obigen Funktionen jetzt eindeutig benennen:
`0x24d8` = `F$Exit`, `0x3140` = `F$NProc` ("nächsten Prozess wählen",
identisch mit dem hier "Scheduler-Dispatch-Kern" genannten Code),
`0x183a` = `F$AProc` ("Prozess aktivieren", identisch mit "Ready-Queue-
Insert"), `0x3370`/`0x131c` gehören zur `F$RetPD`-Familie
("Prozessdeskriptor zurückgeben"). Diese Zuordnung bestätigt, dass die
in dieser Datei beschriebenen Funktionen tatsächlich die zentralen,
dokumentierten Kernel-Dienste sind und keine Sonderfälle.

## Prozess-Exit-Routine (Modul-Offset `0x24d8`)

Markiert den beendeten Prozess mit dem Statusbyte `0x2d` (`'-'`,
klassisches OS-9-„tot"-Kennzeichen an dieser Codebasis). Reaktiviert
einen wartenden Elternprozess **nur wenn alle drei Bedingungen
zutreffen**:

1. Ein Kurzwort-Feld im Prozessdeskriptor bei Offset `+2` ist
   ungleich 0 (Bedeutung noch nicht abschließend geklärt – Verdacht:
   Kindprozess- oder offener-Pfad-Zähler).
2. Ein Typ-Byte im Elterndeskriptor (Offset `+0x20`) hat den Wert
   `'w'`.
3. Eine weitere Prüfung (Aufruf zu Offset `0x2cee`, Rückgabe hier
   `bVar4` genannt) fällt negativ aus.

Sind alle drei erfüllt, wird `0x183a` (Ready-Queue-Insert) aufgerufen,
danach wie immer der Scheduler-Dispatch (`0x3140`). Ist eine der drei
Bedingungen nicht erfüllt – insbesondere Bedingung 1 – springt die
Routine direkt zum Scheduler-Dispatch, **ohne** einen neuen Prozess
einzureihen.

## Scheduler-Dispatch-Kern (Modul-Offset `0x3140`)

Läuft die Ready-Queue ab (Sentinel-Vergleich Kopf==Schwanz erkennt
„leer"). Ist die Queue leer, wird ein Funktionszeiger aus dem
Systemglobal-Bereich (Offset `0x940`) aufgerufen – der Idle-Hook, der
auf dieser Plattform letztlich die `stop`-Instruktion ausführt – und
danach die Prüfung wiederholt. Verhält sich exakt nach dem zu
erwartenden generischen Vertrag eines präemptiven Schedulers: kein
Fehlverhalten hier, nur der Ort, an dem ein leerer Ready-Queue-Zustand
sichtbar wird.

## Offener Punkt: vorzeitiger Prozess-Austritt beim Emulator-Bootvorgang

Im Rahmen der Inbetriebnahme von Q9-Flux-68kQEMU (eigener
QEMU-basierter Emulator, s. dortiges Repo) wurde beobachtet: Während
ein bestimmter Bootvorgang (reales Boot-ROM-Image + reales
CompactFlash-Abbild) beim Referenz-Interpreter erfolgreich bis zum
Erreichen des vollständigen Gerätebetriebs durchläuft, bleibt derselbe
Bootvorgang unter QEMU nach dem Laden des CompactFlash-Treibermoduls in
einer sauberen Leerlaufschleife (`stop`-Instruktion, kein Absturz)
hängen.

Laufzeitbeobachtung (Haltepunkte auf der Prozess-Exit-Routine, mehrere
Durchläufe während desselben Bootvorgangs verglichen):

- Erster und zweiter beobachteter Prozess-Austritt während des Boots:
  das Kurzwort-Feld bei Deskriptor-Offset `+2` ist 3 bzw. 2 (ungleich
  0) → Reaktivierungspfad wird genommen, alles normal.
- Dritter (letzter beobachteter) Prozess-Austritt, Deskriptor bei
  RAM-Adresse `0xffcf30`: das Feld ist 0 → keine Reaktivierung, der
  Scheduler findet keinen weiteren lauffähigen Prozess mehr, das
  System bleibt dauerhaft im Leerlauf.

Das erklärt den unmittelbaren Mechanismus des Hängenbleibens, aber
nicht die eigentliche Ursache: warum genau dieser Prozess mit diesem
Feld gleich 0 austritt, während der Referenz-Interpreter an der
äquivalenten Stelle noch aktiv weiterarbeitet (u. a. mit zusätzlichen
CompactFlash-Zugriffen, darunter ein Schreibzugriff), ist offen. Der
CompactFlash-Zugriffsverkehr selbst wurde bis zum Divergenzpunkt
Byte-für-Byte identisch zwischen beiden Umgebungen nachgewiesen – die
Ursache liegt also nicht im Datenpfad, sondern in einer
Ablaufentscheidung.

**Nachtrag, gleicher Tag:** Der aufrufende Codepfad des dritten
(hängenden) Austritts wurde weiter eingegrenzt. Anders als bei den
ersten beiden, unauffälligen Austritten läuft der dritte über einen
gemeinsamen Fehler-/Abbruch-Nachlauf bei Modul-Offset `0x1a70` (live
verifiziert per Haltepunkt: derselbe Prozessdeskriptor `0xffcf30` wird
dort unmittelbar vor dem Exit-Aufruf gesehen). Dieser Nachlauf ist
selbst kein öffentlicher Einstiegspunkt (kein Treffer in der
namentlich zugeordneten Liste der Syscalls) und hat keine per
Aufruf-Referenz auffindbaren Aufrufer – er wird also, wie an dieser
Codebasis für interne Rücksprünge üblich, über eine zur Laufzeit
berechnete Sprungadresse erreicht, nicht über eine feste `bsr`/`jsr`-
Referenz. Seine erste Instruktion (`movea.l (SP)+,A3`) liest keinen
Rücksprungwert, sondern einen vom Aufrufer bewusst hinterlegten Wert
(den Systemglobal-Basiszeiger) – ein bekanntes Aufrufmuster dieser
Codebasis für Rücksprünge aus einem tabellenbasierten Dispatch heraus,
nicht für gewöhnliche Unterprogrammaufrufe.

Der Nachlauf selbst prüft unmittelbar nach Eintritt einen
Fehlerindikator aus dem vorangegangenen Aufruf und verzweigt bei
"Fehler angezeigt" direkt zum Exit, ansonsten (kein Fehler, aber ein
bestimmtes Deskriptorfeld gesetzt) zu einer Ereigniszustellung, sonst
zu einer normalen Weiterplanung (Ready-Queue-Insert + Scheduler, ohne
Exit).

**Korrektur, Folgesitzung:** Die vorstehende Vermutung ("eigentlicher
Auslöser ist ein fehlgeschlagener Aufruf") wurde live widerlegt. Der
unmittelbare Aufrufer des Nachlaufs bei `0x1a70` wurde gefunden: eine
größere Routine ab Modul-Offset `~0x1900`, die ein neues
Prozessdeskriptor-Feld initialisiert (u. a. die beiden Syscall-
Sprungtabellenzeiger auf 0 setzt), danach über den öffentlichen
Sprungtabellen-Mechanismus `F$Move` aufruft (Syscall `0x38`, live
`0xa1a0`) und bei dessen Rückkehr direkt in eine weitere Hilfsroutine
(Modul-Offset `0x1a26`) fällt, die ihrerseits einen manuell
konstruierten Rücksprungrahmen auf den Stack der Zielroutine legt, der
bei `0x1a70` "zurückkehrt". Das Gesamtmuster (Sprungtabellenzeiger neu
setzen, `F$Move`, danach ein "Rücksprung mit D0=5") entspricht eher der
**normalen Ausführung von `F$Fork` oder `F$Chain`** (Prozesserzeugung
bzw. Bildwechsel) als einem Fehlerpfad – `F$Chain` liegt als
öffentlicher Einstiegspunkt bei live `0x8a28`, unmittelbar in
derselben Codeumgebung.

Live-Verifikation (Haltepunkt direkt bei `0x1a70`, Register- und
Flag-Zustand ausgelesen): Das Carry-Flag ist beim Eintritt **nicht**
gesetzt, `D0=5`. Der Sprung "bei Fehler direkt zum Exit" wird an dieser
Stelle also NICHT genommen – stattdessen läuft die Ausführung normal
weiter über `F$AProc` (Ready-Queue-Insert) und `F$NProc` (Scheduler),
exakt wie bei den ersten beiden, unauffälligen Prozess-Austritten. Das
widerlegt die vorherige Annahme, genau dieser Codepfad sei für den
Hänger verantwortlich.

**Neue Beobachtung, noch nicht abschließend eingeordnet:** Trotz dieses
erfolgreichen Durchlaufs landet derselbe Testlauf am Ende dennoch in
derselben bekannten Leerlaufadresse – der Bootvorgang hängt also
weiterhin, nur läuft der konkret beobachtete Prozessdeskriptor
(`0xffcf30`) diesmal noch mindestens eine Runde weiter, bevor
(vermutlich) ein anderer oder dieselbe Deskriptor-Adresse (durch
Wiederverwendung im Speicherpool nicht immer derselbe *Prozess*) den
tatsächlich folgenlosen Austritt macht. Die genaue Zuordnung "welcher
Prozess-Austritt in welchem Testlauf genau der letzte ist" scheint
also zwischen einzelnen Testläufen (Freilauf vs. Einzelschritt-
Betrieb über die Debug-Schnittstelle) leicht zu variieren, während der
Endzustand (Leerlauf an derselben Adresse) durchgehend gleich bleibt.
Das spricht dafür, den nächsten Untersuchungsschritt nicht mehr auf
eine einzelne, vermeintlich "die" fehlerhafte Deskriptor-Instanz zu
fokussieren, sondern generischer zu fragen: an welcher Stelle wird
letztendlich (gleich in welchem Testlauf) tatsächlich mit leerem
Reaktivierungs-Feld ausgetreten, und welcher Aufrufer führt dorthin.

**Bereits geprüfte und verworfene Nebenhypothese:** Die Wochentags-
Berechnung des Echtzeituhr-Ports (`Q9-Flux-68kQEMU`) unterscheidet sich
zwar im Quelltext von der Referenz (dort ein direkt berechneter
Sakamoto-Algorithmus, hier ein vom Systemaufruf zur Zeit-/Datums-
Abfrage bereits mitgeliefertes Feld) – die zugrundeliegende
Systemfunktion füllt dieses Feld aber nachweislich korrekt (Standard-
Bibliotheksfunktion, kein Sonderfall), sodass beide Werte für dasselbe
Datum übereinstimmen. Diese Spur führt also nicht weiter, obwohl die
zeitlich benachbart gefundene Schaltjahr-Arithmetik (`divsll`-basierte
365/366-Tage-Verzweigung nahe dem Divergenzpunkt) einen Datumsbezug
zunächst nahelegte.

**Durchbruch, dritte Folgesitzung:** Die generische Instrumentierung
(Haltepunkt direkt auf `0x24d8`, jeden Treffer samt Rücksprungadresse
protokolliert) zeigt: in einem sauberen Testlauf treten **alle drei**
beobachteten Prozess-Austritte – auch der folgenlose dritte – über
**dieselbe Rücksprungadresse** auf dem Stack auf. Diese Adresse liegt
NICHT im vorher untersuchten `0x1a70`/`Q9_gap_554`-Bereich, sondern
gehört zu einem echten, per `TRAP #0` ausgelösten Aufruf: der
Syscall-Dispatcher (`0x488`, s. o.) wird unmittelbar vor jedem der
drei Austritte durchlaufen. **Der Prozess ruft `F$Exit` also
selbst, absichtlich und über einen ganz normalen Systemaufruf auf –
es handelt sich um keinen impliziten Kernel-Fehlerpfad.** Die
`0x1a70`/`Q9_gap_554`-Spur aus den vorherigen zwei Sitzungen war damit
komplett verworfen und (wie oben dokumentiert) ohnehin schon als
falsche Fährte erkannt worden.

Eine Ausführungsspur direkt vor dem dritten Austritt zeigt eine kurze,
mehrfach wiederholte Codeschleife **außerhalb des Kernelmoduls**, in
einem separat geladenen Treiber – anhand einer im Speicher gefundenen
Zeichenkette ("CompactFlash driver build 42") als der
**CompactFlash-IDE-Treiber** identifiziert. Die Schleife besteht aus
mehreren kleinen Hilfsfunktionen: einer Leseroutine, die byteweise aus
einem festen, modul-statischen Puffer liest (klassisches Zeichen-für-
Zeichen-Parsing, z. B. eines Konfigurations- oder Gerätenamens), sowie
zwei nahezu identischen Tabellen-Bereichsprüfungen (ein Zeiger wird
gegen mehrere aufeinanderfolgende, gleich große Slots einer
statischen Tabelle verglichen und als Index 0/1/2/... oder
"keiner passt" klassifiziert) – zusammen mit Code, der ein frisch
angelegtes Objekt vollständig nullt (klassische Deskriptor-
Initialisierung). Das Gesamtbild passt zu Geräte-/Pfad-Deskriptor-
Verwaltung innerhalb des Treibers, nicht zu einer einfachen
Hardware-Wartewarteschleife.

Ein bereits für dieses Modul angelegtes, separates Analyseprojekt
existiert, deckt aber nur einen winzigen Ausschnitt ab (rund 1,4 KB,
im Wesentlichen nur der Zeichenketten-Bereich) und war für den
eigentlichen Code-Bereich nicht nutzbar – falls dieses Projekt künftig
weiterverwendet wird, müsste es zunächst um den vollständigen
Speicherabzug des geladenen Treibers ergänzt werden (Adressumrechnung
in dieser Sitzung bereits hergeleitet: der bekannte Zeichenketten-
Fundort im laufenden Emulator abzüglich seiner Position in diesem
Projekt ergibt die Ladebasis).

**Fortsetzung, vierte Folgesitzung – genaue `F$Exit`-Aufrufstelle
gefunden, IDENTIFY-Antwort als Nebenschauplatz ausgeschlossen:**

Zunächst wurde geprüft, ob die eigene IDENTIFY-Implementierung des
CompactFlash-Ports (`Q9-Flux-68kQEMU/devices/cf/q9_cf.c`) vom
Referenzverhalten abweicht – beide füllen nur die Sektorzahl
(Byte 120–123) und lassen den Rest der 512-Byte-IDENTIFY-Antwort auf
Null, wortgleicher Quelltext in beiden Portierungen. Kein Unterschied,
diese Spur führt nicht weiter.

Die exakte Trap-Instruktion für den dritten (folgenlosen) `F$Exit`-
Aufruf wurde über eine frische `-d int`-Spur direkt gefunden (letzte
Trap-Adresse im Treiberbereich vor dem Wechsel zu rein
kernelinternen Adressen). Eine begleitende Ausführungsspur zeigt den
unmittelbaren Kontext: Eine kleine Schleife durchläuft eine
**32-Einträge-Tabelle (44 Byte pro Eintrag, Gesamtgröße exakt
32×44=1408 Byte, klassische OS-9-Pfad-/Deskriptor-Tabellengröße)** –
bei den ersten beiden (unauffälligen) `F$Exit`-Aufrufen wird die
Schleife nach wenigen Durchläufen verlassen, beim dritten läuft sie
bis zum Tabellenende durch.

**Wichtige Korrektur zur vorherigen Einschätzung:** Diese Schleife ist
**kein fehlschlagender Suchlauf** – die Kontrollflussanalyse ihrer
Aufruferfunktion zeigt: Sie wird nur überhaupt betreten, wenn ein
bestimmtes Flag-Bit (Bit 5, Offset `+20`) an einem separaten,
außerhalb der Tabelle liegenden Objekt bereits gesetzt ist – andernfalls
wird sie komplett übersprungen. Innerhalb der Schleife wird pro
Tabelleneintrag nur ein anderes Bit (Bit 9, Offset `+12`) geprüft und
bei Bedarf über zwei Hilfsaufrufe "ausgespült" (Flush-artiges Muster),
danach gelöscht. Das Gesamtbild ist eine **geordnete Aufräum-Passage
("alle offenen Pfad-/Deskriptor-Puffer zurückschreiben") über die
komplette Tabelle, ausgelöst durch ein bereits vorher gesetztes
"wird beendet"-Flag** – nicht die Ursache des Abbruchs, sondern dessen
Konsequenz. `F$Exit` folgt direkt im Anschluss als letzter Schritt
dieser bereits beschlossenen Beendigung.

**Nächster Ansatzpunkt:** die eigentliche Ursache liegt VOR dieser
Aufräum-Passage – dort, wo das auslösende Flag-Bit (Offset `+20`,
Bit 5, am Objekt bei `%fp@(-32712)` zur Laufzeit) gesetzt wird. Das
ist der nächste, noch nicht identifizierte Ansatzpunkt: finden, welche
frühere Codestelle dieses Bit setzt und unter welcher Bedingung – und
ob genau diese Bedingung zwischen Referenzumgebung und
Q9-Flux-68kQEMU unterschiedlich ausfällt. Der noch nicht vollständig
gelesene Codeblock bei Modul-Offset `0x554`–`0xbbf` im Kernel
(Trap-Rückkehr-/Reschedule-Logik) bleibt weiterhin NICHT der relevante
Ansatzpunkt für dieses Problem – das war die bereits verworfene Spur
der ersten beiden Sitzungen.

## Werkzeugnotizen

- Instruktionsspuren und Adress-Beobachtungen lassen sich am
  laufenden Emulator per Monitor-/Debug-Schnittstelle auslesen; für
  Haltepunkte und Einzelschritt eignet sich eine minimale
  GDB-Remote-Protokoll-Anbindung besser als die eingebaute
  Kommandozeile.
- m68k-Registerlayout im `g`-Antwortpaket dieser Anbindung: D0–D7 (je
  4 Byte), A0–A7 (je 4 Byte), danach SR (4 Byte), danach PC (4 Byte).
