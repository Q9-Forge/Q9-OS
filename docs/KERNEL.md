# OS-9/68K Kernel — Notizen aus dem offiziellen SDK

Zusammenfassung/Auszug aus dem Microware/RadiSys-Originalmaterial
(`MWOS/DOC/RadiSys/68k_tech.pdf` = "OS-9 for 68K Processors Technical
Manual", Kapitel 2 "The Kernel"; `68k_bls.pdf` = "OS-9 for 68K
Processors BLS Reference") als Referenz für den eigenen Kernel-Nachbau.
Die Original-Binaries liegen in [`../vendor/`](../vendor/README.md).
Kein Ersatz für die Originaldokumente — bei Detailfragen dort
nachschlagen (Volltext-PDF, per `pdftotext` durchsuchbar).

## Rolle des Kernels

Der Kernel ist der Nukleus von OS-9: ROMable, kompaktes Modul, verwaltet
Systemressourcen, Prozessausführung und Exception-/Interrupt-Verarbeitung.
Seine Kernaufgabe ist die Koordination von System Calls. OS-9 kennt zwei
Arten:

- **I/O-Aufrufe** (read/write) — der Kernel übernimmt nur die erste
  Verarbeitungsstufe und reicht dann an **IOMan** weiter, der wiederum
  den passenden File Manager/Device Driver aufruft.
- **System-Function-Aufrufe** (Speicherverwaltung, Systeminitialisierung,
  Prozesserzeugung/-scheduling, Exception-/Interrupt-Verarbeitung) —
  führt der Kernel direkt aus.

Ein System Call löst einen User-Trap zum Kernel aus; der Kernel bestimmt
den Typ und verzweigt entsprechend.

## Kernel-Typen: Atomic vs. Development

Zwei Klassen, binär als eigene Kernel-Module ausgeliefert (siehe
[`vendor/README.md`](../vendor/README.md) für die Dateinamen):

| | Development Kernel (`dker*`) | Atomic Kernel (`aker*`) |
|---|---|---|
| Zielgruppe | Voll ausgestattet, Embedded **und** Multi-User | primär Embedded (auch Multi-User nutzbar) |
| User-State-Debugging (`F$DFork`/`F$DExec`/`F$DExit`) | ja | nein — nur ROM-Debugger |
| Multi-User-Schutz (Nutzerparameter-Validierung etc.) | ja | nein |
| Externe Cache-Hardware | unterstützt | nur On-Chip-Caches |
| MMU-Speicherschutz | ja | nein |
| Modul/User-ID-Zugriffsprüfung beim Linken | ja | nein — jeder darf an jedes Modul linken |

Empfehlung aus dem Handbuch: unter dem Development-Kernel entwickeln
(bessere Fehlersuche), dann für den produktiven/Atomic-Einsatz wechseln,
sobald der Code als korrekt gilt.

## Speicherallokatoren: Standard vs. Buddy

Ebenfalls Teil der Kernel-Variante (`*s`/`*b`-Suffix):

- **Standard-Allocator** (`*s`) — Auflösung 16 Byte (1025 Byte → 1040
  Byte gerundet). Der klassische, von den meisten Systemen genutzte
  Allocator.
- **Buddy-Allocator** (`*b`) — Binary-Buddy-Algorithmus, rundet auf die
  nächste Zweierpotenz (1025 Byte → 2048 Byte). Weniger
  speichereffizient, dafür deterministischer/schneller — typisch für
  Echtzeit-/Atomic-Systeme. Colored-Memory-Listen (ROM oder Init-Modul)
  müssen bei Buddy auf die Blockgröße ausgerichtet sein.

## Init-Modul (Configuration Module)

Nicht-ausführbares Modul vom Typ `Systm` (Code `$0C`), muss beim
Kernelstart im Speicher liegen (üblich: in `OS9Boot` oder ROM). Beginnt
mit dem Standard-Modulheader, danach zusätzliche Felder ab Offset `$30`
(vollständige Tabelle: Kapitel 2, Table 2-4; Beispielcode: Anhang A,
Offset-Namen in `sys.l`):

| Offset | Name | Bedeutung | Default Atomic / Development |
|---|---|---|---|
| `$34` | `M$PollSz` | Einträge in der IRQ-Polling-Tabelle (1 je interruptfähigem Gerät) | 16 / 32 |
| `$36` | `M$DevCnt` | Größe der System-Device-Tabelle (1 je Gerät) | 8 / 32 |
| `$38` | `M$Procs` | Initiale Prozesstabellengröße (bei Atomic fix, bei Development wachsend) | 32 / 64 |
| `$3A` | `M$Paths` | Initiale Path-Tabellengröße (bei Atomic fix, bei Development wachsend) | 32 / 64 |

Nach dem Hardware-Reset führt das Boot-ROM den Kernel aus (aus ROM oder
Disk geladen); der Kernel initialisiert das System (ROM-Module lokalisieren,
Systemstart-Task — üblich `Sysgo` — starten).

## Prozesserzeugung (`F$Fork`)

Vier Schritte:

1. **Modul lokalisieren/laden** — erst im Speicher suchen, sonst von
   Mass-Storage nachladen (Dateiname = Modulname).
2. **Prozessdeskriptor allozieren+initialisieren** — Tabelle mit Status,
   Speicherbelegung, Priorität, I/O-Pfaden; automatisch verwaltet.
3. **Stack-/Datenbereich allozieren** — Größen stehen im Modulheader,
   ein zusammenhängender Speicherbereich wird reserviert.
4. **Prozess initialisieren** — Register auf Daten-/Codeadressen setzen,
   initialisierte Variablen/Pointer aus dem Objektcode in den Datenbereich
   kopieren.

Scheitert ein Schritt, wird die Prozesserzeugung abgebrochen und der
Fehler an den `fork`-Aufrufer zurückgemeldet. Bei Erfolg: neue Prozess-ID
(plus geerbte Group-/User-ID) und Einreihung in die Scheduling-Queue.

**Registerkonvention beim Prozessstart** (Table 2-6, relevant für
Compiler-Backend/Runtime-Startup):

| Register | Inhalt |
|---|---|
| `pc` | Modul-Einstiegspunkt |
| `a3` | Modul-Startadresse |
| `a1` | Top-of-Memory-Pointer |
| `a5`/`a7` | Parameter-Startadresse / Stack-Top |
| `a6` | Datenbereich-Basisadresse (niedrigste Adresse) |
| `a0`, `a2` | undefiniert |
| `sr` | `N000` (N=0 non-MSP, N=1 MSP-Systeme) |

Terminierung (`F$Exit`, Fatal Signal/Error): offene Pfade schließen,
Speicher freigeben, primäres Modul unlinken.

## Exception-/Interrupt-Verarbeitung

Vektortabelle (68020/030/040, Auszug Table 2-9):

- **Vektoren 0, 1 — Reset**: `SSP`-Initialwert (mind. 4K RAM davor/danach
  für System-Globaldaten) + Coldstart-Einstiegspunkt. Von Usercode nicht
  anfassen.
- **Vektoren 2–8, 10–24, 48–63 — Error Exceptions**: i.d.R. fataler
  Programmfehler → Prozess terminiert unbedingt (bei `F$DFork`-Prozessen
  bleiben Ressourcen für Postmortem-Debugging erhalten). Per `F$STrap`
  lässt sich ein User-Handler für die nicht-fatalen Fälle dieser Gruppe
  installieren. FPCP-Vektoren 48–54 nur bei 68020/030 relevant.
- **Vektor 9 — Trace**: Single-Step (Trace-Bit im SR), Basis für
  `F$DFork`/`F$DExec`/`F$DExit`.
- **Vektoren 64–255 — `F$IRQ`**: nutzerdefinierte vektorisierte
  Interrupts.
- Nicht vektorisierte Polling-Interrupts werden intern wie vektorisierte
  behandelt. Level-7-Interrupts sind non-maskable und sollten nur für
  z.B. DRAM-Refresh genutzt werden — dabei **keine** OS-9-Systemcalls/
  -Datenstrukturen anfassen.
- User-Trap 0 (Vektor 32) ist für OS-9-System-Service-Requests reserviert,
  die restlichen 15 User-Traps für Library-Routinen-Links zur Laufzeit.

## Architektur-Karte (aus der Disassemblierung von `dker030s`)

Die Abschnitte oben sind Theorie aus dem Handbuch. Dieser Abschnitt ist
die **Landkarte dessen, was wir tatsächlich im Binärcode gefunden und
verstanden haben** (Details/Herleitung in `REVERSE_ENGINEERING.md`,
konkrete benannte Funktionen in `src/kernel/kernel.r`). Zweck: bevor
ein eigener Nachbau beginnt, muss klar sein, **was der Kernel grob
tut und wie die Teile zusammenhängen** — nicht jedes Register, aber
jeder Baustein und seine Beziehung zu den anderen.

### 1. Boot-Sequenz

```
Reset-Vektor (SSP) → System-Global-Bereich existiert bereits im RAM
  → Q9_kernel_init_67a0        (zentraler Bootstrap, ~1600 Byte)
       - alloziert D_ExcJmp (256-Eintrags-Exception-Sprungtabelle)
         und befuellt sie aus einer kompakten Quelltabelle im Modul
       - alloziert + kopiert zwei Syscall-Tabellen (0x3a4/0x3a8,A6)
         aus derselben Art Quelle (Fehler-Stub + Registrierungs-
         funktion, KEINE fertige Adressliste)
  → Q9_boot_finalize_6de4      (Fortsetzung, direkt im Anschluss)
       - Prozess-ID-Validierung, ein TRAP-#0-Modulaufruf
       - raeumt im Tabellen-Slot-90-Bereich auf
  → Q9_reschedule_trampolin_3140   (Uebergabe an den Scheduler)
       - Cache-Flush fuer gepatchten Code
       - springt in Syscall-Tabellen-Slot 90 (= der eigentliche
         Kontextwechsel-Einstieg, Ziel wird erst zur Boot-Zeit
         eingetragen, nicht statisch im Modul sichtbar)
```

**Kernaussage:** Der Kernel baut beim Hochfahren zwei RAM-Tabellen auf
(Exception-Sprungtabelle + Syscall-Tabellen) und übergibt am Ende
buchstäblich die Kontrolle an den Scheduler-Trampolin. Alles danach
läuft **tabellengetrieben**, nicht mehr linear.

### 2. Exception-/Interrupt-Dispatch — eine Tabelle, acht Handler

`D_ExcJmp` (System-Global-Offset `0x68`) ist eine 256-Eintrags-Tabelle
(10 Byte/Eintrag), die **jeden** CPU-Vektor auf einen von nur
**8 gemeinsam genutzten Dispatcher-Funktionen** abbildet:

| Vektoren | Bedeutung | Dispatcher |
|---|---|---|
| 2–3 | Bus/Address Error | `Q9_disp_888` (dünner Wrapper, fällt durch in `Q9_disp_8d0`) |
| 4–8, 10–14, 16–23, 48–63 | Illegal Instr, FPU-Exc., MMU-Fehler, reserviert | `Q9_disp_8d0` (der größte, vielseitigste Handler) |
| 9 | Trace (Single-Step) | `Q9_disp_ba4` |
| 15 | Uninitialized Interrupt | `Q9_disp_452` |
| 24 | Spurious Interrupt | `Q9_disp_452` |
| 25–31, 64–255 | Autovektoren + `F$IRQ` | `Q9_disp_180` |
| **32** | **`TRAP #0` = OS-9-Syscall** | `Q9_disp_488` |
| 33–47 | `TRAP #1`–`#15` (User-Traps) | `Q9_disp_5d0` |

Gemeinsame Infrastruktur, die mehrere Dispatcher teilen:
`Q9_signal_pending_bc0` (Signal-/Breakpoint-Zustellung),
`Q9_exc_no_handler_fc4` → `Q9_exc_default_action_24d8` (Terminierung
ohne Handler), `Q9_fpu_save_fe0`/`Q9_fpu_restore_1034` (Lazy-FPU-
Context-Switch), `Q9_panic_report_7f6` (Diagnoseausgabe, kein Halt).

**Kernaussage:** Es gibt keine 256 individuellen Handler, sondern ein
**Trichter-Prinzip** — die meisten CPU-Ausnahmen landen in `Q9_disp_8d0`,
das dann selbst wieder nach Vektor-Bereich verzweigt (Breakpoints,
generisches Vektor-Handler-System pro Prozess, Signal-Zustellung).

### 3. Syscall-Pfad (`TRAP #0`)

```
User-Code: TRAP #0, dann Funktionsnummer als Inline-Wort im Codestrom
  → Q9_disp_488
       - liest Funktionsnummer aus dem geretteten PC
       - waehlt eine von zwei Syscall-Tabellen (0x3a4 oder 0x3a8,A6)
         je nach Verschachtelungstiefe (Bit 5 im Statuswort)
       - Trampolin-Sprung (PEA+RTS) zum Handler in der Tabelle
       - Epilog: Stack-Kanarienvogel pruefen, ggf. Reschedule-Check
```

Dieselbe Zwei-Tabellen-Struktur (`Primaerarray 0x000-0x3FF` +
`Sekundaerarray 0x400-0x7FF`, parallel indiziert) taucht als
wiederkehrendes Muster auch bei den **internen Trampolinen** auf
(Slot 64/88/89/90, siehe Abschnitt 6) — dieselbe Mechanik wird also
sowohl für öffentliche `F$`-Syscalls als auch für interne
Kernel-zu-Kernel-Sprünge verwendet.

### 4. Scheduler

`Q9_scheduler_183a` ist **die** zentrale Ready-Queue-Einfügeroutine
(zirkuläre doppelt verkettete Liste, Priority Aging). Sie fügt nur
ein — der eigentliche Kontextwechsel (nächsten Prozess entnehmen,
CPU-Kontext umschalten) läuft über den nicht statisch auflösbaren
Slot-90-Trampolin (siehe oben), dessen Ziel erst zur Boot-Zeit im RAM
steht.

12 bekannte Aufrufer zeigen, **wer** einen Prozess aufweckt/einreiht:

| Kategorie | Aufrufer (Auszug) |
|---|---|
| Prozess-Erzeugung | `0xd7e` |
| Expliziter Wake-Syscall | `0x182c` |
| Timer-/Alarm-Ablauf | `0x216c` |
| Fork/Duplizierung | `0x2886`/`0x28a6`, `Q9_scheduler_caller_244c` |
| Sleep-Ende | `0x3a5c`/`0x3ad2`, `Q9_scheduler_caller_35f6` |
| Eltern-Aufweck bei Kind-Tod | `Q9_parent_notify_4518` (aus `Q9_exc_default_action_24d8`) |
| Zeitscheiben-Ablauf | `Q9_clock_tick_6a8` (setzt nur das Reschedule-*Flag*, ruft nicht direkt) |

**Kernaussage:** Der Scheduler selbst ist simpel (Insert-Sort mit
Aging). Die Komplexität steckt in den *vielen Stellen*, die ihn
aufrufen — für einen Nachbau muss jede dieser Kategorien einzeln als
eigener Signal-/Weckpfad nachgebildet werden, nicht nur die
Insert-Routine.

### 5. Speicherverwaltung — Pool → Arena → Freiliste

```
Pool (z.B. (0x3fc,A6) oder System-Prozess-Pool (0x50,A6)+0x390)
  └─ Arena-Liste (zirkulaer, nach Adressbereich)
       └─ arena-eigene Freiliste (nach Groesse/Klasse sortiert)
            └─ einzelne freie Bloecke (Boundary-Tag-Coalescing)
```

Funktionspaar: `Q9_mem_alloc_5440` (First-Fit, Split von hinten) und
`Q9_mem_free_5a22` (zwei Pools nacheinander versucht, Coalescing).
Beide nutzen dieselbe Interrupt-Maskierung (`Q9_irq_mask_10e6`/
`Q9_irq_unmask_10f2`) und dieselbe Freilisten-Buchführung
(`Q9_freelist_bysize_5712`). `Q9_arena_lookup_5bac`/`Q9_arena_alloc_526c`
verwalten die Arena-Ebene, `Q9_dealloc_owned_5cd2` ist eine
eigentumsgeprüfte Alternative zur direkten Freigabe.

**Kernaussage:** Kein einfacher Heap — zwei feste Pools, mehrere
Arenen pro Pool (nach Adressbereich getrennt), jede mit eigener
größensortierter Freiliste. Für einen Nachbau reicht vermutlich
zunächst ein einzelner Pool/eine Arena, wenn keine Multi-Board-/
Multi-Region-Unterstützung gebraucht wird.

### 6. Interne Trampoline — ein wiederkehrendes Muster

An mehreren Stellen (`Q9_reschedule_trampolin_3140` Slot 90,
`Q9_trampolin_slot88_4078` Slot 88, `Q9_proc_die_prep_2590` Slot 89,
ein weiterer bei `FUN_000025f8`s Ende Slot 64) wird **derselbe**
Mechanismus verwendet: Wert aus dem Primärarray der Syscall-Tabelle
lesen, Sekundärarray-Gegenstück per PEA+RTS ansteuern. Diese Slots
sind **keine öffentlichen Syscalls**, sondern interne,
kontextbezogene Kernel-Suboperationen (kein Adressparameter, wirken
auf den aktuellen `D_Proc`-Kontext).

### 7. Prozess-Lifecycle

```
Erzeugung: Modul laden → Deskriptor allozieren (ggf. alten Slot per
           Q9_proc_slot_cleanup_25f8 zuerst aufraeumen) → Stack/Daten
           allozieren → Register setzen → Q9_scheduler_183a

Terminierung (zwei Wege, teilen sich denselben Cleanup-Pfad):
  a) unbehandelte Exception → Q9_exc_default_action_24d8
  b) expliziter Exit (0x25f0-Umgebung, genaue Funktionsgrenze
     nicht gefunden, Zweck aber klar)
       beide → Q9_proc_die_prep_2590 (Trampolin Slot 89)
             → Q9_proc_slot_cleanup_25f8 (Ressourcen, offene Pfade,
               FPU-Ownership)
             → Q9_proc_resource_free_62da (Speicherbloecke, Fixgroessen-
               Ressourcen, je ueber Q9_mem_free_5a22)
             → Q9_proc_id_free_3370 (Prozess-ID in Freiliste zurueck)
             → Q9_parent_notify_4518 (weckt wartenden Elternprozess)
```

### 8. Selbstmodifizierender Code — das `0x4AFC`-Muster

An mindestens zwei Stellen (`Q9_reschedule_trampolin_3140`,
`Q9_module_patch_362c`) prüft der Kernel, ob an einer Adresse noch der
Platzhalterwert `0x4AFC` (= 68k-`ILLEGAL`-Opcode, zufällig identisch
mit dem Modulheader-`M$ID`) steht, und patcht dann Code/Daten dort
zur Laufzeit — inklusive expliziter Cache-Zeilen-Invalidierung. Ein
generisches "noch nicht initialisiert/gepatcht"-Signal, das an
mehreren, strukturell unterschiedlichen Stellen wiederverwendet wird.

### 9. `F$Alarm` — der erste sicher identifizierte Syscall

`Q9_alarm_dispatch_1390` (nur aufgerufen aus `Q9_proc_slot_cleanup_25f8`
mit Funktionscode 0 = `A$Delete`, beim Prozess-Aufräumen) ist der
Kernel-interne Teil der Implementierung von **`F$Alarm`** — bestätigt
über drei unabhängige Indizien (Registerkonvention `D3`=Intervall/
`D4`=Datum passend zum Handbuch, die `0xB0BD`-Signatur wird beim
Anlegen gesetzt und beim Löschen geprüft, Lazy-Registrierung des
Uhr-Tick-Hooks). Detailliert in `REVERSE_ENGINEERING.md`. Kein eigener
Deskriptor-Pool — Alarm-Deskriptoren sind normale Heap-Objekte
(`Q9_alarm_desc_alloc_162c` → `Q9_fixed_alloc_wrap_12b4` →
`Q9_arena_alloc_526c`).

Die 6-Einträge-Sprungtabelle von `Q9_alarm_dispatch_1390` selbst liegt
**statisch** im Modul (PC-relative Displacement-Worte bei `0x13c6`,
direkt dekodierbar) — dabei fiel auf, dass Kategorie 5 (vermutlich
A$AtDate/A$AtJul) in diesem Kernel-Build **nicht implementiert** ist
(springt direkt zum Fehler-Stub).

Ein Versuch, auf dieselbe Art auch `F$SRqMem`/`F$SRtMem` rein aus dem
statischen Code zu bestätigen, zeigte zunächst eine Grenze: Die
*große* `TRAP #0`-Tabelle (256 Einträge, `Q9_disp_488`) wird **zur
Boot-Zeit im RAM** angelegt, keine fertige Adressliste steht im
Modul-File — per Xref-Suche im Disassemblierungs-Dump nicht zu finden.
Gelöst wurde das anders (s. Abschnitt 10): per Laufzeit-Inspektion im
Emulator.

### 10. Syscall-Tabelle vollständig namentlich zugeordnet (Laufzeit-Verifikation)

Auf Nutzeranregung eine Debug-Sondertaste (`Ctrl-^`) in den separaten
`Q9-Flux`-Emulator eingebaut, die physischen RAM direkt liest (am
emulierten CPU-Kern vorbei, keine MMU-/Privilegien-Hürde). Ergebnis,
mit den öffentlichen `F$`/`I$`-Funktionscodes der Standard-OS-9-API
abgeglichen (Details, komplette Tabelle: `REVERSE_ENGINEERING.md`):

- **`D_ExcJmp`-Format entschlüsselt** (`PEA (v).W ; JMP.L ziel`,
  Array-Index = Vektor−2) — **alle** 62 dokumentierten Vektorgruppen
  (2–63) stimmen exakt mit unserer statischen Analyse überein, keine
  einzige Abweichung.
- **Praktisch die komplette Syscall-Tabelle** (~100 `F$`/`I$`-Funktionen)
  Adressen zugeordnet, u. a. `F$SRqMem`, `F$SRtMem` und — als
  Bestätigung des `F$Alarm`-Funds von Abschnitt 9 — `F$Alarm` selbst.
  Auffällige Muster: ein klarer Block reiner Supervisor-only-Funktionen
  (nur in `D_SysDis`, nicht in `D_UsrDis`), `F$TLink` umgekehrt nur im
  User-Pfad, und mehrere Codes (`F$SSpd`, `F$AllRAM`, `F$POSK`,
  `F$Panic`) trotz definiertem Code nirgends registriert.

Das ist die erste **Laufzeit**-Verifikation der gesamten statischen
Disassemblierung — nicht nur einzelner Adressen, sondern der
kompletten Exception- und Syscall-Dispatch-Architektur.

### Was auf der Karte noch fehlt

- Genaue Zieladressen der Trampolin-Slots (nur zur Boot-Zeit im RAM
  bekannt, bräuchte einen laufenden Emulator zur Inspektion).
- `0x3dee`–`0x403a` (588 Byte) bleibt eine unbeschriftete Fläche.
- Detailsemantik einiger Statusfelder (z. B. `(0x24/0x26/0x28,Arena)`).
- Weitere Syscalls über den `TRAP #0`-Weg direkt zu finden, bräuchte
  entweder das Verfolgen der noch nicht vollständig gelesenen
  Tabellenbefüllung in `Q9_kernel_init_67a0`, oder einen laufenden
  Emulator zur Laufzeit-Inspektion der Tabellen.

## Offene Punkte / noch nicht ausgewertet

Nicht in dieser Notiz, aber im Technical Manual vorhanden und bei Bedarf
nachzuschlagen: `OS-9 Memory Map`, `Colored Memory` (Definition List,
System Memory Cache Lists), `Customization Modules` (`Syscache`, `SSM`,
`FPU/FPSP`), `Process Memory Areas`/`Process State`/`Process Scheduling`
(inkl. `D_MinPty`/`D_MaxAge`), vollständige Vektortabelle (Table 2-9),
sowie `Appendix D: OS-9 for 68K System Calls` (alle Syscalls +
Verfügbarkeit je Kernel-Typ).
