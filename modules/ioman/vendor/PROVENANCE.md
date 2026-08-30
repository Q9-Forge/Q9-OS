# Provenienz — `vendor/` (OS-9/68K IOMan, Development-Variante)

## Quelldatei

| Feld | Wert |
|---|---|
| Pfad | `MWOS/OS9/68000/CMDS/BOOTOBJS/ioman_DEV` |
| SHA-256 | `0b778f6b1e67ea3d393ed447948639776403fd97c5ef21f6660546ece6c54a39` |
| Herkunft | Microware/RadiSys OS-9/68K SDK, `68000`-Baum -- IOMan ist,
  anders als der Kernel, nicht CPU-familienspezifisch verzweigt (kein
  `aker*`/`dker*`-Äquivalent pro Prozessorfamilie im SDK), passend dazu,
  dass IOMan keine MMU-/Exception-Vektor-Details des jeweiligen Prozessors
  kennen muss -- die 68000-Variante gilt deshalb auch für den 68030-Zielboard |
| Lizenz | Dieselbe proprietäre, auf internen Gebrauch beschränkte Lizenz
  wie die übrigen bereits committeten `vendor/`-Module aus demselben SDK |

## Extrahierte Datei

| Datei | SHA-256 |
|---|---|
| `ioman_DEV` | `0b778f6b1e67ea3d393ed447948639776403fd97c5ef21f6660546ece6c54a39` |

Unverändert aus dem SDK kopiert (keine Extraktion/Konvertierung nötig).
Nachvollziehen: `shasum -a 256 ioman_DEV` gegen die obige Tabelle prüfen.

**Unabhängige Bestätigung, dass dies exakt das live gebootete Modul ist**
(s. [`../docs/REVERSE_ENGINEERING.md`](../docs/REVERSE_ENGINEERING.md)):
Datei ist 5660 Byte groß; die Live-Vermessung in
`Q9-Flux/docs/OS9_SYSCALL_OWNERSHIP.md` (Abschnitt 1, `mdir -e` auf einem
laufenden Boot-Image) ergab denselben IOMan-Adressbereich `$00E03C`–`$00F658`
= exakt `0x161C` = 5660 Byte. Zusätzlich bestätigt `M$Size` im Modulkopf
selbst denselben Wert -- drei unabhängige Quellen stimmen überein.

**Verwendet in**: [`../docs/REVERSE_ENGINEERING.md`](../docs/REVERSE_ENGINEERING.md)
(vierrundige Ghidra-Disassemblierung) und beim eigenen Kernel-Projekt für die
geplante Einbindung des echten, unveränderten IOMan (s.
`Q9-OS/src/kernel/`-Meilensteine zum Thema "IOMan-Einbindung").

**Erstellt**: 2026-08-31 (nachträglich -- die Datei selbst liegt seit
2026-08-12 im Repo, diese PROVENANCE.md wurde erst beim ersten echten
Boot-Test-Einsatz nachgetragen, s. Q9 provenance convention).
