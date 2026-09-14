# Modul- und Syscall-Übersicht für den eigenen Q9-Kernel

Dieses Dokument beantwortet konkret: **welche Module braucht der eigene
Kernel, welche Systemaufrufe bedient jedes davon, und was gehört darüber
hinaus noch an Infrastruktur dazu?** Es ist die "Stückliste" zu
[`OWN_KERNEL_INIT_PLAN.md`](OWN_KERNEL_INIT_PLAN.md) (dort steht das *Wie*
und *Warum* je Entwurfsentscheidung) — hier steht nur, *was* am Ende als
Modul/Funktion existieren muss. Grundlage ausschließlich bereits
verifizierte, intern dokumentierte Funde.

**Zählweise:** die Referenz-Zahlen (61/24/6/1/4/1 = 97 Syscalls) stammen
aus **einem** konkreten 68K-Boot-Image (`dker030s`, Development-Kernel,
Standard-Allocator). Das ist der vollständigste bekannte Datensatz dieser
Session und deshalb die Basis hier — nicht jeder Syscall muss beim
eigenen Kernel gleich beim ersten Bootfähig-Meilenstein existieren (s.
Abschnitt 4, Bauphasen).

## 1. Modul-Landkarte

### 1a. Kernsystem — je einmal vorhanden

| Modul | Rolle | Bedient Syscalls | Muss/Kann | Quelle |
|---|---|---|---|---|
| **Boot-Lader** (kein OS-9-Modul im engeren Sinn) | Legt den System-Global-Bereich an, nullt ihn (16-Byte-Schritte), validiert die Bootdatei-Modulkette (Sync-Wort + 24-Word-XOR-Prüfsumme), springt in den Kernel | keine | **Muss** | Thema 10 |
| **Kernel** | Bootstrap, Exception-/Trap-Dispatch, Scheduler, Speicherallokator, Prozess-Lifecycle | 61 `F$`-Codes (Abschnitt 2a) | **Muss** | Thema 01, 04, 05, 06 |
| **init** (Konfigurationsmodul) | Nicht ausführbar, Typ `Systm`; trägt nur vier Größenfelder (`M$PollSz`/`M$DevCnt`/`M$Procs`/`M$Paths`) für Tabellengrößen beim Kernelstart | keine | **Muss** (auch wenn winzig) | intern dokumentiert |
| **IOMan** | Dreiklang-Linker (`I$Attach` linkt Descriptor→Driver→Fmgr mit Typfiltern `0xF00`/`0xE00`/`0xD00`), `F$Load`, gemeinsamer I$-Dispatcher zum Treiber | 24 `F$`/`I$`-Codes (Abschnitt 2b) | **Muss** | Thema 02, 03, 11 |
| **SysCache** | Cache-Steuerung | 1 `F$`-Code (`F$CCtl`) | Kann (später) | nur Adressbereich bekannt, nicht analysiert |
| **SSM** ("System Security Module") | MMU-/Speicherschutz-Init, als **eigenständiges, optional nachladbares** Modul — nicht Kernel, nicht Treiber | 6 `F$`-Codes (Abschnitt 2c) | Kann (Atomic-Kernel kommt laut Manual ganz ohne aus) | Thema 07 |

### 1b. Der Dreiklang — pro Gerät/Dateisystem, beliebig oft

Diese drei Modularten liefern **keine eigenen `F$`-Syscalls** — sie werden
über IOMans `I$`-Dispatch angesprochen (Abschnitt 2b) und implementieren
intern ihre eigene, callcode-indizierte Sprungtabelle:

| Modul-Art | Rolle | Beispiel (Referenz) | Quelle |
|---|---|---|---|
| **Descriptor** | Reine Daten — beschreibt "was für ein Gerät", verweist per Namen auf Driver + Fmgr | gerätespezifisch, nicht einzeln analysiert | Thema 02 |
| **Driver** | Spricht die Hardware direkt an | `cfide` (68K, lehrbuchmäßiger ATA/IDE-PIO-Treiber) / `scllio` (x86, **kein** Port-I/O — stattdessen `INT 0xFF`-Syscall-Trampolin + 3 indirekte Calls über eine Handler-Liste) | Thema 09 |
| **File-Manager** | Definiert Dateisystem-Semantik, eigene Callcode-Tabelle (68K: 13 Slots ab `M$Exec`, Basis `0x83`; x86: 16 Slots im `m_idata`-Bereich, Basis `Callcode−0x95`) | `RBF` (Block-/Dateisystem) — `I$Write` delegiert komplett an eine Laufzeit-Liste aus (Geräte-ID, Handler-Zeiger)-Paaren (`Q9X_rbf_driver_dispatch`, x86) bzw. ~10 interne `bsr`-Hilfsroutinen (68K) | Thema 03, 08 |

Weitere File-Manager (z. B. `SCF` für zeichenorientierte/serielle Geräte)
sind aus OS-9-Praxis bekannt, aber in dieser Session **nicht** analysiert
— für echte Kompatibilität mit realen 68K-/OS-9000-Installationen müssten
sie bei Bedarf nachgezogen werden (gleiche Vorgehensweise wie bei RBF).

### 1c. x86-spezifischer Sonderfall

| Modul | Rolle | Quelle |
|---|---|---|
| `vectx86` | Trägt feste Handler-Adressen in Kernel-Globals-Felder ein — kein Dreiklang-Mitglied, sondern ein eigenständiger **Bootstrap-Baustein**, den es beim 68K nicht als separates Modul gibt (dort macht das Boot-ROM es implizit) | Thema 10 |

### 1d. Eigene, neue Modularten

Von Andreas ausdrücklich gewünscht ("da bin ich offen für"), aber ohne
Eingrenzung — offene Entscheidung, siehe `OWN_KERNEL_INIT_PLAN.md`
Abschnitt 5, Punkt 5.

## 2. Vollständiger Syscall-Katalog

Quelle: `modules/SYSCALL_MODULE_MAP.md` (per Adressvergleich aus einem
Live-Boot-Image ermittelt, alle 97 im Kernel-Build definierten Codes).
Hier funktional gruppiert statt nach Callcode sortiert — für den
Nachbau ist "was gehört zusammen" hilfreicher als die reine Nummer.

### 2a. Kernel (61 Codes)

| Gruppe | Syscalls |
|---|---|
| Modul-Linking | `F$Link`, `F$UnLink`, `F$TLink`, `F$VModul`, `F$FModul`, `F$GModDr`, `F$UnLoad`, `F$DatMod` |
| Prozess-Lifecycle | `F$Fork`, `F$Wait`, `F$Chain`, `F$Exit`, `F$DFork`, `F$DExec`, `F$DExit`, `F$AllPrc`, `F$DelPrc`, `F$AProc`, `F$NProc`, `F$FindPD`, `F$AllPD`, `F$RetPD`, `F$GPrDsc`, `F$GPrDBT`, `F$GProcP`, `F$ID`, `F$SUser`, `F$UAcct` |
| Scheduling/Signale | `F$SPrior`, `F$STrap`, `F$Sleep`, `F$Send`, `F$Icpt`, `F$SSvc`, `F$Event`, `F$Alarm`, `F$SigMask`, `F$SigReset`, `F$Sema` |
| Speicher (kernelintern) | `F$Mem`, `F$SRqMem`, `F$SRtMem`, `F$SRqCMem` |
| Namen/Muster | `F$PrsNam`, `F$CmpNam` |
| Zeit | `F$Time`, `F$STime`, `F$Julian`, `F$Gregor` |
| System/Diagnose/Sonstiges | `F$CRC`, `F$SetCRC`, `F$SetSys`, `F$SysID`, `F$SysDbg`, `F$CpyMem`, `F$GBlkMp`, `F$Move`, `F$RTE`, `F$Trans` |
| Interrupts | `F$IRQ`, `F$FIRQ` |

Trägt die zentrale Infrastruktur aus Abschnitt 3 (Bootstrap, Exception-
Dispatch, Scheduler, Allokator) — **muss zuerst stehen**, bevor irgendein
anderes Modul sinnvoll arbeiten kann.

### 2b. IOMan (24 Codes)

| Gruppe | Syscalls |
|---|---|
| Modul-/Datei-Laden | `F$Load`, `F$PErr` |
| Path-Bit-Verwaltung | `F$SchBit`, `F$AllBit`, `F$DelBit` |
| I/O-Queue | `F$IOQu`, `F$IODel` |
| Geräte-/Dateizugriff (`I$`-Familie, 16 Codes) | `I$Attach`, `I$Detach`, `I$Dup`, `I$Create`, `I$Open`, `I$MakDir`, `I$ChgDir`, `I$Delete`, `I$Seek`, `I$Read`, `I$Write`, `I$ReadLn`, `I$WritLn`, `I$GetStt`, `I$SetStt`, `I$Close`, `I$SGetSt` |

**Wichtig (Caveat aus `SYSCALL_MODULE_MAP.md`):** "Modul = IOMan" heißt nur
"hier landet der `TRAP #0`/`INT 0xFF` zuerst" — die eigentliche Datei-/
Geräte-Logik der `I$`-Familie läuft über den gemeinsamen Dispatcher direkt
in die Dreiklang-Module weiter (Abschnitt 1b), nicht in IOMan selbst.

### 2c. SSM (6 Codes)

`F$Permit`, `F$Protect`, `F$AllTsk`, `F$DelTsk`, `F$ChkMem`, `F$GSPUMp` —
`F$DelTsk`/`F$ChkMem` zeigen im Referenz-Image je nach Supervisor-/User-
Pfad unterschiedliche Zielmodule (SSM bzw. Kernel-Trampolin) — plausibel
ein interner Weiterreiche-Mechanismus, kein Widerspruch.

### 2d. SysCache (1 Code)

`F$CCtl` — Zielmodul nur über Adressbereich bekannt, nie analysiert.

### 2e. Im Referenz-Build nicht registriert (4 Codes)

`F$SSpd`, `F$AllRAM`, `F$POSK`, `F$Panic` — Callcode ist definiert, aber
im geprüften Kernel-Build zeigt er auf den Fehler-Stub. Für den eigenen
Kernel frei entscheidbar: nachbauen (falls für Kompatibilität mit anderen
68K-Programmen relevant, die diese Codes tatsächlich aufrufen) oder
auslassen.

### 2f. Sonderfall

`F$MBuf` — Supervisor-Zieladresse liegt außerhalb aller bekannten Modul-/
RAM-Bereiche dieser Konfiguration, ungeklärt.

## 3. Was über Module/Syscalls hinaus dazugehört

Das ist Infrastruktur, die **kein eigenes Modul** ist, aber genauso Teil
der Bauliste sein muss — jeweils mit Fundstelle:

| Baustein | Kurzbeschreibung | Quelle |
|---|---|---|
| Modul-Header-Parser | Sync-Wort/Größe/Name/Typ/Einsprungpunkt für alle drei bekannten Layouts (6809/68K/OS-9000) plus einen vierten, eigenen Sync-Wert | Thema 00, bereits als `src/q9moduleheader.h`/`.a` vorbereitet |
| Exception-/Trap-Dispatch-Tabelle | Kompakte Quelltabelle im Modul → beim Boot zur vollen, direkt indizierbaren Tabelle expandiert; 8 gemeinsame Dispatcher-Funktionen für alle CPU-Vektoren | Thema 01, 06 |
| Scheduler-Kern | Zirkuläre, doppelt verkettete Ready-Queue mit Sentinel-Kopf, Prioritäts-Aging, interrupt-maskierte Queue-Operationen | Thema 04 |
| Speicherallokator | Pool → Arena (nach Adressbereich) → Freiliste (nach Größe), Boundary-Tag-Coalescing, Template-Kopie für neue Arena-Deskriptoren | Thema 05 |
| Dreiklang-Linkmechanismus | `F$Link` mit Typfiltern `0xF00`/`0xE00`/`0xD00`, in genau dieser Reihenfolge Descriptor→Driver→Fmgr | Thema 02, 03 |
| Callcode-Dispatch je File-Manager | `(Callcode − Basiswert)` als Index in eine modulinterne Sprungtabelle | Thema 03 |
| Treiber-Dispatch-Pattern | Laufzeit-Liste aus (Geräte-/Treiber-ID, Handler-Zeiger)-Paaren, indirekter Aufruf (x86-Vorbild, Übernahme-Empfehlung für eigene neue Fmgr/Treiber) | Thema 08, 09 |
| `F$Load`-Suchreihenfolge | Erst In-Memory-Modulverzeichnis (Namensvergleich), dann Pfad-vs-Suchlisten-Unterscheidung (`'/'`-Test), erst dann Mass-Storage | Thema 11 |
| Syscall-Auslöser | 68K: `TRAP #0` (naheliegend für Q9-Flux). Für x86-Binärkompatibilität zusätzlich: `INT 0xFF`-Gate | Thema 06, 09 |
| MMU/Speicherschutz | Optional, als eigenständiges SSM-artiges Modul (nicht Kernel-Pflicht) | Thema 07 |

## 4. Vorgeschlagene Baureihenfolge

Abgeleitet aus den Abhängigkeiten oben — jede Phase setzt die vorherige
voraus, nicht umgekehrt:

1. **Modul-Header-Parser** (Voraussetzung für alles Weitere — ohne ihn
   kann nichts als "gültiges Modul" erkannt werden)
2. **Boot-Lader** + **Kernel-Bootstrap** (System-Global-Bereich, Exception-
   Dispatch-Tabelle, Speicherallokator, Ready-Queues, erster
   Ausführungskontext) — Meilenstein: Kernel bootet, ein einzelner
   Prozess läuft
3. **Kernel-Syscalls, Kerngruppe** (Prozess-Lifecycle, Scheduling,
   Speicher, Zeit — Abschnitt 2a) — Meilenstein: mehrere Prozesse,
   `F$Fork`/`F$Exit`/`F$Sleep` funktionieren
4. **IOMan + Dreiklang-Linker** (`F$Link`-Typfilter, `I$Attach`) —
   Voraussetzung für jeden Geräte-/Dateizugriff
5. **Ein erster kompletter Dreiklang** (ein Descriptor + ein Driver + ein
   File-Manager, z. B. RBF-artig) — Meilenstein: echtes I/O
6. **`F$Load`** (Modul aus Mass-Storage nachladen) — Meilenstein:
   Programme von "Platte" starten, nicht nur vorgelinkt im Speicher
7. **Optionale Module**: SSM (MMU), SysCache, weitere File-Manager/Treiber,
   restliche Kernel-Syscalls (Abschnitt 2e/2f)
8. **Eigene, neue Modularten** (offen, s. `OWN_KERNEL_INIT_PLAN.md`
   Abschnitt 5)

**Nicht Teil dieser Liste, weil eine separate, größere Entscheidung:**
echte OS-9000/x86-Module binär auszuführen bräuchte zusätzlich einen
x86-CPU-Interpreter (Musashi deckt nur 68K ab) — siehe
`OWN_KERNEL_INIT_PLAN.md`, Statusabschnitt und Abschnitt 5, Punkt 1.

## Quellen

Wie `OWN_KERNEL_INIT_PLAN.md` — ausschließlich Synthese bereits
verifizierter, intern dokumentierter Funde, keine neuen Behauptungen.
Siehe außerdem [`docs/OWN_KERNEL_INIT_PLAN.md`](OWN_KERNEL_INIT_PLAN.md).

**Erstellt**: 2026-08-15
