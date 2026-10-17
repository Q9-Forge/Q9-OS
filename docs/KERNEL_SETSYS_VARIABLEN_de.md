# F$SetSys: Klassifikation der System-Globals (Arbeitsplan C5)

<!-- erzeugt von tools/setsys_table.py -- nicht von Hand aendern -->

`F$SetSys` liest und aendert System-Globals ueber ihren Offset (`d0.w`, `d1` = Groesse 1/2/4 plus Bit 31 fuer Lesen, `d2` = neuer Wert; Ausgabe `d2` = vorheriger Wert). Nach dem Handbuch (68k_tech.pdf S. 492 f.) darf nur ein Super-User (Gruppe 0) aendern, und veraenderbar sind nur `D_MinPty` und `D_MaxAge`.

Dieser Kernel fuehrt nur einen Teil der Globals an den Originaloffsets; die uebrigen liegen in seinem Speicher auf anderen Daten (live gemessen: `$55E` ist nicht `D_MinPty`, ein Scheduler, der dort las, sperrte jeden Prozess aus). `F$SetSys` liefert deshalb nur Werte, die der Kernel wirklich fuehrt, und sonst `E$UnkSvc` — eine bewusste Abweichung vom Handbuch ("Any system variable may be examined"), weil ein geratener Wert gefaehrlicher ist als ein klarer Fehler.

**Stand:** 108 Variablen im Header, davon 14 lesbar am Originaloffset, 1 mit Q9-eigenem Wert lesbar, 5 lesbar und schreibbar, 87 offen (begruendet), 1 Endmarke.

**Fehler:** Groesse nicht 1/2/4 → `E$Param`; Schreiben ohne Super-User → `E$Permit`; Schreiben einer lesbaren, aber nicht veraenderbaren Variable → `E$Param`; unbekannte oder offene Variable → `E$UnkSvc`.

## Gruende fuer „offen“

- **A** — Q9 fuehrt Prozess-, Pfad- und Moduldaten in eigenen Pools und Listen (s. KERNEL_SPEICHERBELEGUNG_de.md); ein Zeiger auf die Microware-Struktur waere falsch
- **B** — Die Q9-Uhr haelt Tageszahl und Sekunden (q9kernel_clock.c) und den Tickzaehler, nicht diese Felder; als virtuelle Variable moeglich, bisher nicht gebraucht
- **C** — Hardware oder Funktion, die dieses System nicht hat oder nicht modelliert (MMU/SPU, Multiprozessor, Cache-Steuerung, FPU-Zustand, CPU32-Register)
- **D** — Interrupt-Verwaltung: Q9 fuehrt eigene Tabellen (irqtab), das Originalformat wird mit C6 (F$IRQ/F$FIRQ) entschieden
- **E** — Debugger, ROM und Boot: Rueckkehr- und Einstiegsfelder gehoeren zu C7 (F$SysDbg/RomBug) und sind noch nicht umgesetzt
- **F** — Speicherverwaltung: Q9 hat einen eigenen Arena-Allokator, die Felder des Originals (Freiliste, Blockgroessen) gibt es nicht
- **G** — Interner Scheduler- oder Verwaltungszustand, den Q9 anders fuehrt (Alterung, Warteschlangen, Alarm-/Thread-Listen, Fast-Dispatch, Idle)
- **H** — Beim Boot gesetzte Konfiguration oder Kennung, die Q9 nicht als Variable bereithaelt; aus dem Init-Modul oder dem Bauschalter ableitbar, bisher nicht gebraucht

## Tabelle

Spalte „Header“ ist der Pruefstatus der Offsetangabe in `q9sysglob.h` (VERIFIZIERT = am Referenzsystem beobachtet, PLATZHALTER = nur abgeleitet).

| Variable | Offset | Q9 | Grund / Hinweis | Header |
|---|---|---|---|---|
| `D_ID` | `$000` | ⬜ offen H | Sync-Kennzeichen, nach Coldstart gesetzt | PLATZHALTER |
| `D_NOSLEEP` | `$002` | ⬜ offen G | Ungleich 0 verhindert, dass der Systemprozess schlafen geht | PLATZHALTER |
| `D_INIT` | `$020` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_CLOCK` | `$024` | ⬜ offen B | Adresse der Tick-Routine | PLATZHALTER |
| `D_TCKSEC` | `$028` | ✅ lesbar + schreibbar | Q9-Wert 100 (10 ms je Tick); setzbar | PLATZHALTER |
| `D_YEAR` | `$02A` | ⬜ offen B | Jahr | PLATZHALTER |
| `D_MONTH` | `$02C` | ⬜ offen B | Monat | PLATZHALTER |
| `D_DAY` | `$02D` | ⬜ offen B | Tag | PLATZHALTER |
| `D_COMPAT` | `$02E` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_FPU` | `$02F` | ⬜ offen C | FPU-Typ: 0=keine, 1=68881, 2=68882, 40=68040, 60=68060 | PLATZHALTER |
| `D_JULIAN` | `$030` | ⬜ offen B | laufende Tagesnummer im Jahr | PLATZHALTER |
| `D_SECOND` | `$034` | ⬜ offen B | verbleibende Sekunden bis Mitternacht | PLATZHALTER |
| `D_SYSCONF` | `$038` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_IRQFLAG` | `$03A` | ⬜ offen D | IRQ-Statusflag | PLATZHALTER |
| `D_UNKIRQ` | `$03B` | ⬜ offen D | Zaehler fuer unbekannte IRQs in Folge | PLATZHALTER |
| `D_MODDIR` | `$03C` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_PRCDBT` | `$044` | ⬜ offen A | Zeiger auf die Prozessdeskriptor-Tabelle | VERIFIZIERT |
| `D_PTHDBT` | `$048` | ⬜ offen A | Zeiger auf die Pfaddeskriptor-Tabelle | PLATZHALTER |
| `D_PROC` | `$04C` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_SYSPRC` | `$050` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_TICKS` | `$054` | ✅ lesbar | Wert des Q9-Tickzaehlers (D_Ticks selbst wird nicht gepflegt) | PLATZHALTER |
| `D_FPROC` | `$058` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_ABTSTK` | `$05C` | ⬜ offen D | Abort-Stackpointer/Ruecksprungadresse fuer Systemzustands-Bus-Traps | PLATZHALTER |
| `D_SYSSTK` | `$060` | ⬜ offen D | System-IRQ-Stackpointer | PLATZHALTER |
| `D_SYSROM` | `$064` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_EXCJMP` | `$068` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_TOTRAM` | `$06C` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_MINBLK` | `$070` | ⬜ offen F | minimale allozierbare Blockgroesse pro Prozess | PLATZHALTER |
| `D_FREMEM` | `$074` | ⬜ offen F | Kopf der freien Speicherliste | PLATZHALTER |
| `D_BLKSIZ` | `$07C` | ✅ lesbar + schreibbar | Q9-Wert 4096, die Wachstumsgroesse, die csl abfragt; setzbar | PLATZHALTER |
| `D_DEVTBL` | `$080` | ⬜ offen A | Zeiger auf die I/O-Gerätetabelle | PLATZHALTER |
| `D_SPURIRQ` | `$084` | ⬜ offen D | Zaehler fuer Spurious IRQs | PLATZHALTER |
| `D_AUTIRQ2` | `$088` | ⬜ offen D | Polling-Tabellenkoepfe fuer On-Chip-Autovektor-IRQs (68070) | PLATZHALTER |
| `D_VCTIRQ` | `$0A4` | ⬜ offen D | Zeigertabelle fuer vektorisierte Interrupt-Geraete | KONFLIKT |
| `D_SYSDIS` | `$3A4` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_USRDIS` | `$3A8` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_ACTIVQ` | `$3AC` | ⬜ offen G | Kopf der Warteschlange aktiver Prozesse | KONFLIKT |
| `D_SLEEPQ` | `$3B4` | ⬜ offen G | Kopf der Warteschlange schlafender Prozesse | PLATZHALTER |
| `D_WAITQ` | `$3BC` | ⬜ offen G | Kopf der Warteschlange wartender Prozesse | PLATZHALTER |
| `D_ACTAGE` | `$3C4` | ⬜ offen G | Alterungszaehler der aktiven Warteschlange | VERIFIZIERT |
| `D_MPUTYP` | `$3C8` | ⬜ offen C | erkannter CPU-Typ (68000/010/020/030/040/060/070/CPU32) | VERIFIZIERT |
| `D_EVTBL` | `$3CC` | ⬜ offen A | Start-/Endzeiger der System-Event-Tabelle | PLATZHALTER |
| `D_EVID` | `$3D4` | ⬜ offen A | naechste, fortlaufende Event-ID | PLATZHALTER |
| `D_SPUMEM` | `$3D8` | ⬜ offen C | Zeiger auf SPU-Globaldaten (0 = nicht aktiv) | PLATZHALTER |
| `D_ADDRLIM` | `$3DC` | ⬜ offen C | hoechste beim Start gefundene Adresse | PLATZHALTER |
| `D_COMPAT2` | `$3E0` | ✅ lesbar | echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt | VERIFIZIERT |
| `D_SNOOPD` | `$3E1` | ⬜ offen C | ungleich 0, wenn alle Daten-Caches kohaerent/snoopy sind | PLATZHALTER |
| `D_PROCSZ` | `$3E2` | ✅ lesbar (Q9-Wert) | konstant $400 (Groesse des Q9-Prozessdeskriptors) | PLATZHALTER |
| `D_POLTBL` | `$3E4` | ⬜ offen D | Polling-Tabellenkoepfe fuer Autovektor-IRQs | PLATZHALTER |
| `D_ARENA` | `$3FC` | ⬜ offen F | Basis des Arena-/Freispeicher-Listen-Kontrollblocks; Kopf/Ende-Zeiger bei +0x8/+0xC selbstreferenzierend beim Boot initialisiert (Q9_kernel_init_67a0, 0x6879-0x6885); von Q9_arena_alloc_526c referenziert (mehrfach lea/pea $3fc(a6)) | VERIFIZIERT |
| `D_FREEMEM` | `$404` | ⬜ offen F | Kopf der farbklassifizierten freien Speicherliste -- liegt bei Q9_D_ARENA+0x8, vermutlich Teilfeld desselben Kontrollblocks statt eigenstaendige Struktur | PLATZHALTER |
| `D_IPID` | `$40C` | ⬜ offen C | Multiprozessor-Identifikationsnummer | PLATZHALTER |
| `D_CPUS` | `$410` | ⬜ offen C | Zeiger auf ein Array von CPU-Deskriptor-Listenkoepfen | PLATZHALTER |
| `D_IPCMD` | `$414` | ⬜ offen C | Kopf der Inter-Prozessor-Kommandowarteschlange | PLATZHALTER |
| `D_CACHMODE` | `$41C` | ⬜ offen C | CACR-Cachemodus (68020/030/040/060) | PLATZHALTER |
| `D_DISINST` | `$420` | ⬜ offen C | Verschachtelungstiefe der Instruktions-Cache-Deaktivierung | PLATZHALTER |
| `D_DISDATA` | `$424` | ⬜ offen C | Verschachtelungstiefe der Daten-Cache-Deaktivierung | PLATZHALTER |
| `D_CLKMEM` | `$428` | ⬜ offen B | Zeiger auf die statischen Daten des Tick-Threads | PLATZHALTER |
| `D_TICK` | `$42C` | ⬜ offen B | aktueller Tick-Zaehler | PLATZHALTER |
| `D_TSLICE` | `$42E` | ✅ lesbar + schreibbar | Q9-Wert 2 Ticks; setzbar; $76 ist der aeltere Code derselben Zelle | PLATZHALTER |
| `D_SLICE` | `$430` | ⬜ offen B | verbleibende Ticks der aktuellen Zeitscheibe | PLATZHALTER |
| `D_ELAPSE` | `$434` | ⬜ offen B | Ticks bis der Systemprozess geweckt wird | PLATZHALTER |
| `D_THREAD` | `$438` | ⬜ offen G | Kopf der System-Thread-Warteschlange (sofort/absolute Zeit) | KONFLIKT |
| `D_ALARTH` | `$440` | ⬜ offen G | Kopf der zeitgesteuerten Alarm-Threads (relative Zeit) | KONFLIKT |
| `D_SSTKLM` | `$448` | ⬜ offen D | untere Grenze des System-IRQ-Stacks | PLATZHALTER |
| `D_FORKS` | `$44C` | ⬜ offen A | Anzahl aktuell aktiver (geforkter) Prozesse | PLATZHALTER |
| `D_BOOTRAM` | `$450` | ⬜ offen C | beim Boot-ROM-Scan gefundene RAM-Groesse (Integritaetscheck) | PLATZHALTER |
| `D_FPUSIZE` | `$454` | ⬜ offen C | maximale Groesse eines FPU-Zustandsrahmens | VERIFIZIERT |
| `D_FPUMEM` | `$458` | ⬜ offen C | Zeiger auf die Globaldaten des FPU-Emulators | VERIFIZIERT |
| `D_IOGLOB` | `$45C` | ⬜ offen C | hardwareabhaengige System-I/O-Flags | PLATZHALTER |
| `D_DEVSIZ` | `$55C` | ⬜ offen A | Groesse eines Eintrags der IOMan-Geraetetabelle | HANDBUCH |
| `D_MINPTY` | `$55E` | ✅ lesbar + schreibbar | Scheduler-Feld (q9kernel_sched.c): Prozesse darunter werden nicht ausgewaehlt; setzbar, nur Super-User | HANDBUCH |
| `D_MAXAGE` | `$560` | ✅ lesbar + schreibbar | Scheduler-Feld (q9kernel_sched.c): hohe Prioritaet altert nicht, niedrige bis D_MaxAge; setzbar, nur Super-User | HANDBUCH |
| `D_SIEZE` | `$562` | ⬜ offen G | Prozess-ID des Prozesses, der die CPU exklusiv belegt | PLATZHALTER |
| `D_CIGAR` | `$564` | ⬜ offen G | grobe Groessenschaetzung aus Sysglob-/Prozess-/Moduldefinitionen | PLATZHALTER |
| `D_MOVEMIN` | `$568` | ⬜ offen C | Mindestanzahl fuer DMA-/move16-Speicheroperationen | PLATZHALTER |
| `D_PREEMPT` | `$56C` | ⬜ offen G | Systemzustands-Preemption-Sperre (0 = erlaubt) | PLATZHALTER |
| `D_FDISPQ` | `$570` | ⬜ offen G | Zeiger auf die Fast-Dispatch-Warteschlange | PLATZHALTER |
| `D_FDISP` | `$574` | ⬜ offen G | Adresse der Einfuege-Routine fuer Fast-Dispatch-Eintraege | PLATZHALTER |
| `D_PROFMEM` | `$578` | ⬜ offen E | Speicherzeiger des Profilers | PLATZHALTER |
| `D_FIRQVCT` | `$594` | ⬜ offen D | Zeiger auf die schnelle IRQ-Routinen-/Datentabelle | PLATZHALTER |
| `D_VCTJMP` | `$598` | ⬜ offen D | Sicherungstabelle fuer schnelle IRQ-Vektoren/Sprungziele | PLATZHALTER |
| `D_SYSDBG` | `$59C` | ⬜ offen E | Einsprungadresse des Systemdebuggers | ROM |
| `D_DBGMEM` | `$5A0` | ⬜ offen E | Speicherzeiger des Systemdebuggers | PLATZHALTER |
| `D_DBGFLG` | `$5A4` | ⬜ offen E | Aktiv-Flag des Systemdebuggers | PLATZHALTER |
| `D_ALLOCTYPE` | `$5A5` | ⬜ offen F | Typ des verwendeten Speicherallokators | VERIFIZIERT |
| `D_DEVCNT` | `$5A6` | ⬜ offen A | Anzahl der Systemgeraete | PLATZHALTER |
| `D_CACHE` | `$5A8` | ⬜ offen A | Kopf des Disk-Cache-Puffers | PLATZHALTER |
| `D_NUMSIGS` | `$5AC` | ⬜ offen H | Standard-Maximaltiefe fuer Signale | PLATZHALTER |
| `D_PRCDESCSTACK` | `$5AE` | ⬜ offen H | Standard-Stackgroesse eines Prozessdeskriptors | PLATZHALTER |
| `D_FDISPSYS` | `$5B0` | ⬜ offen G | system-eigener Fast-Dispatch-Block | PLATZHALTER |
| `D_KERTYP` | `$5D0` | ⬜ offen H | Kerneltyp (Development oder Atomic) | VERIFIZIERT |
| `D_INIRQ` | `$5D1` | ⬜ offen D | Flag: befinden wir uns im IRQ-Kontext | PLATZHALTER |
| `D_FIRQOFF` | `$5D2` | ⬜ offen D | FIRQ-System: Offset zum Stack-Frame | PLATZHALTER |
| `D_IRQOFF` | `$5D3` | ⬜ offen D | IRQ-System: Offset zum Stack-Frame | PLATZHALTER |
| `D_IRQSPOFF` | `$5D4` | ⬜ offen D | IRQ-System: Offset zum (ISP-)Stackpointer | PLATZHALTER |
| `D_MBAR` | `$5D8` | ⬜ offen C | Modul-Basisadresse (CPU32-Familie) | PLATZHALTER |
| `D_CRYSTAL` | `$5DC` | ⬜ offen C | Boot-Flags | PLATZHALTER |
| `D_IDLE` | `$5E0` | ⬜ offen G | Callout-Routine fuer die Idle-Schleife | PLATZHALTER |
| `D_IDLEDATA` | `$5E4` | ⬜ offen G | Datenzeiger fuer die Idle-Callout-Routine | PLATZHALTER |
| `D_SWITCHES` | `$5E8` | ⬜ offen G | Zaehler fuer Kontextwechsel (Idle-Pruefung) | PLATZHALTER |
| `D_IRQHEADS` | `$600` | ⬜ offen D | IRQ-Kopfregionen (fuer Nicht-MSP-Kernel) | PLATZHALTER |
| `D_ALMQ1` | `$774` | ⬜ offen G | F$Alarm-Warteschlange 1 (sofortige/D1=0-Variante, Q9_alarm_set_157e), sortiert nach Faelligkeit (Knotenfelder +0x20/+0x24), Verkettung ueber +0xC/+0x10; beim Boot als leere Ringliste initialisiert (Q9_kernel_init_67a0, 0x6886-0x688e); Walk/Insert in Q9_alarm_insert_15c4 | VERIFIZIERT |
| `D_ALMQ2` | `$77C` | ⬜ offen G | F$Alarm-Warteschlange 2 (intervallbasierte Variante, Q9_alarm_set_1580), sonst identischer Aufbau zu Q9_D_ALMQ1 | VERIFIZIERT |
| `D_UNKN8A6` | `$8A6` | ⬜ offen H | NEU GEFUNDEN 2026-08-17, Bedeutung noch offen | PLATZHALTER |
| `D_UNKN8A8` | `$8A8` | ⬜ offen H | NEU GEFUNDEN 2026-08-17, Bedeutung noch offen | PLATZHALTER |
| `D_BOOTFLAGS` | `$93C` | ⬜ offen H | NEU GEFUNDEN 2026-08-17 | VERIFIZIERT |
| `D_END` | `$1000` | — | Endmarke des Bereichs ($1000), keine Variable | - |
