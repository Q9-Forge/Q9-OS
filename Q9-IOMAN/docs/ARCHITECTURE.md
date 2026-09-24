# Q9-IOMAN Architektur – Startentwurf

## Grenze und Verantwortlichkeiten

Q9-IOMAN soll Kernel-Systemaufrufe und geräte-/dateisystemspezifische Manager
entkoppeln. Der Kerneladapter übersetzt später Trap-Register in eine
Q9-IOMAN-Anfrage; Q9-IOMAN ordnet den lokalen Pfad einem Backend zu und
delegiert; ein File-Manager wie ein künftiges Q9-RBF implementiert
Dateisystemsemantik und ruft einen Treiber für Block-I/O auf. Q9-SCF wird ein
separates zeichenorientiertes Backend.

```text
Prozess / Syscall
       |
Kerneladapter (Register-ABI; noch offen)
       |
Q9-IOMAN (Pfadzuordnung und Dispatch)
       |------------------|
Q9-SCF / Q9-RBF       weitere Q9-Manager
       |
Q9-Treiber
```

Die Schnittstelle in `include/qioman.h` abstrahiert den Kernel-Trap-Frame
absichtlich. Damit bleiben die Hosttests unabhängig von 68K-Registern und der
noch nicht fertigen QCC-Syscall-Codegenerierung.

## MVP-Reihenfolge

Der Kommunikationsvertrag wird separat und ausdrücklich als Entwurf in
[IO_PROTOCOL_SPEC.md](IO_PROTOCOL_SPEC.md) geführt. Er hält bestätigte
Leitlinien und offene ABI-/Lebenszyklusfragen getrennt fest.

1. Kerneladapter und Status-/Fehlervertrag festlegen.
2. Attach/Detach sowie Device-/Manager-Registrierung ergänzen.
3. Open/Close und Pfadtabellen an Q9-Prozessdeskriptoren anbinden.
4. Dispatch für Read/Write/GetStat/SetStat/Seek integrieren.
5. Pfad-Sperre/Warteverhalten und Fehler-/Rollback-Semantik ergänzen.
6. Mit einem einfachen Q9-SCF-Terminalpfad und danach Q9-RBF/CF end-to-end
   testen.

Noch keine Entscheidung über Microware-binäre Kompatibilität: Ziel ist
zunächst ein dokumentierter Q9-eigener Modulvertrag.
