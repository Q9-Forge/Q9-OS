# vendor/

Original-Kernelmodule aus dem Microware/RadiSys OS-9/68K SDK
(`MWOS/OS9/<CPU>/CMDS/BOOTOBJS/`), unverändert übernommen — proprietäre
Microware-Binaries, keine eigene Portierung. Referenzmaterial für den
eigenen Kernel-Nachbau, siehe [`../docs/KERNEL.md`](../docs/KERNEL.md).

## Struktur

Ein Unterverzeichnis je CPU-Familie, wie im SDK selbst:

- `68000/` — 68000/68010/68070
- `68020/` — 68020 **und** 68030 (das SDK legt beide in denselben
  `68020`-Baum, da sich Kernel-seitig kaum etwas unterscheidet außer der
  PMMU-Unterstützung; **CB030/Q9 nutzt `aker030*`/`dker030*`**)
- `68040/`
- `68060/`
- `CPU32/` — CPU32-Kerne (349, generisches `c32`)

## Namenskonvention

`[a|d]ker<CPU-Suffix>[s|b]`

| Präfix/Suffix | Bedeutung |
|---|---|
| `aker*` | **Atomic Kernel** — schlanke Variante für eingebettete Systeme. Kein User-State-Debugging (`F$DFork`/`F$DExec`/`F$DExit`), keine Multi-User-Schutzmechanismen, keine MMU-basierte Speicherschutz, keine Modul/User-ID-Zugriffsprüfung. |
| `dker*` | **Development Kernel** — vollständige Variante mit Debugging- und Schutzfunktionen, für Multi-User-Umgebungen. |
| `*s` | **Standard-Allocator** — klassischer OS-9-Speicherallokator, 16-Byte-Auflösung. |
| `*b` | **Buddy-Allocator** — Binary-Buddy-Algorithmus (Zweierpotenz-Blöcke), deterministischer, aber speicherineffizienter; typisch für Atomic-Kernel/Echtzeit. |

Passendes IOMan muss zum gewählten Kernel passen (`ioman_ATOM` zu
`aker*`, `ioman_DEV` zu `dker*` — nicht Teil dieses Verzeichnisses, siehe
SDK `CMDS/BOOTOBJS/ioman_*`).

**Quelle**: `MWOS/OS9/<CPU>/CMDS/BOOTOBJS/`, Microware/RadiSys OS-9/68K
SDK v3.2.4. Details siehe `OS-9 for 68K Processors Technical Manual`
(`68k_tech.pdf`) und `OS-9 for 68K Processors BLS Reference`
(`68k_bls.pdf`).
