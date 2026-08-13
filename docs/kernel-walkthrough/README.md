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
| [01](01-kernel-bootstrap/) | Kompletter Boot-Bootstrap: vom Einsprung bis zur Übergabe an den Scheduler (Speicher-/Arena-Init, Exception-Dispatch-Tabelle, Warteschlangen, erster Ausführungskontext — alles in einem Thema statt fünf einzelnen) | ✅ fertig |
| 02 | IO-Manager: Syscall-Dispatch (F$/I$-Aufrufe) | 🚧 geplant |
| 03 | RBF/Descriptor/Driver — der OS-9-"Dreiklang" | 🚧 geplant |

**Hinweis zur Konsolidierung:** Themen 01-05 aus der ursprünglichen Planung
(Speicher-Init, Exception-Dispatch, Prozesstabellen, Modul-Nachladen,
erster Ausführungskontext) wurden zu **einem** Thema 01 zusammengefasst —
Andreas wollte "alles bis zum Scheduler" an einem Stück, chronologisch,
nicht in fünf separaten Verzeichnissen. Die frühere 68K-Wissenslücke
(woher kommt der Speicher für `D_ExcJmp`?) ist jetzt geklärt — siehe
Thema 01.

## Konvention

Jedes Themen-Verzeichnis enthält:
- `README.md` — Erklärung + eingebettete Assembler-/C-Ausschnitte
- `asm-68k.r` — wortwörtlicher Auszug aus [`../../src/kernel/kernel.r`](../../src/kernel/kernel.r) (dem eigenen, byte-exakten Nachbau)
- `asm-x86.txt` — wortwörtlicher Auszug aus den Ghidra-Rohdisassemblierungen unter [`../../modules/os9000-x86/disasm/`](../../modules/os9000-x86/disasm/)
- optional `decompiled-x86.c` — Ghidra-Pseudo-C, nur wenn es fürs Verständnis wirklich etwas bringt (reine Datenbereiche/Header brauchen das nicht)

**Erstellt**: 2026-08-13
