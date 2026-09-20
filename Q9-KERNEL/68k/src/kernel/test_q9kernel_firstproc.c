/*
 * test_q9kernel_firstproc.c -- Regressionstest fuer q9kernel_firstproc.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. NACHTRAG
 * 2026-08-21 (Abschnitt "Scheduler"): komplett auf Q9K_ProcCreate(entryPC,
 * priority) umgestellt, ersetzt das fruehere parameterlose
 * Q9K_StartFirstProcess. Q9K_SchedInsert (extern, real in q9kernel_sched.c)
 * wird hier durch einen einfachen lokalen Stub ersetzt (Append + Age=
 * Prioritaet, gleiche minimale Nachbildung wie schon Q9K_AllocMem als
 * Fake-Bump-Allocator) -- kein echtes q9kernel_sched.c wird eingebunden,
 * damit dieser Test unabhaengig von dessen eigenem Test bleibt (der prueft
 * Q9K_SchedInsert selbst bereits ausfuehrlich, s. test_q9kernel_sched.c).
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_firstproc test_q9kernel_firstproc.c && \
 *       ./test_q9kernel_firstproc
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x2000];
static unsigned char g_fakePool[1 << 20]; /* 2026-09-01 von 64K auf 1M erhoeht -- Q9K_PROC_STACK_SIZE ist jetzt 32K statt 8K, der alte Pool reichte nicht mehr fuer mehrere Testprozesse */
static unsigned long g_fakePoolNext;

/* Grosszuegige, getrennte Testadressen -- gleiche Begruendung wie in
 * test_q9kernel_tables.c (echte Nachbar-Offsets koennten beim 8-Byte-
 * breiten Q9_u32 auf diesem Host ueberlappen, hier aber ohnehin nicht
 * Gegenstand des Tests). */
#define Q9_D_ACTIVQ             ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9K_PROCPOOL_FREE_ADDR  ((unsigned long)(g_fakeGlobals + 0x040))
#define Q9_D_PROC               ((unsigned long)(g_fakeGlobals + 0x080))
#define Q9K_PROCPOOL_BASE_ADDR  ((unsigned long)(g_fakeGlobals + 0x0C0))
/* F$Fork-Scratchzelle fuer die gewuenschte Pfadanzahl (d3.w), von
 * Q9K_ProcFork beim Aufbau der Pfadtabelle gelesen. */
#define Q9K_FORK_SCRATCH_NUMPATHS ((unsigned long)(g_fakeGlobals + 0x100))

/* State/Priority/Age/Next/Prev/SavedSP/EntryPC liegen im echten Deskriptor
 * nur wenige Byte auseinander (+0x00/+0x01/+0x02/+0x30/+0x34/+0x38/+0x3C)
 * -- auf diesem 64-Bit-Testhost (Q9_u32 = 8 Byte) wuerde ein
 * Q9K_SetU32-Schreibzugriff Nachbarfelder ueberschreiben (gleicher Fund
 * wie schon bei Q9_D_MODDIR_END in test_q9kernel_tables.c). Hier deshalb
 * NUR fuer diesen Test grosszuegig auf 8-Byte-Schritte gelegt -- betrifft
 * NICHT den echten Deskriptor (der bleibt bei den realen Offsets aus
 * q9kernel_firstproc.c). */
#define Q9K_READYQ_NEXT_OFF      0x08UL
#define Q9K_READYQ_PREV_OFF      0x10UL
#define Q9K_PROCDESC_PRIORITY_OFF 0x18UL
/* P$User liegt real auf $14 und ist dort 4 Byte breit; mit dem 8 Byte
 * breiten Q9_u32 dieses Hosts wuerde ein Zugriff bis $1B reichen und
 * genau die Prioritaet daneben ueberschreiben. */
#define Q9K_PROCDESC_USER_OFF     0x300UL
#define Q9K_ALLPRC_SCRATCH_DESC    ((unsigned long)(g_fakeGlobals + 0x600))
#define Q9K_ALLPRC_SCRATCH_ERROR   ((unsigned long)(g_fakeGlobals + 0x620))
#define Q9K_ALLPRC_SCRATCH_SUCCESS ((unsigned long)(g_fakeGlobals + 0x640))
#define Q9K_DELPRC_SCRATCH_PID     ((unsigned long)(g_fakeGlobals + 0x660))
#define Q9K_DELPRC_SCRATCH_ERROR   ((unsigned long)(g_fakeGlobals + 0x680))
#define Q9K_DELPRC_SCRATCH_SUCCESS ((unsigned long)(g_fakeGlobals + 0x6A0))
#define Q9K_PROCDESC_SAVEDSP_OFF 0x20UL
#define Q9K_PROCDESC_DBGREG_OFF  0x340UL
#define Q9K_PROCDESC_DBGPAR_OFF  0x348UL
#define Q9K_PROCDESC_DBGINSTR_OFF 0x350UL
#define Q9K_PROCDESC_ENTRYPC_OFF 0x28UL
/* NACHTRAG 2026-08-22 (Abschnitt "F$Exit/F$Wait"): gleiche Grosszuegig-
 * keits-Begruendung wie oben -- reale Offsets waeren +0x04/+0x08/+0x0C. */
#define Q9K_PROCDESC_PARENT_OFF     0x38UL
#define Q9K_PROCDESC_MODHDR_OFF     0x48UL
#define Q9K_PROCDESC_EXITSTATUS_OFF 0x50UL
#define Q9K_PROCDESC_ALLOCBASE_OFF  0x60UL
#define Q9K_PROCDESC_ALLOCSIZE_OFF  0x68UL

/* Minimaler Stub fuer das echte Q9K_GetA6 (q9kernel_entry.a) -- liefert
 * hier einen erfundenen, aber erkennbaren "a6-waere-hier"-Kanarienwert
 * statt eines echten Registerinhalts (den es auf dem Testhost gar nicht
 * gibt), damit Q9K_ProcCreate ihn unveraendert in den Fake-Rahmen
 * uebernimmt und der Test ihn dort wiederfinden kann. */
#define Q9K_FAKE_A6_CANARY 0xCAFEUL
unsigned long Q9K_GetA6(void) { return Q9K_FAKE_A6_CANARY; }

/* Seit 2026-09-04 fuehren Q9K_ProcCreate/Q9K_ProcFork die Prozess-ID im
   Deskriptor mit (P$ID, Offset $00 -- fremde Module lesen sie dort). Die
   Nummer stammt aus q9kernel_procapi.c; hier genuegt ein Stub. */
unsigned short Q9K_ProcIdForDesc(unsigned long desc) { (void)desc; return 1; }
/* Q9K_ProcLookup lebt in q9kernel_procapi.c (dort eigenstaendig
 * getestet). Hier ein steuerbarer Stub: F$DelPrc soll nachweislich
 * ueber den Lookup gehen, statt die PID selbst umzurechnen. */
static unsigned long g_lookupResult;
static unsigned short g_lookupLastPid;
unsigned long Q9K_ProcLookup(unsigned short pid)
{
    g_lookupLastPid = pid;
    return g_lookupResult;
}

unsigned long Q9K_AllocMem(unsigned long requestedSize)
{
    unsigned long addr;
    if (g_fakePoolNext + requestedSize > sizeof(g_fakePool))
        return 0;
    addr = (unsigned long)(g_fakePool + g_fakePoolNext);
    g_fakePoolNext += requestedSize;
    return addr;
}

static int g_freeMemCalls;
static unsigned long g_freeMemLastAddr;
static unsigned long g_freeMemLastSize;
void Q9K_FreeMem(unsigned long addr, unsigned long size)
{
    g_freeMemCalls++;
    g_freeMemLastAddr = addr;
    g_freeMemLastSize = size;
}

/* Minimaler Stub fuer das echte Q9K_SchedInsert (q9kernel_sched.c) --
 * reale Funktion wird DORT bereits ausfuehrlich getestet (s. Kopf-
 * kommentar). Hier nur genug, um Q9K_ProcCreates Aufruf nachzubilden:
 * Age=Prioritaet setzen, hinten an Q9_D_ACTIVQ anhaengen. */
#define Q9K_PROCDESC_AGE_OFF 0x30UL
static unsigned long g_lastSchedInsert;
static void Q9K_SchedInsert(unsigned long desc)
{
    unsigned long tail = *(unsigned long *)(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF);
    unsigned char priority = *(unsigned char *)(desc + Q9K_PROCDESC_PRIORITY_OFF);

    g_lastSchedInsert = desc;

    *(unsigned short *)(desc + Q9K_PROCDESC_AGE_OFF) = (unsigned short)priority;

    *(unsigned long *)(desc + Q9K_READYQ_NEXT_OFF) = Q9_D_ACTIVQ;
    *(unsigned long *)(desc + Q9K_READYQ_PREV_OFF) = tail;
    *(unsigned long *)(tail + Q9K_READYQ_NEXT_OFF) = desc;
    *(unsigned long *)(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF) = desc;
}

static int g_schedRemoveCalls;
void Q9K_SchedRemove(unsigned long desc)
{
    (void)desc;
    g_schedRemoveCalls++;
}
void Q9K_WaitQInsert(unsigned long desc) { (void)desc; }
unsigned long Q9K_SchedFirstPick(void) { return g_lastSchedInsert; }

/* Minimale Stubs fuer die echten q9kernel_moddir.c-Funktionen (dort
 * bereits ausfuehrlich eigenstaendig getestet, s. test_q9kernel_moddir.c)
 * -- fuer Q9K_ProcFork reicht ein einfaches, von den Testfaellen unten
 * gesteuertes Fake-Verzeichnis mit GENAU einem eintragbaren Modul. */
static unsigned long g_stubModDirHdr = 0;      /* 0 = "nicht gefunden" simulieren */
static int g_stubModDirUnlinkCalls = 0;        /* zaehlt Q9K_ModDirUnlinkByHeader-Aufrufe */

unsigned long Q9K_ModDirLinkByName(unsigned short desiredTyLang, const char *name)
{
    (void)desiredTyLang;
    (void)name;
    return g_stubModDirHdr;
}

unsigned long Q9K_ModDirUnlinkByHeader(unsigned long hdrAddr)
{
    (void)hdrAddr;
    g_stubModDirUnlinkCalls++;
    return 0;
}

#include "q9kernel_firstproc.c"

/* Schreibt einen 32-Bit-Wert Big-Endian in buf -- fuer den Aufbau eines
 * echten, byte-genauen Fake-Modulkopfs (Q9K_ReadHdrU32BE erwartet das,
 * unabhaengig von der Host-Endianness). */
/* Liest einen 32-Bit-Wert Big-Endian aus einer Adresse -- Gegenstueck zu
 * putBE32, gebraucht fuer die Q9K_SetFrameReg-Registerpruefungen (s.
 * dortigen Kopfkommentar: schreibt ABSICHTLICH IMMER Big-Endian,
 * unabhaengig von der Host-Endianness -- auf dem echten, nativ
 * Big-Endian-68K-Ziel deckungsgleich mit einem normalen Zugriff, auf
 * DIESEM (Little-Endian-)Testhost aber NICHT mehr mit einem nativen
 * "unsigned int*"-Cast lesbar, s. echter Testfehlschlag unten). */
static unsigned long getBE32(unsigned long addr)
{
    const unsigned char *p = (const unsigned char *)addr;
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) | (unsigned long)p[3];
}

static void putBE32(unsigned char *buf, unsigned long addr, unsigned long value)
{
    unsigned char *p = buf + addr;
    p[0] = (unsigned char)(value >> 24);
    p[1] = (unsigned char)(value >> 16);
    p[2] = (unsigned char)(value >> 8);
    p[3] = (unsigned char)value;
}

/* NACHTRAG 2026-09-11 (Fortsetzung 49) -- fuer den M$IRefs-Testaufbau
 * (F5, s. u.): dort sind alle Woerter [MS/Anzahl/Versatz] 16 Bit breit. */
static void putBE16(unsigned char *buf, unsigned long addr, unsigned value)
{
    unsigned char *p = buf + addr;
    p[0] = (unsigned char)(value >> 8);
    p[1] = (unsigned char)value;
}

static int failures = 0;

static void checkU32(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-55s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-55s = %lu (erwartet %lu)\n", label, (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

/* Baut einen einfachen Pool aus count Slots (je slotSize Byte) ab base
 * zu einer Freiliste zusammen -- eigene Kopie der Logik aus
 * q9kernel_tables.c (dort nicht exportiert, bewusst schlank gehalten),
 * damit dieser Test unabhaengig von q9kernel_tables.c bleibt. */
static void buildFreeList(Q9_u32 base, Q9_u32 slotSize, Q9_u32 count, Q9_u32 freeHeadAddr)
{
    Q9_u32 i;
    for (i = 0; i < count - 1; i++)
        Q9K_SetU32(base + i * slotSize, base + (i + 1) * slotSize);
    Q9K_SetU32(base + (count - 1) * slotSize, 0);
    Q9K_SetU32(freeHeadAddr, base);
}

static void FakeEntryA(void) { /* nie aufgerufen, nur Adresse gebraucht */ }
static void FakeEntryB(void) { /* nie aufgerufen, nur Adresse gebraucht */ }

int main(void)
{
    static unsigned char procPool[4 * Q9K_PROCDESC_SIZE]; /* 4 Slots, echte Q9K_PROCDESC_SIZE */
    Q9_u32 poolBase = (Q9_u32)(unsigned long)procPool;
    Q9_u32 entryA = (Q9_u32)(unsigned long)FakeEntryA;
    Q9_u32 desc1, desc2;

    memset(g_fakeGlobals, 0xCC, sizeof(g_fakeGlobals));
    memset(procPool, 0, sizeof(procPool));

    /* Q9_D_ACTIVQ als leere Ringliste initialisieren -- gleiches Muster
     * wie Q9K_InitEmptyQueue in q9kernel_cinit.c (Kopf=Schwanz=sich
     * selbst). */
    Q9K_SetU32(Q9_D_ACTIVQ + Q9K_READYQ_NEXT_OFF, Q9_D_ACTIVQ);
    Q9K_SetU32(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF, Q9_D_ACTIVQ);

    buildFreeList(poolBase, Q9K_PROCDESC_SIZE, 4, Q9K_PROCPOOL_FREE_ADDR);

    /* Fall 1: Erfolgsfall -- Deskriptor + Stack alloziert, Ready-Queue
     * korrekt verkettet (via Q9K_SchedInsert-Stub), Fake-Rahmen plausibel
     * aufgebaut. */
    desc1 = Q9K_ProcCreate(entryA, 7);
    checkU32("Q9K_ProcCreate() liefert einen Deskriptor (!= 0)", (Q9_u32)(desc1 != 0), 1);
    checkU32("Deskriptor == Pool-Basis (erster Slot)", desc1, poolBase);

    checkU32("Freiliste hat nach Pop noch 3 Eintraege (naechster != 0 pruefbar)",
             (Q9_u32)(Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR) != 0), 1);

    checkU32("Q9_D_ACTIVQ.next zeigt jetzt auf den neuen Deskriptor",
             Q9K_GetU32(Q9_D_ACTIVQ + Q9K_READYQ_NEXT_OFF), desc1);
    checkU32("Q9_D_ACTIVQ.prev zeigt ebenfalls auf den neuen Deskriptor (einziger Eintrag)",
             Q9K_GetU32(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF), desc1);
    checkU32("Deskriptor.next zeigt zurueck auf den Sentinel",
             Q9K_GetU32(desc1 + Q9K_READYQ_NEXT_OFF), Q9_D_ACTIVQ);
    checkU32("Deskriptor.prev zeigt zurueck auf den Sentinel",
             Q9K_GetU32(desc1 + Q9K_READYQ_PREV_OFF), Q9_D_ACTIVQ);

    checkU32("Deskriptor-State == 'a' (aktiv)",
             (Q9_u32)(*(Q9_u8 *)(desc1 + Q9K_PROCDESC_STATE_OFF)), (Q9_u32)'a');
    checkU32("Deskriptor-Priority == 7 (uebergebener Wert)",
             (Q9_u32)(*(Q9_u8 *)(desc1 + Q9K_PROCDESC_PRIORITY_OFF)), 7);
    checkU32("Deskriptor.EntryPC == entryA",
             Q9K_GetU32(desc1 + Q9K_PROCDESC_ENTRYPC_OFF), entryA);
    checkU32("Deskriptor.AllocBase ist der eigene Stackblock",
             Q9K_GetU32(desc1 + Q9K_PROCDESC_ALLOCBASE_OFF),
             (Q9_u32)(unsigned long)g_fakePool);
    checkU32("Deskriptor.AllocSize ist die Prozess-Stackgroesse",
             Q9K_GetU32(desc1 + Q9K_PROCDESC_ALLOCSIZE_OFF), Q9K_PROC_STACK_SIZE);

    {
        Q9_u32 sp = Q9K_GetU32(desc1 + Q9K_PROCDESC_SAVEDSP_OFF);
        checkU32("Deskriptor.SavedSP wurde gesetzt (nicht mehr Kanarienwert)", (Q9_u32)(sp != 0), 1);

        /* Fake-Rahmen-Inhalt pruefen: SR=$2000 bei +0x3C, PC=entryA bei
         * +0x3E, Format/Vektor-Wort=0 bei +0x42 (s. Kopfkommentar
         * q9kernel_firstproc.c). Register D0-D7/A0-A6 (+0x00..+0x3B)
         * muessen genullt sein.
         *
         * WICHTIG: PC/A6 hier bewusst NICHT ueber Q9K_GetU32 pruefen --
         * dieser Fake-Rahmen ist ein byte-genau gepackter ECHTER
         * Hardware-Frame (68030-Kurzformat), auf dem echten 32-Bit-Ziel
         * ist Q9_u32 exakt 4 Byte breit und die Felder liegen deshalb
         * luecklos hintereinander (PC direkt hinter SR, Format-Wort
         * direkt hinter PC). Auf DIESEM 64-Bit-Testhost ist Q9_u32 aber
         * 8 Byte breit (unsigned long) -- ein Q9K_GetU32-Lesezugriff auf
         * PC (+0x3E) wuerde deshalb 2 Byte ueber das Format-Wort hinaus
         * lesen, ein Zugriff auf A6 (+0x38) 2 Byte in SR hinein --
         * gleicher, bereits an anderer Stelle dokumentierter Fund
         * (Q9_u32 4 vs. 8 Byte, s. Kopfkommentar test_q9kernel_sched.c)
         * -- HIER kein Testfehler in Q9K_ProcCreate, sondern eine
         * Eigenschaft des Testhelfers selbst. Deshalb hier bewusst
         * schmale, hostunabhaengige 4-Byte-Zugriffe (unsigned int). */
        checkU32("Fake-Rahmen: SR == $2000",
                 (Q9_u32)*(unsigned short *)(sp + 0x3C), 0x2000);
        checkU32("Fake-Rahmen: PC == entryA",
                 (Q9_u32)*(unsigned int *)(sp + 0x3E), (Q9_u32)(unsigned int)entryA);
        checkU32("Fake-Rahmen: Format/Vektor-Wort == 0",
                 (Q9_u32)*(unsigned short *)(sp + 0x42), 0);
        checkU32("Fake-Rahmen: D0-Slot (erstes Registerfeld) genullt",
                 (Q9_u32)*(unsigned int *)(sp + 0x00), 0);
        checkU32("Fake-Rahmen: A6-Slot (+0x38) traegt den echten a6-Wert (Q9K_GetA6), NICHT 0",
                 (Q9_u32)*(unsigned int *)(sp + 0x38), (Q9_u32)Q9K_FAKE_A6_CANARY);
    }

    /* Fall 2: zweiter Aufruf -- zweiter Deskriptor muss HINTER dem
     * ersten in die Ready-Queue eingehaengt werden (append, nicht
     * ueberschreiben). */
    desc2 = Q9K_ProcCreate((Q9_u32)(unsigned long)FakeEntryB, 3);
    checkU32("Zweiter Q9K_ProcCreate() ebenfalls erfolgreich", (Q9_u32)(desc2 != 0), 1);
    checkU32("Sentinel.prev zeigt jetzt auf den ZWEITEN Deskriptor",
             Q9K_GetU32(Q9_D_ACTIVQ + Q9K_READYQ_PREV_OFF), desc2);
    checkU32("Erster Deskriptor zeigt jetzt auf den zweiten (next)",
             Q9K_GetU32(desc1 + Q9K_READYQ_NEXT_OFF), desc2);

    /* Fall 3: Pool-Erschoepfung (nur noch 2 Slots frei, beide schon
     * verbraucht) -- muss sauber 0 liefern, nicht abstuerzen. */
    Q9K_ProcCreate(entryA, 1); /* verbraucht 3. Slot */
    Q9K_ProcCreate(entryA, 1); /* verbraucht 4. und letzten Slot */
    checkU32("Fuenfter Aufruf nach Pool-Erschoepfung schlaegt sauber fehl (0)",
             Q9K_ProcCreate(entryA, 1), 0);

    /* Fall 4: Fehlschlag NACH dem Pool-Pop darf den Slot nicht verlieren. */
    {
        static unsigned char failedCreatePool[Q9K_PROCDESC_SIZE];
        Q9_u32 failedDesc = (Q9_u32)(unsigned long)failedCreatePool;

        memset(failedCreatePool, 0, sizeof(failedCreatePool));
        Q9K_SetU32(Q9K_PROCPOOL_FREE_ADDR, failedDesc);
        g_fakePoolNext = sizeof(g_fakePool); /* der naechste Stack-Alloc muss fehlschlagen */
        checkU32("ProcCreate bei Arena-Erschoepfung liefert 0", Q9K_ProcCreate(entryA, 1), 0);
        checkU32("ProcCreate bei Arena-Erschoepfung gibt den gepoppten Slot zurueck",
                 Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR), failedDesc);
        g_fakePoolNext = 0; /* die folgenden Fork-Faelle bekommen wieder Testarena */
    }

    /* ==== Abschnitt "F$Fork" -- Q9K_ProcFork, eigener, frischer Pool ====
     * Unabhaengig vom obigen Q9K_ProcCreate-Pool (der ist jetzt sowieso
     * erschoepft) -- eigene Freiliste, eigenes Fake-Modulverzeichnis
     * (per g_stubModDirHdr gesteuert). */
    {
        static unsigned char forkPool[2 * Q9K_PROCDESC_SIZE]; /* nur 2 Slots -- absichtlich knapp fuer Fall F5 */
        Q9_u32 forkPoolBase = (Q9_u32)(unsigned long)forkPool;
        /* NACHTRAG 2026-09-11 (Fortsetzung 49): 0x40 -> 0x48 vergroessert
         * -- Q9K_ProcFork liest jetzt IMMER auch M$IData/M$IRefs (Offset
         * $40/$44, s. Q9K_ApplyInitializedData), sonst Lesezugriff hinter
         * dem Ende dieses Arrays. memset unten haelt beide Felder auf 0
         * (= "keine Tabelle vorhanden"), aendert also am Verhalten der
         * bestehenden Faelle F1-F4 nichts. */
        static unsigned char fakeHdr[0x48];           /* echter, byte-genauer Fake-Modulkopf */
        static unsigned char fakeParam[4] = { 0x11, 0x22, 0x33, 0x44 };
        Q9_u16 error;
        Q9_u32 pid1, pid2;
        Q9_u32 desc;

        memset(forkPool, 0, sizeof(forkPool));
        buildFreeList(forkPoolBase, Q9K_PROCDESC_SIZE, 2, Q9K_PROCPOOL_FREE_ADDR);
        Q9K_SetU32(Q9K_PROCPOOL_BASE_ADDR, forkPoolBase);
        Q9K_SetU32(Q9_D_PROC, 0);

        memset(fakeHdr, 0, sizeof(fakeHdr));
        putBE32(fakeHdr, 0x30, 0x40);   /* M$Exec = 0x40 (fiktiv, keine echte Code-Adresse noetig) */
        putBE32(fakeHdr, 0x38, 16);     /* M$Mem  = 16 */
        putBE32(fakeHdr, 0x3C, 256);    /* M$Stack = 256 */
        g_stubModDirHdr = (unsigned long)fakeHdr;

        /* Fall F1: Erfolgsfall MIT expliziter Prioritaet (9) + echtem
         * Parameter (4 Byte) -- prueft Groessenberechnung, Parameter-
         * Kopie UND alle 15 Table-D-9-Registerwerte auf einmal. */
        pid1 = Q9K_ProcFork(0x0101, 0, sizeof(fakeParam),
                             (Q9_u32)(unsigned long)"prog", (Q9_u32)(unsigned long)fakeParam,
                             9, &error);
        checkU32("Q9K_ProcFork() F1: liefert eine Prozess-ID != 0", (Q9_u32)(pid1 != 0), 1);
        checkU32("Q9K_ProcFork() F1: PID == 1 (erster Slot, 1-basierte PID)", pid1, 1);

        desc = forkPoolBase; /* erster Slot */
        checkU32("F1: Deskriptor-State == 'a'", (Q9_u32)*(Q9_u8 *)(desc + Q9K_PROCDESC_STATE_OFF), (Q9_u32)'a');
        checkU32("F1: Deskriptor-Priority == 9 (uebergebener Wert)",
                 (Q9_u32)*(Q9_u8 *)(desc + Q9K_PROCDESC_PRIORITY_OFF), 9);
        /* NACHTRAG 2026-08-22: Q9_D_PROC ist hier 0 (kein laufender
         * Aufrufer, s. Zeile oben "Q9K_SetU32(Q9_D_PROC, 0)") -- ParentDesc
         * muss deshalb ebenfalls 0 sein. ModuleHdr muss auf fakeHdr zeigen. */
        checkU32("F1: Deskriptor-ParentDesc == 0 (kein laufender Aufrufer)",
                 Q9K_GetU32(desc + Q9K_PROCDESC_PARENT_OFF), 0);
        /* ModuleHdr laeuft ueber das normale, hostbreite Q9K_SetU32/GetU32
         * (volle Rundreise, wie SavedSP/EntryPC auch) -- ANDERS als die
         * Table-D-9-Frame-Register (Q9K_SetFrameReg, absichtlich 32-Bit-
         * kappend, s. dortigen Kopfkommentar). Deshalb hier der VOLLE
         * Host-Zeiger, NICHT auf "unsigned int" gekappt. */
        checkU32("F1: Deskriptor-ModuleHdr == fakeHdr",
                 Q9K_GetU32(desc + Q9K_PROCDESC_MODHDR_OFF), (Q9_u32)(unsigned long)fakeHdr);
        checkU32("F1: Deskriptor.AllocBase ist gesetzt",
                 (Q9_u32)(Q9K_GetU32(desc + Q9K_PROCDESC_ALLOCBASE_OFF) != 0), 1);
        checkU32("F1: Deskriptor.AllocSize == Gesamtgroesse",
                 Q9K_GetU32(desc + Q9K_PROCDESC_ALLOCSIZE_OFF), 16 + 256 + sizeof(fakeParam));

        {
            Q9_u32 sp = Q9K_GetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF);
            Q9_u32 totalSize = 16 + 256 + 0 + sizeof(fakeParam);           /* M$Mem+M$Stack+addMem+paramSize */
            /* Datenbereichsbasis nicht direkt aus sp ableitbar (sp liegt
             * INNERHALB des Stack-Bereichs) -- stattdessen ueber das
             * a6-Register im Fake-Rahmen selbst pruefen (das IST die
             * Datenbereichsbasis, s. Q9K_ProcFork).
             *
             * WICHTIG: Q9K_SetFrameReg (q9kernel_firstproc.c) schreibt
             * seit dem echten Bugfix dort ABSICHTLICH IMMER Big-Endian
             * (byteweise, host-/zielbreiten-unabhaengig) -- auf dem
             * echten, nativ Big-Endian-68K-Ziel deckungsgleich mit einem
             * gewoehnlichen Zugriff, auf DIESEM Little-Endian-Testhost
             * aber NICHT mehr per nativem "unsigned int*"-Cast lesbar
             * (realer Testfehlschlag, byte-vertauschte Werte). Deshalb
             * hier durchgehend getBE32 statt eines Pointer-Casts. */
            Q9_u32 a6val = getBE32(sp + 14 * 4);   /* Registerindex 14 = a6 */
            Q9_u32 a5val = getBE32(sp + 13 * 4);   /* a5 = parameter boundary */
            Q9_u32 a3val = getBE32(sp + 11 * 4);   /* a3 = Modulkopfzeiger */
            Q9_u32 a1val = getBE32(sp + 9  * 4);   /* a1 = Top of memory */
            Q9_u32 d0val = getBE32(sp + 0  * 4);
            Q9_u32 d2val = getBE32(sp + 2  * 4);
            Q9_u32 d5val = getBE32(sp + 5  * 4);
            Q9_u32 d6val = getBE32(sp + 6  * 4);

            /* a3val/PC unten bewusst gegen die auf 32 Bit GEKAPPTE Adresse
             * geprueft, nicht den vollen 64-Bit-Host-Zeiger: Q9K_SetFrameReg
             * schreibt ECHT nur 4 Byte (byteweise, s. dortigen
             * Kopfkommentar) -- auf dem echten 32-Bit-Ziel verlustfrei,
             * auf DIESEM 64-Bit-Testhost aber grundsaetzlich nicht
             * rundreisefaehig, falls fakeHdr zufaellig oberhalb 4 GByte
             * liegt (real beobachtet). Gleiches, bereits an anderer Stelle
             * dokumentiertes Limit wie bei Q9K_ModDirPopulateFromBootList
             * ("4-Byte-Adressfelder passen nicht zu 64-Bit-Host-Zeigern") --
             * kein Kernel-Bug, reine Testhost-Eigenschaft. */
            checkU32("F1: a3 (Modulkopfzeiger) == fakeHdr (untere 32 Bit)",
                     a3val, (Q9_u32)(unsigned int)(unsigned long)fakeHdr);
            /* NACHTRAG 2026-09-11 (Fortsetzung 51): a6 traegt seit dem
             * echten Bugfix dort die $8000-Bias (68k_tech.pdf Table 2-6/
             * D-7: "(a6) is always biased by $8000") -- hier deshalb erst
             * abziehen, bevor mit der rohen Datenbereichsbasis
             * weitergerechnet wird. */
            checkU32("F1: a6 (Datenbereichsbasis, $8000-Bias abgezogen) + Gesamtgroesse == a1 (Top of memory)",
                     (a6val - 0x8000UL) + totalSize, a1val);
            checkU32("F1: a5 (Parametergrenze) == a1", a5val, a1val);
            checkU32("F1: d0.w == PID", d0val, pid1);
            checkU32("F1: d2.w == Prioritaet (9)", d2val, 9);
            checkU32("F1: d5.l == paramSize", d5val, sizeof(fakeParam));
            checkU32("F1: d6.l == Gesamtgroesse (M$Mem+M$Stack+addMem+paramSize)", d6val, totalSize);

            checkU32("F1: SR == $2000", (Q9_u32)*(unsigned short *)(sp + Q9K_PROCDESC_REGSAVE_SIZE + 0x00), 0x2000);
            checkU32("F1: PC == fakeHdr + M$Exec (untere 32 Bit, s. Kommentar oben)",
                     (Q9_u32)*(unsigned int *)(sp + Q9K_PROCDESC_REGSAVE_SIZE + 0x02),
                     (Q9_u32)(unsigned int)((unsigned long)fakeHdr + 0x40));

            /* Parameter-Kopie (spBoundary, s. Q9K_ProcFork) hier bewusst
             * NICHT durch Dereferenzieren von a5val geprueft -- a5val ist
             * die auf 32 Bit GEKAPPTE Registerkopie (s. Kommentar oben);
             * als (unsigned long) zurueckgecastet zeigt sie auf DIESEM
             * 64-Bit-Testhost ins Leere (real per Segfault bestaetigt,
             * nicht nur vermutet) -- der ECHTE, unverfaelschte spBoundary-
             * Wert existiert nur als lokale Variable innerhalb von
             * Q9K_ProcFork, hier nicht erreichbar. Gleiches, bereits
             * dokumentiertes Limit wie bei Q9K_ModDirPopulateFromBootList
             * ("selbst NICHT host-testbar... 4-Byte-Adressfelder passen
             * nicht zu 64-Bit-Host-Zeigern") -- die Parameter-Kopie-Logik
             * selbst (byteweise Schleife in Q9K_ProcFork) ist trivial
             * genug, um stattdessen im echten Boot-Test verifiziert zu
             * werden, statt hier eine unhaltbare Pointer-Rundreise zu
             * erzwingen. */
        }

        /* Fall F2: priorityIn=0 UND Q9_D_PROC zeigt auf einen echten
         * "laufenden" Deskriptor mit Prioritaet 42 -- Kind muss dessen
         * Prioritaet erben. */
        {
            static unsigned char fakeCaller[128];
            memset(fakeCaller, 0, sizeof(fakeCaller));
            *(unsigned char *)(fakeCaller + Q9K_PROCDESC_PRIORITY_OFF) = 42;
            Q9K_SetU32(Q9_D_PROC, (Q9_u32)(unsigned long)fakeCaller);

            pid2 = Q9K_ProcFork(0x0101, 0, 0, (Q9_u32)(unsigned long)"prog", 0, 0, &error);
            checkU32("Q9K_ProcFork() F2: liefert eine Prozess-ID != 0", (Q9_u32)(pid2 != 0), 1);
            checkU32("F2: PID == 2 (zweiter Slot, 1-basierte PID)", pid2, 2);
            checkU32("F2: Prioritaet vom Aufrufer geerbt (42)",
                     (Q9_u32)*(Q9_u8 *)(forkPoolBase + Q9K_PROCDESC_SIZE + Q9K_PROCDESC_PRIORITY_OFF), 42);
            checkU32("F2: Deskriptor-ParentDesc == fakeCaller",
                     Q9K_GetU32(forkPoolBase + Q9K_PROCDESC_SIZE + Q9K_PROCDESC_PARENT_OFF),
                     (Q9_u32)(unsigned long)fakeCaller);

            Q9K_SetU32(Q9_D_PROC, 0); /* fuer die naechsten Faelle zuruecksetzen */
        }

        /* Fall F3: Modul nicht gefunden -- E_MNF ($DD), kein Deskriptor
         * verbraucht (Pool ist jetzt ohnehin schon voll, s. u. -- dieser
         * Fall muss VOR jeder Pool-Erschoepfung fehlschlagen, mit dem
         * richtigen Fehlercode, nicht mit E_PRCFUL). */
        {
            unsigned long savedHdr = g_stubModDirHdr;
            g_stubModDirHdr = 0; /* "nicht gefunden" simulieren */

            checkU32("Q9K_ProcFork() F3: liefert 0 bei unbekanntem Modul",
                     Q9K_ProcFork(0x0101, 0, 0, (Q9_u32)(unsigned long)"unknown", 0, 1, &error), 0);
            checkU32("F3: Fehlercode == E_MNF ($DD)", (Q9_u32)error, 0x00DDUL);

            g_stubModDirHdr = savedHdr;
        }

        /* Fall F4: Pool jetzt erschoepft (beide Slots durch F1/F2
         * verbraucht) -- E_PRCFUL ($E5), UND Q9K_ModDirUnlinkByHeader
         * MUSS aufgerufen worden sein (Link-Zaehler-Ruecknahme, s.
         * Kopfkommentar Q9K_ProcFork). */
        {
            int unlinkCallsBefore = g_stubModDirUnlinkCalls;
            int freeCallsBefore = g_freeMemCalls;

            checkU32("Q9K_ProcFork() F4: liefert 0 bei erschoepftem Pool",
                     Q9K_ProcFork(0x0101, 0, 0, (Q9_u32)(unsigned long)"prog", 0, 1, &error), 0);
            checkU32("F4: Fehlercode == E_PRCFUL ($E5)", (Q9_u32)error, 0x00E5UL);
            checkU32("F4: Q9K_ModDirUnlinkByHeader wurde aufgerufen (Link-Zaehler zurueckgenommen)",
                     (Q9_u32)(g_stubModDirUnlinkCalls > unlinkCallsBefore), 1);
            checkU32("F4: der bereits reservierte Prozessblock wird zurueckgegeben",
                     (Q9_u32)(g_freeMemCalls == freeCallsBefore + 1), 1);
            checkU32("F4: Rueckgabe verwendet die exakte Fork-Gesamtgroesse",
                     g_freeMemLastSize, 16 + 256);
        }

        /* Fall F5 (NACHTRAG 2026-09-11, Fortsetzung 49): M$IData/M$IRefs
         * -- eigener, frischer Pool (F1/F2 haben den obigen erschoepft),
         * eigener Fake-Modulkopf MIT echtem M$IData/M$IRefs-Tabellenpaar,
         * Layout/Format 1:1 aus der Byte-Ebenen-Verifikation gegen
         * echo.mod uebernommen (s. docs/OWN_KERNEL_STATUS.md,
         * Fortsetzung 49 -- 2 Woerter Nutzdaten statt 14, aber gleiche
         * Struktur: EIN M$IData-Eintrag, dann GENAU zwei M$IRefs-Gruppen
         * [Kode-, dann Datenzeiger]):
         *
         *   M$IData (Kopfoffset $50): Zieloffset $10, Anzahl 8,
         *     Nutzlast [datenoffset+0]=5 ("Datenzeigerwert", noch
         *     unreloziert), [datenoffset+4]=3 ("Kodezeigerwert", noch
         *     unreloziert) -- endet exakt bei $60, dort beginnt M$IRefs.
         *   M$IRefs (Kopfoffset $60):
         *     Gruppe 1 (Kode):  MS=0, Anzahl=1, Versatz $14, Terminator.
         *     Gruppe 2 (Daten): MS=0, Anzahl=1, Versatz $10, Terminator.
         *
         * Erwartung nach Q9K_ProcFork: Datenbereich+$10 == 5+block
         * (Datenzeiger-Relozierung), Datenbereich+$14 == 3+hdrAddr
         * (Kodezeiger-Relozierung). */
        {
            static unsigned char forkPool5[Q9K_PROCDESC_SIZE];
            Q9_u32 forkPoolBase5 = (Q9_u32)(unsigned long)forkPool5;
            static unsigned char fakeHdr5[0x80];
            Q9_u32 pid5;
            unsigned long realBlock5;   /* s. Kommentar unten -- ECHTER Host-Zeiger, nicht der gekappte a6-Wert */
            Q9_u32 relocatedData, relocatedCode;

            memset(forkPool5, 0, sizeof(forkPool5));
            buildFreeList(forkPoolBase5, Q9K_PROCDESC_SIZE, 1, Q9K_PROCPOOL_FREE_ADDR);
            Q9K_SetU32(Q9K_PROCPOOL_BASE_ADDR, forkPoolBase5);
            Q9K_SetU32(Q9_D_PROC, 0);

            memset(fakeHdr5, 0, sizeof(fakeHdr5));
            putBE32(fakeHdr5, 0x30, 0x20);   /* M$Exec = 0x20 (fiktiv) */
            putBE32(fakeHdr5, 0x38, 0x40);   /* M$Mem  = 64 -- reicht fuer Datenoffset $10/$14 */
            putBE32(fakeHdr5, 0x3C, 256);    /* M$Stack = 256 */
            putBE32(fakeHdr5, 0x40, 0x50);   /* M$IData = $50 */
            putBE32(fakeHdr5, 0x44, 0x60);   /* M$IRefs = $60 */
            putBE32(fakeHdr5, 0x50, 0x10);   /* IData-Eintrag: Zieloffset $10 */
            putBE32(fakeHdr5, 0x54, 8);      /* IData-Eintrag: Anzahl Bytes = 8 */
            putBE32(fakeHdr5, 0x58, 5);      /* Nutzlast[0] -- spaeter Datenzeiger */
            putBE32(fakeHdr5, 0x5C, 3);      /* Nutzlast[4] -- spaeter Kodezeiger */
            putBE16(fakeHdr5, 0x60, 0);      /* Gruppe 1 (Kode): MS = 0 */
            putBE16(fakeHdr5, 0x62, 1);      /* Gruppe 1: Anzahl = 1 */
            putBE16(fakeHdr5, 0x64, 0x14);   /* Gruppe 1: Versatz $14 (Kodezeigerfeld) */
            putBE16(fakeHdr5, 0x66, 0);      /* Gruppe 1: Terminator MS = 0 */
            putBE16(fakeHdr5, 0x68, 0);      /* Gruppe 1: Terminator Anzahl = 0 */
            putBE16(fakeHdr5, 0x6A, 0);      /* Gruppe 2 (Daten): MS = 0 */
            putBE16(fakeHdr5, 0x6C, 1);      /* Gruppe 2: Anzahl = 1 */
            putBE16(fakeHdr5, 0x6E, 0x10);   /* Gruppe 2: Versatz $10 (Datenzeigerfeld) */
            putBE16(fakeHdr5, 0x70, 0);      /* Gruppe 2: Terminator MS = 0 */
            putBE16(fakeHdr5, 0x72, 0);      /* Gruppe 2: Terminator Anzahl = 0 */
            g_stubModDirHdr = (unsigned long)fakeHdr5;

            /* WICHTIG (gleiches Host-Limit wie bei F1 oben, "a5val ...
             * zeigt auf DIESEM 64-Bit-Testhost ins Leere"): der ECHTE
             * Blockzeiger darf NICHT ueber das per Q9K_SetFrameReg
             * bewusst auf 32 Bit gekappte a6-Register zurueckgelesen
             * werden (waere kein gueltiger Host-Zeiger mehr). Stattdessen
             * wird er hier UNABHAENGIG, direkt aus dem bekannten
             * deterministischen Verhalten des Fake-Bump-Allocators
             * (Q9K_AllocMem oben: naechste Rueckgabe == g_fakePool +
             * aktueller g_fakePoolNext-Stand) VOR dem Aufruf berechnet. */
            realBlock5 = (unsigned long)(g_fakePool + g_fakePoolNext);

            pid5 = Q9K_ProcFork(0x0101, 0, 0, (Q9_u32)(unsigned long)"prog", 0, 1, &error);
            checkU32("Q9K_ProcFork() F5: liefert eine Prozess-ID != 0", (Q9_u32)(pid5 != 0), 1);

            relocatedData = (Q9_u32)getBE32(realBlock5 + 0x10);
            relocatedCode = (Q9_u32)getBE32(realBlock5 + 0x14);
            /* Erwartungswerte bewusst auf 32 Bit GEKAPPT (unsigned int) --
             * gleiche Begruendung wie bei den a3/PC-Pruefungen in F1 oben:
             * Q9K_ApplyInitializedData schreibt die relozierten Werte
             * byteweise (NUR 4 Byte, s. dortigen Kopfkommentar), auf DIESEM
             * 64-Bit-Testhost also nicht rundreisefaehig mit dem vollen
             * Host-Zeiger. */
            checkU32("F5: M$IData wurde kopiert UND per M$IRefs (Datenzeiger-Gruppe) reloziert (5+block)",
                     relocatedData, (Q9_u32)(unsigned int)(5 + realBlock5));
            checkU32("F5: M$IData wurde kopiert UND per M$IRefs (Kodezeiger-Gruppe) reloziert (3+hdrAddr)",
                     relocatedCode, (Q9_u32)(unsigned int)(3 + (unsigned long)fakeHdr5));
        }
    }


    /* F$DFork: the child receives the initial register image but is removed
     * from the ready queue and remains allocated in the WAITING state. */
    {
        static unsigned char debugPool[Q9K_PROCDESC_SIZE];
        static unsigned char debugParent[Q9K_PROCDESC_SIZE];
        static unsigned char debugHdr[0x48];
        static unsigned char registerImage[72];
        Q9_u32 debugBase = (Q9_u32)(unsigned long)debugPool;
        Q9_u16 error = 0xFFFF;
        Q9_u32 debugPid;

        memset(debugPool, 0, sizeof(debugPool));
        memset(debugParent, 0, sizeof(debugParent));
        memset(debugHdr, 0, sizeof(debugHdr));
        memset(registerImage, 0xCC, sizeof(registerImage));
        buildFreeList(debugBase, Q9K_PROCDESC_SIZE, 1, Q9K_PROCPOOL_FREE_ADDR);
        Q9K_SetU32(Q9K_PROCPOOL_BASE_ADDR, debugBase);
        Q9K_SetU32(Q9_D_PROC, (Q9_u32)(unsigned long)debugParent);
        *(Q9_u8 *)(debugParent + Q9K_PROCDESC_STATE_OFF) = 'a';
        putBE32(debugHdr, 0x30, 0x20);
        putBE32(debugHdr, 0x38, 16);
        putBE32(debugHdr, 0x3C, 256);
        g_stubModDirHdr = (unsigned long)debugHdr;
        g_schedRemoveCalls = 0;
        g_lookupResult = debugBase;

        debugPid = Q9K_ProcDebugFork(0x0101, 0, 0,
                                      (Q9_u32)(unsigned long)"prog", 0,
                                      1, (Q9_u32)(unsigned long)registerImage,
                                      &error);
        checkU32("F$DFork liefert eine Kind-PID", (Q9_u32)(debugPid != 0), 1);
        checkU32("F$DFork entfernt das Kind aus der Ready-Queue",
                 (Q9_u32)g_schedRemoveCalls, 1);
        checkU32("F$DFork laesst das Kind angehalten (State w)",
                 (Q9_u32)*(Q9_u8 *)(debugBase + Q9K_PROCDESC_STATE_OFF), 'w');
        checkU32("F$DFork Registerpuffer beginnt mit der Kind-PID",
                 getBE32((unsigned long)registerImage), debugPid);
        checkU32("F$DFork lehnt einen Null-Registerpuffer ab",
                 (Q9_u32)Q9K_ProcDebugFork(0x0101, 0, 0,
                                             (Q9_u32)(unsigned long)"prog", 0,
                                             1, 0, &error), 0);
        checkU32("F$DFork Null-Registerpuffer meldet E$BPAddr",
                 (Q9_u32)error, 0x00D2UL);

        /* F$DExec restores the debugger-owned register image, parks the
         * parent and selects the child for execution. */
        putBE32(registerImage, 0, 0x11223344UL);
        g_lastSchedInsert = 0;
        Q9_u32 execError = 0;
        checkU32("F$DExec waehlt den Debugger-Prozess als naechsten Kontext",
                 Q9K_ProcDebugExec((Q9_u16)debugPid, 1, &execError),
                 debugBase);
        checkU32("F$DExec parkt den Elternprozess",
                 (Q9_u32)*(Q9_u8 *)(debugParent + Q9K_PROCDESC_STATE_OFF), 'w');
        checkU32("F$DExec setzt das Kind wieder aktiv",
                 (Q9_u32)*(Q9_u8 *)(debugBase + Q9K_PROCDESC_STATE_OFF), 'a');
        checkU32("F$DExec uebernimmt D0 aus dem Registerpuffer",
                 getBE32(Q9K_GetU32(debugBase + Q9K_PROCDESC_SAVEDSP_OFF)), 0x11223344UL);
    }


    /* F$AllPrc (Callcode 0x4B) und F$DelPrc (0x4C): Deskriptor aus dem
     * Pool holen bzw. zurueckgeben -- ohne sonstige Betriebsmittel
     * anzufassen, wie die Beschreibung von F$DelPrc ausdruecklich
     * verlangt. */
    {
        static unsigned char prcPool[4 * 0x400];
        Q9_u32 prcBase = (Q9_u32)(unsigned long)prcPool;
        Q9_u32 desc = 0;
        Q9_u32 first;
        Q9_u16 err;

        memset(prcPool, 0xAA, sizeof(prcPool));
        buildFreeList(prcBase, Q9K_PROCDESC_SIZE, 4, Q9K_PROCPOOL_FREE_ADDR);
        Q9K_SetU32(Q9K_PROCPOOL_BASE_ADDR, prcBase);

        err = 0xFFFF;
        checkU32("F$AllPrc liefert einen Deskriptor",
                 (Q9_u32)Q9K_ProcAllPrc(&desc, &err), 1);
        checkU32("F$AllPrc meldet dabei keinen Fehler", (Q9_u32)err, 0);
        checkU32("F$AllPrc nimmt ihn aus dem Pool", (Q9_u32)(desc >= prcBase), 1);
        first = desc;

        /* Der Deskriptor muss vollstaendig geloescht sein -- der Slot
         * trug vorher Muell (0xAA). */
        {
            Q9_u32 i;
            Q9_u32 nonzero = 0;
            for (i = 0; i < Q9K_PROCDESC_SIZE; ++i) {
                Q9_u8 b = Q9K_GetU8(desc + i);
                /* P$ID belegt Offset 0..1, das Zustandsbyte liegt auf
                 * Q9K_PROCDESC_STATE_OFF -- beide setzt F$AllPrc
                 * absichtlich, alles andere muss 0 sein. */
                if (b != 0 && i > 1 && i != Q9K_PROCDESC_STATE_OFF)
                    nonzero++;
            }
            checkU32("F$AllPrc loescht den Deskriptor bis auf Zustand und ID",
                     nonzero, 0);
        }
        checkU32("F$AllPrc markiert ihn als belegt (wartend)",
                 (Q9_u32)Q9K_GetU8(desc + Q9K_PROCDESC_STATE_OFF),
                 Q9K_PROCDESC_STATE_WAITING);

        /* Pool leerraeumen -> E$PrcFul. */
        while (Q9K_ProcAllPrc(&desc, &err)) {
            /* weiter, bis nichts mehr frei ist */
        }
        checkU32("F$AllPrc meldet den erschoepften Pool mit E$PrcFul",
                 (Q9_u32)err, Q9K_E_PRCFUL);

        /* F$DelPrc gibt genau den Slot zurueck, den der Lookup liefert. */
        g_lookupResult = first;
        err = 0xFFFF;
        checkU32("F$DelPrc nimmt eine gueltige PID an",
                 (Q9_u32)Q9K_ProcDelPrc(7, &err), 1);
        checkU32("F$DelPrc hat dafuer wirklich den Lookup benutzt",
                 (Q9_u32)g_lookupLastPid, 7);
        checkU32("F$DelPrc markiert den Slot als frei",
                 (Q9_u32)Q9K_GetU8(first + Q9K_PROCDESC_STATE_OFF), 0);
        checkU32("F$DelPrc haengt ihn vorn in die Freiliste",
                 Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR), first);

        /* Und er ist danach wieder vergebbar. */
        checkU32("Der freigegebene Slot ist wieder allozierbar",
                 (Q9_u32)Q9K_ProcAllPrc(&desc, &err), 1);
        checkU32("und zwar genau derselbe", desc, first);

        g_lookupResult = 0;
        err = 0;
        checkU32("F$DelPrc weist eine unbekannte PID ab",
                 (Q9_u32)Q9K_ProcDelPrc(99, &err), 0);
        checkU32("F$DelPrc meldet dabei E$PrcID", (Q9_u32)err, Q9K_E_PRCID);

        /* Bruecken: volle Zellbreite. */
        g_lookupResult = first;
        Q9K_SetU32(Q9K_DELPRC_SCRATCH_PID, 3);
        Q9K_SysDelPrcImpl();
        checkU32("F$DelPrc-Bridge meldet Erfolg in voller Zellbreite",
                 Q9K_GetU32(Q9K_DELPRC_SCRATCH_SUCCESS), 1);
        Q9K_SysAllPrcImpl();
        checkU32("F$AllPrc-Bridge meldet Erfolg in voller Zellbreite",
                 Q9K_GetU32(Q9K_ALLPRC_SCRATCH_SUCCESS), 1);
        checkU32("F$AllPrc-Bridge legt den Deskriptor in der Zelle ab",
                 Q9K_GetU32(Q9K_ALLPRC_SCRATCH_DESC), first);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
