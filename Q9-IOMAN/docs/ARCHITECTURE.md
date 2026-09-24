# Q9-IOMAN Architektur – Startentwurf

## Grenze und Verantwortlichkeiten

Q9-IOMAN entkoppelt Kernel-Systemaufrufe von geräte- und
dateisystemspezifischen Managern. Ein Assembly-/C-Kerneladapter übersetzt den
Registerframe für `I$Open`, `I$Read` und `I$Close` in Q9-IOMAN-Anfragen;
Q9-IOMAN ordnet den lokalen Pfad einem Backend zu und delegiert. Ein
File-Manager wie ein künftiges Q9-RBF implementiert Dateisystemsemantik und
ruft einen Treiber für Block-I/O auf. Q9-SCF wird ein separates
zeichenorientiertes Backend.

```text
Prozess / Syscall
       |
Kerneladapter (Open/Read/Close teilweise integriert)
       |
Q9-IOMAN (Pfadzuordnung und Dispatch)
       |------------------|
Q9-SCF / Q9-RBF       weitere Q9-Manager
       |
Q9-Treiber
```

Die Schnittstelle in `include/qioman.h` abstrahiert den Kernel-Trap-Frame
absichtlich. Damit bleiben die Hosttests unabhängig von 68K-Registern und der
noch nicht fertigen QCC-Syscall-Codegenerierung. Die Registerframe-Brücke
liegt separat in `src/qioman_kernel.c` und `68k/qioman_entry.a`; sie ist für
Open/Read/Close gebaut und hostgetestet. Der Emulator belegt bisher den
Open-Einstieg und dessen Rückkehr, aber keinen vollständigen Backend-I/O-Pfad.

## MVP-Reihenfolge

Der Kommunikationsvertrag wird separat und ausdrücklich als Entwurf in
[IO_PROTOCOL_SPEC.md](IO_PROTOCOL_SPEC.md) geführt. Er hält bestätigte
Leitlinien und offene ABI-/Lebenszyklusfragen getrennt fest.
Die lokale Q9-Registerbelegung der eingehenden Kernel-I/O-Aufrufe steht in
[KERNEL_IO_ABI.md](KERNEL_IO_ABI.md); sie ist nicht mit dem neuen
Managerkommando-ABI gleichzusetzen.

1. Den schmalen Open/Read/Close-Kerneladapter vervollständigen und mit
   Emulator-I/O nachweisen. `I$Write`, `I$Seek`, Status- und Line-I/O sind
   nicht als IOMan-Schattenhandler integriert; dafür ist eine Erweiterung der
   Kernel-Schattenroute nötig.
2. Attach/Detach sowie Device-/Manager-Registrierung ergänzen.
3. Open/Close und Pfadtabellen an Q9-Prozessdeskriptoren anbinden.
4. Dispatch für Read/Write/GetStat/SetStat/Seek integrieren.
5. Pfad-Sperre/Warteverhalten und Fehler-/Rollback-Semantik ergänzen.
6. Mit einem einfachen Q9-SCF-Terminalpfad und danach Q9-RBF/CF end-to-end
   testen.

Für Attach/Detach ist das gewünschte Zustands- und Rollbackmodell in Abschnitt
4.3 der Protokollspezifikation festgehalten. Es ist zunächst ein Entwurf und
noch keine implementierte Device-Lifecycle-API.

Noch keine Entscheidung über Microware-binäre Kompatibilität: Ziel ist
zunächst ein dokumentierter Q9-eigener Modulvertrag.
