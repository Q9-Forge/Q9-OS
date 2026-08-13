/*
 * q9sysglob.h -- Q9-OS eigene Definition des OS-9/68K-Kernel-
 *                System-Global-Bereichs und der Exception-Sprung-
 *                tabelle (C-Variante von q9sysglob.a).
 *
 * Eigenstaendig per Disassemblierung des Original-Kernels rekon-
 * struiert, siehe q9sysglob.a fuer Details und Statuskennzeichnung.
 * Fuer Host-seitiges Tooling/Tests, nicht fuer den echten OS-9-Build.
 */

#ifndef Q9SYSGLOB_H
#define Q9SYSGLOB_H

/* System-Global-Bereich */
#define Q9_D_ID             0x0000  /* Sync-Kennzeichen, nach Coldstart gesetzt [PLATZHALTER] */
#define Q9_D_NOSLEEP        0x0002  /* Ungleich 0 verhindert, dass der Systemprozess schlafen geht [PLATZHALTER] */
#define Q9_D_INIT           0x0020  /* Zeiger auf das Init-Konfigurationsmodul [HANDBUCH] */
#define Q9_D_CLOCK          0x0024  /* Adresse der Tick-Routine [PLATZHALTER] */
#define Q9_D_TCKSEC         0x0028  /* Ticks pro Sekunde [PLATZHALTER] */
#define Q9_D_YEAR           0x002A  /* Jahr [PLATZHALTER] */
#define Q9_D_MONTH          0x002C  /* Monat [PLATZHALTER] */
#define Q9_D_DAY            0x002D  /* Tag [PLATZHALTER] */
#define Q9_D_COMPAT         0x002E  /* Kompatibilitaets-Flags (1) [PLATZHALTER] */
#define Q9_D_FPU            0x002F  /* FPU-Typ: 0=keine, 1=68881, 2=68882, 40=68040, 60=68060 [PLATZHALTER] */
#define Q9_D_JULIAN         0x0030  /* laufende Tagesnummer im Jahr [PLATZHALTER] */
#define Q9_D_SECOND         0x0034  /* verbleibende Sekunden bis Mitternacht [PLATZHALTER] */
#define Q9_D_SYSCONF        0x0038  /* Systemkonfigurations-Flags [PLATZHALTER] */
#define Q9_D_IRQFLAG        0x003A  /* IRQ-Statusflag [PLATZHALTER] */
#define Q9_D_UNKIRQ         0x003B  /* Zaehler fuer unbekannte IRQs in Folge [PLATZHALTER] */
#define Q9_D_MODDIR         0x003C  /* Modulverzeichnis: Start-/Endzeiger [VERIFIZIERT -- (0x3c,A6)/(0x40,A6) in Q9_syscall_27d6 bestaetigt] */
#define Q9_D_PRCDBT         0x0044  /* Zeiger auf die Prozessdeskriptor-Tabelle [VERIFIZIERT -- ID-zu-Deskriptor-Tabelle in 0x3370/0x2cee bestaetigt] */
#define Q9_D_PTHDBT         0x0048  /* Zeiger auf die Pfaddeskriptor-Tabelle [PLATZHALTER] */
#define Q9_D_PROC           0x004C  /* Zeiger auf den aktuell laufenden Prozessdeskriptor [VERIFIZIERT] */
#define Q9_D_SYSPRC         0x0050  /* Zeiger auf den Systemprozess-Deskriptor [VERIFIZIERT -- parallel zu D_Proc beim Boot gesetzt] */
#define Q9_D_TICKS          0x0054  /* fortlaufender Systemtick-Zaehler [PLATZHALTER] */
#define Q9_D_FPROC          0x0058  /* Prozess, dessen Kontext gerade in den FPU-Registern steckt [VERIFIZIERT] */
#define Q9_D_ABTSTK         0x005C  /* Abort-Stackpointer/Ruecksprungadresse fuer Systemzustands-Bus-Traps [PLATZHALTER] */
#define Q9_D_SYSSTK         0x0060  /* System-IRQ-Stackpointer [PLATZHALTER] */
#define Q9_D_SYSROM         0x0064  /* Einsprungpunkt des Boot-ROMs [VERIFIZIERT -- (0x64,A6), Konsolen-Ausgabe ueber (0x8,A1)-Funktionszeiger in 0x850/0x868 bestaetigt] */
#define Q9_D_EXCJMP         0x0068  /* Zeiger auf die Exception-Sprungtabelle (siehe Q9_T_*-Struktur) [VERIFIZIERT] */
#define Q9_D_TOTRAM         0x006C  /* vom Boot-ROM ermittelte Gesamt-RAM-Groesse [VERIFIZIERT -- Register D0 beim Boot direkt hierher kopiert, siehe docs/kernel-walkthrough/01-kernel-bootstrap/] */
#define Q9_D_MINBLK         0x0070  /* minimale allozierbare Blockgroesse pro Prozess [PLATZHALTER] */
#define Q9_D_FREMEM         0x0074  /* Kopf der freien Speicherliste [PLATZHALTER] */
#define Q9_D_BLKSIZ         0x007C  /* minimale allozierbare Systemblockgroesse [PLATZHALTER] */
#define Q9_D_DEVTBL         0x0080  /* Zeiger auf die I/O-Gerätetabelle [PLATZHALTER] */
#define Q9_D_SPURIRQ        0x0084  /* Zaehler fuer Spurious IRQs [PLATZHALTER] */
#define Q9_D_AUTIRQ2        0x0088  /* Polling-Tabellenkoepfe fuer On-Chip-Autovektor-IRQs (68070) [PLATZHALTER] */
#define Q9_D_VCTIRQ         0x00A4  /* Zeigertabelle fuer vektorisierte Interrupt-Geraete [KONFLIKT -- Groesse/Lage unsicher, echte Ready-Queue liegt bei 0x37C mitten in diesem Bereich, siehe q9sysglob.a] */
#define Q9_D_SYSDIS         0x03A4  /* Zeiger auf die System-Service-Dispatch-Tabelle [VERIFIZIERT -- Syscall-Tabelle fuer verschachtelte Aufrufe, siehe Q9_disp_488] */
#define Q9_D_USRDIS         0x03A8  /* Zeiger auf die User-Service-Dispatch-Tabelle [VERIFIZIERT -- Syscall-Tabelle fuer normale User-Aufrufe, siehe Q9_disp_488] */
#define Q9_D_ACTIVQ         0x03AC  /* Kopf der Warteschlange aktiver Prozesse [KONFLIKT -- echte Ready-Queue per Disassemblierung bei 0x37C gefunden, siehe q9sysglob.a und REVERSE_ENGINEERING.md] */
#define Q9_D_SLEEPQ         0x03B4  /* Kopf der Warteschlange schlafender Prozesse [PLATZHALTER] */
#define Q9_D_WAITQ          0x03BC  /* Kopf der Warteschlange wartender Prozesse [PLATZHALTER] */
#define Q9_D_ACTAGE         0x03C4  /* Alterungszaehler der aktiven Warteschlange [VERIFIZIERT -- Aging-Countdown in Q9_scheduler_183a bestaetigt] */
#define Q9_D_MPUTYP         0x03C8  /* erkannter CPU-Typ (68000/010/020/030/040/060/070/CPU32) [VERIFIZIERT -- Register D1 beim Boot direkt hierher kopiert, siehe docs/kernel-walkthrough/01-kernel-bootstrap/] */
#define Q9_D_EVTBL          0x03CC  /* Start-/Endzeiger der System-Event-Tabelle [PLATZHALTER] */
#define Q9_D_EVID           0x03D4  /* naechste, fortlaufende Event-ID [PLATZHALTER] */
#define Q9_D_SPUMEM         0x03D8  /* Zeiger auf SPU-Globaldaten (0 = nicht aktiv) [PLATZHALTER] */
#define Q9_D_ADDRLIM        0x03DC  /* hoechste beim Start gefundene Adresse [PLATZHALTER] */
#define Q9_D_COMPAT2        0x03E0  /* Cache-Kompatibilitaets-/Konfigurationsflags [PLATZHALTER] */
#define Q9_D_SNOOPD         0x03E1  /* ungleich 0, wenn alle Daten-Caches kohaerent/snoopy sind [PLATZHALTER] */
#define Q9_D_PROCSZ         0x03E2  /* Groesse eines Prozessdeskriptors [PLATZHALTER] */
#define Q9_D_POLTBL         0x03E4  /* Polling-Tabellenkoepfe fuer Autovektor-IRQs [PLATZHALTER] */
#define Q9_D_FREEMEM        0x0404  /* Kopf der farbklassifizierten freien Speicherliste [PLATZHALTER] */
#define Q9_D_IPID           0x040C  /* Multiprozessor-Identifikationsnummer [PLATZHALTER] */
#define Q9_D_CPUS           0x0410  /* Zeiger auf ein Array von CPU-Deskriptor-Listenkoepfen [PLATZHALTER] */
#define Q9_D_IPCMD          0x0414  /* Kopf der Inter-Prozessor-Kommandowarteschlange [PLATZHALTER] */
#define Q9_D_CACHMODE       0x041C  /* CACR-Cachemodus (68020/030/040/060) [PLATZHALTER] */
#define Q9_D_DISINST        0x0420  /* Verschachtelungstiefe der Instruktions-Cache-Deaktivierung [PLATZHALTER] */
#define Q9_D_DISDATA        0x0424  /* Verschachtelungstiefe der Daten-Cache-Deaktivierung [PLATZHALTER] */
#define Q9_D_CLKMEM         0x0428  /* Zeiger auf die statischen Daten des Tick-Threads [PLATZHALTER] */
#define Q9_D_TICK           0x042C  /* aktueller Tick-Zaehler [PLATZHALTER] */
#define Q9_D_TSLICE         0x042E  /* Ticks pro Zeitscheibe [PLATZHALTER] */
#define Q9_D_SLICE          0x0430  /* verbleibende Ticks der aktuellen Zeitscheibe [PLATZHALTER] */
#define Q9_D_ELAPSE         0x0434  /* Ticks bis der Systemprozess geweckt wird [PLATZHALTER] */
#define Q9_D_THREAD         0x0438  /* Kopf der System-Thread-Warteschlange (sofort/absolute Zeit) [PLATZHALTER] */
#define Q9_D_ALARTH         0x0440  /* Kopf der zeitgesteuerten Alarm-Threads (relative Zeit) [PLATZHALTER] */
#define Q9_D_SSTKLM         0x0448  /* untere Grenze des System-IRQ-Stacks [PLATZHALTER] */
#define Q9_D_FORKS          0x044C  /* Anzahl aktuell aktiver (geforkter) Prozesse [PLATZHALTER] */
#define Q9_D_BOOTRAM        0x0450  /* beim Boot-ROM-Scan gefundene RAM-Groesse (Integritaetscheck) [PLATZHALTER] */
#define Q9_D_FPUSIZE        0x0454  /* maximale Groesse eines FPU-Zustandsrahmens [VERIFIZIERT] */
#define Q9_D_FPUMEM         0x0458  /* Zeiger auf die Globaldaten des FPU-Emulators [VERIFIZIERT] */
#define Q9_D_IOGLOB         0x045C  /* hardwareabhaengige System-I/O-Flags [PLATZHALTER] */
#define Q9_D_DEVSIZ         0x055C  /* Groesse eines Eintrags der IOMan-Geraetetabelle [HANDBUCH] */
#define Q9_D_MINPTY         0x055E  /* minimale Systemprozesspriorität [HANDBUCH] */
#define Q9_D_MAXAGE         0x0560  /* maximales Prioritaets-Alterungslimit [HANDBUCH] */
#define Q9_D_SIEZE          0x0562  /* Prozess-ID des Prozesses, der die CPU exklusiv belegt [PLATZHALTER] */
#define Q9_D_CIGAR          0x0564  /* grobe Groessenschaetzung aus Sysglob-/Prozess-/Moduldefinitionen [PLATZHALTER] */
#define Q9_D_MOVEMIN        0x0568  /* Mindestanzahl fuer DMA-/move16-Speicheroperationen [PLATZHALTER] */
#define Q9_D_PREEMPT        0x056C  /* Systemzustands-Preemption-Sperre (0 = erlaubt) [PLATZHALTER] */
#define Q9_D_FDISPQ         0x0570  /* Zeiger auf die Fast-Dispatch-Warteschlange [PLATZHALTER] */
#define Q9_D_FDISP          0x0574  /* Adresse der Einfuege-Routine fuer Fast-Dispatch-Eintraege [PLATZHALTER] */
#define Q9_D_PROFMEM        0x0578  /* Speicherzeiger des Profilers [PLATZHALTER] */
#define Q9_D_FIRQVCT        0x0594  /* Zeiger auf die schnelle IRQ-Routinen-/Datentabelle [PLATZHALTER] */
#define Q9_D_VCTJMP         0x0598  /* Sicherungstabelle fuer schnelle IRQ-Vektoren/Sprungziele [PLATZHALTER] */
#define Q9_D_SYSDBG         0x059C  /* Einsprungadresse des Systemdebuggers [PLATZHALTER] */
#define Q9_D_DBGMEM         0x05A0  /* Speicherzeiger des Systemdebuggers [PLATZHALTER] */
#define Q9_D_DBGFLG         0x05A4  /* Aktiv-Flag des Systemdebuggers [PLATZHALTER] */
#define Q9_D_ALLOCTYPE      0x05A5  /* Typ des verwendeten Speicherallokators [VERIFIZIERT] */
#define Q9_D_DEVCNT         0x05A6  /* Anzahl der Systemgeraete [PLATZHALTER] */
#define Q9_D_CACHE          0x05A8  /* Kopf des Disk-Cache-Puffers [PLATZHALTER] */
#define Q9_D_NUMSIGS        0x05AC  /* Standard-Maximaltiefe fuer Signale [PLATZHALTER] */
#define Q9_D_PRCDESCSTACK   0x05AE  /* Standard-Stackgroesse eines Prozessdeskriptors [PLATZHALTER] */
#define Q9_D_FDISPSYS       0x05B0  /* system-eigener Fast-Dispatch-Block [PLATZHALTER] */
#define Q9_D_KERTYP         0x05D0  /* Kerneltyp (Development oder Atomic) [VERIFIZIERT] */
#define Q9_D_INIRQ          0x05D1  /* Flag: befinden wir uns im IRQ-Kontext [PLATZHALTER] */
#define Q9_D_FIRQOFF        0x05D2  /* FIRQ-System: Offset zum Stack-Frame [PLATZHALTER] */
#define Q9_D_IRQOFF         0x05D3  /* IRQ-System: Offset zum Stack-Frame [PLATZHALTER] */
#define Q9_D_IRQSPOFF       0x05D4  /* IRQ-System: Offset zum (ISP-)Stackpointer [PLATZHALTER] */
#define Q9_D_MBAR           0x05D8  /* Modul-Basisadresse (CPU32-Familie) [PLATZHALTER] */
#define Q9_D_CRYSTAL        0x05DC  /* Boot-Flags [PLATZHALTER] */
#define Q9_D_IDLE           0x05E0  /* Callout-Routine fuer die Idle-Schleife [PLATZHALTER] */
#define Q9_D_IDLEDATA       0x05E4  /* Datenzeiger fuer die Idle-Callout-Routine [PLATZHALTER] */
#define Q9_D_SWITCHES       0x05E8  /* Zaehler fuer Kontextwechsel (Idle-Pruefung) [PLATZHALTER] */
#define Q9_D_IRQHEADS       0x0600  /* IRQ-Kopfregionen (fuer Nicht-MSP-Kernel) [PLATZHALTER] */
#define Q9_D_END          0x1000

/* Exception-Sprungtabelle (Basis: *Q9_D_ExcJmp) */
#define Q9_T_COLDSP         0x0000  /* Reset: initialer Supervisor-Stackpointer [HANDBUCH] */
#define Q9_T_COLDPC         0x0004  /* Reset: initialer Programmzaehler (Coldstart-Einsprung) [HANDBUCH] */
#define Q9_T_BUSERR         0x0008  /* Bus Error [HANDBUCH] */
#define Q9_T_ADDERR         0x000C  /* Address Error [HANDBUCH] */
#define Q9_T_ILLINS         0x0010  /* Illegal Instruction [HANDBUCH] */
#define Q9_T_ZERDIV         0x0014  /* Integer-Division durch Null [HANDBUCH] */
#define Q9_T_CHK            0x0018  /* CHK/CHK2-Instruktion [HANDBUCH] */
#define Q9_T_TRAPV          0x001C  /* TRAPV/TRAPcc/FTRAPcc-Instruktion [HANDBUCH] */
#define Q9_T_PRIV           0x0020  /* Privilegverletzung [HANDBUCH] */
#define Q9_T_TRACE          0x0024  /* Trace-Exception [HANDBUCH] */
#define Q9_T_E1010          0x0028  /* Line-1010-Emulator (A-Line) [HANDBUCH] */
#define Q9_T_E1111          0x002C  /* Line-1111-Emulator (F-Line) -- vermutlich hier der EA-Decoder-Aufrufer [VERIFIZIERT] */
#define Q9_T_CPROTO         0x0034  /* Koprozessor-Protokollverletzung [HANDBUCH] */
#define Q9_T_STKFMT         0x0038  /* Stack-Frame-Formatfehler [HANDBUCH] */
#define Q9_T_UNIRQ          0x003C  /* uninitialisierter Interrupt-Vektor [HANDBUCH] */
#define Q9_T_SPURIO         0x0060  /* Spurious Interrupt [HANDBUCH] */
#define Q9_T_AUTIRQ         0x0064  /* Autovektor-Interrupts Level 1-7 [HANDBUCH] */
#define Q9_T_TRAP           0x0080  /* TRAP #0-#15 Vektoren (Systemaufrufe liegen hier, TRAP #0 = Offset 0) [VERIFIZIERT] */
#define Q9_T_FPUNORDC       0x00C0  /* FP: Branch/Set bei unordered Condition [VERIFIZIERT] */
#define Q9_T_FPINXACT       0x00C4  /* FP: ungenaues Ergebnis [VERIFIZIERT] */
#define Q9_T_FPDIVZER       0x00C8  /* FP: Division durch Null [VERIFIZIERT] */
#define Q9_T_FPUNDRFL       0x00CC  /* FP: Unterlauf [VERIFIZIERT] */
#define Q9_T_FPOPRERR       0x00D0  /* FP: Operandenfehler [VERIFIZIERT] */
#define Q9_T_FPOVERFL       0x00D4  /* FP: Ueberlauf [VERIFIZIERT] */
#define Q9_T_FPNOTNUM       0x00D8  /* FP: signalisierendes NaN [VERIFIZIERT] */
#define Q9_T_FPUNDATA       0x00DC  /* FP: nicht implementierter Datentyp -- vermutlich unser 0xb04-Handler [VERIFIZIERT] */
#define Q9_T_MMUCONF        0x00E0  /* PMMU-Konfigurationsfehler [HANDBUCH] */
#define Q9_T_AUTIRQ2        0x00E4  /* 68070 On-Chip Autovektor-Interrupts Level 1-7 [PLATZHALTER] */
#define Q9_T_VCTIRQ         0x0100  /* vektorisierte Interrupts (nutzerdefiniert) [HANDBUCH] */
#define Q9_T_END          0x400

#endif /* Q9SYSGLOB_H */
