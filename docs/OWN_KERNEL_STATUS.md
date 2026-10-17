# Kernelstatus und Arbeitshistorie

Stand: 2026-10-15. Die aktuelle Syscall- und Integrationsübersicht steht in
[`Q9-KERNEL/STATUS.md`](../Q9-KERNEL/STATUS.md); offene Arbeitsschritte stehen
in [`KERNEL_ARBEITSPLAN_de.md`](KERNEL_ARBEITSPLAN_de.md).

## Phase A, Stand 15.10.2026 (Branch `kernel/phase-a-fundament`)

Erledigt mit Beleg (Details und Commits in `KERNEL_ARBEITSPLAN_de.md`): A1, A4, B3.

- **Regressionslauf:** `tools/kernel_regress.sh` — Hosttests 30/30, Login-Test beider
  Varianten (development, atomic), `startup_shell_test.sh`, `mgrpath_probe_test.sh`; Lauf
  am 15.10.2026 vollständig grün.
- **Korrektur:** Die frühere Angabe „Hosttests 28/28 grün“ stimmte nicht — es gibt 30 Suiten,
  `test_q9kernel_moddir` (Zugriff auf absolute Diagnoseadresse) und `dbg_live` (ohne
  `-DQ9K_DEBUG`) waren rot und blieben unbemerkt.
- **`build.sh`:** bricht bei Fehlern in `mwos-build`, `r68` und `l68` ab und löscht alte
  Artefakte vor dem Bau; das alte Skript meldete bei einem C-Fehler Erfolg.
- **Scratch-Belegung:** drei echte Doppelbelegungen behoben (u. a. zerstörte jeder
  `F$FModul`-Aufruf Alarm 0); Prüfung läuft im Bau, Tabelle in `KERNEL_SPEICHERBELEGUNG_de.md`.
- **Offen (neu, A9):** Der Entwickler-Kernel hält nach einigen Forks an (`Q9K_ExcTrapSpin`),
  wenn Eingaben ohne Pause aufeinanderfolgen. Ursache nicht geklärt, s. `KERNEL_IRQ_SPERRE_de.md`.

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
  Fallback für globale Code-Symbole (Q9-Flux `6271936`); die gespeicherte
  24-Bit-CRC wird gegen das geladene Gastmodul geprüft (Q9-Flux `5ebf5b5`).

## Historische Befunde

Die vollständige, unveränderte Chronik bis Fortsetzung 144 liegt in
[`archive/OWN_KERNEL_STATUS_chronik_2026-10-12.md`](archive/OWN_KERNEL_STATUS_chronik_2026-10-12.md).
Sie enthält Messprotokolle und Begründungen zu historischen Kerneländerungen;
für aktuelle Statusentscheidungen gelten `Q9-KERNEL/STATUS.md` und der
Arbeitsplan.
