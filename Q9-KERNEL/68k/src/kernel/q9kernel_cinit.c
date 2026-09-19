/*
 * q9kernel_cinit.c -- Q9-OS eigener Kernel: C-Fortsetzung nach dem
 *                     Assembler-Einstieg (q9kernel_entry.a, Q9K_Entry).
 *
 * ERSTER ENTWURF, 2026-08-17 -- ERFOLGREICH gegen die komplette echte
 * Microware-C-Pipeline getestet (xcc -> cpfe -> ilink -> iopt -> be68k
 * -> opt68k -> r68, ueber ein minimales Makefile + mwos-build): alle
 * Stufen "exit status = 0", echtes 375-Byte-Objekt (.r) erzeugt. Nicht
 * mit dem Kernel-Modul selbst (q9kernel_entry.a) gelinkt -- dafuer
 * braeuchte es ein gemeinsames Makefile-Ziel, noch nicht angelegt.
 *
 * NACHTRAG 2026-08-18: bindet jetzt q9kernel_config.h ein, braucht also
 * zwingend -DQ9K_KERNEL_ATOMIC/-DQ9K_KERNEL_DEVELOPMENT PLUS
 * -DQ9K_ALLOC_STANDARD/-DQ9K_ALLOC_BUDDY beim Bauen (sonst #error,
 * s. q9kernel_config.h) -- inhaltlich noch ohne Wirkung, reine
 * Vorbereitung fuer kuenftigen variantenabhaengigen Code. Erneut real
 * kompiliert (Development+Standard), weiterhin exit status = 0.
 * BEWUSST klassische C-Typen (unsigned long/short/char) statt stdint.h/
 * uint32_t -- die xcc-Pipeline zeigt Defines wie _OSK/_MPF68000/_BIG_END,
 * die auf einen aelteren C-Sprachstand hindeuten; stdint.h-Verfuegbarkeit
 * fuer DIESE Toolchain nicht verifiziert (anders als q9moduleheader.c,
 * das explizit reines Host-gcc-Tooling ist, siehe dessen eigenen
 * Kopfkommentar -- diese Datei hier NICHT).
 *
 * Wenn Q9K_CInit zurueckkehrt, faengt Q9K_Entry's Q9K_HaltLoop das ab
 * (kein Absturz, aber auch kein Fortschritt) -- bewusst so belassen statt
 * hier zusaetzlich eine eigene Panik-Schleife zu duplizieren.
 *
 * Kernel-Globals-Zugriff: Basisadresse ist bei diesem Kernel fest $000000
 * (s. q9kernel_entry.a, Q9K_GlobBase) -- die Offset-Konstanten aus
 * src/q9sysglob.h sind dadurch fuer UNS direkt absolute Adressen, kein
 * A6-relativer Zugriff noetig wie beim echten 68K-Referenzkernel. Deshalb
 * hier reine Pointer-Casts auf absolute Adressen, keine A6-Simulation.
 *
 * In dieser Runde implementiert: die sechs leeren, zirkulaeren
 * Bereitschafts-/Warteschlangen (Kopf=Schwanz=sich selbst), exakt nach
 * dem verifizierten Fund aus intern dokumentiert
 * README.md ("Zweiter Bonus-Fund"). ALLES Weitere aus der Boot-Reihenfolge
 * (intern dokumentiert: Speichergroesse/Arena richtig
 * aufsetzen -- nicht nur die leere Liste --, Dispatch-Tabelle, Init-Modul-
 * Suche/Schritt 6a, Prozess-/Pfad-Tabellen, Scheduler-Sprung) ist bewusst
 * NICHT implementiert, klar als TODO markiert, keine Attrappen/Fake-Logik.
 *
 * NACHTRAG 2026-08-18: die Speicherregion-Liste fuer Schritt 6a ist
 * inzwischen geklaert und in q9kernel_entry.a als Q9K_BootList gesichert
 * (die urspruengliche "keine Quelle"-Frage war ein Irrtum -- der Boot-
 * ROM uebergibt sie ueber sp beim Einsprung, s. Thema 01). Ebenfalls
 * fertig: Q9K_CheckSyncWord/Q9K_ValidModuleHeader (q9kernel_modcheck.c)
 * fuer die Kandidatenpruefung. Der eigentliche Such-/Vergleichs-Code
 * (Namensvergleich "init", Revisions-Tiebreak) fehlt noch.
 *
 * Ebenfalls neu: die Modulverzeichnis-Anlage (Q9_D_MODDIR) direkt nach
 * der Init-Suche haengt an M$MDirSz AUS dem gefundenen Init-Modul --
 * kann also erst NACH Schritt 6a passieren, nicht parallel dazu (echte
 * Reihenfolge-Zwangslaeufigkeit im Original, s. Thema 01, nicht nur
 * Design-Praeferenz). CPU-Anzahl fuer SMP kommt optional ueber dieselbe
 * Art Erweiterungsmechanismus (Q9K_GetCpuCount, q9kernel_initext.c) --
 * Default 1, falls das Init-Modul keine Q9-Erweiterung hat.
 */

#include "../q9sysglob.h"
#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned char  Q9_u8;

/* Aus q9kernel_modsearch.c/q9kernel_initext.c -- externe Deklarationen
 * statt gemeinsamer Header, gleiche bewusst schlanke Konvention wie
 * ueberall in diesem Verzeichnis. */
extern const Q9_u8 *Q9K_FindModuleByName(const Q9_u8 *regionList, const char *targetName, Q9_u32 *outAvailableLen);
extern Q9_u32 Q9K_GetCpuCount(const Q9_u8 *initModAddr, Q9_u32 availableLen);
extern void Q9K_ArenaInit(Q9_u32 freeBase, Q9_u32 freeSize);
extern Q9_u32 Q9K_BuildExcTable(void);
extern Q9_u32 Q9K_SetupTables(const Q9_u8 *initMod);
extern Q9_u32 Q9K_ProcCreate(Q9_u32 entryPC, Q9_u8 priority);  /* q9kernel_firstproc.c */
extern Q9_u32 Q9K_SchedFirstPick(void);    /* q9kernel_sched.c -- waehlt+setzt Q9_D_PROC, 0 = keiner angelegt */
extern void   Q9K_SchedRun(void);          /* q9kernel_entry.a, kein Ruecksprung vorgesehen */
extern void   Q9K_TimerActivate(void);     /* q9kernel_entry.a -- aktiviert den Board-Timer (Level 6, Autovector 30) */
extern void   Q9K_TestProcA(void);         /* q9kernel_entry.a -- Test-"Prozess" A, s. dortigen Kommentar */
extern void   Q9K_TestProcB(void);         /* q9kernel_entry.a -- Test-"Prozess" B, s. dortigen Kommentar */
extern void   Q9K_StartupProc(void);       /* q9kernel_entry.a -- minimal sysgo-style startup test */
extern Q9_u32 Q9K_ModDirPopulateFromBootList(const Q9_u8 *bootList); /* q9kernel_moddir.c */
extern void   Q9K_SysFLink(void);    /* q9kernel_entry.a, TRAP-#0-Handler fuer F$Link (Callcode 0x00) */
extern void   Q9K_SysFUnLink(void);  /* q9kernel_entry.a, TRAP-#0-Handler fuer F$UnLink (Callcode 0x02) */
extern void   Q9K_SysFFork(void);    /* q9kernel_entry.a, TRAP-#0-Handler fuer F$Fork (Callcode 0x03) */
extern void   Q9K_SysFWait(void);    /* q9kernel_entry.a, TRAP-#0-Handler fuer F$Wait (Callcode 0x04) */
extern void   Q9K_SysFExit(void);    /* q9kernel_entry.a, TRAP-#0-Handler fuer F$Exit (Callcode 0x06) */
extern void   Q9K_SysFSleep(void);   /* q9kernel_entry.a, TRAP-#0-Handler fuer F$Sleep (Callcode 0x0a) */
extern void   Q9K_SysFSRqMem(void);  /* q9kernel_entry.a, TRAP-#0-Handler fuer F$SRqMem (Callcode 0x28) */
extern void   Q9K_SysFSRtMem(void);  /* q9kernel_entry.a, TRAP-#0-Handler fuer F$SRtMem (Callcode 0x29) */
extern void   Q9K_SysFSSvc(void);    /* q9kernel_entry.a, TRAP-#0-Handler fuer F$SSvc (Callcode 0x32) */
extern void   Q9K_SysFGProcP(void);  /* q9kernel_entry.a, TRAP-#0-Handler fuer F$GProcP (Callcode 0x37) */
extern void   Q9K_SysFIOpen(void);
extern void   Q9K_SysFIDup(void);   /* q9kernel_entry.a, I$Dup (Callcode 0x82) */
extern void   Q9K_SysFIWritLn(void); /* q9kernel_entry.a, I$WritLn (Callcode 0x8c) */
extern void   Q9K_SysFIWrite(void);  /* q9kernel_entry.a, I$Write (Callcode 0x8a) */
extern void   Q9K_SysFIRead(void);   /* q9kernel_entry.a, I$Read (Callcode 0x89) */
extern void   Q9K_SysFIReadLn(void); /* q9kernel_entry.a, I$ReadLn (Callcode 0x8b) */
extern void   Q9K_SysFIGetStt(void); /* q9kernel_entry.a, I$GetStt (Callcode 0x8d) */
extern void   Q9K_SysFISGetSt(void); /* q9kernel_entry.a, I$SGetSt (Callcode 0x92) */
extern void   Q9K_SysFISetStt(void); /* q9kernel_entry.a, I$SetStt (Callcode 0x8e) */
extern void   Q9K_SysFISeek(void);   /* q9kernel_entry.a, I$Seek (Callcode 0x88) */
extern void   Q9K_SysFIClose(void); /* q9kernel_entry.a, I$Close (Callcode 0x8f) */
extern void   Q9K_SysFIChgDir(void); /* q9kernel_entry.a, I$ChgDir (Callcode 0x86) */
extern void   Q9K_SysFAllPD(void);   /* q9kernel_entry.a, F$AllPD (Callcode 0x30) */
extern void   Q9K_SysFIRQ(void);     /* q9kernel_entry.a, F$IRQ  (Callcode 0x2a) */
extern void   Q9K_SysFChkMem(void);  /* q9kernel_entry.a, F$ChkMem (Callcode 0x58) */
extern void   Q9K_SysFPermit(void);  /* q9kernel_entry.a, F$Permit (Callcode 0x3a) */
extern void   Q9K_SysFProtect(void); /* q9kernel_entry.a, F$Protect (Callcode 0x3b) */
extern void   Q9K_SysFAllTsk(void);  /* q9kernel_entry.a, F$AllTsk (Callcode 0x3f) */
extern void   Q9K_SysFDelTsk(void);  /* q9kernel_entry.a, F$DelTsk (Callcode 0x40) */
extern void   Q9K_SysFGSPUMp(void);  /* q9kernel_entry.a, F$GSPUMp (Callcode 0x5b) */
extern void   Q9K_SysFSend(void);    /* q9kernel_entry.a, F$Send   (Callcode 0x08) */
extern void   Q9K_SysFPrsNam(void);  /* q9kernel_entry.a, F$PrsNam (Callcode 0x10) */   /* q9kernel_entry.a, TRAP-#0-Handler fuer I$Open (Callcode 0x84) */
extern void   Q9K_SysFID(void);      /* q9kernel_entry.a, TRAP-#0-Handler fuer F$ID (Callcode 0x0c) */
extern void   Q9K_SysFSPrior(void);  /* q9kernel_entry.a, TRAP-#0-Handler fuer F$SPrior (Callcode 0x0d) */
extern void   Q9K_SysFCmpNam(void);  /* q9kernel_entry.a, F$CmpNam (Callcode 0x11) */
extern void   Q9K_SysFCpyMem(void);  /* q9kernel_entry.a, F$CpyMem (Callcode 0x1b) */
extern void   Q9K_SysFSUser(void);   /* q9kernel_entry.a, F$SUser  (Callcode 0x1c) */
extern void   Q9K_SysFCRC(void);     /* q9kernel_entry.a, F$CRC    (Callcode 0x17) */
extern void   Q9K_SysFUnLoad(void);  /* q9kernel_entry.a, F$UnLoad (Callcode 0x1d) */
extern void   Q9K_SysFFModul(void);  /* q9kernel_entry.a, F$FModul (Callcode 0x4e) */
extern void   Q9K_SysFSetCRC(void);  /* q9kernel_entry.a, F$SetCRC (Callcode 0x26) */
extern void   Q9K_SysFGModDr(void);  /* q9kernel_entry.a, F$GModDr (Callcode 0x1a) */
extern void   Q9K_SysFGPrDsc(void);  /* q9kernel_entry.a, F$GPrDsc (Callcode 0x18) */
extern void   Q9K_SysFJulian(void);  /* q9kernel_entry.a, F$Julian (Callcode 0x20) */
extern void   Q9K_SysFGregor(void);  /* q9kernel_entry.a, F$Gregor (Callcode 0x54) */
extern void   Q9K_SysFAProc(void);   /* q9kernel_entry.a, F$AProc  (Callcode 0x2c) */
extern void   Q9K_SysFGPrDBT(void);  /* q9kernel_entry.a, F$GPrDBT (Callcode 0x1f) */
extern void   Q9K_SysFAllPrc(void);  /* q9kernel_entry.a, F$AllPrc (Callcode 0x4b) */
extern void   Q9K_SysFDelPrc(void);  /* q9kernel_entry.a, F$DelPrc (Callcode 0x4c) */
extern void   Q9K_SysFSysID(void);   /* q9kernel_entry.a, F$SysID  (Callcode 0x55) */
extern void   Q9K_SysFFindPD(void);  /* q9kernel_entry.a, F$FindPD (Callcode 0x2f) */
extern void   Q9K_SysFSigMask(void); /* q9kernel_entry.a, F$SigMask(Callcode 0x57) */
extern void   Q9K_SysFIcpt(void);    /* q9kernel_entry.a, F$Icpt   (Callcode 0x09) */
extern void   Q9K_SysFTrans(void);   /* q9kernel_entry.a, F$Trans  (Callcode 0x60) */
extern void   Q9K_SysFDatMod(void);  /* q9kernel_entry.a, F$DatMod (Callcode 0x25) */
extern void   Q9K_SysFAlarm(void);   /* q9kernel_entry.a, F$Alarm  (Callcode 0x56) */
extern void   Q9K_SysFPanic(void);   /* q9kernel_entry.a, TRAP-#0-Handler fuer F$Panic (Callcode 0x5e) */
extern void   Q9K_SysFRetPD(void);        /* q9kernel_entry.a, F$RetPD (Callcode 0x31) */
extern void   Q9K_SysFMove(void);         /* q9kernel_entry.a, F$Move  (Callcode 0x38) */
extern void   Q9K_SysFVModul(void);       /* q9kernel_entry.a, F$VModul  (Callcode 0x2e) */
extern void   Q9K_SysFSRqCMem(void);      /* q9kernel_entry.a, F$SRqCMem (Callcode 0x5c) */
extern void   Q9K_SysFTLink(void);        /* q9kernel_entry.a, F$TLink   (Callcode 0x21) */
extern void   Q9K_SysFCCtl(void);         /* q9kernel_entry.a, F$CCtl  (Callcode 0x5a) */
extern void   Q9K_SysFSetSys(void);       /* q9kernel_entry.a, F$SetSys (Callcode 0x27) */
extern void   Q9K_SysFTime(void);         /* q9kernel_entry.a, F$Time (Callcode 0x15) */
extern void   Q9K_SysFSTime(void);        /* q9kernel_entry.a, F$STime (Callcode 0x16) */
extern void   Q9K_SysFSchBit(void);       /* q9kernel_entry.a, F$SchBit (Callcode 0x12) */
extern void   Q9K_SysFAllBit(void);       /* q9kernel_entry.a, F$AllBit (Callcode 0x13) */
extern void   Q9K_SysFDelBit(void);       /* q9kernel_entry.a, F$DelBit (Callcode 0x14) */
extern void   Q9K_SysFGBlkMp(void);       /* q9kernel_entry.a, F$GBlkMp (Callcode 0x19) */
extern void   Q9K_SysFSema(void);         /* q9kernel_entry.a, F$Sema (Callcode 0x62) */
extern void   Q9K_SysFChain(void);        /* q9kernel_entry.a, F$Chain (Callcode 0x05) */
extern void   Q9K_SysFRTE(void);          /* q9kernel_entry.a, F$RTE (Callcode 0x1E) */
extern void   Q9K_SysFSigReset(void);     /* q9kernel_entry.a, F$SigReset (Callcode 0x63) */
extern void   Q9K_SysFSTrap(void);        /* q9kernel_entry.a, F$STrap (Callcode 0x0E) */
extern void   Q9K_SysFEvent(void);        /* q9kernel_entry.a, F$Event (Callcode 0x53) */
extern void   Q9K_SysEventImpl(void);     /* q9kernel_event.c -- Adresse s. unten */
extern void   Q9K_SysEventWaitImpl(void); /* q9kernel_event.c, zweiter Teil von Ev$Wait */
extern void   Q9K_SysNProcImpl(void);     /* q9kernel_nproc.c, C-Teil von F$NProc */
/* C-Teile der zuletzt gelinkten Module, ueber Zellen erreicht -- s. den
 * ausfuehrlichen Grund bei Q9K_StrapImplPtr in q9kernel_entry.a: ein
 * "bsr" bis dorthin sprengt die 16-Bit-Reichweite. */
extern void   Q9K_SysSTrapImpl(void);
extern void   Q9K_SysRTEImpl(void);
extern void   Q9K_SysSigResetImpl(void);
extern void   Q9K_SysChainImpl(void);
extern void   Q9K_SysChainReleaseImpl(void);
extern void   Q9K_SysSemaImpl(void);
extern void   Q9K_SysSemaWaitImpl(void);
extern void   Q9K_SysFNProc(void);        /* q9kernel_entry.a, Handler fuer F$NProc (0x2d) */
extern void   Q9K_SysUnimplemented(void); /* q9kernel_entry.a, genereller Fehler-Stub fuer alle nicht registrierten Slots */
extern void   Q9K_MemTraceInit(void);     /* q9kernel_debug.c */
extern void   Q9K_ProcMemTrackInit(void); /* q9kernel_sysmem.c */
/* TEMPORAERE DIAGNOSE (2026-08-18) -- s. Kopfkommentar bei Q9K_Entry in
 * q9kernel_entry.a. Vor dem naechsten "echten" Meilenstein-Commit
 * wieder entfernen oder hinter ein Q9K_DIAG-Flag stellen (TODO). */
extern void Q9K_Diag4(void);
extern void Q9K_Diag5(void);
extern void Q9K_Diag6(void);

/* Eigene Kernel-Global-Erweiterungen (KEIN Feld aus dem echten Kernel-
 * Layout, deshalb hier lokal definiert statt in q9sysglob.h, s. dessen
 * eigenen Kopfkommentar) -- direkt hinter dem legacy-kompatiblen
 * Bereich (Q9_D_End=$1000):
 *   Q9K_BootList  ($1000, 64x8=512 Byte)  -- s. q9kernel_entry.a
 *   Q9K_CpuCount  ($1200, 4 Byte)          -- neu hier, Ergebnis von
 *                                             Q9K_GetCpuCount, noch von
 *                                             niemandem konsumiert
 *                                             (kein SMP-Scheduler
 *                                             existiert bisher) --
 *                                             trotzdem schon ein
 *                                             permanenter Platz dafuer,
 *                                             intern dokumentiert,
 *                                             SMP-Abschnitt. */
#define Q9K_BOOTLIST_ADDR   0x1000UL
#define Q9K_CPUCOUNT_ADDR   0x1200UL

/* Eigene Ready-Queue-Sentinel-Adresse (Abschnitt "Scheduler", 2026-08-21)
 * -- MUSS mit Q9K_READYQ_SENTINEL_ADDR in q9kernel_sched.c uebereinstimmen.
 * ECHTER BUG GEFUNDEN + GEFIXT (per Boot-Test, s. ausfuehrlichen
 * Kommentar dort): die reale Q9_D_ACTIVQ-Adresse ($3AC) ist dafuer NICHT
 * geeignet -- sie liegt zu dicht neben anderen echten, dokumentierten
 * Kernel-Globals (Q9_D_ACTIVQ+0x34 kollidiert wortgleich mit dem echten
 * Q9_D_COMPAT2), um dort zusaetzlich eigene Next/Prev-Selbstverweis-
 * Felder bei +0x30/+0x34 unterzubringen. Q9_D_ACTIVQ selbst bleibt weiter
 * unten als leere Ringliste initialisiert (Kompat-Vollstaendigkeit, TODO
 * bei Gelegenheit pruefen ob ueberhaupt noch noetig), wird aber von
 * unserem eigenen Scheduler NICHT mehr gelesen/geschrieben. */
#define Q9K_READYQ_SENTINEL_ADDR 0x1240UL

/* Eigene Wait-Queue-Sentinel-Adresse (Abschnitt "F$Exit/F$Wait",
 * 2026-08-22) -- MUSS mit Q9K_WAITQ_SENTINEL_ADDR in q9kernel_sched.c
 * uebereinstimmen, gleiche Begruendung/Kollisionsvermeidung wie oben bei
 * Q9K_READYQ_SENTINEL_ADDR (dort ausfuehrlich dokumentiert, s. dortigen
 * Kommentar). Q9_D_WAITQ selbst bleibt weiter unten als leere Ringliste
 * initialisiert (Kompat-Vollstaendigkeit), wird aber von unserem eigenen
 * F$Wait NICHT mehr gelesen/geschrieben. */
#define Q9K_WAITQ_SENTINEL_ADDR 0x12A0UL

/* Eigene Sleep-Queue-Sentinel-Adresse (Abschnitt "F$Sleep", 2026-08-30)
 * -- MUSS mit Q9K_SLEEPQ_SENTINEL_ADDR in q9kernel_sched.c
 * uebereinstimmen, gleiche Begruendung/Kollisionsvermeidung wie oben.
 * Q9_D_SLEEPQ selbst bleibt weiter unten als leere Ringliste
 * initialisiert (Kompat-Vollstaendigkeit), wird aber von unserem eigenen
 * F$Sleep NICHT mehr gelesen/geschrieben. */
#define Q9K_SLEEPQ_SENTINEL_ADDR 0x12F0UL

/* Freispeicher-Basis fuer die Arena (Abschnitt 2, Punkt 3).
 *
 * ECHTER BUG GEFUNDEN + GEFIXT (2026-09-01, Stack-Corruption-Suche nach
 * Q9K_PROCDESC_SIZE 128->512): der urspruengliche Wert $2000 war NIE
 * gegen den tatsaechlichen Kernel-Stack geprueft worden (s. alter
 * Kommentar unten, "VORLAEUFIG ... noch NICHT verifiziert" -- die
 * Annahme stimmte nicht). Der eigene Boot-/Supervisor-Stack
 * (Q9K_StackTop, q9kernel_entry.a) liegt bei
 * Q9K_GlobBase($0)+Q9K_GlobSize($8000)+Q9K_StackSize($4000) = $C000 --
 * der Stack belegt also $8000..$C000 (waechst abwaerts). Die Arena
 * wuchs von $2000 aufwaerts OHNE Ruecksicht auf diesen Bereich: bei der
 * fruehen, kleineren Q9K_PROCDESC_SIZE(128) blieb die Arena zufaellig
 * unter $8000 (nie kollidiert), aber mit der jetzt noetigen Groesse
 * (512, s. q9kernel_tables.c) wuchs sie bis $BC00 -- MITTEN in den
 * Stack hinein. Die ProcPool-Nullungsschleife ueberschrieb dabei den
 * eigenen Aufruf-Stack waehrend sie noch lief; der naechste
 * verschachtelte Funktionsaufruf (Q9K_BuildFreeList) las danach eine
 * bereits zerstoerte Ruecksprungadresse -> Sprung in zufaelligen
 * Speicher ("random code execution", per Q9_BOARD_DEBUG-PC-Trace
 * verifiziert). Per Bisektion mit Kanarien-Werten exakt auf diesen
 * Speicherbereichs-Ueberlapp zurueckgefuehrt (Details: Memory-Notiz
 * q9-os-eigener-kernel-c).
 *
 * Fix: Arena-Basis auf $10000 (64K) verschoben -- deutlich oberhalb von
 * Q9K_StackTop ($C000), mit Sicherheitsmarge fuer kuenftiges
 * Stack-Wachstum (Q9K_StackSize koennte spaeter erhoeht werden, ohne
 * dass die Arena-Basis wieder verschoben werden muss).
 *
 * Nachtrag 2026-09-15: Der Bootmodulbereich ist nicht auf eine feste
 * Groesse begrenzt. Grosse residente Module wie csl koennen bis in den
 * bisherigen Arena-Bereich reichen. Die tatsaechliche Arena-Basis wird
 * deshalb unten mindestens hinter das Ende der Bootlist-RAM-Region
 * verschoben; dieser Wert bleibt nur die Untergrenze fuer kleine Boots. */
/* NACHTRAG 2026-09-01 (BUGFIX "IOMan-Modul wird zur Laufzeit zerschossen",
 * s. ausfuehrlichen Kommentar bei Q9K_StackBase in q9kernel_entry.a): Der
 * Boot-Stack musste ueber die Bootkette ($7100..~$C400) hinaus verschoben
 * werden und belegt jetzt $10000..$18000 -- also genau die bisherige
 * Arena-Basis. Arena entsprechend mitgezogen, damit die Reihenfolge
 * Globals / Bootkette / Stack / Arena ueberlappungsfrei bleibt. */
#define Q9K_FREEMEM_BASE    0x18000UL

/* NACHTRAG 2026-09-18: Der Boot-/Supervisor-Stack liegt nicht mehr bei
 * $10000..$18000 (dort stand laengst die Bootkette, s. q9kernel_entry.a,
 * NACHTRAG 2026-09-18 bei Q9K_StackSize), sondern am RAM-ENDE. Die Arena
 * laesst ihm deshalb die letzten Q9K_BOOTSTACK_SIZE Byte frei. Der Wert
 * MUSS zu Q9K_StackSize in q9kernel_entry.a passen -- lokal dupliziert,
 * gleiche schlanke Konvention wie bei allen ASM<->C-Konstanten hier. */
#define Q9K_BOOTSTACK_SIZE  0x8000UL

/* Schreibt einen 32-Bit-Wert an eine absolute Adresse (=Kernel-Global-
 * Offset, da Kernel-Globals-Basis bei diesem Kernel $000000 ist) */
static void Q9K_PutU32(Q9_u32 addr, Q9_u32 value)
{
    *(volatile Q9_u32 *)addr = value;
}

/* Gegenstueck zu Q9K_PutU32 -- fuer die Q9K_SysUnimplemented-
 * Fallback-Schleife gebraucht (pruefen, ob ein Slot noch 0/unregistriert
 * ist), s. dort. */
static Q9_u32 Q9K_GetU32(Q9_u32 addr)
{
    return *(volatile Q9_u32 *)addr;
}

/* Wie Q9K_PutU32, aber fuer 1-/2-Byte-Felder -- WICHTIG, nicht einfach
 * Q9K_PutU32 fuer schmalere Felder wiederverwenden, das wuerde
 * benachbarte Kernel-Global-Bytes ueberschreiben (Q9_D_COMPAT/
 * Q9_D_COMPAT2 sind je 1 Byte breit, s. q9sysglob.h). */
static void Q9K_PutU8(Q9_u32 addr, Q9_u8 value)
{
    *(volatile Q9_u8 *)addr = value;
}

static void Q9K_PutU16(Q9_u32 addr, unsigned short value)
{
    *(volatile unsigned short *)addr = value;
}

/* Initialisiert eine leere, zirkulaere Warteschlange: Kopf- und Schwanz-
 * Zeiger (an queueBase+headOff bzw. queueBase+tailOff) zeigen beide auf
 * die Warteschlange selbst (queueBase) -- das beim echten Kernel
 * verifizierte Terminierungsmuster fuer "leer" (s. Kopfkommentar). */
static void Q9K_InitEmptyQueue(Q9_u32 queueBase, Q9_u32 headOff, Q9_u32 tailOff)
{
    Q9K_PutU32(queueBase + headOff, queueBase);
    Q9K_PutU32(queueBase + tailOff, queueBase);
}

void Q9K_CInit(void)
{
    Q9K_MemTraceInit();
    Q9K_ProcMemTrackInit();
    Q9K_Diag4(); /* TEMPORAERE DIAGNOSE, s. o. */

    /* Sechs leere Ringlisten -- exakte Offsets aus q9sysglob.h bzw. dem
     * verifizierten Fund in Thema 01 (Kopf-/Schwanz-Unteroffsets je
     * Warteschlangenart unterschiedlich, s. dortige Tabelle):
     *   Q9_D_ACTIVQ/Q9_D_SLEEPQ/Q9_D_WAITQ -- Prozess-Warteschlangen, +0x30/+0x34
     *   Q9_D_ARENA                        -- Speicher-Kontrollblock,  +0x8/+0xC
     *   Q9_D_ALMQ1/Q9_D_ALMQ2              -- F$Alarm-Warteschlangen,  +0xC/+0x10
     */
    Q9K_InitEmptyQueue(Q9_D_ACTIVQ, 0x30, 0x34);   /* NACHTRAG: nicht mehr von unserem Scheduler benutzt, s. Q9K_READYQ_SENTINEL_ADDR-Kommentar oben */
    Q9K_InitEmptyQueue(Q9K_READYQ_SENTINEL_ADDR, 0x30, 0x34); /* die WIRKLICH vom Scheduler benutzte eigene Ready-Queue */
    Q9K_InitEmptyQueue(Q9_D_SLEEPQ, 0x30, 0x34);   /* NACHTRAG: nicht mehr von F$Sleep benutzt, s. Q9K_SLEEPQ_SENTINEL_ADDR-Kommentar oben */
    Q9K_InitEmptyQueue(Q9_D_WAITQ,  0x30, 0x34);   /* NACHTRAG: nicht mehr von F$Wait benutzt, s. Q9K_WAITQ_SENTINEL_ADDR-Kommentar oben */
    Q9K_InitEmptyQueue(Q9K_WAITQ_SENTINEL_ADDR, 0x30, 0x34);  /* die WIRKLICH von F$Wait benutzte eigene Wait-Queue */
    Q9K_InitEmptyQueue(Q9K_SLEEPQ_SENTINEL_ADDR, 0x30, 0x34); /* die WIRKLICH von F$Sleep benutzte eigene Sleep-Queue */
    Q9K_InitEmptyQueue(Q9_D_ARENA,  0x08, 0x0C);
    Q9K_InitEmptyQueue(Q9_D_ALMQ1,  0x0C, 0x10);
    Q9K_InitEmptyQueue(Q9_D_ALMQ2,  0x0C, 0x10);

    /* Abschnitt 2, Punkt 3: Speichergroesse aus Q9_D_TOTRAM lesen (vom
     * Assembler-Einstieg schon aus D0 gesichert, s. q9kernel_entry.a),
     * echten freien Speicherblock im Arena-Kontrollblock registrieren --
     * die Ringliste oben war nur der leere Ausgangszustand.
     * Q9K_FREEMEM_BASE ist VORLAEUFIG (s. dortiger Kommentar) -- falls
     * Q9_D_TOTRAM kleiner als die Basis ist (unplausibel kleines RAM
     * oder falsch gelesener Wert), bewusst KEINE Initialisierung statt
     * mit einer negativ/riesig unterlaufenden Groesse zu rechnen. */
    {
        Q9_u32 totalRam = *(volatile Q9_u32 *)Q9_D_TOTRAM;
        Q9_u32 freeBase = Q9K_FREEMEM_BASE;
        Q9_u32 i;

        /* Boot modules occupy the RAM regions reported by the ROM.  Start
         * the private arena after the highest such region, not at a fixed
         * address that may cut through a large resident module. */
        for (i = 0; i < 64UL; i++) {
            Q9_u32 base = Q9K_GetU32(Q9K_BOOTLIST_ADDR + i * 8UL);
            Q9_u32 size = Q9K_GetU32(Q9K_BOOTLIST_ADDR + i * 8UL + 4UL);
            Q9_u32 end;

            if (base == 0)
                break;
            if (totalRam != 0 && base >= totalRam)
                continue;
            end = base + size;
            if (end > freeBase)
                freeBase = end;
        }

        /* Keep allocator blocks 16-byte aligned. */
        freeBase = (freeBase + 15UL) & ~15UL;
        /* Die letzten Q9K_BOOTSTACK_SIZE Byte gehoeren dem Boot-Stack
         * (s. Definition oben) -- die Arena endet davor. */
        if (totalRam > freeBase + Q9K_BOOTSTACK_SIZE) {
            Q9K_ArenaInit(freeBase, totalRam - Q9K_BOOTSTACK_SIZE - freeBase);
        }
    }

    /* Abschnitt 2, Punkt 4: Exception-/Trap-Dispatch-Tabelle aus
     * kompakter Quelltabelle expandieren (q9kernel_exctable.c) --
     * Q9_D_EXCJMP zeigt auf den vom Boot-ROM bereitgestellten
     * Speicherblock dafuer (s. q9kernel_entry.a). Alle Vektoren zeigen
     * bisher auf denselben generischen Halt-Handler (kein IRQ-/Syscall-
     * Dispatcher existiert noch, s. dortige Kopfkommentare/TODOs).
     * Rueckgabewert (Konsistenzcheck der Quelltabelle) noch nicht
     * ausgewertet -- kein Panic-Mechanismus vorhanden, dem ein
     * Fehlschlag hier ohnehin mitgeteilt werden koennte (TODO, sobald
     * es einen gibt). */
    Q9K_BuildExcTable();

    /* Schritt 6a: Init-Modul per Namenssuche finden (2026-08-18,
     * verdrahtet nach einer Session-Pause -- Q9K_FindModuleByName/
     * Q9K_GetCpuCount existierten schon, waren aber noch nicht
     * aufgerufen). */
    {
        Q9_u32 initAvailableLen = 0;
        const Q9_u8 *initMod = Q9K_FindModuleByName((const Q9_u8 *)Q9K_BOOTLIST_ADDR, "init", &initAvailableLen);

        Q9K_Diag5(); /* TEMPORAERE DIAGNOSE, s. o. -- Suche abgeschlossen, unabhaengig vom Ergebnis */

        if (initMod != 0) {
            Q9K_PutU32(Q9_D_INIT, (Q9_u32)(unsigned long)initMod);

            /* Einfache Init-Modul-Konfigurationsfelder direkt uebernehmen
             * -- exakt wie beim echten Kernel verifiziert (Thema 01):
             * M$Compat->D_Compat, M$Compat2->D_Compat2, M$SysConf->
             * D_SysConf. Offsets sind Init-Modul-interne Felder (Manual
             * Table 2-4), NICHT Q9_MH68K_*-Header-Offsets -- liegen
             * ausserhalb des von Q9K_ValidModuleHeader geprueften
             * 0x30-Byte-Bereichs, deshalb Bounds-Check gegen
             * initAvailableLen zwingend vor jedem Zugriff. */
            if (initAvailableLen >= 0x7C) {
                Q9K_PutU8(Q9_D_COMPAT,  initMod[0x68]);
                Q9K_PutU8(Q9_D_COMPAT2, initMod[0x69]);
                Q9K_PutU16(Q9_D_SYSCONF, (unsigned short)((initMod[0x7A] << 8) | initMod[0x7B]));

                /* SMP-CPU-Anzahl: optional, Default 1 falls das
                 * Init-Modul keine Q9-Erweiterung hat (s.
                 * q9kernel_initext.c). Noch von niemandem konsumiert --
                 * kein SMP-Scheduler existiert bisher, aber der
                 * permanente Platz dafuer ist jetzt belegt. */
                Q9K_PutU32(Q9K_CPUCOUNT_ADDR, Q9K_GetCpuCount(initMod, initAvailableLen));

                /* Abschnitt 2, Punkt 5/6: M$Procs/M$Paths/M$MDirSz lesen
                 * (alle < 0x7C, durch denselben Bounds-Check oben
                 * abgedeckt), SYSDIS/USRDIS/Modulverzeichnis + Prozess-/
                 * Pfad-Pools ueber den Arena-Allokator aufsetzen
                 * (q9kernel_tables.c). Rueckgabewert (Allokationsfehler)
                 * noch nicht ausgewertet -- gleiche Begruendung wie bei
                 * Q9K_BuildExcTable (kein Panic-Mechanismus vorhanden). */
                Q9K_SetupTables(initMod);

                /* NACHTRAG 2026-08-21 (Abschnitt "F$Link/F$UnLink"):
                 * Modulverzeichnis JETZT befuellen (braucht den von
                 * Q9K_SetupTables gerade erst als Freiliste vorbereiteten
                 * Slot-Pool) -- durchsucht Q9K_BootList kein zweites Mal
                 * fuer "init" gezielt, sondern traegt JEDES gueltige
                 * Modul ein (also auch "init" selbst und den Kernel,
                 * sofern er als eigenes Modul im Boot-Bereich erkennbar
                 * ist). Rueckgabewert (Anzahl) noch nicht ausgewertet --
                 * kein Panic-Mechanismus vorhanden, gleiche Begruendung
                 * wie an anderen Stellen. Danach F$Link/F$UnLink in die
                 * echte, per Manual dokumentierte Callcode-Position der
                 * User-Service-Dispatch-Tabelle eintragen (0x00/0x02,
                 * s. modules/SYSCALL_MODULE_MAP.md) -- Q9_D_USRDIS ist
                 * ein ZEIGER-Feld (von Q9K_SetupTables gerade gesetzt),
                 * kein direkter Adressbereich. */
                Q9K_ModDirPopulateFromBootList((const Q9_u8 *)Q9K_BOOTLIST_ADDR);
                {
                    Q9_u32 usrdisBase = *(volatile Q9_u32 *)Q9_D_USRDIS;
                    /* NACHTRAG 2026-08-31 (Abschnitt "IOMan-Einbindung, Punkt 1"):
                     * Q9_D_SYSDIS ($3a4) MUSS ebenfalls befuellt werden -- real
                     * per eigener Analyse gefunden (intern dokumentiert,
                     * Fund "Q9_disp_488"): welche der
                     * beiden Tabellen ein TRAP #0 benutzt, haengt vom
                     * Supervisor-Bit im geretteten SR ab (schon dokumentiert,
                     * "wir haben noch keine echte User-/Supervisor-Trennung" --
                     * betraf bisher nur TRAP #0). NEU gefunden (per echtem
                     * Boot-Test, IOMan-Einbindung): bereits-residente,
                     * bereits-supervisor-state Systm-Module wie IOMan rufen
                     * F$-Primitive INTERN NIE ueber TRAP #0, sondern ueber
                     * einen eigenen A6-relativen PEA+RTS-Tabellensprung DIREKT
                     * auf Q9_D_SYSDIS (intern dokumentiert, Fund "IOMan ruft
                     * Kernel-Primitive ueber dieselbe Trampolin-Tabelle wie
                     * der Kernel selbst auf"). Ohne diese Befuellung blieb Q9_D_SYSDIS auf dem
                     * genullten Anfangszustand (s. q9kernel_tables.c) --
                     * IOMans eigener Trampolin sprang dadurch real auf Adresse
                     * 0 (per Boot-Test als Fehlschlag/Q9K_StkHandler-Ausloesung
                     * beobachtet, urspruenglich faelschlich fuer einen
                     * Stack-Ueberlauf gehalten -- Verdopplung von
                     * Q9K_PROC_STACK_SIZE aenderte NICHTS am Fehlerbild, ein
                     * starkes Indiz gegen einen echten Stack-Ueberlauf).
                     * Gleiche Handler-Adressen wie in Q9_D_USRDIS -- unsere
                     * Handler unterscheiden ohnehin nicht zwischen den beiden
                     * Aufrufkontexten (kein Nested-Call-Zaehler-Bezug in den
                     * Handlern selbst). */
                    Q9_u32 sysdisBase = *(volatile Q9_u32 *)Q9_D_SYSDIS;

                    Q9K_PutU32(usrdisBase + 0x00UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFLink);
                    Q9K_PutU32(usrdisBase + 0x02UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFUnLink);
                    Q9K_PutU32(usrdisBase + 0x03UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFFork);
                    Q9K_PutU32(usrdisBase + 0x04UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFWait);
                    Q9K_PutU32(usrdisBase + 0x06UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFExit);
                    Q9K_PutU32(usrdisBase + 0x0aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSleep);
                    Q9K_PutU32(usrdisBase + 0x0cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFID);
                    Q9K_PutU32(usrdisBase + 0x0dUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSPrior);
                    Q9K_PutU32(usrdisBase + 0x28UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSRqMem);
                    Q9K_PutU32(usrdisBase + 0x29UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSRtMem);
                    Q9K_PutU32(usrdisBase + 0x32UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSSvc);
                    Q9K_PutU32(usrdisBase + 0x37UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGProcP);
                    Q9K_PutU32(usrdisBase + 0x84UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIOpen);
                    Q9K_PutU32(usrdisBase + 0x82UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIDup);
                    Q9K_PutU32(usrdisBase + 0x8cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIWritLn);
                    Q9K_PutU32(usrdisBase + 0x8aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIWrite);
                    Q9K_PutU32(usrdisBase + 0x89UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIRead);
                    Q9K_PutU32(usrdisBase + 0x8bUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIReadLn);
                    Q9K_PutU32(usrdisBase + 0x8dUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIGetStt);
                    Q9K_PutU32(usrdisBase + 0x92UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFISGetSt);
                    Q9K_PutU32(usrdisBase + 0x8eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFISetStt);
                    Q9K_PutU32(usrdisBase + 0x88UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFISeek);
                    Q9K_PutU32(usrdisBase + 0x8fUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIClose);
                    Q9K_PutU32(usrdisBase + 0x86UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIChgDir);
                    Q9K_PutU32(usrdisBase + 0x30UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAllPD);
                    Q9K_PutU32(usrdisBase + 0x31UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFRetPD);
                    Q9K_PutU32(usrdisBase + 0x38UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFMove);
                    Q9K_PutU32(usrdisBase + 0x21UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFTLink);
                    Q9K_PutU32(usrdisBase + 0x5aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFCCtl);
                    Q9K_PutU32(usrdisBase + 0x27UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSetSys);
                    Q9K_PutU32(usrdisBase + 0x2eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFVModul);
                    Q9K_PutU32(usrdisBase + 0x5cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSRqCMem);
                    Q9K_PutU32(usrdisBase + 0x2aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIRQ);
                    Q9K_PutU32(usrdisBase + 0x10UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFPrsNam);
                    Q9K_PutU32(usrdisBase + 0x11UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFCmpNam);
                    Q9K_PutU32(usrdisBase + 0x1bUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFCpyMem);
                    Q9K_PutU32(usrdisBase + 0x1cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSUser);
                    Q9K_PutU32(usrdisBase + 0x17UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFCRC);
                    Q9K_PutU32(usrdisBase + 0x1dUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFUnLoad);
                    Q9K_PutU32(usrdisBase + 0x18UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGPrDsc);
                    Q9K_PutU32(usrdisBase + 0x1aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGModDr);
                    Q9K_PutU32(usrdisBase + 0x26UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSetCRC);
                    Q9K_PutU32(usrdisBase + 0x20UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFJulian);
                    Q9K_PutU32(usrdisBase + 0x54UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGregor);
                    Q9K_PutU32(usrdisBase + 0x1fUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGPrDBT);
                    Q9K_PutU32(usrdisBase + 0x2cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAProc);
                    Q9K_PutU32(usrdisBase + 0x4bUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAllPrc);
                    Q9K_PutU32(usrdisBase + 0x4cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFDelPrc);
                    Q9K_PutU32(usrdisBase + 0x55UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSysID);
                    Q9K_PutU32(usrdisBase + 0x2fUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFFindPD);
                    Q9K_PutU32(usrdisBase + 0x57UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSigMask);
                    Q9K_PutU32(usrdisBase + 0x09UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIcpt);
                    Q9K_PutU32(usrdisBase + 0x60UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFTrans);
                    Q9K_PutU32(usrdisBase + 0x25UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFDatMod);
                    Q9K_PutU32(usrdisBase + 0x56UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAlarm);
                    Q9K_PutU32(usrdisBase + 0x15UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFTime);
                    Q9K_PutU32(usrdisBase + 0x16UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSTime);
                    Q9K_PutU32(usrdisBase + 0x12UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSchBit);
                    Q9K_PutU32(usrdisBase + 0x13UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAllBit);
                    Q9K_PutU32(usrdisBase + 0x14UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFDelBit);
                    Q9K_PutU32(usrdisBase + 0x19UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGBlkMp);
                    Q9K_PutU32(usrdisBase + 0x62UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSema);
                    Q9K_PutU32(usrdisBase + 0x05UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFChain);
                    Q9K_PutU32(usrdisBase + 0x1eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFRTE);
                    Q9K_PutU32(usrdisBase + 0x63UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSigReset);
                    Q9K_PutU32(usrdisBase + 0x0eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSTrap);
                    Q9K_PutU32(usrdisBase + 0x53UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFEvent);
                    /* Einsprung von Q9K_SysEventImpl fuer Q9K_SysFEvent
                     * hinterlegen: der Assembler-Handler kann die
                     * C-Funktion weder per bsr (zu weit) noch per
                     * absolutem jsr (kam nicht zurueck) erreichen -- s.
                     * Q9K_EventImplPtr in q9kernel_entry.a. */
                    Q9K_PutU32(0x1E40UL, (Q9_u32)(unsigned long)Q9K_SysEventImpl);
                    Q9K_PutU32(0x1E4CUL, (Q9_u32)(unsigned long)Q9K_SysEventWaitImpl);
                    Q9K_PutU32(0x1E54UL, (Q9_u32)(unsigned long)Q9K_SysNProcImpl);
                    Q9K_PutU32(0x1E58UL, (Q9_u32)(unsigned long)Q9K_SysSTrapImpl);
                    Q9K_PutU32(0x1E5CUL, (Q9_u32)(unsigned long)Q9K_SysRTEImpl);
                    Q9K_PutU32(0x1E60UL, (Q9_u32)(unsigned long)Q9K_SysSigResetImpl);
                    Q9K_PutU32(0x1E64UL, (Q9_u32)(unsigned long)Q9K_SysChainImpl);
                    Q9K_PutU32(0x1E68UL, (Q9_u32)(unsigned long)Q9K_SysChainReleaseImpl);
                    Q9K_PutU32(0x1E6CUL, (Q9_u32)(unsigned long)Q9K_SysSemaImpl);
                    Q9K_PutU32(0x1E70UL, (Q9_u32)(unsigned long)Q9K_SysSemaWaitImpl);
                    Q9K_PutU32(usrdisBase + 0x5eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFPanic);
                    Q9K_PutU32(usrdisBase + 0x2dUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFNProc);
                    Q9K_PutU32(usrdisBase + 0x58UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFChkMem);
                    Q9K_PutU32(usrdisBase + 0x4eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFFModul);
                    Q9K_PutU32(usrdisBase + 0x3aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFPermit);
                    Q9K_PutU32(usrdisBase + 0x3bUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFProtect);
                    Q9K_PutU32(usrdisBase + 0x40UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFDelTsk);
                    Q9K_PutU32(usrdisBase + 0x5bUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGSPUMp);
                    Q9K_PutU32(usrdisBase + 0x08UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSend);

                    Q9K_PutU32(sysdisBase + 0x00UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFLink);
                    Q9K_PutU32(sysdisBase + 0x02UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFUnLink);
                    Q9K_PutU32(sysdisBase + 0x03UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFFork);
                    Q9K_PutU32(sysdisBase + 0x04UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFWait);
                    Q9K_PutU32(sysdisBase + 0x06UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFExit);
                    Q9K_PutU32(sysdisBase + 0x0aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSleep);
                    Q9K_PutU32(sysdisBase + 0x0cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFID);
                    Q9K_PutU32(sysdisBase + 0x0dUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSPrior);
                    Q9K_PutU32(sysdisBase + 0x28UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSRqMem);
                    Q9K_PutU32(sysdisBase + 0x29UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSRtMem);
                    Q9K_PutU32(sysdisBase + 0x32UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSSvc);
                    Q9K_PutU32(sysdisBase + 0x37UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGProcP);
                    Q9K_PutU32(sysdisBase + 0x84UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIOpen);
                    Q9K_PutU32(sysdisBase + 0x82UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIDup);
                    Q9K_PutU32(sysdisBase + 0x8cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIWritLn);
                    Q9K_PutU32(sysdisBase + 0x8aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIWrite);
                    Q9K_PutU32(sysdisBase + 0x89UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIRead);
                    Q9K_PutU32(sysdisBase + 0x8bUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIReadLn);
                    Q9K_PutU32(sysdisBase + 0x8dUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIGetStt);
                    Q9K_PutU32(sysdisBase + 0x92UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFISGetSt);
                    Q9K_PutU32(sysdisBase + 0x8eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFISetStt);
                    Q9K_PutU32(sysdisBase + 0x88UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFISeek);
                    Q9K_PutU32(sysdisBase + 0x8fUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIClose);
                    Q9K_PutU32(sysdisBase + 0x86UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIChgDir);
                    Q9K_PutU32(sysdisBase + 0x30UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAllPD);
                    Q9K_PutU32(sysdisBase + 0x31UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFRetPD);
                    Q9K_PutU32(sysdisBase + 0x38UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFMove);
                    Q9K_PutU32(sysdisBase + 0x21UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFTLink);
                    Q9K_PutU32(sysdisBase + 0x5aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFCCtl);
                    Q9K_PutU32(sysdisBase + 0x27UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSetSys);
                    Q9K_PutU32(sysdisBase + 0x2eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFVModul);
                    Q9K_PutU32(sysdisBase + 0x5cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSRqCMem);
                    Q9K_PutU32(sysdisBase + 0x2aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIRQ);
                    Q9K_PutU32(sysdisBase + 0x10UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFPrsNam);
                    Q9K_PutU32(sysdisBase + 0x11UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFCmpNam);
                    Q9K_PutU32(sysdisBase + 0x1bUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFCpyMem);
                    Q9K_PutU32(sysdisBase + 0x1cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSUser);
                    Q9K_PutU32(sysdisBase + 0x17UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFCRC);
                    Q9K_PutU32(sysdisBase + 0x1dUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFUnLoad);
                    Q9K_PutU32(sysdisBase + 0x18UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGPrDsc);
                    Q9K_PutU32(sysdisBase + 0x1aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGModDr);
                    Q9K_PutU32(sysdisBase + 0x26UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSetCRC);
                    Q9K_PutU32(sysdisBase + 0x20UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFJulian);
                    Q9K_PutU32(sysdisBase + 0x54UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGregor);
                    Q9K_PutU32(sysdisBase + 0x1fUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGPrDBT);
                    Q9K_PutU32(sysdisBase + 0x2cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAProc);
                    Q9K_PutU32(sysdisBase + 0x4bUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAllPrc);
                    Q9K_PutU32(sysdisBase + 0x4cUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFDelPrc);
                    Q9K_PutU32(sysdisBase + 0x55UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSysID);
                    Q9K_PutU32(sysdisBase + 0x2fUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFFindPD);
                    Q9K_PutU32(sysdisBase + 0x57UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSigMask);
                    Q9K_PutU32(sysdisBase + 0x09UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFIcpt);
                    Q9K_PutU32(sysdisBase + 0x60UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFTrans);
                    Q9K_PutU32(sysdisBase + 0x25UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFDatMod);
                    Q9K_PutU32(sysdisBase + 0x56UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAlarm);
                    Q9K_PutU32(sysdisBase + 0x15UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFTime);
                    Q9K_PutU32(sysdisBase + 0x16UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSTime);
                    Q9K_PutU32(sysdisBase + 0x12UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSchBit);
                    Q9K_PutU32(sysdisBase + 0x13UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAllBit);
                    Q9K_PutU32(sysdisBase + 0x14UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFDelBit);
                    Q9K_PutU32(sysdisBase + 0x19UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGBlkMp);
                    Q9K_PutU32(sysdisBase + 0x62UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSema);
                    Q9K_PutU32(sysdisBase + 0x05UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFChain);
                    Q9K_PutU32(sysdisBase + 0x1eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFRTE);
                    Q9K_PutU32(sysdisBase + 0x63UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSigReset);
                    Q9K_PutU32(sysdisBase + 0x0eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSTrap);
                    Q9K_PutU32(sysdisBase + 0x53UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFEvent);
                    Q9K_PutU32(sysdisBase + 0x5eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFPanic);
                    Q9K_PutU32(sysdisBase + 0x2dUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFNProc);
                    Q9K_PutU32(sysdisBase + 0x58UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFChkMem);
                    Q9K_PutU32(sysdisBase + 0x4eUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFFModul);
                    Q9K_PutU32(sysdisBase + 0x3aUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFPermit);
                    Q9K_PutU32(sysdisBase + 0x3bUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFProtect);
                    Q9K_PutU32(sysdisBase + 0x3fUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFAllTsk);
                    Q9K_PutU32(sysdisBase + 0x40UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFDelTsk);
                    Q9K_PutU32(sysdisBase + 0x5bUL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFGSPUMp);
                    Q9K_PutU32(sysdisBase + 0x08UL * 4UL, (Q9_u32)(unsigned long)Q9K_SysFSend);

                    /* ECHTER BUG GEFUNDEN + GEFIXT (2026-08-31, per
                     * Root-Cause-Suche eines echten IOMan-Stack-Crashs):
                     * alle NOCH NICHT registrierten Slots (Wert 0) mit
                     * Q9K_SysUnimplemented befuellen, statt sie auf 0 zu
                     * lassen -- der reale, ECHTE Microware-Kernel macht
                     * das ebenfalls (gemeinsamer Fehler-Stub, intern
                     * dokumentiert). Grund: der PEA+RTS-
                     * Trampolin-Mechanismus (den externe Module wie
                     * IOMan fuer performancekritische Primitive DIREKT
                     * nutzen, intern dokumentiert, Fund "Runde 2") liest
                     * den Primaerarray-Wert und macht ein
                     * "rts" DAHIN -- bei 0 waere das ein Sprung zu
                     * physischer Adresse 0, mitten in die eigenen Kernel-
                     * Global-DATEN. */
                    {
                        Q9_u32 slot;
                        for (slot = 0; slot < 256UL; slot++) {
                            if (Q9K_GetU32(usrdisBase + slot * 4UL) == 0)
                                Q9K_PutU32(usrdisBase + slot * 4UL, (Q9_u32)(unsigned long)Q9K_SysUnimplemented);
                            if (Q9K_GetU32(sysdisBase + slot * 4UL) == 0)
                                Q9K_PutU32(sysdisBase + slot * 4UL, (Q9_u32)(unsigned long)Q9K_SysUnimplemented);
                        }
                    }
                }
            }
            /* TODO: initAvailableLen < 0x7C waere ein sehr kleines/
             * unplausibles Init-Modul -- bewusst KEIN Zugriff auf die
             * Konfigurationsfelder in diesem Fall, aber auch keine
             * explizite Fehlerbehandlung dafuer (noch offen). */
        }
        /* TODO: kein Init-Modul gefunden -- der echte Kernel gibt eine
         * feste Meldung aus ("kernel: can't find Init module", Thema 01)
         * und bricht vermutlich ab. Fuer uns noch nicht entschieden --
         * aktuell faellt die Funktion einfach durch bis zum return
         * unten, Q9K_HaltLoop faengt das ab (kein Fortschritt, aber
         * auch kein Absturz). */
    }

    /* Abschnitt "Scheduler" (2026-08-21, im Anschluss an F$Link/
     * F$UnLink): ersetzt das fruehere "minimale Geruest" (Abschnitt 2,
     * Punkt 7, EIN Kontext, roher Sprung) durch einen ECHTEN,
     * Timer-getriebenen preemptiven Scheduler. Zwei Test-"Prozesse"
     * (Q9K_TestProcA/B, q9kernel_entry.a) werden angelegt -- absichtlich
     * mit UNTERSCHIEDLICHER Prioritaet (5 bzw. 3), um sowohl den
     * Normalfall (Zeitscheiben-Ablauf) als auch das Altern/Nicht-
     * Verhungern des niedrigpriorigeren Prozesses real zu zeigen, s.
     * Kopfkommentar q9kernel_sched.c. Reihenfolge zwingend:
     *   1. beide Prozesse anlegen (Q9K_ProcCreate haengt sie bereits in
     *      die Ready-Queue ein, s. dortigen Kommentar)
     *   2. Board-Timer aktivieren (Q9K_TimerActivate) -- ERST NACHDEM
     *      Q9_D_PROC unten gesetzt ist waere ein zufaelliger Tick
     *      zwischen Aktivierung und Q9K_SchedRun unproblematisch (VBR
     *      steht bereits laenger, der Handler ist real eingetragen),
     *      aus Vorsicht trotzdem so spaet wie sinnvoll platziert
     *   3. Q9K_SchedFirstPick waehlt den ersten Prozess aus und setzt
     *      Q9_D_PROC direkt (KEIN "aktueller" Prozess existiert vorher,
     *      anders als bei jedem spaeteren Q9K_SchedReschedule-Aufruf)
     *   4. Q9K_SchedRun schaltet auf dessen Fake-Rahmen um und "kehrt"
     *      per RTE erstmals hinein zurueck -- kein Ruecksprung erwartet.
     * Bei jedem Fehlschlag (Pool/Arena erschoepft, oder KEIN Prozess
     * ueberhaupt anlegbar) fallen wir bewusst durch bis zum return
     * unten -- Q9K_HaltLoop faengt das ab, kein Fake-Fortschritt. */
    {
        Q9_u32 picked;

#if Q9K_BOOT_STARTUP
        /* Q9K_TestProcA performs the resident IOMan initialization before
         * entering the sysgo-style shell path.  Starting Q9K_StartupProc
         * directly would use I$ChgDir/I$Open before D_DevTbl and the
         * process paths exist. */
        Q9K_ProcCreate((Q9_u32)(unsigned long)Q9K_TestProcA, 5);
#else
        Q9K_ProcCreate((Q9_u32)(unsigned long)Q9K_TestProcA, 5);
        Q9K_ProcCreate((Q9_u32)(unsigned long)Q9K_TestProcB, 3);
#endif

        picked = Q9K_SchedFirstPick();

        if (picked != 0) {
            Q9K_Diag6(); /* TEMPORAERE DIAGNOSE, s. o. */
            Q9K_TimerActivate();
            Q9K_SchedRun(); /* kein Ruecksprung erwartet */
        }
    }

    return; /* -> Q9K_HaltLoop in q9kernel_entry.a (nur bei Fehlschlag oben) */
}
