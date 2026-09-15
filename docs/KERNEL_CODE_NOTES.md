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
Ablaufentscheidung. Nächster Ansatzpunkt: der noch nicht vollständig
gelesene Codeblock bei Modul-Offset `0x554`–`0xbbf` (enthält
Trap-Rückkehr- und Reschedule-Logik) sowie dessen Aufrufer – vermutlich
eine Verzeichnis- oder Modul-Scan-Schleife, die "fertig, nichts mehr zu
tun" einen Schritt zu früh entscheidet.

## Werkzeugnotizen

- Instruktionsspuren und Adress-Beobachtungen lassen sich am
  laufenden Emulator per Monitor-/Debug-Schnittstelle auslesen; für
  Haltepunkte und Einzelschritt eignet sich eine minimale
  GDB-Remote-Protokoll-Anbindung besser als die eingebaute
  Kommandozeile.
- m68k-Registerlayout im `g`-Antwortpaket dieser Anbindung: D0–D7 (je
  4 Byte), A0–A7 (je 4 Byte), danach SR (4 Byte), danach PC (4 Byte).
