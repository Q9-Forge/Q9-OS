# Treiber `cfide` — Grundgerüst analysiert (1462 Byte)

**Quelle:** `MWOS/OS9/68030/PORTS/CB030/CMDS/BOOTOBJS/cfide` — der
CompactFlash-IDE-Treiber der CB030-Portierung (CB030-lokal, nicht Teil des
generischen SDK). M$Type bestätigt `0x0E` (Driver), passend zum
`I$Attach`-Fund in `../ioman/docs/REVERSE_ENGINEERING.md`.

## Ergebnis: klassische 6-Slot-Treiber-Tabelle empirisch bestätigt

Byte-Struktur (Long bei `0x30` = Tabellen-Offset, ab dort 6 Word-Slots):

| Slot | Byte-Offset (von Tabellenbasis) | Wert | Bedeutung (Hypothese, s. u.) |
|---|---|---|---|
| 0 | `+0x00` | `0x4A` | **Init** — bestätigt: exakt der Sprungpunkt, den `I$Attach` anspringt (`D2 = long(A0+0x30)`, kein Zusatzversatz) |
| 1 | `+0x02` | `0x72` | vermutlich **Read** |
| 2 | `+0x04` | `0x1DE` | vermutlich **Write** |
| 3 | `+0x06` | `0x35E` | vermutlich **GetStat** |
| 4 | `+0x08` | `0x4EE` | vermutlich **SetStat** |
| 5 | `+0x0A` | `0x4F8` | **Term** — bestätigt: exakt der Sprungpunkt, den `I$Detach` anspringt (`D2 = 0xA + long(A0+0x30)`) |

Slots 1–4 (Read/Write/GetStat/SetStat) sind aus der Tabellen-**Position**
plausibel, aber NICHT einzeln durch einen `I$`-Aufrufer bestätigt wie
Slots 0 und 5 — IOMans "einfache" `I$`-Aufrufe (`I$Read` etc.) laufen laut
`../ioman/docs/REVERSE_ENGINEERING.md` (Runde 4) über `FUN_000014f8` in
eine **andere** Tabelle (s. `../rbf-filemanager/docs/FINDINGS.md`) — nicht
direkt in diese 6er-Treiber-Tabelle. Die Zuordnung Slot 1–4 bleibt also
eine Namens-Hypothese nach Standard-OS-9-Konvention (Init/Read/Write/
GetStat/SetStat/Term, Technical Manual), nicht live verifiziert.

Modul-Größe 1462 Byte, davon aktuell nur ~150 Byte disassembliert
(Init-Routine bei Slot 0 + eine kleine Hilfsfunktion mit `moveq 0x1,D0`,
`moveq 0x50,D1`, `trap #0x0` — die Bedeutung von `D0`/`D1` hier ist NICHT
der `F$`-Inline-Callcode (der steht laut Kernel-Fund als Wort DIREKT NACH
der `trap`-Instruktion, nicht in einem Register) — noch nicht interpretiert,
kein Rückschluss gezogen). Vollständige Disassemblierung der restlichen
~1300 Byte (Slots 1–4, die eigentliche Hardware-I/O-Logik) ist ein
möglicher nächster Schritt, aber nicht mehr nötig, um die Kernfrage
(Treiber-Tabellen-Struktur) zu beantworten.

**Erstellt**: 2026-08-12
