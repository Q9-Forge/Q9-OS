# File-Manager `rbf` — Tabellen-Struktur bestätigt (9638 Byte)

**Quelle:** `MWOS/OS9/68000/CMDS/BOOTOBJS/rbf` — der RBF-File-Manager
(Random Block File, OS-9s natives Disk-Dateisystem), generisch im SDK,
nicht CB030-spezifisch. M$Type bestätigt `0x0D` (Fmgr), passend zum
`I$Attach`-Fund in `../ioman/docs/REVERSE_ENGINEERING.md`.

## Ergebnis: 13-Slot-Tabelle bestätigt `FUN_000014f8`s Callcode-Indexierung

Byte-Struktur (Long bei `0x30` = `0xA6`, Tabellenbasis; direkter Bytezugriff
per Python, kein Ghidra nötig für diesen Schritt):

| Slot | Offset | Wert | `I$`-Code (Slot + `0x83`) | Name |
|---|---|---|---|---|
| 0 | `0xA6` | `0x003A` | `0x83` | I$Create |
| 1 | `0xA8` | `0x0230` | `0x84` | I$Open |
| 2 | `0xAA` | `0x0336` | `0x85` | I$MakDir |
| 3 | `0xAC` | `0x04B4` | `0x86` | I$ChgDir |
| 4 | `0xAE` | `0x04EA` | `0x87` | I$Delete |
| 5 | `0xB0` | `0x08D8` | `0x88` | I$Seek |
| 6 | `0xB2` | `0x05D0` | `0x89` | I$Read |
| 7 | `0xB4` | `0x0778` | `0x8A` | I$Write |
| 8 | `0xB6` | `0x05CC` | `0x8B` | I$ReadLn |
| 9 | `0xB8` | `0x0756` | `0x8C` | I$WritLn |
| 10 | `0xBA` | `0x090A` | `0x8D` | I$GetStt |
| 11 | `0xBC` | `0x0A6C` | `0x8E` | I$SetStt |
| 12 | `0xBE` | `0x03C0` | `0x8F` | I$Close |

Ab Slot 13 (`0xC0`) folgt kein weiterer plausibler Tabellenwert mehr —
`0x48E7` ist der 68k-Opcode `MOVEM.L {...},-(SP)` (klassischer
Funktionsprolog), d. h. die Tabelle endet exakt nach 13 Einträgen und
direkt danach beginnt echter Code. **Exakte Übereinstimmung mit den 13
`I$`-Callcodes `0x83`–`0x8F`**, die `FUN_000014f8` (IOMan, Runde 4) per
`(Callcode − 0x83) × 2` indiziert — das war dort nur aus dem Callcode-
Bereich vermutet, jetzt am realen File-Manager-Modul bestätigt: **die
Tabelle, die `FUN_000014f8` anspringt, gehört zum File-Manager (RBF),
nicht zum Treiber** (der Treiber `cfide` hat nur 6 Slots für
Init/Read/Write/GetStat/SetStat/Term, s.
[`../cfide-driver/docs/FINDINGS.md`](../cfide-driver/docs/FINDINGS.md)).

## Einordnung der vollen OS-9-Aufrufkette (jetzt lückenlos)

```
TRAP #0 (Gast-Prozess ruft z.B. I$Read auf)
  -> Kernel-Dispatcher Q9_disp_488, D_SysDis/D_UsrDis-Tabelle (Kernel-Fund)
  -> Zieladresse liegt in IOMan (../SYSCALL_MODULE_MAP.md)
  -> IOMan: FUN_000014f8 (gemeinsamer Dispatcher, Runde 4)
     -> Callcode-Index in RBFs 13-Slot-Tabelle (DIESER Fund)
     -> Sprung DIREKT in RBF-Code (kein Rücksprung über IOMan)
  -> RBF liest/schreibt seine Datenstrukturen; für den eigentlichen
     Blockzugriff vermutlich ein weiterer Sprung in cfides 6-Slot-Tabelle
     (Read/Write, Slots 1/2) -- noch nicht einzeln nachverfolgt
  -> cfide spricht die CF-Hardware an
```

## Gegenprobe zu Andreas' Buch-Beobachtung ("File-Manager selbst enthält keine Kernel-Calls") — Ergebnis nuanciert, nicht eindeutig

Byte-Muster-Suche (kein Ghidra, reine Byte-Übereinstimmung gegen die aus
IOMan bereits bestätigte Instruktion `movea.l (0x3a4,A6),A3`, Opcode-Bytes
`26 6e 03 a4`) direkt im RBF-Binary:

- **`(0x3a4,A6)`-Zugriff (Supervisor-Kernel-Tabelle): 15 exakte Treffer**
  im 9638-Byte-Modul, byte-identisch zu IOMans bestätigtem
  Trampolin-Aufruf-Muster.
- **`(0x3a8,A6)`-Zugriff (User-Kernel-Tabelle): 0 Treffer.**

**Einordnung:** RBF referenziert also durchaus denselben Kernel-Trampolin-
Mechanismus wie IOMan (Runde 2) — 15 Stellen sind zu viele für einen
Zufallstreffer in unabhängigen Datenkonstanten. Das widerspricht Andreas'
Buch-Beobachtung nicht zwangsläufig, **relativiert sie aber**: vermutlich
bezog sich die Buch-Aussage auf ein einfacheres/anderes Beispiel, oder
auf die Kern-Dispatch-Logik im engeren Sinn (nicht auf jede interne
Hilfsroutine). Plausible Erklärung: RBF braucht wie IOMan eigene
Systemdienste (z. B. `F$SRqMem` für Puffer-/Cache-Verwaltung, die RBF laut
Technical-Manual-Architektur selbst betreibt) — das sind Kernel-Calls für
RBFs EIGENE Buchhaltung, nicht für die eigentliche Blockgeräte-I/O (die
läuft weiterhin über den Treiber-Sprung, s. o.). Nicht verifiziert: OB
diese 15 Treffer tatsächlich Instruktionen sind (nicht zufällig in
Datenbereichen liegen) und WELCHE der (aus `../SYSCALL_MODULE_MAP.md`
bekannten) Kernel-Funktionen sie aufrufen — das bräuchte einzelnes Lesen
der Fundstellen, noch nicht gemacht.

**Erstellt**: 2026-08-12
