# Kernel-Disassemblierung — Arbeitsstand

Ziel: byte-exakte Rekonstruktion des Original-Kernels als eigenständig
assemblierbaren Quellcode (nicht Dekompilierung nach C — siehe
[`KERNEL.md`](KERNEL.md) für die Architektur-Grundlagen aus dem
offiziellen Handbuch). Werkzeug: Ghidra 12.1.2, headless über
`analyzeHeadless` gesteuert (Skripte + Projekt liegen **nicht** in
diesem Repo, sondern lokal unter
`/Volumes/SSD1TB/projects/Q9-OS-ghidra/` — Ghidra-Projekte sind groß/
binär und gehören nicht ins Git).

## Untersuchtes Modul

[`vendor/68020/dker030s`](../vendor/68020/dker030s) — Development-
Kernel, Standard-Allocator, 68030. Das ist die Variante, die die echte
CB030-Boot-Konfiguration tatsächlich lädt (`MWOS/OS9/68030/PORTS/CB030/
BOOTFILE/diskboot.bl` und `dev.bl` referenzieren explizit
`68020/CMDS/BOOTOBJS/dker030s`).

Ghidra-Setup: Prozessor `68000:BE:32:MC68030`, Raw-Binary-Import bei
Basisadresse 0 (Dateiadressen = Ghidra-Adressen), Einstiegspunkt manuell
auf Dateioffset `0x54` gesetzt (siehe unten), danach automatische
Kontrollfluss-Analyse.

## Modul-Header

Standard-Header (`0x00`–`0x2F`) folgt exakt Table 1-7 aus dem Technical
Manual, Parity-Check verifiziert (XOR aller 24 Words `0x00`–`0x2F`
ergibt `0xFFFF`). Bestätigte Werte: `M$ID=0x4AFC`, `M$Type=12` (Systm),
`M$Lang=1` (Objct/68k), `M$Attr=0xA0` (system-state + reentrant, nicht
sticky), Modulname `"kernel"`.

Typ-spezifische Erweiterung ab `0x30` (im Technical Manual **nicht**
dokumentiert, empirisch aus dem Binary ermittelt):

| Offset | Wert (dker030s) | Interpretation |
|---|---|---|
| `0x30` | `0x00000054` | `M$Exec` — Offset des Einsprungpunkts |
| `0x34` | `0x00000000` | unbekannt, immer 0 beobachtet |
| `0x38`–`0x3F` | 0 | unbekannt |
| `0x40`–`0x43` | `b0bd b0bd` | unbekannt — Musterwert oder Füllwert, wiederholt sich identisch in `aker030s` |
| `0x44`–`0x4B` | `00000001 00000001` | unbekannt |
| `0x4C`–`0x53` | 0 | unbekannt |
| `0x54` | `BRA.W +0x674A` | erste echte Instruktion — springt über den ID-String |

Direkt nach dem Sprungziel-Überspringen folgt ein eingebetteter
Identifikationsstring (kein Teil des Standard-Headers, aber offenbar
Konvention bei Microware-Kerneln): `"68030\0 OS-9/68K Kernel (Dev-Std)
V3.2.0\0Copyright (c) 1999 by Microware ..."`.

Als Ghidra-Struktur angelegt (`OS9_ModuleHeader_Std` +
`OS9_KernelHeader_Ext`), Skript `ApplyModuleHeader.java`.

## Auto-Analyse-Ausgangslage

Kontrollfluss-Verfolgung ab `0x54`: **123 Funktionen**, **55 % des
28476-Byte-Moduls als Code disassembliert**, 3 % als Daten, **40 %
undefiniert**. Die undefinierten Bereiche sind überwiegend **echter,
nicht erreichter Code** (kein Datenmüll) — z. B. enthält der größte
Block (`0x888`–`0xc73`) klare Funktions-Prologe (`MOVEM.L`,
`MOVEC`), ist aber nicht über direkte Sprünge/Referenzen aus dem
bekannten Code erreichbar. Vermutlich separate Exception-Handler-
Cluster, die nur über die CPU-eigene Vektortabelle (VBR-relativ)
erreicht werden, nicht über normale Aufrufe im Modul selbst.

## Fund: System-Global-Zeiger über VBR (grundlegend, gilt vermutlich
   modulweit)

Wiederkehrendes Idiom an mehreren Stellen (`0xac4`, `0xbc4`, `0xc52`,
`0xc62`, `0x454`, `0x47a`, ...):

```
movec   VBR,A6        ; A6 = Basis der CPU-Vektortabelle
movea.l (A6),A6        ; A6 = *(A6)  -- Inhalt von Vektor 0 (Reset-SSP!)
movea.l (0x4c,A6),A4   ; A4 = Zeiger aus Offset 0x4C dieses Bereichs
```

Deckt sich exakt mit dem Technical Manual (Kapitel 2, "System
Initialization"): *"The Reset SSP vector points to the system global
area... Each time an exception occurs, OS-9 uses this vector to find
the base address of system global data."* D.h. OS-9 zweckentfremdet
den Reset-Initial-SSP-Eintrag (Vektor 0 der CPU-Vektortabelle) nicht
als Stackpointer-Wert, sondern als **Zeiger auf den System-Global-
Bereich** (die `D_`-präfigierten Variablen). `A4 = *(A6+0x4C)` danach
ist ein häufig gecachter Zeiger daraus.

**Verifiziert** gegen die Offset-Namen aus dem lizenzierten SDK
(`MWOS/OS9/SRC/DEFS/sysglob.a` — Microware-Copyright, nur zum
Quer-Check der eigenen, unabhängig per Disassemblierung gefundenen
Offsets verwendet, nicht als Quelle kopiert):

| Offset | Name | Bedeutung |
|---|---|---|
| `0x4C` | `D_Proc` | Zeiger auf den aktuellen Prozessdeskriptor |
| `0x58` | `D_FProc` | Prozess, dessen Kontext gerade in den FPU-Registern steht |
| `0x68` | `D_ExcJmp` | **Exception Jump Table Ptr** — die "Sprungtabelle" aus der urspr. Frage; liegt als Zeiger im System-Global-Bereich, wird zur Bootzeit vom Kernel im RAM aufgebaut, nicht statisch im Modul |

Damit ist der FPU/FPSP-Verdacht von oben bestätigt: der Code bei
`0xac4`–`0xaec` liest `D_FProc` (Offset `0x58`, exakt der bei `0xad8`
gelesene Wert) und vergleicht ihn mit dem aktuellen Prozess — klassischer
**Lazy-FPU-Context-Switch**: nur speichern/restaurieren, wenn der
FPU-Registersatz gerade einem ANDEREN Prozess gehört.

`D_ExcJmp` (`0x68`) wäre der nächste konkrete Ansatzpunkt, um die
eigentliche Syscall-/Interrupt-Sprungtabelle im RAM-Layout zu finden —
dafür müsste man den Kernel live im Emulator laufen lassen und den
Speicher an `*(VBR-Inhalt) + 0x68` inspizieren, da diese Tabelle nicht
statisch im Modul-File steht.

## Fund: Adressierungsart-Decoder bei `0xb3a`

Kein Syscall-Dispatch, sondern ein **generischer Effective-Address-
Decoder** für alle 8 68000-Standard-Adressierungsmodi (Bits 3–5 eines
Opcode-Worts). Aufgerufen via
`jsr (0xb3a,PC,D1w*0x1)` bei `0xb1c`, mit vorherigem
`move.w (0xb3a,PC,D1w*0x1),D1w` (2-stufige Indirektion: Tabellenwert
ist ein PC-relativer Wort-Displacement zur jeweiligen Handler-Routine).

Tabelle bei `0xb3a` (8 Worte, direkt gefolgt von den 8 Handlern ab
`0xb4a`):

| Mode | Displacement | Ziel | Semantik | Verhalten |
|---|---|---|---|---|
| 0 Dn direkt | `0x0010` | `0xb4a` | `lea (2,A5,D0w),A1` | Adresse des Register-Slots im Save-Bereich |
| 1 An direkt | `0x0064` | `0xb9e` | `ori #1,CCR` | **Fehler** — kein Speicher-EA für ein Register |
| 2 (An) | `0x001a` | `0xb54` | `movea.l (0x20,A5,D0w),A1` | EA = An |
| 3 (An)+ | `0x0020` | `0xb5a` | EA=An, danach `addq.l #2,(0x20,A5,D0w)` | Post-Increment |
| 4 -(An) | `0x0016` | `0xb50` | `subq.l #2,...` zuerst, dann EA=An | Pre-Decrement |
| 5 (d16,An) | `0x002a` | `0xb64` | EA=An `+ adda.w (A0)+,A1` | Displacement aus Instruktionsstrom (A0) |
| 6 (d8,An,Xn) | `0x0032` | `0xb6c` | liest Extension-Word, Word/Long-Index je nach Bit, addiert 8-Bit-Displacement | volle Brief-Extension-Word-Dekodierung |
| 7 Extended | `0x0054` | `0xb8e` | Sub-Mode-Prüfung (`cmpi.w #4`), `>4` = Fehler | nur Sub-Mode 0 (abs. short) im bisher gedumpten Ausschnitt verifiziert, Rest noch offen |

Fehlerkommunikation an den Aufrufer über das **Carry-Flag** (Mode 1
setzt es, Aufrufer prüft direkt danach `bcs.b` → Fehlerpfad) — passt
exakt zusammen.

**Hypothese (noch nicht verifiziert):** Der Aufrufer bei `0xb04`–`0xb38`
sichert `USP`, ruft den Decoder, schreibt danach einen Wert an die
berechnete Adresse und aktualisiert den gesicherten Instruktionszeiger.
Zusammen mit den an anderer Stelle gefundenen `fsave`/`frestore`/
`fmove`/`fmovem`-Opcodes im selben Modul spricht das für einen
**FPSP-Handler** (Floating-Point Software Package, emuliert fehlende
FPU-Instruktionen) — das Technical Manual listet "FPU/FPSP" explizit
als Customization-Modul (Kapitel 2, System Initialization). Noch offen:
wer ruft `0xb04` auf (vermutlich über die VBR-Vektortabelle, Line-F-
Emulator-Vektor 11), und was genau macht der Rest der Routine nach dem
Decoder-Aufruf.

## Werkzeug-Hinweise (Ghidra headless)

- Java: Homebrew-OpenJDK wird nicht automatisch gefunden —
  `JAVA_HOME=$(brew --prefix openjdk@21)/libexec/openjdk.jdk/Contents/Home`
  vor `analyzeHeadless` setzen.
- `.py`-Skripte funktionieren NICHT ohne expliziten PyGhidra-Start
  (`ghidra was not started with PyGhidra`) — `.java`-Skripte (klassische
  `GhidraScript`-Subklassen) funktionieren dagegen direkt mit
  `-scriptPath`/`-preScript`/`-postScript`, keine Kompilierung von Hand
  nötig.
- **Wichtige Falle:** `Instruction.getMnemonicString()` liefert
  **kleingeschriebene** Mnemonics (`jmp`, `jsr`, `trap`, `movec`, `rte`
  — nicht `JMP`/`JSR`/...). Ein `startsWith("JMP")`-Vergleich läuft
  dadurch ins Leere, ohne Fehlermeldung — genau das ist uns in dieser
  Sitzung passiert und hat eine bereits vorhandene Sprungtabelle
  zunächst verdeckt.
- 68030-Sprachdefinition: `68000:BE:32:MC68030` (nicht `68000:BE:32:
  default`, das ist 68040).

## Eigene System-Global-/Exception-Tabellen-Definition

[`src/q9sysglob.a`](../src/q9sysglob.a) / [`src/q9sysglob.h`](../src/q9sysglob.h)
legen den kompletten System-Global-Bereich (`Q9_D_*`, bis `Q9_D_End` =
`$1000`) und die Exception-Sprungtabelle (`Q9_T_*`, bis `Q9_T_End` =
`$400`) als eigene, umbenannte Struktur an — Grundgerüst aus dem
Technical Manual bzw. der Struktur des lizenzierten SDKs abgeleitet
(nur als Fakten-Check verwendet, keine Übernahme von Microwares
Originaltext), aber mit eigenen Namen/Beschreibungen. Nur 17 Felder
sind bisher per eigener Disassemblierung tatsächlich verifiziert
(`VERIFIZIERT`-Markierung); der Rest ist `PLATZHALTER`/`HANDBUCH` und
muss noch einzeln bestätigt werden, bevor man sich darauf verlässt.

Bemerkenswert: `Q9_T_E1111` (Line-1111/F-Line-Emulator, Vektor 11) fällt
mit `Q9_T_FpUnData` (FP: nicht implementierter Datentyp) zusammen als
wahrscheinlichster Installationsort für unseren `0xb04`-Handler — beide
sind naheliegende Kandidaten für den FPU-Emulations-Einstieg, aber noch
nicht gegeneinander verifiziert.

## Nächste Schritte

1. Aufrufer von `0xb04` finden (VBR-Vektor-Installation im Init-Code
   suchen, vermutlich `Q9_T_E1111` oder `Q9_T_FpUnData`).
2. Restliche Sub-Modi von Mode 7 (`0xb8e`+) vollständig disassemblieren.
3. Weitere `PLATZHALTER`-Felder in `q9sysglob.a`/`.h` einzeln per
   Disassemblierung verifizieren (nicht blind übernehmen).
4. Die übrigen ~40 % unerreichten Code planvoll disassemblieren (nicht
   mehr blind an geratenen Blockgrenzen, sondern von gefundenen
   Exception-Vektoren aus kontrollflussbasiert).
5. Sobald ein Bereich vollständig verstanden ist: als eigene `.s`-Quelle
   nachbauen, mit `vasm`/echtem `r68` assemblieren, Bytes gegen das
   Original diffen (siehe Zieldefinition oben).
