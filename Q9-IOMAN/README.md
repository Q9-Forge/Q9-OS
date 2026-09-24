# Q9-IOMAN

Eigenständiger, sauber neu entwickelter I/O-Manager für Q9-OS. Hier entsteht
Q9-eigener Code; `.os9-original/` ist lokales, ignoriertes Referenzmaterial
und wird weder eingebunden noch als Implementierungsvorlage kopiert.

Der vollständige Funktions- und Aufgabenstatus steht in [STATUS.md](STATUS.md).

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

`src/qioman.c` enthält einen minimalen Pfad-Lebenszyklus und Backend-Router:
Öffnen über ein vom Aufrufer ausgewähltes Backend, lokale Pfadnummern,
Operationen weiterleiten und erfolgreiches Schließen ausbuchen. Er kennt
weder Hardware noch ein Dateisystem und ruft noch keine Kernel-Syscalls auf.
Insbesondere sind Attach-/Descriptor-Auflösung, Pfadnamen-Suche,
Nebenläufigkeit/Warteschlangen, SCF/RBF und die Trap-Integration noch offen.

Hosttests:

```sh
make test
```

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
