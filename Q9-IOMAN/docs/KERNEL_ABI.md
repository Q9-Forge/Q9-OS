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
