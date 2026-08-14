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
| [02](02-io-manager-syscall-dispatch/) | IO-Manager: wie ein Syscall beim Treiber landet — Dreiklang (Descriptor/Driver/Fmgr) mit identischen Typ-Filtern 0xF00/0xE00/0xD00 auf beiden Architekturen bestätigt | ✅ fertig — inkl. IOMan-seitiger Dispatcher-Aufrufer (`Q9X_ioman_callcode_dispatch`), siehe Nachtrag im Thema selbst |
| [03](03-dreiklang/) | RBF/Descriptor/Driver im Detail — x86-Callcode-Dispatch-Tabelle gefunden (16 Slots im `m_idata`-Bereich statt bei `m_exec` wie beim 68K) | ✅ fertig (x86: nur File-Manager untersucht, Descriptor/Driver offen) |
| [04](04-scheduler-prozesslebenszyklus/) | Scheduler & Prozess-Lebenszyklus — 68K-Vorarbeit erweitert, x86 neu untersucht; Kettenschluss gefunden: x86-Boot-Trampolin endet in `Q9X_scheduler_insert` | ✅ fertig (x86: nur Scheduler-Kern gelesen, `F$Fork`/`F$Exit` auf x86 noch offen) |
| [05](05-speicherverwaltung/) | Speicherverwaltung/Allokator — 68K-Vorarbeit erweitert, x86 neu untersucht; identische Fehlercodes (`0xDB`/`0xD2`/`0xAB`/`0xED`/`0xE1`) und identische Template-Kopie-Technik über beide Architekturen bestätigt | ✅ fertig (x86: eigentlicher First-Fit-Allokator `FUN_002228ac` nicht mehr benannt, Syscall-Einstiegspunkt bei keiner Architektur gefunden) |

**Hinweis zur Konsolidierung:** Themen 01-05 aus der ursprünglichen Planung
(Speicher-Init, Exception-Dispatch, Prozesstabellen, Modul-Nachladen,
erster Ausführungskontext) wurden zu **einem** Thema 01 zusammengefasst —
Andreas wollte "alles bis zum Scheduler" an einem Stück, chronologisch,
nicht in fünf separaten Verzeichnissen. Die frühere 68K-Wissenslücke
(woher kommt der Speicher für `D_ExcJmp`?) ist jetzt geklärt — siehe
Thema 01.

**Hinweis zu Thema 02:** die 68K-Seite stützt sich auf bereits vorhandene
4-Runden-Vorarbeit (`modules/ioman/`), die x86-Seite wurde in dieser Runde
erstmals disassembliert und weniger tief untersucht (16 von 67 Funktionen
benannt) — der Dreiklang-Mechanismus UND der IOMan-seitige Callcode-
Dispatcher (`Q9X_ioman_callcode_dispatch`/`Q9X_ioman_dispatch_invoke`,
in einer Folgerunde nach Thema 03 gefunden) sind geklärt.

**Hinweis zu Thema 03:** rein x86-seitiger Fortschritt (68K-Teil ist
Zusammenfassung bestehender Vorarbeit) — die File-Manager-Dispatch-Tabelle
gefunden und bestätigt (9 von 16 Slots zeigten auf zuvor völlig unbekannte,
aber gültige Funktionsprologe), der IOMan-seitige Aufrufer wurde
zwischenzeitlich ebenfalls gefunden (s. Thema 02). Descriptor/Driver auf
x86-Seite weiterhin nicht untersucht.

**Hinweis zu Thema 04:** 68K-Teil ist Zusammenfassung bereits vorhandener,
sehr detaillierter Vorarbeit aus `docs/REVERSE_ENGINEERING.md` (nicht neu
disassembliert). x86-Teil neu: der Boot-Trampolin aus Thema 01 endet
nachweislich in `Q9X_scheduler_insert` — Queue-Walk mit Timer-Vergleich
und Unlink-bei-Fälligkeit gefunden, aber nicht die komplette 743-Byte-
Funktion gelesen; `F$Fork`/`F$Exit` auf x86 in dieser Runde nicht gesucht.

**Hinweis zu Thema 05:** 68K-Teil ist Zusammenfassung bereits vorhandener
Vorarbeit aus `docs/REVERSE_ENGINEERING.md` (Pool→Arena→Freiliste-Schema,
vollständig gelesen). x86-Teil neu, gefunden über eine gezielte Suche nach
den fünf bereits bekannten 68K-Fehlercodes (`0xDB`/`0xD2`/`0xAB`/`0xED`/
`0xE1`) — alle fünf tauchen identisch im x86-Kernel wieder auf, ebenso die
"Template-Kopie in einen neuen Arena-Deskriptor"-Technik. Der eigentliche
First-Fit-Allokator (`FUN_002228ac`) wurde referenziert, aber nicht mehr
selbst disassembliert/benannt.

## Konvention

Jedes Themen-Verzeichnis enthält:
- `README.md` — Erklärung + eingebettete Assembler-/C-Ausschnitte
- `asm-68k.r` — wortwörtlicher Auszug aus [`../../src/kernel/kernel.r`](../../src/kernel/kernel.r) (dem eigenen, byte-exakten Nachbau)
- `asm-x86.txt` — wortwörtlicher Auszug aus den Ghidra-Rohdisassemblierungen unter [`../../modules/os9000-x86/disasm/`](../../modules/os9000-x86/disasm/)
- optional `decompiled-x86.c` — Ghidra-Pseudo-C, nur wenn es fürs Verständnis wirklich etwas bringt (reine Datenbereiche/Header brauchen das nicht)

**Erstellt**: 2026-08-13
