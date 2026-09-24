# Q9-IOMAN

Eigenständiger, sauber neu entwickelter I/O-Manager für Q9-OS. Hier entsteht
Q9-eigener Code; `.os9-original/` ist lokales, ignoriertes Referenzmaterial
und wird weder eingebunden noch als Implementierungsvorlage kopiert.

Der vollständige Funktions- und Aufgabenstatus steht in [STATUS.md](STATUS.md).
Der aktuelle, ausdrücklich vorläufige Kommunikationsentwurf steht in
[docs/IO_PROTOCOL_SPEC.md](docs/IO_PROTOCOL_SPEC.md); bestätigte Leitlinien
und offene ABI-/Lebenszyklusfragen sind dort getrennt.
Die Eingaberegister der derzeitigen Q9-Kernel-I/O-Aufrufe sind separat in
[docs/KERNEL_IO_ABI.md](docs/KERNEL_IO_ABI.md) inventarisiert.

## Struktur

- `src/` – Q9-IOMAN Kernlogik
- `include/` – öffentliche, kernelneutrale Manager-/Backend-Schnittstelle
- `tests/` – Host-Regressionstests
- `docs/` – Architektur, Kernel-ABI und Integrationsnotizen
- `examples/` – spätere Beispiel-Backends
- `tools/` – spätere Modul-/Image-Testwerkzeuge
- `common/`, `68k/` – reserviert für plattformneutrale bzw. 68K-spezifische
  Modul- und Build-Integration

## Erster Baustein

`src/qioman_system.c` stellt einen residenten Managerzustand mit statischer
Pfadtabelle bereit. Der Start ist idempotent; damit kann ein späterer
Systemmodul-Einstieg zunächst Speicher und Tabellen einrichten.
`src/qioman.c` enthält den minimalen Pfad-Lebenszyklus und Backend-Router:
Öffnen über ein vom Aufrufer ausgewähltes Backend, lokale Pfadnummern,
Operationen weiterleiten, Präfixrouten registrieren/auflösen und nach dem
Schließen sicher wieder lösen. Ein aktiver Pfad verhindert das Lösen seiner
Route. Der Kern speichert außerdem die Read-/Write-Rechte, reserviert Slots
vor reentrant aufrufenden Backends und schützt aktive Operationen vor Close.
Er kennt weder Hardware noch ein Dateisystem und ruft noch keine
Kernel-Syscalls auf.
Insbesondere sind Attach-/Descriptor-Auflösung und Backendregistrierung,
prozessgebundene Pfadlebensdauer (`I$Dup`/Prozessende), Nebenläufigkeit/
Warteschlangen sowie SCF/RBF noch offen. Der Registerframe-Dispatcher und
seine drei 68K-Schatten-Einstiege sind integriert und gebaut; ein Lauf im
Kernel/Emulator steht noch aus.

Hosttests und Erstellung eines OS-9-Moduls mit Q9-eigener Toolchain:

```sh
make
```

`make` führt Hosttests aus und erzeugt `build/qioman` (interner OS-9-Modulname
`ioman`, wie vom Q9-Bootpfad gesucht) per
`qcpp → qcir → qir68k → qr68k → ql68k`. Die F$SSvc-Tabelle verbindet die
Q9-Kernel-Schatten für `I$Open`, `I$Read` und `I$Close` mit dem C-Dispatcher.
Open löst den begrenzten Pfadzeiger auf, Read reicht die numerische
Bufferadresse ans Backend weiter und Close gibt den verwalteten Slot frei.
Weil noch kein Backend aus einem Descriptor/Modul registriert wird, endet ein
Open derzeit bei `E$MNF`. Der Resolver kann in Q9s flachem Adressraum keine
ungültigen Speicherbereiche abfangen; `I$Dup`- und Prozessende-Cleanup fehlen.

## Kernel-Aufrufe und QCC

QCC spezifiziert `modul syscall(callcode, archetype)` für direkte
`TRAP #0`-Aufrufe mit Register-Archetypen. Der QCC-Status führt diese Phase
aber noch als offen; Parser-/Backend-Codegen und ein End-to-End-Test sind
nicht belegt. Deshalb benutzt der neue Router diese Syntax noch nicht. Die
spätere Kernel-Adapter-Schicht soll direkte Syscall-Deklarationen verwenden,
wo die unterstützte Archetype- und Rückgabekonvention exakt passt, und
Sonderfälle isoliert kapseln. Das spart gegebenenfalls Wrapper-Aufrufcode,
ändert aber nicht die Kernel-Trap-Dispatch-Kosten.

Vor einer produktiven QCC-Anbindung sind außerdem die konkreten Eingabe- und
Ausgaberegister jedes verwendeten Kernel-Dienstes gegen Q9-Kernel und QCC
Codegen abzugleichen; ein C-Rückgabewert allein bildet nicht automatisch
mehrere Register-Rückgaben ab.
