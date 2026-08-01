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

## Fund: `Q9_scheduler_183a` — Ready-Queue-Einfügeroutine des Schedulers

Vollständig gelesen (`0x183a`–`0x1920`, 230 Byte, linear disassembliert
— Ghidras automatische Funktionsgrenzenerkennung hatte den Körper
fälschlich auf 10 Byte verkürzt, siehe `scripts/DumpLinear183a.java`).
Aufgerufen von `Q9_disp_180` mit `A0 = D_Proc` (aktueller
Prozessdeskriptor). Das ist **keine reine Weiterleitung, sondern die
eigentliche Ready-Queue-Einfüge-Logik** des Schedulers:

- **Prozesszustand als ASCII-Zeichen**: `(0x20,A0)` — `0x61` (`'a'`)
  bedeutet aktiv/laufend. Ist der Prozess schon aktiv, kehrt die Routine
  sofort zurück (nichts einzufügen). Bestätigt das aus dem Technical
  Manual bekannte Zustandscodierungsschema mit lesbaren Buchstaben-Codes.
- **Ready-Queue** ist eine zirkuläre doppelt verkettete Liste mit
  Sentinel-Kopf bei `(0x37c,A6)` (derselbe Bereich, den die Boot-Init-
  Funktion bei `0x6856` als selbstreferenzierenden Kopf anlegt — next
  `(0x30,A0)` und prev `(0x34,A0)` zeigen dort anfangs auf sich selbst).
  Der Prozessdeskriptor selbst ist der Listenknoten (`(0x30,A0)`/
  `(0x34,A0)` = Next/Prev-Zeiger, direkt im Deskriptor).
- **Priority Aging**: Ein globaler Countdown-Zähler `(0x3c4,A6)` wird
  bei jedem Aufruf dekrementiert; erreicht er negativ, wird er auf
  `0x7fff0000` zurückgesetzt und **alle** Warteschlangen-Einträge
  bekommen ihren gespeicherten Sortier-Schlüssel `(0x2e0,A1)` erhöht
  (mit Sättigung/Vorzeichenkorrektur via `not.w` bei Überlauf) — klassische
  Prioritäts-Alterung gegen Verhungern niedrigpriorer Prozesse.
- **Sortier-Schlüssel-Berechnung** für den einzufügenden Prozess:
  - Schneller Sonderfall: falls ein systemweiter Cache-Wert
    `(0x8aa,A6)` gesetzt ist und mit dem Typfeld `(0,A0)` des Prozesses
    übereinstimmt → Schlüssel `= -1` (sortiert ans Ende).
  - Sonst: Priorität `(0x18,A0)` gegen Systemschwellen `(0x8a6,A6)`
    (Maximum) und `(0x8a8,A6)` (zweite Schwelle, vermutlich
    Echtzeit-Grenze) geprüft; ist Bit 7 in `(0x1c,A0)` gesetzt
    (Echtzeit-/Boost-Flag) oder die Priorität über der zweiten
    Schwelle, wird Bit 31 im Schlüssel gesetzt (sortiert die
    Echtzeit-Klasse garantiert vor der normalen Klasse, da der
    Vergleich vorzeichenbehaftet erfolgt); sonst normaler Schlüssel
    `= Aging-Zähler + Priorität`.
  - Sonderfall Priorität `≥ (0x8a6,A6)` bzw. Bit 7 gesetzt: Schlüssel
    `= 0` (sofort an den Anfang, höchste Dringlichkeit).
- **Einfügen**: lineare Suche ab dem Sentinel-Kopf, bis ein Eintrag mit
  größerem Schlüssel gefunden wird (`cmp.l (0x2e0,A1),D2; bhi ...`),
  danach klassisches Doppel-Verkettungs-Splicing.
- Nebenbei: falls der neue Prozess eine höhere Priorität als der
  aktuelle Prozess `D_Proc` hat (`(0x18,A2)` verglichen), wird Bit 5 in
  `(0x1c,A2)` (Prozessflag des *aktuellen* Prozesses) gesetzt — das ist
  vermutlich das **Reschedule-Anforderungs-Flag**, das an mehreren
  anderen Stellen (`Q9_disp_180`, `Q9_disp_488`) bereits als
  Bedingung für einen Kontextwechsel geprüft wurde.

**Noch offen:** Wer ruft diese Routine sonst noch auf (vermutlich jeder
Punkt, der einen Prozess aus dem Schlaf/Wartezustand aufweckt, z. B.
Timer-Ablauf, Signal-Zustellung, I/O-Komplettierung) — bisher nur der
eine bekannte Aufrufer aus `Q9_disp_180`. Die eigentliche
Kontextwechsel-Routine (die den *nächsten* Prozess aus der Queue
entnimmt und den CPU-Kontext umschaltet) ist noch nicht gefunden —
`Q9_scheduler_183a` fügt nur ein, entfernt/dispatcht aber nicht.

## Fund: `0x3140` — Cache-Flush + Sprung in Syscall-Tabellen-Slot 90 (kein direkter Kontextwechsel)

Von `Q9_disp_180`s Reschedule-Pfad aus per `bra.w 0x3140` erreicht
(`0x3140`–`0x31bc`, vollständig gelesen). **Ist nicht die
Kontextwechsel-Routine selbst**, sondern zwei Dinge nacheinander:

1. **Cache-Flush-Schleife** (`0x3150`–`0x3194`): läuft über eine Liste
   von Codezeigern (Basis `(0x2ac,A4)`, Länge `(0x2e8,A4)`, negativ als
   Sentinel für "nichts zu tun"), prüft an jeder Adresse, ob dort noch
   der Platzhalterwert `0x4AFC` (68k-`ILLEGAL`-Opcode, gleichzeitig
   `M$ID` aus dem Modul-Header — hier als "noch nicht gepatcht"-
   Markierung zweckentfremdet) steht, patcht ihn bei Treffer mit einem
   vorbereiteten Wort und **invalidiert die CPU-Datencache-Zeile** an
   dieser Adresse einzeln über `MOVEC CAAR`/`CACR`-Bit-2-Toggle. Klassischer
   68030-Mechanismus für selbstmodifizierenden/frisch geladenen Code
   (z. B. nach dynamischem Modul-Binding).
2. **Trampolin-Sprung in Tabellen-Slot 90** (`0x31a8`–`0x31bc`): lädt
   Tabelle `(0x3a4,A6)` (dieselbe "nested-Syscall"-Tabelle wie in
   `Q9_disp_488`), liest `D0=0x44` (Parameter, kein Tabellenindex),
   pusht `(0x168,A3)` (Primärarray, Slot `0x168/4=90`) als Wert und
   springt per PEA+RTS-Trampolin zu `(0x568,A3)` — das ist exakt
   `0x400+0x168`, also derselbe Slot 90 im **Sekundärarray**
   (Bestätigung der schon bei `Q9_disp_488` vermuteten Zwei-Array-
   Struktur: Primärarray `0x0–0x3FF`, Sekundärarray `0x400–0x7FF`,
   parallel indiziert).

**Zieladresse nicht statisch auflösbar**: Tabelle `(0x3a4,A6)` wird erst
zur Boot-Zeit im RAM befüllt, der eigentliche Slot-90-Handler
(vermutlich der echte Scheduler-/Kontextwechsel-Einstieg) ist per reiner
Modul-Disassemblierung nicht direkt sichtbar (Details zur Befüllung
siehe nächster Abschnitt — **anders als zunächst vermutet keine simple
Zeiger-Tabelle**).

## Fund (Korrektur einer Fehlannahme): Syscall-Tabellen sind KEIN
   statisches Zeiger-Array, sondern werden dynamisch registriert

Ursprüngliche Annahme war, `(0x3a4,A6)`/`(0x3a8,A6)` würden — wie
`D_ExcJmp` — aus einer kompakten Tabelle im Modul entpackt. Das ist
**widerlegt**: In der Boot-Init-Funktion (`0x6b34`–`0x6b54`) wird zwar
mit `bsr 0x10ba` (Signatur `D0=Länge, D1=Ziel, Stack=Quelladresse` —
Memcpy-artig) je `0x400` Byte aus derselben PC-relativen Quelladresse
`0x1380` in beide Primärarrays kopiert — aber diese Quelladresse enthält
**keine Adressliste**, sondern tatsächlichen **Code**:

- `0x1380`–`0x1388`: gemeinsamer Fehler-Stub für "ungültiger Syscall"
  (`move.w #0xd0,D1w; ori #1,CCR; rts` — Fehlercode `0xD0`, exakt der
  in `Q9_disp_488`s Fehlerpfad gesetzte Wert; `ori #1,CCR` setzt das
  Carry-Flag als Fehlersignal an den Aufrufer).
- `0x1390`–`0x13c4`: eine Funktion, die selbst `(0x3a4,A6)` liest und
  in `(0x420,A3)` schreibt (`0x420 = 0x400+0x20`, also Sekundärarray-
  Slot `8`) — sieht nach einer **Syscall-/Handler-Registrierungsfunktion**
  aus (Parameter `D2`, `(0x0,A4)`), die über eine Unterfunktion bei
  `0x13d2` nach Kategorie verzweigt (`D1`, verdoppelt, gegen `0xc` (12)
  geprüft, sonst 12-Wege-Sprungtabelle bei PC-relativ `-0x2c`).

**Interpretation (vorläufig, noch zu verifizieren):** Die 256 Slots
werden vermutlich nicht komplett statisch vorbelegt, sondern zu Boot-
Zeit größtenteils auf den gemeinsamen Fehler-Stub gesetzt und dann
**dynamisch von ladenden Modulen/Treibern über diese
Registrierungsfunktion mit echten Handler-Adressen befüllt** — passend
zu OS-9s bekanntem Konzept ladbarer Treiber-/Dateimanager-Module, die
sich zur Laufzeit in Systemtabellen eintragen. Das würde auch erklären,
warum keine einfache "eine Tabelle, alle 256 Einträge fix im File"-
Lösung wie bei `D_ExcJmp` existiert.

**Update — vollständig gelesen (`0x1390`–`0x1422`, `scripts/DumpRegisterFunc.java`):**

- **Korrektur der Kategorie-Anzahl**: Es sind **6 Kategorien, nicht
  12** — `D1` (Kategorie-Nummer) wird vor dem Vergleich verdoppelt
  (`add.w D1w,D1w`), dann gegen `0xc` (12, als *Wort*-Offset-Grenze für
  6 Zwei-Byte-Einträge) geprüft. Die Sprungtabelle bei `0x13c6`–`0x13d1`
  hat entsprechend nur 6 gültige Einträge; alles ab `0x13d2` ist bereits
  der nächste Code. 6 Ziele: `0x1424` (Kat. 0), `0x1580` (Kat. 1),
  `0x1584` (Kat. 2), `0x153a` (Kat. 3), `0x1548` (Kat. 4), `0x16aa`
  (Kat. 5) — alle innerhalb desselben kopierten `0x1380`–`0x177F`-Blocks,
  noch nicht einzeln gelesen.
- **Korrektur der Gesamtinterpretation** (nach Lesen des einzigen
  Aufrufers, siehe unten): `0x1390` ist **keine "Syscall-Handler-
  Registrierung"**, sondern eher ein **genereller Kategorie-Dispatcher
  für kernel-interne Primitive** — baut aus Tabelle1-Slot 8 (Primär-
  *und* Sekundärarray, `(0x20,A3)`/`(0x420,A3)`) und dem aktuellen
  Prozessdeskriptor (`(0,A4)`) einen kleinen lokalen Kontext-Frame und
  verzweigt per Kategorie (`D1`, hier von einem Aufrufer mit `D1=0`
  aufgerufen) — mutmaßlich aufrufbar sowohl über den öffentlichen
  `F$`-Callpfad als auch direkt für interne, performancekritische
  Kernel-zu-Kernel-Aufrufe (analog zum bereits bei `0x3140` beobachteten
  Slot-90-Trampolin).

## Fund: `FUN_000025f8` — Prozess-Exit-/Aufräumroutine (vollständig gelesen)

Der einzige Aufrufer von `0x1390` (`0x2602`, innerhalb `0x25f8`–`0x26b1`,
184 Byte). Entpuppt sich als **vollständige Prozess-Terminierungslogik**,
kein Modul-Lade-Code:

1. `EXG A0,A4` tauscht den zu terminierenden Prozess (Parameter in `A0`)
   nach `A4`, ruft dann `0x1390` mit Kategorie `D1=0` auf (Zweck der
   Kategorie-0-Aktion noch nicht im Detail gelesen), tauscht zurück.
2. **Setzt `D_Proc` (`(0x4c,A6)`) temporär auf den zu terminierenden
   Prozess** — der Trick: der Kernel "wird" kurzzeitig der sterbende
   Prozess, um dessen Aufräumarbeiten über die normalen, prozess-
   bezogenen Mechanismen laufen zu lassen.
3. **Ressourcen-Freigabeschleife 1** (`0xc8,A0`–`0x100,A0`, 15 Langworte):
   nicht-Null-Einträge werden über `bsr 0x403a` einzeln freigegeben
   (vermutlich gehaltene Speicherblöcke/Deskriptoren).
4. **Pfad-Schließschleife** (`(0x1a8,A0)` abwärts, bis zu 32 Wort-
   Einträge): nicht-Null-Einträge (offene Pfad-Deskriptoren/Datei-
   Handles) werden gelöscht und **per echtem `TRAP #0`** geschlossen —
   klarer Beleg, dass dies die **Tabelle offener Pfade eines Prozesses**
   ist (`I$Close`-artiger Aufruf über den ganz normalen Syscall-Weg,
   da `D_Proc` ja gerade auf diesen Prozess zeigt).
5. Setzt Flag-Bit 0 in `(0x1c,A0)` (weiteres Prozessflag, zusätzlich zu
   den bereits bekannten Bits 5/7).
6. Ruft `bsr 0x62da` (noch nicht gelesen), **stellt danach `D_Proc`
   wieder auf den ursprünglichen (aufrufenden) Prozess zurück** (`A4`).
7. **FPU-Ownership-Aufräumen**: prüft ein globales Flag `(0x2f,A6)` und
   ob der terminierte Prozess gerade `D_FProc` (`(0x58,A6)`) ist — falls
   ja, `FRESTORE` mit Null-Frame (setzt FPU-Zustand zurück) und löscht
   `D_FProc`. **Bestätigt die ganz am Anfang der Untersuchung
   aufgestellte Lazy-FPU-Context-Switch-Hypothese**: Wenn der FPU-
   Registersatz-Besitzer stirbt, muss der Besitz explizit invalidiert
   werden, sonst würde der nächste Prozess versehentlich als "gleicher
   Owner" erkannt.
8. **Ressourcen-Freigabe 2**: `(0x38,A0)` (weiterer Zeiger, Zweck offen)
   wird gelesen, gelöscht, und falls gesetzt über `bsr 0x4078`
   freigegeben.
9. **Abschluss-Trampolin**: derselbe Mechanismus wie bei `0x3140`
   (Tabelle1 `(0x3a4,A6)`, Slot **64** diesmal statt 90 — Primärarray-
   Wert gepusht, Sekundärarray-Ziel per RTS angesprungen) — vermutlich
   der Schritt, der den Prozessdeskriptor-Speicher selbst endgültig
   freigibt.

**Neue Prozessdeskriptor-Felder bestätigt:** `(0xc8..0x100,A0)` =
Ressourcenliste 1 (15 Langworte), `(0x1a8,A0)` abwärts = Tabelle offener
Pfade (Wort-Einträge, per `TRAP #0` geschlossen), `(0x1c,A0)` Bit 0 =
weiteres Prozessflag, `(0x38,A0)` = weitere Ressourcenliste.

**Noch offen:** Was `0x1390`-Kategorie-0 (`0x1424`) und `0x4078` konkret
tun; Zweck von Tabellen-Slot 64 (mutmaßlich Deskriptor-Deallokation);
ob und wie sich der Kernel selbst beim Boot in Slot 90 (Reschedule,
siehe `0x3140`-Fund) einträgt, bleibt weiterhin ungeklärt.

## Fund: `FUN_000062da` — Speicher-/Ressourcen-Freigabe eines Prozesses (vollständig gelesen)

Aufgerufen von `FUN_000025f8` (Prozess-Exit) mit `D0 = Prozessdeskriptor`.
Gibt zwei getrennte, pro Prozess gehaltene Ressourcenlisten frei:

1. **Speicherblock-Liste** bei `(0x2d8,A4)`: verkettete `0x100`-Byte-
   Chunks, jeder Chunk enthält bis zu 32 Block-Deskriptor-Paare
   (`{Größe, Adresse}`, je 8 Byte) plus einen Next-Zeiger bei
   `(0x4,Chunk)`. Für jeden belegten Eintrag wird `bsr 0x5a22`
   aufgerufen (Parameter: `D0=Größe`, `D1=Adresse`, zusätzlich `1` auf
   den Stack gepusht) — starker Kandidat für die eigentliche
   **Speicherfreigabe-Primitive** (`F$SRtMem`-Analogon, selbst noch
   nicht gelesen). Am Ende wird auch der Chunk selbst freigegeben, dann
   `(0x2d8,A4)`, `(0x32c,A4)`, `(0x330,A4)` genullt.
2. **Feste-Größe-Ressourcenliste** bei `(0x390,A4)+8`: einfach verkettet
   über `(0x8,Eintrag)`, jeder Eintrag wird mit fester Größe `0x2a`
   (42 Byte) über dieselbe `bsr 0x5a22`-Primitive freigegeben — Kandidat:
   Liste fester-Größe-Strukturen wie Signal-Handler oder Timer-
   Deskriptoren. Danach `(0x39c,A4)`, `(0x398,A4)` genullt.

**Neue Felder:** `(0x2d8,A4)` = Kopf der Speicherblock-Chunk-Liste,
`(0x32c,A4)`/`(0x330,A4)` = zugehörige Zähler/Grenzen (Zweck offen),
`(0x390,A4)` = Kopf-Header der Fixgrößen-Ressourcenliste (Nutzdaten ab
Offset `+8`), `(0x398,A4)`/`(0x39c,A4)` = zugehörige Zähler/Grenzen.

## Fund: `FUN_00005a22` — die zentrale Speicherfreigabe-Primitive (Kernverhalten identifiziert, nicht jede Verzweigung im Detail verifiziert)

394 Byte, Aufruf-Konvention: `D0=Größe`, `D1=Adresse`, Stack-Parameter
`1` (Bedeutung noch offen, evtl. "Pool-Typ"). Klassischer
**Free-List-Speicherverwalter** (Gegenstück zum bereits bekannten
Allocator `0x4978`/`0x498e`):

- **Größen-Ausrichtung** (`0x5a5e`–`0x5a6e`): rundet die angeforderte
  Freigabegröße auf eine Blockgröße auf (`(0x70,A6)` = Alignment-Wert,
  `neg.l`+`and.l`-Muster = klassisches Zweierpotenz-Runden).
- **Pool-Lookup** (`bsr 0x55a4`, zweimal aufgerufen): sucht anhand
  Adresse/Größe den zuständigen Speicherpool-Deskriptor — einmal gegen
  einen Pool bei `(0x3fc,A6)` (globaler Pool?), einmal gegen
  `(0x50,A6)+0x390` (Pool-Anker beim "System-Prozess" `(0x50,A6)` — dem
  bei Boot parallel zu `D_Proc` gesetzten zweiten Prozesszeiger).
  Rückgabewert `0xDB` aus dem ersten Aufruf signalisiert vermutlich
  "nicht in diesem Pool, nächsten versuchen" (Fehlercode-artiger
  Vergleich `cmpi.l #0xdb,D0`).
- **Freilisten-Einfügen mit Verschmelzung** (`0x5aba`–`0x5b2a`):
  klassisches Boundary-Tag-Coalescing — prüft, ob der freizugebende
  Block direkt an einen bereits freien Nachbarblock angrenzt
  (`sub.l`/`cmp.l` gegen die Blockgröße), verschmilzt ggf. mit dem
  Nachbarn statt einen neuen Freilisten-Eintrag anzulegen, sonst wird
  ein neuer Eintrag mit Next/Prev-Verkettung (`(0,A?)`/`(0x4,A?)`) in
  die sortierte Freiliste eingefügt.
- Danach (`0x5b30`ff., nicht mehr im Detail gelesen) offenbar
  Buchführung/Statistik-Aktualisierung und ein zweiter Pool-Versuch
  über eine weitere Hilfsfunktion (`bsr 0x5bac`, noch nicht gelesen) —
  die Funktion ist insgesamt 394 Byte lang, es wurden bislang die
  ersten ~330 Byte gelesen.

**Update — komplett gelesen (`0x5a22`–`0x5baa`, sauber begrenzt durch
`LINK A5,-0xc` … `UNLK A5; RTS`):**

- **Zwei Pools, nacheinander versucht**: Erst wird der freizugebende
  Block gegen den Pool bei `(0x3fc,A6)` geprüft (`bsr 0x55a4`,
  Rückgabe `0xDB` = "gehört nicht zu diesem Pool"). Bei Ablehnung wird
  derselbe Block gegen den Pool bei `(0x50,A6)+0x390` versucht — also
  dem Pool-Anker des bei Boot parallel zu `D_Proc` gesetzten
  "System-Prozess"-Zeigers. Erst *innerhalb* dieses zweiten Versuchs
  passiert das tatsächliche Einfügen/Verschmelzen.
- **Korrekt geklammerte Interrupt-Maskierung** (kein adressbasiertes
  Lock, siehe Korrektur unten): `bsr 0x10e6` am Anfang des zweiten
  Pool-Versuchs, `bsr 0x10f2` an **jedem** Ausstiegspunkt der Funktion
  (`0x5b30`, `0x5b76`, `0x5ba0`) — sauber auf allen Pfaden
  zurückgesetzt (wichtig für den exakten Nachbau: kein Pfad "vergisst"
  die Wiederherstellung).
- **Zwei verschiedene Einfüge-Helfer**: `bsr 0x5bac` (eigene Funktion,
  beginnt direkt im Anschluss an `0x5a22` im Speicher) für den
  Normalfall; falls dieser einen Fehler zurückgibt, Fallback auf
  `bsr 0x5712` mit neu ausgerichteter Blockadresse — vermutlich
  unterschiedliche Einfügestrategien (z. B. "an bekannter Stelle
  einfügen" vs. "komplett neu in die sortierte Liste einsortieren").
- Pool-Such-Helfer `0x55a4` (teilweise gelesen, `0x55a4`–`0x5644`):
  vergleicht die Blockadresse gegen Pool-Grenzen (`D6`/`D7`, Bitmasken-
  Vergleiche mit `AND`/`OR`, Rückgabe `0xDB` bei Nichtübereinstimmung),
  ruft bei Treffer selbst wieder `0x5bac` als Einfüge-Helfer auf und
  durchläuft danach eine Freilisten-Suche (`0x5628`ff., Ringliste über
  `(A2)`/`(0x4,A1)`) — noch nicht vollständig gelesen (Funktion reicht
  mindestens bis über `0x5644` hinaus).

**Fazit:** Architektur ist jetzt klar: mindestens **zwei separate
Speicherpools** (einer bei `(0x3fc,A6)`, einer beim "System-Prozess"
`(0x50,A6)+0x390`), Zugriff auf den zweiten Pool ist durch temporäres
Maskieren aller Interrupts geschützt (siehe `0x10e6`/`0x10f2` unten),
Freigabe probiert Pools der Reihe nach durch. Für den `.s`-Nachbau noch
offen: `0x55a4` vollständig, `0x5bac`, `0x5712`.

## Fund: `0x10e6`/`0x10f2` — Interrupt-Masken-Primitive (Korrektur: kein adressbasiertes Lock)

Vollständig gelesen, sehr kurz:

```
0x10e6: tst.l D0
        move SR,D0w        ; alte SR immer gesichert (Rückgabewert)
        beq.b 0x10f0        ; D0==0 -> nichts maskieren, direkt zurück
        ori #0x700,SR       ; sonst: IPL auf 7 anheben (alle Interrupts sperren)
0x10f0: rts

0x10f2: move D1w,SR         ; SR aus D1 wiederherstellen
        rts
```

**Korrektur einer Fehlannahme**: Ursprünglich als "aus der Blockadresse
abgeleiteter Sperr-Schlüssel" interpretiert — tatsächlich ist es eine
simple **bedingte Interrupt-Maskierung** (kritischer Abschnitt), der
Parameter in `D0` ist nur ein Boolean ("überhaupt maskieren?"), keine
Adresse/kein Hash. Wird an vielen Stellen im Modul als generisches
"kurzzeitig alle Interrupts sperren"-Paar verwendet, nicht spezifisch
für Speicherpools.

## Fund: `0x5bac` — Arena-Lookup-oder-Erzeugen (nicht "Einfügen", wie zunächst vermutet)

Vollständig gelesen (`0x5bac`–`0x5c7c`, 208 Byte). Aufruf-Konvention:
`D0=Größe`, `D1=Adresse`, Stack: `Pool-Header`, `Ausgabe-Zeiger`
(Ergebnis wird dorthin geschrieben). **Kein reiner Freilisten-Insert**,
sondern findet — oder erzeugt bei Bedarf — den **Arena-Deskriptor**,
der die freizugebende Adresse abdeckt:

- Pool hat eine zirkuläre doppelt verkettete **Arena-Liste**
  (Next/Prev bei Offset `0x8`/`0xc` relativ zum Arena-Deskriptor,
  Sentinel = der Pool-Header selbst).
- Sucht die Arena, deren Adressbereich (`(0,A?)`–`(0x4,A?)`) die
  freizugebende Adresse enthält.
- **Nicht gefunden** → durchläuft eine zweite Kandidatenliste ab
  `(0x404,A6)` (mutmaßlich System-weite Speicherregionen/Boards), bis
  entweder eine passende Region gefunden wird oder der Sentinel
  `(0x3fc,A6)` erreicht ist (→ Fehlercode `0xD2`, "Adresse gehört zu
  keiner bekannten Region"). Bei Treffer: neuer Arena-Deskriptor wird
  per `bsr 0x526c` alloziert — **feste Größe `0x2a` (42 Byte)**,
  identisch mit der in `0x62da` gefundenen Fixgrößen-Ressourcenliste
  eines Prozesses. Damit bestätigt: **Arena-Deskriptoren selbst sind
  die 42-Byte-Objekte** aus jener Liste. Die 42 Byte werden per
  Kopierschleife aus der Kandidatenregion in den neuen Deskriptor
  übernommen (Vorlage/Template-Mechanismus), danach über einen weiteren,
  noch nicht gelesenen Helfer (`bsr 0x5c7c`) in die Arena-Liste des
  Pools eingehängt.
- **Gefunden** → prüft nur noch, dass Adresse+Größe nicht über das
  Arena-Ende hinausragt (sonst wieder Fehlercode `0xD2`).
- Ergebnis (gefundene oder neu erzeugte Arena) wird über den
  Ausgabe-Zeiger zurückgegeben.

**Update:** `0x5c7c` (86 Byte) vollständig gelesen — ein **generischer,
nach Klassen-/Typ-Tag (Offset `0x28`, aufsteigend) sortierter
Doppelverkettungs-Insert**: initialisiert bei leerer Liste den
Sentinel selbstreferenzierend, löscht drei Felder im neuen Knoten
(`0x14`, `0x10`, `0x20` — vermutlich Statistik-/Cache-Felder), sucht
die erste Position mit gleichem oder größerem Tag und splict davor ein.
Dieselbe Routine wird also nicht nur für Arenen verwendet, sondern ist
eine generische sortierte-Listen-Grundoperation.

`0x526c` (468 Byte, erst die ersten ~90 Byte gelesen) ist der
**Arena-Deskriptor-Allocator**: rundet die Zielgröße aus (dasselbe
`neg.l`/`and.l`-Ausrichtungsmuster wie in `0x5a22`), versucht dann eine
Allokation über eine tiefer liegende Funktion `bsr 0x5440` — mit
Fallback auf den zweiten Pool `(0x50,A6)+0x390`, falls der erste
Versuch mit Fehlercode `0xED` scheitert ("Pool voll", analog zum
bereits bekannten `0xD2`/`0xDB`).

## Fund: `0x5440` — die eigentliche Allokations-Primitive (Gegenstück zu `0x5a22`, vollständig gelesen)

280 Byte, sauber begrenzt (`LINK A5,0`…, Rücksprung über `bra.w
0x5706` — echtes Funktionsende liegt dort, nicht bei der von Ghidra
automatisch erkannten Grenze `0x5557`). Aufruf-Konvention: `D0=Größe`,
`D1=Klassen-/Typ-Tag`, Stack: `Ausgabe-Zeiger`, `Arena-Listenkopf`,
`Interrupt-Maskieren-Flag` (direkt an `0x10e6` durchgereicht — dieselbe
Interrupt-Masken-Primitive wie beim Freigeben).

**Algorithmus: First-Fit innerhalb der ersten passenden Arena-Klasse:**

1. Läuft die Arena-Liste ab dem übergebenen Kopf durch. Pro Arena:
   Klassen-Tag `(0x26,A4)` muss zum angeforderten Tag passen (oder
   Tag `0` = Wildcard, akzeptiert jede Arena), Arena muss "aktiviert"
   sein (`(0x28,A4)` ungleich 0), angeforderte Größe muss in die
   Arena-Gesamtgröße `(0x20,A4)` passen, ein Sperr-/Statusbit
   (Bit 4 in `(0x24,A4)`) darf nicht gesetzt sein (Arena vermutlich
   gerade in Benutzung/gesperrt). Nichtpassende Arenen werden
   übersprungen, kein Fehlerabbruch.
2. Innerhalb einer passenden Arena: durchsucht deren **eigene
   Freiliste** (eingebettet bei Offset `0x10` innerhalb des
   Arena-Deskriptors, Next/Prev-Paar dort bei Offset `0`/`4` — bewusst
   *andere* Offsets als die Arena-Verkettung selbst, die bei `0x8`/`0xc`
   liegt) nach dem **ersten Block, der groß genug ist** (First-Fit,
   nicht Best-Fit).
3. **Größentreffer**: `D7 = Blockgröße − angeforderte Größe`.
   - `D7 == 0` (exakter Treffer): Block wird komplett aus der Freiliste
     ausgehängt (klassisches Doppelverkettungs-Unlink), seine Adresse
     direkt als Ergebnis zurückgegeben.
   - `D7 > 0` (Restfläche bleibt übrig): **Split von hinten** —
     ungewöhnlich, aber eindeutig im Code: der Freilisten-Eintrag
     *behält seine ursprüngliche Adresse* und wird nur auf die
     Restgröße `D7` verkleinert (bleibt an Ort und Stelle in der
     Freiliste, keine Neuverkettung nötig); der **allozierte Bereich
     ist das hintere Ende** des ursprünglichen Blocks
     (`Ergebnisadresse = alte Blockadresse + D7`). Das erspart ein
     Neusortieren der Freiliste bei jeder Teilzuweisung.
   - In beiden Fällen wird die Arena-weite Frei-Byte-Buchführung über
     `bsr 0x5712` (dieselbe größensortierte Freiliste-Funktion wie
     schon bei `0x5a22`/`0x526c` gesehen) mit Parameter `-1`
     ("schrumpfen") aktualisiert.
4. Kein Treffer in der aktuellen Arena → weiter zur nächsten Arena in
   der Kette; komplette Liste einmal durchlaufen ohne Erfolg → Fehler
   `0xAB` ("keine Arena mit ausreichend freiem Speicher") bzw. `0xE1`
   (Größe `0`) oder `0xED` (Arena-Liste leer) als jeweilige
   Sonderfälle.

**Fazit — Speicherverwaltung jetzt im Kern vollständig verstanden:**
`0x5440` (Allozieren) und `0x5a22` (Freigeben) bilden das
Funktionspaar; beide arbeiten auf derselben zweistufigen Struktur
(Pool → Arena → Freiliste), beide nutzen `0x5712` zur
Frei-Byte-Buchführung und `0x10e6`/`0x10f2` zur Interrupt-Maskierung
während der Listenmanipulation. Für den vollständigen `.s`-Nachbau noch
offen: exakte Feldbedeutung von `(0x24,A4)` (Statuswort, nur Bit 4
verifiziert) und `(0x26,A4)`/`(0x28,A4)` (Klassen-Tag/Aktiviert-Flag),
sowie der Rest von `0x526c` (ab `0x5326`) und `0x55a4` (ab `0x5644`).

## Fund: `0x5712` — größensortierte Freiliste auf Arena-Ebene

Vollständig gelesen (`0x5712`–`0x57bc`, 172 Byte). Pflegt eine **nach
Größe sortierte zirkuläre Freiliste** von Speicherbereichen innerhalb
einer Arena — eine zweite, höhere Abstraktionsebene über der bereits in
`0x5a22` direkt gesehenen Boundary-Tag-Verschmelzung:

- Next/Prev bei Offset `0x8`/`0xc` (dieselbe Konvention wie bei den
  Arena-Deskriptoren in `0x5bac`), Sortierschlüssel bei Offset `0x20`
  (Größe, Langwort) und `0x28` (Wort, vermutlich Typ-/Klassen-Tag,
  Vergleich vor dem Größenvergleich — d. h. primär nach Klasse, sekundär
  nach Größe sortiert).
- Fallunterscheidung über den Parameter, der mit `0` verglichen wird
  (`ble`): **Fall ≤ 0** (Schrumpfen eines bestehenden Eintrags):
  Größe wird direkt am bestehenden Eintrag verändert (`add.l`/`sub.l`
  auf `(0x20,A0)`), kein neuer Listenknoten. **Fall > 0** (neuer/
  wachsender Eintrag): läuft die sortierte Liste ab `A1` ab, splict den
  neuen/veränderten Eintrag an die passende Position (klassisches
  Doppel-Verkettungs-Einfügen nach Sortierkriterium), sowohl vorwärts
  (`0x572c`ff.) als auch rückwärts (`0x5774`ff.) je nachdem, wo der
  Eintrag einsortiert werden muss.

**Einordnung:** Zusammen mit `0x5bac` ergibt sich ein zweistufiges
Speicherverwaltungsschema: **Pool → Arena (nach Adressbereich) → nach
Größe/Klasse sortierte Freiliste innerhalb der Arena** — deutlich
differenzierter als ein einfacher First-Fit-Allocator, passt zu einem
produktionsreifen Multi-Region-Speichermanager.

**Nebenfund:** 10 Aufrufer von `Q9_scheduler_183a` gefunden (per
Xref-Suche), nicht nur der eine aus `Q9_disp_180` — bestätigt die
Vermutung, dass die Ready-Queue-Insert-Routine von vielen Stellen
(Timer, Signal-Zustellung, I/O-Completion, ...) genutzt wird. Adressen:
`0xd7e`, `0xdce`, `0x182c`, `0x216c`, `0x2548`, `0x2886`, `0x28a6`,
`0x2bc` (bekannt, `Q9_disp_180`), `0x3a5c`, `0x3ad2` — noch nicht
einzeln untersucht.

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

1. Speicherverwaltungs-Kern (`0x5440` Allozieren, `0x5a22` Freigeben,
   `0x10e6`/`0x10f2`, `0x5bac`, `0x5712`, `0x5c7c`) ist **fertig
   gelesen und verstanden**. Restliche offene Detailfragen dort (siehe
   "Fazit" oben): `0x526c` ab `0x5326` zu Ende, `0x55a4` ab `0x5644`
   zu Ende, exakte Statuswort-Felder `(0x24/0x26/0x28,Arena)`.
2. `0x4078` lesen (weitere Ressourcenfreigabe aus `FUN_000025f8`).
3. Kategorie-0-Handler `0x1424` lesen (was `0x1390` mit `D1=0` im
   Exit-Pfad tatsächlich bewirkt).
4. Tabellen-Slot 64 (Abschluss-Trampolin in `FUN_000025f8`) und Slot 90
   (Reschedule-Trampolin in `0x3140`) — beide Zieladressen bleiben ohne
   Laufzeit-Speicherinspektion unbekannt; ggf. im Emulator nachsehen,
   sobald einer läuft.
5. Aufrufer von `FUN_000025f8` suchen (wer terminiert Prozesse?) —
   nächster Baustein für das Gesamtbild des Prozess-Lebenszyklus.
6. Die 10 gefundenen Aufrufer von `Q9_scheduler_183a` einzeln
   untersuchen (`0xd7e`, `0xdce`, `0x182c`, `0x216c`, `0x2548`,
   `0x2886`, `0x28a6`, `0x3a5c`, `0x3ad2`) — vermutlich Timer-,
   Signal- und I/O-Completion-Pfade, die Prozesse aufwecken.
7. Die übrigen neu erschlossenen Dispatch-Funktionen inhaltlich lesen:
   `Q9_disp_452` (76 B, inkl. `0x472`), `Q9_disp_5d0` (TRAP #1–15,
   880 B), `Q9_disp_8d0` (Sammel-Handler für CPU-/FPU-/MMU-Exceptions,
   1258 B — vermutlich hier auch der schon vermutete FPSP-Einstieg bei
   `0xb04`), `Q9_disp_888` (1354 B), `Q9_disp_ba4` (698 B).
8. Verbleibende undefinierte Blöcke (siehe Liste oben, insgesamt nur
   noch 17 %/5102 B) einzeln prüfen — vermutlich überwiegend Daten-
   tabellen, aber nicht blind annehmen.
9. Allocator-Routine `0x4978` verstehen (liefert den Speicher für
   `D_ExcJmp`, `moveq #0x10,D0` / Größe `0x100` deuten auf einen
   generischen Systempool-Request hin — evtl. `F$SRqMem`-Analogon).
10. Panic-/Fehlerroutine `0x7f6` (aufgerufen bei Tabellen-Inkonsistenzen
    und diversen Bound-Checks) verstehen — wichtig für Robustheits-
    Annahmen beim eigenen Nachbau.
11. Restliche Sub-Modi von EA-Mode 7 (`0xb8e`+) vollständig
    disassemblieren.
12. Weitere `PLATZHALTER`-Felder in `q9sysglob.a`/`.h` einzeln per
    Disassemblierung verifizieren (nicht blind übernehmen).
13. Sobald ein Bereich vollständig verstanden ist: als eigene `.s`-Quelle
    nachbauen, mit `vasm`/echtem `r68` assemblieren, Bytes gegen das
    Original diffen (siehe Zieldefinition oben).
