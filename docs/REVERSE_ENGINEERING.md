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

## Fund: Trap-/Exception-Tabellen-Initialisierung und Dispatch-Ziele (großer Meilenstein)

**Frage beantwortet: "Wo werden die Trap-Handler initialisiert?"**

Die riesige Init-Funktion `FUN_000067a0` (`0x67a0`–`0x6de1`, ~1600 Byte,
noch nicht umbenannt) ist der zentrale Kernel-Bootstrap. Darin, bei
`0x68b4`–`0x68ec`:

```
movea.l D7,A0
move.l  #0x400,(0x4,A0)   ; Allokations-Deskriptor: Größe
movec   VBR,D1
move.l  D1,(0x0,A0)
...
move.l  (0x1c,SP),D0
bsr.w   0x00004978        ; Speicher-Allokator (moveq #0x10,D0; ...
                           ;  #0x100 -> vermutlich Systempool-Request)
movea.l (0x34,SP),A0       ; A0 = frisch allozierter Block
move.l  A0,(0x68,A6)       ; *** Schreibzugriff auf D_ExcJmp! ***
```

Das ist der einzige **Schreibzugriff** auf Offset `0x68` (`D_ExcJmp`) im
gesamten Modul (alle anderen Zugriffe auf `0x68,A6` sind Lesezugriffe,
siehe oben) — hier wird also die 256-Eintrags-Sprungtabelle (10 Byte pro
Eintrag = 2560 Byte) zur Boot-Zeit alloziert und ihr Zeiger im
System-Global-Bereich hinterlegt.

Direkt danach (`0x68f0`–`0x6912`) füllt eine Schleife die Tabelle aus
einer **kompakt kodierten Quelltabelle im Modul selbst** bei `0x3802`
(Format: Wortpaare `(count, offset)`, `offset` ist ein **vorzeichen-
behafteter** 16-Bit-PC-relativer Wert, Basis ist die Tabellenadresse
`0x3802` selbst — `Ziel = 0x3802 + (int16)offset`). `D2` (Eintrags-
zähler) startet bei `2` (Einträge 0/1 offenbar anderswo vorbelegt) und
muss am Ende exakt `0x100` (256) erreichen, sonst Panic
(`bsr 0x7f6`, generische Fehlerroutine, sichert alle Register und
zweigt weiter — Details noch nicht untersucht).

**Verifiziert per Skript** (`scripts/ResolveDispatchTargets.java` im
Ghidra-Projekt): Die Gruppengrößen entsprechen **exakt** der
MC68030-Standard-Exception-Vektortabelle:

| Vektoren | Anzahl | CPU-Bedeutung | Ziel-Adresse |
|---|---|---|---|
| 2–3 | 2 | Bus/Address Error | `0x888` |
| 4–8 | 5 | Illegal Instr, Zero Div, CHK, TRAPV, Priv. Violation | `0x8d0` |
| 9 | 1 | Trace | `0xba4` |
| 10–14 | 5 | Line-A/F-Emulator, reserviert, Coproc, Format Error | `0x8d0` |
| 15 | 1 | Uninitialized Interrupt | `0x472` |
| 16–23 | 8 | reserviert | `0x8d0` |
| 24 | 1 | Spurious Interrupt | `0x452` |
| 25–31 | 7 | Interrupt-Autovektoren Level 1–7 | `0x180` |
| **32** | **1** | **TRAP #0 — OS-9-Systemaufruf-Trap** | **`0x488`** |
| 33–47 | 15 | TRAP #1–15 | `0x5d0` |
| 48–54 | 7 | FPU-Exceptions (Branch/Unord., Inexact, DivZero, Underflow, Operand, Overflow, ...) | `0x8d0` |
| 55–56 | 2 | FP Unimpl. Datatype, MMU Config Error | `0x8d0` |
| 57–63 | 7 | MMU-Fehler + reserviert | `0x8d0` |
| 64–255 | 192 | User-Defined Vectors | `0x180` |

**Bedeutung:** Der bisher als "unerreicht, aber echter Code" beschriebene
40%-Block (`0x180`–`0xba4`, inkl. des schon bekannten Prolog-reichen
Bereichs `0x888`–`0xc73`) ist damit vollständig erklärt — er wird nicht
über direkte Sprünge im Modul erreicht, sondern ausschließlich über
diese zur Boot-Zeit aufgebaute Dispatch-Tabelle. Das löst gleichzeitig
den in "Nächste Schritte" (alte Fassung) offenen Punkt zum `0xb04`-
FPSP-Handler: er hängt vermutlich unter dem `0x8d0`-Sammel-Handler für
FPU-Exceptions (Vektoren 48–54).

**Wichtigster neuer Fixpunkt: `0x488` (TRAP #0) ist der OS-9-Syscall-
Dispatcher** — hier landet jeder `F$`-Aufruf mit Funktionscode in `D0`.
Das ist der bislang wertvollste bekannte Einstiegspunkt, um die
eigentliche Betriebssystem-Aufruftabelle zu finden (analog zu
`D_ExcJmp`, vermutlich existiert ein `D_SysCall`-artiges Feld im
System-Global-Bereich, oder `0x488` indiziert direkt in eine eigene
Tabelle im Modul).

Skripte (im Ghidra-Projekt, nicht im Git-Repo):
`scripts/FindTrapInit.java` (MOVEC-VBR-Suche + Offset-0x68-Zugriffe),
`scripts/DumpExcJmpInit.java` (Volldump der Init-Funktion),
`scripts/DumpDispatchTable.java` + `scripts/ResolveDispatchTargets.java`
(Quelltabelle bei `0x3802` auflösen und gegen Funktionen matchen).

**Follow-up:** Alle 8 Dispatch-Ziele wurden per
`scripts/FollowDispatchTargets.java` (`createFunction` je Zieladresse,
kontrollflussbasiert statt linear) tatsächlich disassembliert und als
Funktionen angelegt: `Q9_disp_180`, `Q9_disp_452` (`0x472` liegt im
selben Funktionskörper, wurde mitdiskutiert), `Q9_disp_488`,
`Q9_disp_5d0`, `Q9_disp_888`, `Q9_disp_8d0`, `Q9_disp_ba4`. Ergebnis:
**Code-Abdeckung springt von 55 % auf 78 %**, undefiniert von 40 % auf
**17 %** (nur noch 5102 Byte, verteilt auf viele kleinere Blöcke statt
einem großen zusammenhängenden — vermutlich zum großen Teil lokale
Datentabellen/Strings innerhalb der neu erschlossenen Funktionen, nicht
mehr ein separater unerreichter Codebereich). Größte verbleibende
Lücken (Kandidaten für die nächste Sichtung):
`0x22d4`–`0x24d7` (516 B), `0x3dce`–`0x4039` (620 B),
`0x1e36`–`0x1fff` (458 B), `0x36ec`–`0x3801` (278 B, direkt vor der
bekannten Dispatch-Quelltabelle bei `0x3802`), `0x3816`–`0x3983`
(366 B, direkt danach — evtl. weitere Tabellen desselben Systems),
`0x4824`–`0x4937` (276 B).

## Fund: `Q9_disp_488` — der TRAP-#0-Syscall-Dispatcher (Volltext gelesen)

Vollständig gelesen und verstanden (`0x488`–`0x5c9`, 978 Byte). Bestätigt
das klassische OS-9-Aufrufschema:

- **Inline-Funktionscode**: Direkt nach der `TRAP #0`-Instruktion steht
  im Code ein Wort mit der Funktionsnummer (0–255). Der Handler liest es
  über den geretteten PC auf dem Exception-Frame (`(0x42,SP)`), zieht es
  in `D7` und erhöht den geretteten PC um 2, damit `RTE` hinter die
  Funktionsnummer zurückspringt. Funktionsnummer `≥ 0x100` ist unmöglich
  (nur ein Byte praktisch genutzt, aber als Word geprüft) → Fehlerpfad,
  Fehlercode `0xD0` wird gesetzt.
- **Zwei parallele Syscall-Tabellen** im System-Global-Bereich:
  `(0x3a4,A6)` und `(0x3a8,A6)` (beide zur Boot-Zeit alloziert, siehe
  Init-Funktion `0x6ac0`ff., exakt `0x800` (2048) Byte auseinander).
  Welche Tabelle verwendet wird, hängt von Bit 5 des geretteten
  Statusworts `(0x40,SP)` ab: Bit gesetzt (Aufruf erfolgte bereits im
  Supervisor-/verschachtelten Zustand) → Tabelle bei `0x3a4`;
  Bit gelöscht (normaler Aufruf aus dem User-Zustand) → Tabelle bei
  `0x3a8`. Beide Tabellen sind selbst wieder in zwei `0x400`-Byte-
  Hälften unterteilt (256 Einträge × 4 Byte = `0x400`) — vermutlich
  Haupt-Handler-Zeiger-Array + eine parallele Zusatzdaten-Array pro
  Funktionsnummer (Zweck noch offen, wird über `(0x400,A3)` nach der
  Indizierung nachgeladen).
- **Dispatch-Trampolin** (klassisches 68k-Idiom): `PEA` legt die
  Rücksprungadresse (zurück in den Epilog bei `0x518`) auf den Stack,
  danach wird die eigentliche Handler-Adresse aus der Tabelle ebenfalls
  gepusht, ein `RTS` "springt" dann in den Handler; der Handler kehrt
  über sein eigenes `RTS` in den Epilog zurück (keine echte
  Unterprogrammverschachtelung nötig).
- **Stack-Kanarienvogel**: Magischer Wert `0x4A696D69` = ASCII `"Jimi"`
  (vermutlich eine Anspielung auf Jimi Hendrix, Microware-Insider-Scherz)
  wird nach Rückkehr des Handlers an einer prozesslokalen Stack-Guard-
  Position geprüft (`cmpi.l #0x4a696d69,...`); bei Mismatch Sprung in
  eine Fehlerroutine (`bsr 0x32c8`, vermutlich Stack-Overflow-Panic).
- **Epilog-Scheduling-Check**: Falls der Aufruf nicht verschachtelt war
  (`(0x3ac,A4)`-Zähler = 0) und ein Prozessflag (Bit 5 in `(0x1c,A4)`)
  gesetzt ist, wird vor der Rückkehr `bsr 0x3a06` aufgerufen — starker
  Kandidat für den **Preemption-/Signal-Zustellungspunkt** direkt vor
  der Rückkehr zum User-Code.

Noch offen: Inhalt der zweiten `0x400`-Byte-Tabellenhälfte (Zweck),
was genau `0x3a06` (Scheduling-Check) und `0x32c8`
(Stack-Overflow-Handler) tun, und ob die beiden Tabellen `0x3a4`/`0x3a8`
inhaltlich identisch vorbelegt werden oder unterschiedliche
Handler-Sätze enthalten (noch nicht verglichen).

## Fund: `Q9_disp_180` — der IRQ-Dispatcher (Volltext gelesen)

Vollständig gelesen (`0x180`–`0x347`, 456 Byte). Das ist der Sammel-
Handler für Interrupt-Autovektoren (Level 1–7) und alle User-Defined
Vectors (zusammen 199 der 256 Tabelleneinträge, siehe oben).

- **IRQ-Verschachtelungszähler** `(0x8bc,A6)` wird als erstes erhöht,
  am Ende wieder verringert — dasselbe Feld, das `Q9_disp_488`
  (Syscall-Dispatcher) in seinem Epilog abfragt, um zu entscheiden, ob
  gerade "top-level" (nicht aus einem IRQ heraus) zurückgekehrt wird.
  Beide Dispatcher sind also über dieses Feld gekoppelt.
- **Vektorindizierte Handler-Ketten-Tabelle**: Vektorwert `D0` (aus dem
  Exception-Frame, `(0x1a,SP)`) indiziert je nach Bereich in
  `A6+D0+0x384` (Vektoren `< 0x80`) oder `A6+D0-0x5c` (Vektoren
  `≥ 0x80`, physisch *vor* dem System-Global-Basiszeiger liegend —
  vermutlich fester Bereich am unteren Speicherende). Jeder Eintrag ist
  der Kopf einer **verketteten Liste von Interrupt-Handler-
  Deskriptoren** `{ Handler-Funktion(A0), Kontext(A2), nächster
  Deskriptor(A3) }`. Aufruf per `JSR (A0)`; Rückgabe über Carry-Flag:
  gesetzt = "nicht meiner, weiter in der Kette" (klassisches OS-9-
  Polling-Schema für mehrere Geräte an einer gemeinsamen IRQ-Leitung),
  gelöscht = "behandelt", Spurious-Zähler `(0x3b,A6)` wird
  zurückgesetzt. Leere Liste bzw. keiner in der Kette meldet
  "behandelt" → Spurious-Interrupt-Zähler wird hochgezählt und bei
  Überlauf die Interrupt-Maske im geretteten Statuswort angepasst
  (Selbstschutz gegen Interrupt-Sturm durch defekte/nicht erkannte
  Hardware).
- **Optionaler Scheduler-Tick-Hook** bei `(0x8c0,A6)`: falls gesetzt,
  wird er mit temporär abgesenkter Interrupt-Maske aufgerufen (eigene
  Supervisor-Stack-Umschaltung über `MSP`/`movec`) — Kandidat für einen
  periodischen Scheduler-Tick (z. B. Zeitscheiben-Ablauf).
- **Reschedule-Aufruf**: Ist die IRQ-Verschachtelung wieder bei 0 und
  ein Prozessflag (Bit 5 in `(0x1c,A3)`, `A3 = D_Proc`) gesetzt, wird
  `bsr 0x183a` aufgerufen — **starker Kandidat für den eigentlichen
  Prozess-Dispatcher/Scheduler** (noch nicht gelesen).
- **Signal-Zustellungs-Pfade** (`0x2e4`ff., `0x300`ff.): bauen bzw.
  verändern den Exception-Rückkehr-Frame auf dem Stack (u. a. wird ein
  Format-`$3`/Throwback-Frame mit `MOVEC A2,MSP` konstruiert) — passt
  zum OS-9-Mechanismus, ein Signal in den unterbrochenen Prozess
  "einzuschleusen", noch nicht im Detail verifiziert.

Nächster logischer Ansatzpunkt: `0x183a` (Scheduler-Verdacht).

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

1. Inhaltlich verstehen, **was** `Q9_disp_488` (TRAP #0 / Syscall-
   Dispatcher) tatsächlich tut — bisher nur als Funktion angelegt
   (978 Byte), noch nicht gelesen/analysiert. Höchste Priorität, da er
   vermutlich auf die eigentliche `F$`-Aufruftabelle verweist.
2. Die übrigen neu erschlossenen Dispatch-Funktionen inhaltlich lesen:
   `Q9_disp_180` (Default-/User-Vector-Handler, 456 B, größter Nutzen
   wegen 192+7 zugeordneter Vektoren), `Q9_disp_452` (76 B, inkl.
   `0x472`), `Q9_disp_5d0` (TRAP #1–15, 880 B), `Q9_disp_8d0`
   (Sammel-Handler für CPU-/FPU-/MMU-Exceptions, 1258 B — vermutlich
   hier auch der schon vermutete FPSP-Einstieg bei `0xb04`),
   `Q9_disp_888` (1354 B), `Q9_disp_ba4` (698 B).
3. Verbleibende undefinierte Blöcke (siehe Liste oben, insgesamt nur
   noch 17 %/5102 B) einzeln prüfen — vermutlich überwiegend Daten-
   tabellen, aber nicht blind annehmen.
4. Allocator-Routine `0x4978` verstehen (liefert den Speicher für
   `D_ExcJmp`, `moveq #0x10,D0` / Größe `0x100` deuten auf einen
   generischen Systempool-Request hin — evtl. `F$SRqMem`-Analogon).
5. Panic-/Fehlerroutine `0x7f6` (aufgerufen bei Tabellen-Inkonsistenzen
   und diversen Bound-Checks) verstehen — wichtig für Robustheits-
   Annahmen beim eigenen Nachbau.
6. Restliche Sub-Modi von EA-Mode 7 (`0xb8e`+) vollständig
   disassemblieren.
7. Weitere `PLATZHALTER`-Felder in `q9sysglob.a`/`.h` einzeln per
   Disassemblierung verifizieren (nicht blind übernehmen).
8. Sobald ein Bereich vollständig verstanden ist: als eigene `.s`-Quelle
   nachbauen, mit `vasm`/echtem `r68` assemblieren, Bytes gegen das
   Original diffen (siehe Zieldefinition oben).
