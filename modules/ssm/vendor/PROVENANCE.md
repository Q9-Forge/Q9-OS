# Provenienz — `vendor/` (OS-9/68K SSM851)

## Quelldatei

| Feld | Wert |
|---|---|
| Pfad | `MWOS/OS9/68020/CMDS/BOOTOBJS/ssm851` |
| SHA-256 | `b555473ebe34ff23cbcb61795c22c94bfef69db135c30c753acc3ed5223e1c13` |
| Herkunft | Microware/RadiSys OS-9/68K SDK, `68020`-Baum (gilt laut SDK-Konvention
  auch für 68030 — siehe [`../../../vendor/README.md`](../../../vendor/README.md)) |
| Lizenz | Dieselbe proprietäre, auf internen Gebrauch beschränkte Lizenz
  wie die bereits committeten `vendor/68020/{aker,dker}030*`-Module aus
  demselben SDK-Verzeichnis |

## Extrahierte Datei

| Datei | SHA-256 |
|---|---|
| `ssm851` | `b555473ebe34ff23cbcb61795c22c94bfef69db135c30c753acc3ed5223e1c13` |

Unverändert aus dem SDK kopiert (keine Extraktion/Konvertierung nötig,
anders als bei den x86-Live-Modulen). Nachvollziehen:
`shasum -a 256 ssm851` gegen die obige Tabelle prüfen.

**Verwendet in**: [`../../../docs/kernel-walkthrough/07-ssm-mmu/`](../../../docs/kernel-walkthrough/07-ssm-mmu/README.md)
(MMU-Initialisierung, System Security Module).

**Erstellt**: 2026-08-14
