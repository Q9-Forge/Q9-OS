/*
 * test_q9kernel_traplink.c -- Regressionstest fuer q9kernel_traplink.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. Zwei externe
 * Abhaengigkeiten werden hier durch einfache, aufrufzaehlende/konfigurier-
 * bare Stubs ersetzt: Q9K_ModDirLinkByName (real in q9kernel_moddir.c,
 * dort bereits eigenstaendig getestet) und Q9K_ProcSRqMem (real in
 * q9kernel_sysmem.c, ebenfalls eigenstaendig getestet, s.
 * test_q9kernel_sysmem.c) -- dieser Test prueft NUR die F$TLink-eigene
 * Buchhaltungslogik (Trap-Tabellen-Slot-Pruefung, Modulsuche, bedingte
 * Speicheranforderung, Header-Feld-Auswertung).
 *
 * WICHTIG (gleicher Fund wie in test_q9kernel_tables.c/firstproc.c):
 * Q9_u32 ist in q9kernel_traplink.c als "unsigned long" typedef'd, auf
 * diesem 64-Bit-Testhost also 8 statt 4 Byte breit. Q9K_TLINK_SCRATCH_*
 * (per #ifndef VOR dem #include ueberschreibbar) bekommen deshalb
 * grosszuegige 0x40-Byte-Abstaende. Die drei Trap-Tabellen-Felder
 * (Q9K_TRAPTBL_OFF_MODPTR/EXECENTRY/STATICPTR) sind dagegen NICHT
 * ueberschreibbar (echte, feste 4-Byte-Abstaende, Q9K_TRAPTBL_ENTRY_SIZE
 * = 12) -- die drei Q9K_SetU32-Aufrufe in Q9K_ProcTLink schreiben in
 * aufsteigender Reihenfolge (ModPtr, ExecEntry, StaticPtr), jeder
 * schreibt 8 Byte ab seinem jeweiligen 4-Byte-Offset. Weil die
 * Schreibreihenfolge aufsteigend ist, landen die "echten" unteren 4 Byte
 * jedes Feldes exakt an dessen eigenem Offset (die oberen 4 Byte jedes
 * Schreibvorgangs -- reine Nullpadding wegen der Hostbreite -- werden vom
 * naechsten Feld ueberschrieben, nie dessen echte unteren 4 Byte). Diese
 * Tests lesen die Felder deshalb bewusst als reine 4-Byte-Werte (LE, wie
 * auf diesem Testhost ueblich) zurueck statt per Q9K_GetU32 (das wuerde 8
 * Byte lesen und damit Nachbarfeld-Reste mit einmischen).
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_traplink test_q9kernel_traplink.c && \
 *       ./test_q9kernel_traplink
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x4000];
static unsigned char g_fakeModule[0x100];

/* Q9_D_PROC zeigt auf einen Fake-Prozessdeskriptor am Anfang von
 * g_fakeGlobals; Q9K_PROCDESC_TRAPTBL_OFF grosszuegig dahinter (die
 * echte pro-Slot-Breite von 12 Byte bleibt unveraendert, s. Kopfkommentar
 * -- nur die STARTADRESSE der Tabelle ist hier frei waehlbar). */
#define Q9_D_PROC                ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9K_PROCDESC_TRAPTBL_OFF 0x100UL

/* Q9_D_PROC (s. o.) ist die Adresse EINES ZEIGERFELDS -- die echte
 * Konvention (jede andere Datei dieses Kernels, die Q9_D_Proc benutzt)
 * ist "curProc = Q9K_GetU32(Q9_D_PROC)": dort steht der Zeiger auf den
 * AKTUELLEN Prozessdeskriptor, nicht der Deskriptor selbst. Der Fake-
 * Deskriptor braucht deshalb eine eigene, davon getrennte Adresse. */
#define Q9K_FAKE_PROCDESC        ((unsigned long)(g_fakeGlobals + 0x2000))

#define Q9K_TLINK_SCRATCH_TRAPNUM   ((unsigned long)(g_fakeGlobals + 0x800))
#define Q9K_TLINK_SCRATCH_MEMOVR    ((unsigned long)(g_fakeGlobals + 0x840))
#define Q9K_TLINK_SCRATCH_NAMEPTR   ((unsigned long)(g_fakeGlobals + 0x880))
#define Q9K_TLINK_SCRATCH_PASTNAME  ((unsigned long)(g_fakeGlobals + 0x8C0))
#define Q9K_TLINK_SCRATCH_MODPTR    ((unsigned long)(g_fakeGlobals + 0x900))
#define Q9K_TLINK_SCRATCH_EXECENTRY ((unsigned long)(g_fakeGlobals + 0x940))
#define Q9K_TLINK_SCRATCH_INITENTRY ((unsigned long)(g_fakeGlobals + 0x980))
#define Q9K_TLINK_SCRATCH_STATICPTR ((unsigned long)(g_fakeGlobals + 0x9C0))
#define Q9K_TLINK_SCRATCH_ERROR     ((unsigned long)(g_fakeGlobals + 0xA00))
#define Q9K_TLINK_SCRATCH_SUCCESS   ((unsigned long)(g_fakeGlobals + 0xA40))

/* Aufrufzaehlende/konfigurierbare Stubs fuer die beiden echten
 * Abhaengigkeiten (s. Kopfkommentar). Q9_u32/Q9_u16 sind erst NACH dem
 * #include unten verfuegbar -- hier bewusst "unsigned long"/"unsigned
 * short" (identische Typen), gleiches Muster wie in den anderen
 * Kernel-Tests. */
static unsigned long g_modDirHdr = 0;      /* 0 = "nicht gefunden" */
static int           g_modDirCalls = 0;
static char          g_modDirLastName[64];
unsigned long Q9K_ModDirLinkByName(unsigned short desiredTyLang, const char *name)
{
    (void)desiredTyLang;
    g_modDirCalls++;
    strncpy(g_modDirLastName, name, sizeof(g_modDirLastName) - 1);
    g_modDirLastName[sizeof(g_modDirLastName) - 1] = 0;
    return g_modDirHdr;
}

static int           g_srqmemReturn = 1;
static unsigned long g_srqmemOutAddr = 0;
static unsigned long g_srqmemOutSize = 0;
static unsigned short g_srqmemOutError = 0;
static unsigned long g_srqmemLastRequested = 0;
static int           g_srqmemCalls = 0;
int Q9K_ProcSRqMem(unsigned long requestedSize, unsigned long *outAddr, unsigned long *outSize, unsigned short *outError)
{
    g_srqmemCalls++;
    g_srqmemLastRequested = requestedSize;
    *outAddr = g_srqmemOutAddr;
    *outSize = g_srqmemOutSize;
    *outError = g_srqmemOutError;
    return g_srqmemReturn;
}

#include "q9kernel_traplink.c"

static int g_failures = 0;

static void checkU32(const char *label, unsigned long got, unsigned long want)
{
    if (got != want) {
        printf("FAIL %s: got=0x%lx want=0x%lx\n", label, got, want);
        g_failures++;
    } else {
        printf("ok   %s\n", label);
    }
}

static void checkStr(const char *label, const char *got, const char *want)
{
    if (strcmp(got, want) != 0) {
        printf("FAIL %s: got=\"%s\" want=\"%s\"\n", label, got, want);
        g_failures++;
    } else {
        printf("ok   %s\n", label);
    }
}

/* Liest ein Trap-Tabellen-Feld als reinen 4-Byte-LE-Wert (s. Kopf-
 * kommentar -- Q9K_GetU32 selbst waere hier wegen der 8-Byte-Hostbreite
 * irrefuehrend). */
static unsigned long readSlotField(unsigned long slotAddr, unsigned long fieldOff)
{
    unsigned char *p = (unsigned char *)(slotAddr + fieldOff);
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

static unsigned long slotAddrFor(unsigned long trapNum)
{
    return Q9K_FAKE_PROCDESC + Q9K_PROCDESC_TRAPTBL_OFF + (trapNum - 1UL) * Q9K_TRAPTBL_ENTRY_SIZE;
}

/* Baut ein Fake-Modulheader-Fragment: an den ECHTEN, festen Offsets
 * Q9K_MH_EXEC/Q9K_MH_MEM/Q9K_MH_INIT (0x30/0x38/0x48, s. q9moduleheader.h
 * -- in q9kernel_traplink.c bewusst nicht ueberschreibbar dupliziert)
 * je ein grossenendianes 32-Bit-Wort ablegen, wie es Q9K_TLinkReadU32BE
 * erwartet. */
static void setModuleField(unsigned long fieldOff, unsigned long value)
{
    unsigned char *p = g_fakeModule + fieldOff;
    p[0] = (unsigned char)(value >> 24);
    p[1] = (unsigned char)(value >> 16);
    p[2] = (unsigned char)(value >> 8);
    p[3] = (unsigned char)value;
}

static void resetAll(void)
{
    memset(g_fakeGlobals, 0, sizeof(g_fakeGlobals));
    memset(g_fakeModule, 0, sizeof(g_fakeModule));
    *(unsigned long *)Q9_D_PROC = Q9K_FAKE_PROCDESC;   /* Q9_D_Proc-Zeigerfeld, s. Kopfkommentar */
    g_modDirHdr = 0;
    g_modDirCalls = 0;
    g_modDirLastName[0] = 0;
    g_srqmemReturn = 1;
    g_srqmemOutAddr = 0;
    g_srqmemOutSize = 0;
    g_srqmemOutError = 0;
    g_srqmemLastRequested = 0;
    g_srqmemCalls = 0;
}

int main(void)
{
    unsigned long pastName, modPtr, execEntry, initEntry, staticPtr;
    unsigned short err;
    int ok;

    /* Fall 1: Erfolg ohne eigenen statischen Speicher (M$Mem=0, kein
     * Aufrufer-Override) -- das im Kapitel-5-Beispiel gezeigte Muster
     * ("TrapInit tut nichts mit ihrem Speicher"). */
    {
        resetAll();
        strcpy((char *)g_fakeGlobals + 0x1000, "csl");
        setModuleField(Q9K_MH_EXEC, 0x40UL);
        setModuleField(Q9K_MH_MEM, 0UL);
        setModuleField(Q9K_MH_INIT, 0x50UL);
        g_modDirHdr = (unsigned long)g_fakeModule;

        ok = Q9K_ProcTLink(13UL, 0UL, (unsigned long)(g_fakeGlobals + 0x1000),
                            &pastName, &modPtr, &execEntry, &initEntry, &staticPtr, &err);
        checkU32("F1: Erfolg (Rueckgabe 1)", (unsigned long)ok, 1);
        checkU32("F1: Modulzeiger == Fake-Header", modPtr, (unsigned long)g_fakeModule);
        checkU32("F1: ExecEntry == hdr+0x40", execEntry, (unsigned long)g_fakeModule + 0x40UL);
        checkU32("F1: InitEntry == hdr+0x50", initEntry, (unsigned long)g_fakeModule + 0x50UL);
        checkU32("F1: kein statischer Speicher angefordert", staticPtr, 0);
        checkU32("F1: Q9K_ProcSRqMem NICHT aufgerufen (M$Mem=0, kein Override)",
                 (unsigned long)g_srqmemCalls, 0);
        checkU32("F1: past-Name zeigt hinter \"csl\\0\" (4 Byte)",
                 pastName, (unsigned long)(g_fakeGlobals + 0x1000 + 4));
        checkStr("F1: Q9K_ModDirLinkByName bekam den echten Namen", g_modDirLastName, "csl");

        /* Q9K_TRAPTBL_OFF_MODPTR/EXECENTRY sind echte 32-Bit-Felder --
         * auf diesem 64-Bit-Testhost kann g_fakeModule (ein echter
         * Hostzeiger) ueber 32 Bit hinausgehen, deshalb Vergleich nur
         * gegen die unteren 32 Bit (das entspricht exakt dem, was ein
         * reales 32-Bit-Feld ueberhaupt fassen koennte). */
        checkU32("F1: Trap-Tabelle Slot 13 ModPtr gesetzt",
                 readSlotField(slotAddrFor(13UL), Q9K_TRAPTBL_OFF_MODPTR),
                 (unsigned long)g_fakeModule & 0xFFFFFFFFUL);
        checkU32("F1: Trap-Tabelle Slot 13 ExecEntry gesetzt",
                 readSlotField(slotAddrFor(13UL), Q9K_TRAPTBL_OFF_EXECENTRY),
                 ((unsigned long)g_fakeModule + 0x40UL) & 0xFFFFFFFFUL);
        checkU32("F1: Trap-Tabelle Slot 13 StaticPtr == 0",
                 readSlotField(slotAddrFor(13UL), Q9K_TRAPTBL_OFF_STATICPTR), 0);
    }

    /* Fall 2: trapNum ausserhalb 1-15 -- E$Param, kein Modulsuche-Aufruf. */
    {
        resetAll();
        ok = Q9K_ProcTLink(0UL, 0UL, (unsigned long)(g_fakeGlobals + 0x1000),
                            &pastName, &modPtr, &execEntry, &initEntry, &staticPtr, &err);
        checkU32("F2a: trapNum=0 -> Fehlschlag", (unsigned long)ok, 0);
        checkU32("F2a: E$Param ($E1)", err, 0x00E1UL);
        checkU32("F2a: Q9K_ModDirLinkByName NICHT aufgerufen", (unsigned long)g_modDirCalls, 0);

        resetAll();
        ok = Q9K_ProcTLink(16UL, 0UL, (unsigned long)(g_fakeGlobals + 0x1000),
                            &pastName, &modPtr, &execEntry, &initEntry, &staticPtr, &err);
        checkU32("F2b: trapNum=16 -> Fehlschlag", (unsigned long)ok, 0);
        checkU32("F2b: E$Param ($E1)", err, 0x00E1UL);
    }

    /* Fall 3: Trap-Slot bereits belegt -- E$ModBsy, kein Modulsuche-
     * Aufruf (Kollisionspruefung kommt VOR der Modulsuche). */
    {
        resetAll();
        /* Slot 5 "von Hand" als belegt markieren (ModPtr != 0). */
        {
            unsigned char *p = (unsigned char *)(slotAddrFor(5UL) + Q9K_TRAPTBL_OFF_MODPTR);
            p[0] = 0; p[1] = 0; p[2] = 0x12; p[3] = 0x34;
        }
        ok = Q9K_ProcTLink(5UL, 0UL, (unsigned long)(g_fakeGlobals + 0x1000),
                            &pastName, &modPtr, &execEntry, &initEntry, &staticPtr, &err);
        checkU32("F3: bereits belegter Slot -> Fehlschlag", (unsigned long)ok, 0);
        checkU32("F3: E$ModBsy ($D1)", err, 0x00D1UL);
        checkU32("F3: Q9K_ModDirLinkByName NICHT aufgerufen", (unsigned long)g_modDirCalls, 0);
    }

    /* Fall 4: Modul nicht gefunden -- E$MNF. */
    {
        resetAll();
        g_modDirHdr = 0;
        ok = Q9K_ProcTLink(1UL, 0UL, (unsigned long)(g_fakeGlobals + 0x1000),
                            &pastName, &modPtr, &execEntry, &initEntry, &staticPtr, &err);
        checkU32("F4: Modul nicht gefunden -> Fehlschlag", (unsigned long)ok, 0);
        checkU32("F4: E$MNF ($DD)", err, 0x00DDUL);
        checkU32("F4: Trap-Tabelle Slot 1 bleibt frei",
                 readSlotField(slotAddrFor(1UL), Q9K_TRAPTBL_OFF_MODPTR), 0);
    }

    /* Fall 5: M$Mem != 0 (kein Aufrufer-Override) -- Q9K_ProcSRqMem MUSS
     * mit genau dieser Groesse aufgerufen werden, Erfolg. */
    {
        resetAll();
        setModuleField(Q9K_MH_EXEC, 0x40UL);
        setModuleField(Q9K_MH_MEM, 256UL);
        setModuleField(Q9K_MH_INIT, 0x50UL);
        g_modDirHdr = (unsigned long)g_fakeModule;
        g_srqmemReturn = 1;
        g_srqmemOutAddr = 0x7000UL;
        g_srqmemOutSize = 256UL;

        ok = Q9K_ProcTLink(7UL, 0UL, (unsigned long)(g_fakeGlobals + 0x1000),
                            &pastName, &modPtr, &execEntry, &initEntry, &staticPtr, &err);
        checkU32("F5: Erfolg mit M\\$Mem-Speicher", (unsigned long)ok, 1);
        checkU32("F5: Q9K_ProcSRqMem mit M\\$Mem-Groesse (256) aufgerufen",
                 g_srqmemLastRequested, 256UL);
        checkU32("F5: staticPtr == Q9K_ProcSRqMem-Ergebnis", staticPtr, 0x7000UL);
        checkU32("F5: Trap-Tabelle Slot 7 StaticPtr gesetzt",
                 readSlotField(slotAddrFor(7UL), Q9K_TRAPTBL_OFF_STATICPTR), 0x7000UL);
    }

    /* Fall 6: Aufrufer-Override (d1.l) hat Vorrang vor M$Mem. */
    {
        resetAll();
        setModuleField(Q9K_MH_EXEC, 0x40UL);
        setModuleField(Q9K_MH_MEM, 256UL);
        setModuleField(Q9K_MH_INIT, 0x50UL);
        g_modDirHdr = (unsigned long)g_fakeModule;
        g_srqmemReturn = 1;
        g_srqmemOutAddr = 0x9000UL;
        g_srqmemOutSize = 64UL;

        ok = Q9K_ProcTLink(2UL, 64UL, (unsigned long)(g_fakeGlobals + 0x1000),
                            &pastName, &modPtr, &execEntry, &initEntry, &staticPtr, &err);
        checkU32("F6: Erfolg mit Aufrufer-Override", (unsigned long)ok, 1);
        checkU32("F6: Q9K_ProcSRqMem mit Override-Groesse (64), NICHT M\\$Mem (256)",
                 g_srqmemLastRequested, 64UL);
    }

    /* Fall 7: Speicheranforderung schlaegt fehl -- Fehlercode wird
     * durchgereicht, Trap-Tabelle bleibt unveraendert. */
    {
        resetAll();
        setModuleField(Q9K_MH_EXEC, 0x40UL);
        setModuleField(Q9K_MH_MEM, 256UL);
        setModuleField(Q9K_MH_INIT, 0x50UL);
        g_modDirHdr = (unsigned long)g_fakeModule;
        g_srqmemReturn = 0;
        g_srqmemOutError = 0x00CFUL; /* E_MEMFUL, wie in test_q9kernel_sysmem.c */

        ok = Q9K_ProcTLink(3UL, 0UL, (unsigned long)(g_fakeGlobals + 0x1000),
                            &pastName, &modPtr, &execEntry, &initEntry, &staticPtr, &err);
        checkU32("F7: Speicheranforderung fehlgeschlagen -> Fehlschlag", (unsigned long)ok, 0);
        checkU32("F7: Fehlercode durchgereicht (E_MEMFUL)", err, 0x00CFUL);
        checkU32("F7: Trap-Tabelle Slot 3 bleibt frei",
                 readSlotField(slotAddrFor(3UL), Q9K_TRAPTBL_OFF_MODPTR), 0);
    }

    /* Fall 8: Bruecke Q9K_SysTLinkImpl -- Scratch-Zellen EIN, echter
     * Aufruf, Scratch-Zellen AUS (gleiches Muster wie test_q9kernel_
     * sysmem.c fuer Q9K_SysSRqMemImpl). */
    {
        resetAll();
        strcpy((char *)g_fakeGlobals + 0x1000, "csl");
        setModuleField(Q9K_MH_EXEC, 0x40UL);
        setModuleField(Q9K_MH_MEM, 0UL);
        setModuleField(Q9K_MH_INIT, 0x50UL);
        g_modDirHdr = (unsigned long)g_fakeModule;

        *(unsigned long *)Q9K_TLINK_SCRATCH_TRAPNUM = 13UL;
        *(unsigned long *)Q9K_TLINK_SCRATCH_MEMOVR = 0UL;
        *(unsigned long *)Q9K_TLINK_SCRATCH_NAMEPTR = (unsigned long)(g_fakeGlobals + 0x1000);

        Q9K_SysTLinkImpl();

        checkU32("F8: Bruecke meldet Erfolg", *(unsigned long *)Q9K_TLINK_SCRATCH_SUCCESS, 1);
        checkU32("F8: Bruecke liefert ModPtr", *(unsigned long *)Q9K_TLINK_SCRATCH_MODPTR,
                 (unsigned long)g_fakeModule);
        checkU32("F8: Bruecke liefert ExecEntry", *(unsigned long *)Q9K_TLINK_SCRATCH_EXECENTRY,
                 (unsigned long)g_fakeModule + 0x40UL);
        checkU32("F8: Bruecke liefert InitEntry", *(unsigned long *)Q9K_TLINK_SCRATCH_INITENTRY,
                 (unsigned long)g_fakeModule + 0x50UL);

        /* Zweiter Aufruf, DIESMAL fuer denselben Trap (13) -- Slot ist
         * jetzt belegt, Bruecke muss den Fehlerfall korrekt melden. */
        Q9K_SysTLinkImpl();
        checkU32("F8b: zweiter Aufruf auf belegtem Slot -> Fehlschlag",
                 *(unsigned long *)Q9K_TLINK_SCRATCH_SUCCESS, 0);
        checkU32("F8b: E$ModBsy ($D1)", *(unsigned long *)Q9K_TLINK_SCRATCH_ERROR, 0x00D1UL);
    }

    if (g_failures == 0) {
        printf("Alle Tests erfolgreich.\n");
        return 0;
    }
    printf("%d Test(s) fehlgeschlagen.\n", g_failures);
    return 1;
}
