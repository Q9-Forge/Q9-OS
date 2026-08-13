# Kernel-Walkthrough: Boot-Reihenfolge OS-9/68K vs. OS-9000/x86, Schritt für Schritt

Diese Reihe erzählt den Kernel-Bootvorgang **thematisch statt chronologisch
nach Recherchedatum** — jedes Thema bekommt ein eigenes, nummeriertes
Unterverzeichnis mit: einer kurzen Erklärung, dem tatsächlichen
Assembler-Ausschnitt aus **beiden** Kernen (OS-9/68K `dker030s` und
OS-9000/x86 `kernel`, live extrahiert), und — wo es inhaltlich etwas bringt
— dem passenden dekompilierten C-Auszug. Ziel: am Ende steht ein
verständlicher Fahrplan "was der Kernel beim Start grundsätzlich tut",
als Vorlage für den Entwurf des eigenen Q9-Kernels.

Diese Doku fasst nur bereits vorhandene Recherche neu zusammen (aus
[`../REVERSE_ENGINEERING.md`](../REVERSE_ENGINEERING.md) für den 68K-Kernel
und [`../../modules/os9000-x86/docs/`](../../modules/os9000-x86/docs/) für
den x86-Kernel) — für den vollen Detailgrad bleiben diese Dokumente die
Primärquelle, hier geht es um die aufbereitete, vergleichende Kurzform mit
echtem Code direkt dabei.

## Themen

| # | Thema | Status |
|---|---|---|
| [00](00-modul-aufbau-und-header/) | Kernel-Modul-Aufbau und Header | ✅ fertig |
| 01 | Speicher-/Arena-Init | 🚧 geplant — 68K-Seite hat eine offene Frage (siehe unten) |
| 02 | Exception-/Trap-Dispatch-Tabelle aufbauen | 🚧 geplant |
| 03 | Prozess-/Deskriptor-Tabellen vorbereiten | 🚧 geplant |
| 04 | Modul-Header-Prüfung + Nachladen weiterer Module (Relozierer/Scanner) | 🚧 geplant |
| 05 | Erster Ausführungskontext / Sprung in den ersten Prozess | 🚧 geplant |
| x | IO-Manager: Syscall-Dispatch (F$/I$-Aufrufe) | 🚧 geplant |
| x | RBF/Descriptor/Driver — der OS-9-"Dreiklang" | 🚧 geplant |

**Bekannte Hürde für Thema 01:** Eine frühe Annahme in
`REVERSE_ENGINEERING.md` ("`0x4978` ist der Speicher-Allokator für die
Exception-Tabelle") wurde später **explizit widerlegt** ("Fund (Korrektur
einer Fehlannahme): `0x4978` ist KEIN Allocator, sondern reine
Konstanten-Initialisierung", Zeile ~1211) — die tatsächliche Herkunft des
in `D_ExcJmp` gespeicherten Zeigers ist beim 68K-Kernel also noch nicht
zweifelsfrei geklärt. Für die x86-Seite ist der Speicher-/Arena-Aufbau
dagegen bereits vollständig verstanden (`KERNEL_INIT.md`, Fund 3/5). Bevor
Thema 01 geschrieben wird, lohnt sich vermutlich eine kurze zusätzliche
Ghidra-Runde, um diese 68K-Lücke zu schließen — sonst müsste das Thema mit
einer echten Wissenslücke auf der 68K-Seite starten.

## Konvention

Jedes Themen-Verzeichnis enthält:
- `README.md` — Erklärung + eingebettete Assembler-/C-Ausschnitte
- `asm-68k.r` — wortwörtlicher Auszug aus [`../../src/kernel/kernel.r`](../../src/kernel/kernel.r) (dem eigenen, byte-exakten Nachbau)
- `asm-x86.txt` — wortwörtlicher Auszug aus den Ghidra-Rohdisassemblierungen unter [`../../modules/os9000-x86/disasm/`](../../modules/os9000-x86/disasm/)
- optional `decompiled-x86.c` — Ghidra-Pseudo-C, nur wenn es fürs Verständnis wirklich etwas bringt (reine Datenbereiche/Header brauchen das nicht)

**Erstellt**: 2026-08-13
