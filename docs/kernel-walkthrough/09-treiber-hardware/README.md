# Thema 09: Treiber-Hardware-Zugriff — wie kommt RBF/IOMan wirklich an die Platte?

Thema 08 hatte gezeigt, dass RBF den eigentlichen Zugriff komplett an den
Treiber delegiert (68K: mehrere interne Hilfsroutinen; x86:
`Q9X_rbf_driver_dispatch`, indirekter Aufruf). Dieses Thema geht den
letzten Schritt: was macht der Treiber **selbst**, wenn er die Hardware
anspricht? Komplett neues Thema, keine Vorarbeit vorhanden.

## Label-Konvention

- **68K**: `Q9_cfide_*`, Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-cfide`, Skript `../../../modules/cfide-driver/ghidra_scripts/DumpReadWriteHandlers.java`.
- **x86**: `Q9X_scllio_*`/`Q9X_*`, neues Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-scllio`, Skript `../../../modules/os9000-x86/ghidra_scripts/scllio_AnalyzeAndDump.java`.

## Die zentrale Erkenntnis: 68K greift direkt auf I/O-Register zu — x86 tut das (in diesem Modul) nirgends

**68K (`cfide`)**: ein lehrbuchmäßiger ATA/IDE-PIO-Treiber. Alle sechs
Tabellenslots aus `modules/cfide-driver/docs/FINDINGS.md` sind jetzt als
echte, unterschiedliche Funktionen bestätigt (vorher nur Positions-
Hypothese):

| Slot | Funktion | Größe |
|---|---|---|
| Init | `Q9_cfide_init` @ `0x4A` | 40 B |
| Read | `Q9_cfide_read` @ `0x72` | 364 B |
| Write | `Q9_cfide_write` @ `0x1DE` | 384 B |
| GetStat | `Q9_cfide_getstat` @ `0x35E` | 400 B |
| SetStat | `Q9_cfide_setstat` @ `0x4EE` | 10 B |
| Term | `Q9_cfide_term` @ `0x4F8` | 4 B |

Read/Write teilen sich dieselbe Grundstruktur (siehe
[`asm-68k.txt`](asm-68k.txt) für die volle Disassemblierung):

1. Status-Register (I/O-Basis+7) pollen, bis `BSY`(Bit7)=0 und `DRDY`(Bit6)=1 — mit Timeout-Zähler (`0x123456` Iterationen), sonst Fehler `0xF6`.
2. Drive/Head-Register (+6) aus dem Path-Descriptor-Byte `(0x81,A1)` aufbauen, LBA-Modus-Bits (`0xE0`) einblenden.
3. LBA-Adresse aus der Blocknummer `D2` per `ror.l #8,D2`-Rotation byteweise in die Register +3/+4/+5 schreiben (Sector-Number/Cylinder-Low/Cylinder-High).
4. Sector-Count-Register (+1 bzw. +2) = 1.
5. Command-Register (+7) = `0x20` (ATA READ SECTOR) bzw. `0x30` (ATA WRITE SECTOR) — **echte, standardkonforme ATA-Kommandobytes**.
6. Erneut pollen, diesmal auf `DRQ`(Bit3), dann 512 Byte byteweise über das Datenregister (+0) transferieren (`move.b`, `dbf`-Schleife, 512 Iterationen).

**Zusatzfund**: `Read` cached Block 0 — bei wiederholtem Lesen von Block 0
wird nicht erneut auf die Hardware zugegriffen, sondern direkt aus einem
512-Byte-Cache-Puffer kopiert (Flag `(0x28,A0)`). `Write` hat **keinen**
Cache-Pfad, schreibt immer echt durch. `GetStat`/`SetStat` unterstützen
nur zwei Codes (`0x44`=Blockgröße zurückgeben, `0x40`=Kapazität, formatiert
über eine kleine BCD→ASCII-Hilfsroutine); jeder andere Code liefert
Fehler **`0xD0`** — bemerkenswert, siehe Vergleichstabelle unten.

**x86 (`scllio`)**: das einzige echte Driver-Typ-Modul (`M$Type`/`m_tylan`
= `0x0E`) im gesamten x86-Fundus außer dem trivialen `null`-Gerät — keine
Live-Extraktion vorhanden, nur die statische Kopie. Eine vollständige
`grep` über alle 308 disassemblierten Zeilen findet **keine einzige**
x86-Port-I/O-Instruktion (`IN`/`OUT`/Varianten) im ganzen Modul. Der
Treiber greift also — soweit in dieser Runde sichtbar — **nicht direkt**
auf Hardware-Ports zu.

Stattdessen enthält das Modul einen zehn Byte kleinen Trampolin
(`FUN_00000870`), der einen **echten Software-Interrupt `INT 0xFF`**
auslöst:

```asm
00000870: PUSH ECX
00000871: MOV ECX,EAX        ; ECX = Zeiger auf Parameterblock
00000873: INT 0xff           ; <<< der x86-Systemaufruf
00000875: MOV EAX,dword ptr [ECX + 0x4]
00000878: POP ECX
00000879: RET
```

Zwei Wrapper-Funktionen bauen dafür einen 24-Byte-Parameterblock
(Callcode-Wort + Flag-Wort + Blockgröße + Nutzdaten) und rufen diesen
Trampolin auf: `FUN_00000800` mit Callcode `0x29` (liest 1/2/4 Byte in
eine vom Aufrufer übergebene Adresse — ein "Get"-artiger Aufruf) und
`FUN_00000880` mit Callcode `0x1D` (schreibt Daten — ein "Set"-artiger
Aufruf). Beide Muster passen zu generischen Kernel-Zugriffsfunktionen
(z. B. "hole/setze Systemglobal"), nicht zu einem Block-I/O-Aufruf im
engeren Sinn.

**Das ist ein wichtiger, bisher fehlender Fund für Thema 06**: dort
konnte im Kernel-Modul selbst kein `INT`/`IRET` gefunden werden, weil die
Dispatch-Quelltabelle zur Laufzeit in Kernel-Globals liegt. Hier, auf der
**aufrufenden** Seite in einem Treiber-Modul, zeigt sich jetzt, wie der
x86-Systemaufruf tatsächlich ausgelöst wird: `INT 0xFF` — das
x86-Gegenstück zu 68Ks `trap #0`. Der eigentliche Handler-Körper (die
`IDT`-Vektor-0xFF-Routine im Kernel) bleibt weiterhin ungefunden, aber der
Auslösemechanismus ist jetzt geklärt.

Die drei indirekten Aufrufe in `Q9X_scllio_entry` (`CALL EDI`/`CALL ECX`/
`CALL ECX`, über Funktionszeiger aus einer zur Laufzeit durchsuchten
Liste, geprüft gegen eine Magic-Konstante `0xA800`) folgen demselben
Muster wie Thema 08s `Q9X_rbf_driver_dispatch` — auch hier wieder
Handler-Suche über eine verkettete Liste statt fester Sprungtabelle.
**Ob dahinter echter Hardware-Zugriff steckt, wurde in dieser Runde nicht
weiterverfolgt** (Zieladressen sind Laufzeitdaten, nicht statisch
auflösbar).

**Hypothese zum Namen** (nicht verifiziert): `scllio` liest sich als
"SCF Line I/O" — OS-9000s SCF (Sequential Character File, das
Terminal-/serielle Subsystem) hat auf 68K ein direktes Gegenstück. Das
würde erklären, warum das Modul so klein ist (2504 Byte, keine
LBA-Register-Logik wie `cfide`) und warum kein Block-Geräte-typischer
Code (Sektorgröße, Zylinder/Kopf-Register) auftaucht — passt eher zu
einem zeichenorientierten (seriellen) Gerät als zu einem Massenspeicher.
Nicht bestätigt, da die Ziel-Handler der indirekten Aufrufe nicht gelesen
wurden.

## Vergleichstabelle

| # | Frage | 68K (`cfide`) | x86 (`scllio`) |
|---|---|---|---|
| 1 | Direkter Hardware-Portzugriff? | **Ja** — Statusregister/Datenregister/Drive-Head-Register über feste Offsets `+0`..`+7` relativ zur I/O-Basisadresse | **Nein** — kein `IN`/`OUT` im ganzen Modul gefunden |
| 2 | Wie löst der Treiber Kernel-Dienste aus? | `trap #0` (zwei Stellen, D0=1 als Aufrufklasse, D1=Callcode `0x50`/`1`, aber nur im GetStat-0x40-Formatierpfad, nicht im Read/Write-Hotpath) | **`INT 0xFF`** über einen generischen Trampolin (`FUN_00000870`), aufgerufen aus "Get"/"Set"-Wrappern mit Callcode `0x29`/`0x1D` — **dies ist der bislang gesuchte x86-Systemaufruf-Auslöser aus Thema 06** |
| 3 | Blockadressierung | LBA aus Blocknummer `D2`, byteweise per `ror.l #8` in drei Register verteilt | Nicht beobachtet (Modul enthält keine sichtbare LBA-Logik — spricht für "kein Block-Gerät", s. Namens-Hypothese oben) |
| 4 | Datentransfer | 512 Byte PIO, byteweise, feste `dbf`-Schleife | Nicht identifiziert — kein PIO-Muster im Hauptteil, evtl. hinter einem der drei indirekten Aufrufe |
| 5 | Fehlercode bei unbekanntem GetStat/SetStat-Code | **`0xD0`** | (nicht anwendbar hier — aber `0xD0` war bereits Thema 08s ungeklärter x86-RBF-Fehlercode; passt zusammen: `0xD0` = "unbekannter Dienstcode", konsistent über beide Architekturen) |
| 6 | Caching | Block 0 wird gecacht (Flag-Byte, 512-Byte-Puffer) | Nicht untersucht |
| 7 | Dispatch-Muster für Sub-Handler | Keins — alle sechs Slots sind eigenständige, feste Funktionen | Wie Thema 08: verkettete Liste + indirekter `CALL` (`CALL EDI`/`CALL ECX`) |

## Für den eigenen Kernel (Ergänzung zu `docs/OWN_KERNEL_INIT_PLAN.md`)

Siehe dortige neue Sektion 3b — kurz zusammengefasst: der eigene x86-Q9-Kernel
braucht einen **IDT-Gate für Vektor `0xFF`** als Systemaufruf-Einsprung,
analog zum 68K-`trap #0`-Handler — das ist jetzt ein konkreter,
architekturübergreifend vergleichbarer Baustein (68K: eine Trap-Vektor-
Nummer über `Q9_D_ExcJmp`-Tabelle, s. Thema 01/06; x86: `INT 0xFF` über
die `IDT`). Der Parameterblock-Aufbau (Callcode-Wort + Flag-Wort +
Größenfeld + Nutzdaten, alles auf dem Stack) ist ein einfaches, klar
nachbaubares Kontrakt-Muster für eigene Systemaufrufe.

## Offene Punkte

- x86: die drei indirekten `CALL`-Ziele (`EDI`/`ECX`/`ECX`) wurden nicht
  aufgelöst — dort könnte der eigentliche Hardware-Zugriff stecken (falls
  `scllio` doch mehr als ein reiner Kernel-Proxy ist).
- x86: `INT 0xFF`s Ziel-Handler (die `IDT`-Vektor-0xFF-Routine im Kernel
  selbst) wurde nicht lokalisiert — nur der Auslöser auf Aufruferseite.
- x86: die Namens-Hypothese "SCF Line I/O" ist nicht verifiziert.
- 68K: die genaue Bedeutung der beiden `trap #0`-Callcodes (`D1=0x50` bzw.
  `D1=1`) wurde nicht identifiziert.
- 68K: Bedeutung von GetStat-Code `0x44` als "SS.Ready" ist eine
  Namenshypothese nach Konvention, nicht durch Doku-Abgleich verifiziert.

## Quellen

- 68K: [`modules/cfide-driver/docs/FINDINGS.md`](../../../modules/cfide-driver/docs/FINDINGS.md), [`asm-68k.txt`](asm-68k.txt), Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-cfide/`, Skript `../../../modules/cfide-driver/ghidra_scripts/DumpReadWriteHandlers.java`
- x86: [`asm-x86.txt`](asm-x86.txt), Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-scllio/`, Skript `../../../modules/os9000-x86/ghidra_scripts/scllio_AnalyzeAndDump.java`
- Vorheriges Thema: [Thema 08](../08-rbf-handler/) (RBF → Treiber-Übergang), [Thema 06](../06-exception-handler/) (offene Frage zum x86-Syscall-Auslöser, hier teilweise beantwortet)

**Erstellt**: 2026-08-15
