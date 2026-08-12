# Syscall → Modul: vollständige Zuordnung

Ziel dieser Recherche (ursprünglich an eine frühere Sitzung vergeben): eine
Liste, welches OS-9/68K-Systemmodul jeden `F$`/`I$`-Systemaufruf tatsächlich
bedient. Diese Datei führt zwei bereits unabhängig entstandene Recherchen
zusammen und schließt die Lücke zwischen ihnen per reinem Adressvergleich —
**keine neue Disassemblierung nötig**, nur Arithmetik über bereits
verifizierte Daten.

## Quellen

1. **Live-Modul-Adressbereiche** — `mdir -e` auf einem laufenden,
   gebooteten Q9-Flux-Emulator-Image (Q9-Forge/Q9-Flux, echtes
   OS-9/68K-Boot, nicht Q9s eigene Reimplementierung), dokumentiert in
   [`Q9-Flux/docs/OS9_SYSCALL_OWNERSHIP.md`](../../Q9-Flux/docs/OS9_SYSCALL_OWNERSHIP.md),
   Abschnitt 1:

   | Modul | Start | Ende |
   |---|---|---|
   | Kernel | `$007100` | `$00E03C` |
   | IOMan | `$00E03C` | `$00F658` |
   | init | `$00F658` | `$00F7A6` |
   | SysCache | `$00F7A6` | `$00F93C` |
   | SSM | `$00F93C` | `$0100B0` |

2. **Vollständige Syscall-Adresstabelle** (`D_SysDis`/`D_UsrDis`) — per
   Ghidra-Disassemblierung des Kernel-Moduls `dker030s` und Laufzeit-
   Verifikation über einen physischen RAM-Dump desselben Boot-Images,
   dokumentiert in
   [`kernel/docs/REVERSE_ENGINEERING.md`](kernel/docs/REVERSE_ENGINEERING.md),
   Abschnitt "Komplette Syscall-Tabelle (`D_SysDis`/`D_UsrDis`) namentlich
   zugeordnet" — physische Zieladresse für **jeden** der ~90 im
   Kernel-Build definierten Callcodes, getrennt nach Supervisor-Aufruf
   (`D_SysDis`) und User-Aufruf (`D_UsrDis`).

Beide Quellen stammen vom **selben Boot-Image**, sind also direkt
vergleichbar.

## Methode

Für jeden Callcode: Zieladresse aus Quelle 2 gegen die Adressbereiche aus
Quelle 1 geprüft (`lo <= Adresse < hi`) → Modulname. Adressen, die dem
gemeinsamen Fehler-Stub (`0x8480`, "ungültiger Syscall") entsprechen, gelten
als "nicht registriert" statt einem Modul zugeordnet.

## Ergebnis

**61 Kernel · 24 IOMan · 6 SSM · 1 SysCache · 4 nicht registriert · 1
Sonderfall (`F$MBuf`)** — Summe 97, deckt sich mit der aus `funcs.h`
bekannten Gesamtzahl aller definierten OS-9-Callcodes
(vgl. `Q9-Flux/docs/SYSCALL_ROADMAP.md`: "97 Calls insgesamt: 80× F$, 17×
I$" — dort allerdings über Q9s eigene Reimplementierung, nicht über das
reale OS-9, ein anderer Kontext, nur die Gesamtzahl ist ein nützlicher
Quer-Check).

**Deckt sich vollständig mit den ~12 im Live-Trace
(`OS9_SYSCALL_OWNERSHIP.md`) stichprobenhaft geprüften Fällen** — kein
Widerspruch. Diese Auflösung erweitert die Live-Stichprobe (~40 tatsächlich
ausgelöste Aufrufe) auf **alle** ~90 im Kernel-Build definierten Codes,
unabhängig davon, ob sie im Testlauf tatsächlich aufgerufen wurden.

<!-- BEGIN GENERATED TABLE -->
### Kernel (61 Codes)

| Code | Name | D_SysDis-Modul | D_UsrDis-Modul |
|---|---|---|---|
| `0x00` | F$Link | Kernel | Kernel |
| `0x02` | F$UnLink | Kernel | Kernel |
| `0x03` | F$Fork | Kernel | Kernel |
| `0x04` | F$Wait | Kernel | Kernel |
| `0x05` | F$Chain | Kernel | Kernel |
| `0x06` | F$Exit | Kernel | Kernel |
| `0x07` | F$Mem | Kernel | Kernel |
| `0x08` | F$Send | Kernel | Kernel |
| `0x09` | F$Icpt | Kernel | Kernel |
| `0x0a` | F$Sleep | Kernel | Kernel |
| `0x0c` | F$ID | Kernel | Kernel |
| `0x0d` | F$SPrior | Kernel | Kernel |
| `0x0e` | F$STrap | Kernel | Kernel |
| `0x10` | F$PrsNam | Kernel | Kernel |
| `0x11` | F$CmpNam | Kernel | Kernel |
| `0x15` | F$Time | Kernel | Kernel |
| `0x16` | F$STime | Kernel | Kernel |
| `0x17` | F$CRC | Kernel | Kernel |
| `0x18` | F$GPrDsc | Kernel | Kernel |
| `0x19` | F$GBlkMp | Kernel | Kernel |
| `0x1a` | F$GModDr | Kernel | Kernel |
| `0x1b` | F$CpyMem | Kernel | Kernel |
| `0x1c` | F$SUser | Kernel | Kernel |
| `0x1d` | F$UnLoad | Kernel | Kernel |
| `0x1e` | F$RTE | Kernel | Kernel |
| `0x1f` | F$GPrDBT | Kernel | Kernel |
| `0x20` | F$Julian | Kernel | Kernel |
| `0x21` | F$TLink | — (nicht registriert) | Kernel |
| `0x22` | F$DFork | Kernel | Kernel |
| `0x23` | F$DExec | Kernel | Kernel |
| `0x24` | F$DExit | Kernel | Kernel |
| `0x25` | F$DatMod | Kernel | Kernel |
| `0x26` | F$SetCRC | Kernel | Kernel |
| `0x27` | F$SetSys | Kernel | Kernel |
| `0x28` | F$SRqMem | Kernel | Kernel |
| `0x29` | F$SRtMem | Kernel | Kernel |
| `0x2a` | F$IRQ | Kernel | — (nur Supervisor) |
| `0x2c` | F$AProc | Kernel | — (nur Supervisor) |
| `0x2d` | F$NProc | Kernel | — (nur Supervisor) |
| `0x2e` | F$VModul | Kernel | — (nur Supervisor) |
| `0x2f` | F$FindPD | Kernel | — (nur Supervisor) |
| `0x30` | F$AllPD | Kernel | — (nur Supervisor) |
| `0x31` | F$RetPD | Kernel | — (nur Supervisor) |
| `0x32` | F$SSvc | Kernel | — (nur Supervisor) |
| `0x37` | F$GProcP | Kernel | Kernel |
| `0x38` | F$Move | Kernel | Kernel |
| `0x4b` | F$AllPrc | Kernel | — (nur Supervisor) |
| `0x4c` | F$DelPrc | Kernel | — (nur Supervisor) |
| `0x4e` | F$FModul | Kernel | — (nur Supervisor) |
| `0x52` | F$SysDbg | Kernel | Kernel |
| `0x53` | F$Event | Kernel | Kernel |
| `0x54` | F$Gregor | Kernel | Kernel |
| `0x55` | F$SysID | Kernel | Kernel |
| `0x56` | F$Alarm | Kernel | Kernel |
| `0x57` | F$SigMask | Kernel | Kernel |
| `0x59` | F$UAcct | Kernel | Kernel |
| `0x5c` | F$SRqCMem | Kernel | Kernel |
| `0x60` | F$Trans | Kernel | Kernel |
| `0x61` | F$FIRQ | Kernel | — (nur Supervisor) |
| `0x62` | F$Sema | Kernel | Kernel |
| `0x63` | F$SigReset | Kernel | Kernel |

### IOMan (24 Codes)

| Code | Name | D_SysDis-Modul | D_UsrDis-Modul |
|---|---|---|---|
| `0x01` | F$Load | IOMan | IOMan |
| `0x0f` | F$PErr | IOMan | IOMan |
| `0x12` | F$SchBit | IOMan | IOMan |
| `0x13` | F$AllBit | IOMan | IOMan |
| `0x14` | F$DelBit | IOMan | IOMan |
| `0x2b` | F$IOQu | IOMan | — (nur Supervisor) |
| `0x33` | F$IODel | IOMan | — (nur Supervisor) |
| `0x80` | I$Attach | IOMan | IOMan |
| `0x81` | I$Detach | IOMan | IOMan |
| `0x82` | I$Dup | IOMan | IOMan |
| `0x83` | I$Create | IOMan | IOMan |
| `0x84` | I$Open | IOMan | IOMan |
| `0x85` | I$MakDir | IOMan | IOMan |
| `0x86` | I$ChgDir | IOMan | IOMan |
| `0x87` | I$Delete | IOMan | IOMan |
| `0x88` | I$Seek | IOMan | IOMan |
| `0x89` | I$Read | IOMan | IOMan |
| `0x8a` | I$Write | IOMan | IOMan |
| `0x8b` | I$ReadLn | IOMan | IOMan |
| `0x8c` | I$WritLn | IOMan | IOMan |
| `0x8d` | I$GetStt | IOMan | IOMan |
| `0x8e` | I$SetStt | IOMan | IOMan |
| `0x8f` | I$Close | IOMan | IOMan |
| `0x92` | I$SGetSt | IOMan | IOMan |

### SysCache (1 Code)

| Code | Name | D_SysDis-Modul | D_UsrDis-Modul |
|---|---|---|---|
| `0x5a` | F$CCtl | SysCache | SysCache |

### SSM (6 Codes)

| Code | Name | D_SysDis-Modul | D_UsrDis-Modul | Bemerkung |
|---|---|---|---|---|
| `0x3a` | F$Permit | SSM | SSM | |
| `0x3b` | F$Protect | SSM | SSM | |
| `0x3f` | F$AllTsk | SSM | — (nur Supervisor) | |
| `0x40` | F$DelTsk | SSM | Kernel | Supervisor- und User-Variante liegen in unterschiedlichen Modulen — plausibel: User-Aufruf ruft einen kernel-internen Trampolin, der erst intern zu SSM weiterspringt |
| `0x58` | F$ChkMem | SSM | Kernel | dieselbe Asymmetrie wie F$DelTsk; passt zur bereits in `OS9_SYSCALL_OWNERSHIP.md` beobachteten Live-Klassifizierung (kernel=1/1 UND ssm=1/1 in derselben Aufrufsequenz) |
| `0x5b` | F$GSPUMp | SSM | SSM | |

### Nicht registriert in diesem Kernel-Build (4 Codes)

| Code | Name |
|---|---|
| `0x0b` | F$SSpd |
| `0x39` | F$AllRAM |
| `0x5d` | F$POSK |
| `0x5e` | F$Panic |

### Sonderfall

| Code | Name | Bemerkung |
|---|---|---|
| `0x5f` | F$MBuf | Supervisor-Adresse `0x00EE3D08` liegt außerhalb aller bekannten Modul-/RAM-Bereiche dieser Konfiguration — ungeklärt, s. `kernel/docs/REVERSE_ENGINEERING.md` |
<!-- END GENERATED TABLE -->

## Caveats

- Basis ist **ein** Boot-Image/Kernel-Build (`dker030s`, Development-Kernel,
  Standard-Allocator). Andere Kernel-Varianten (`aker*`, Buddy-Allocator,
  andere CPU-Familien) haben andere Modulgrößen/-adressen — diese Tabelle
  gilt für **dieses** Image, nicht universell für "OS-9/68K".
- `F$DelTsk`/`F$ChkMem` zeigen unterschiedliche Module je nach Supervisor-/
  User-Tabelle — die Tabelle oben nennt beide, statt sich auf eines
  festzulegen. Siehe `OS9_SYSCALL_OWNERSHIP.md` Abschnitt 3 zur
  MMU-Adressübersetzungs-Hypothese, warum SSM bei scheinbar unbeteiligten
  Aufrufen mitläuft.
- init-Modul: keine der ~90 Callcode-Adressen fiel in dessen Bereich — passt
  dazu, dass `init` laut Live-Boot-Reihenfolge ein einmaliges
  Bootstrap-Modul ist, kein dauerhafter Syscall-Diensteanbieter.

## Nächste Schritte (offen)

- IOMan intern verstehen (wie dispatcht IOMan seine 24 Aufrufe auf
  Datei-Manager/Treiber?) — Runde 1 läuft, s.
  [`ioman/docs/REVERSE_ENGINEERING.md`](ioman/docs/REVERSE_ENGINEERING.md).
  Für die reine Modul-Zuordnung (diese Datei) nicht mehr nötig.
- SysCache/SSM sind bisher nur als Adressbereich bekannt, nicht
  disassembliert — bei Bedarf gleiches Vorgehen wie bei IOMan
  (`modules/syscache/`, `modules/ssm/`, aktuell nur als leere
  Platzhalterverzeichnisse angelegt).
- `init`-Modul (`$00F658`–`$00F7A6`, 328 Byte) ist namentlich bekannt, aber
  fachlich nicht untersucht.

**Erstellt**: 2026-08-12
