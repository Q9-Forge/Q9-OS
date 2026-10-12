# Kernelstatus und Arbeitshistorie

Stand: 2026-10-12. Die aktuelle Syscall- und Integrationsübersicht steht in
[`Q9-KERNEL/STATUS.md`](../Q9-KERNEL/STATUS.md); offene Arbeitsschritte stehen
in [`KERNEL_ARBEITSPLAN_de.md`](KERNEL_ARBEITSPLAN_de.md).

## Aktueller Debugger- und Trace-Stand

- Der Syscall-Logger schreibt gefilterte Ereignisse in den Kernel-Ringpuffer;
  das `trace`-Programm und der Host-Dekoder lesen und formatieren die Sätze.
  Der Emulator-Live-Lauf bestätigte Parameter für `F$PrsNam`; Pfadfilter für
  lokale `I$`-Aufrufe sind host- und emulatorgeprüft.
- Der Q9-Flux-Monitor zeigt Register, Stack, Symbol, Quellzeile und eine sichere
  Instruktionsvorschau. GDB-RSP unterstützt Breakpoints und Einzelschritt.
  Ein dynamisches `xcc -g`-Modul wurde bei `0x83916` angehalten und danach
  bei `0x8391a` schrittweise fortgesetzt.
- Q9K-Quellzeilenkarten sind auf das instrumentierte Kernelmodul begrenzt.
  Microware-`.map`-Dateien haben Vorrang; Q9-Flux liest v1-`.stb`-Module als
  Fallback für globale Code-Symbole (Q9-Flux `6271936`). Der Gastmodul-
  CRC-Abgleich bleibt offen.

## Historische Befunde

Die vollständige, unveränderte Chronik bis Fortsetzung 144 liegt in
[`archive/OWN_KERNEL_STATUS_chronik_2026-10-12.md`](archive/OWN_KERNEL_STATUS_chronik_2026-10-12.md).
Sie enthält Messprotokolle und Begründungen zu historischen Kerneländerungen;
für aktuelle Statusentscheidungen gelten `Q9-KERNEL/STATUS.md` und der
Arbeitsplan.
