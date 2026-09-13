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

/* NACHTRAG 2026-09-13 (Fortsetzung 58, Q9K_PatchCslFreelistBug): in
 * den bestehenden Testfaellen F1-F9 nie wirklich erreicht (M$Size
 * steht in keinem der dortigen Fake-Module auf einen Wert >=
 * Q9K_CSL_PATCH_OFF, s. dortige Groessenpruefung) -- eigenstaendiger
 * Testfall F10 unten nutzt den Stub gezielt. */
static int g_allocMemCalls = 0;
static unsigned char g_fakeStubBuf[64];
unsigned long Q9K_AllocMem(unsigned long requestedSize)
{
    (void)requestedSize;
    g_allocMemCalls++;
    return (unsigned long)g_fakeStubBuf;
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

/* Wie setModuleField, aber fuer einen BELIEBIGEN Puffer (nicht nur
 * g_fakeModule) -- gebraucht in Fall 10 (Q9K_PatchCslFreelistBug), der
 * einen eigenen, groesseren Fake-Modulpuffer braucht. */
static void setModuleFieldAt(unsigned long hdr, unsigned long fieldOff, unsigned long value)
{
    unsigned char *p = (unsigned char *)(hdr + fieldOff);
    p[0] = (unsigned char)(value >> 24);
    p[1] = (unsigned char)(value >> 16);
    p[2] = (unsigned char)(value >> 8);
    p[3] = (unsigned char)value;
}

/* NACHTRAG 2026-09-11 (Fortsetzung 49) -- fuer den M$IRefs-Testaufbau
 * (F9, s. u.): dort sind MS-Wort/Anzahl/Versatz je 16 Bit breit. */
static void putBE16(unsigned long addr, unsigned value)
{
    unsigned char *p = (unsigned char *)addr;
    p[0] = (unsigned char)(value >> 8);
    p[1] = (unsigned char)value;
}

/* Grossgeschriebenes Big-Endian-Lesen -- Gegenstueck zu setModuleField/
 * putBE16, gebraucht um die von Q9K_ApplyInitializedData relozierten
 * Werte im Zielspeicher zu pruefen. */
static unsigned long getBE32(unsigned long addr)
{
    const unsigned char *p = (const unsigned char *)addr;
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) | (unsigned long)p[3];
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

    /* Fall 9 (NACHTRAG 2026-09-11, Fortsetzung 49): M$IData/M$IRefs des
     * Trap-Moduls selbst -- gleicher Tabellenaufbau wie F5 in
     * test_q9kernel_firstproc.c (dort ausfuehrlicher Kommentar zum
     * Format), hier gegen den ECHTEN staticPtr aus Q9K_ProcSRqMem
     * geprueft (NICHT wie F5/F6 oben ein reiner Wertevergleich mit einer
     * kleinen erfundenen Zahl -- diesmal wird tatsaechlich hineingeschrieben,
     * braucht deshalb einen ECHTEN, dereferenzierbaren Host-Zeiger als
     * g_srqmemOutAddr). Layout in g_fakeModule (256 Byte, reicht):
     *   M$IData @ $60: Zieloffset $10, Anzahl 8, Nutzlast [5, 3]
     *   M$IRefs @ $70: Gruppe 1 (Kode) Versatz $14, Gruppe 2 (Daten)
     *                  Versatz $10 -- je mit eigenem Terminator. */
    {
        unsigned long staticMem = (unsigned long)(g_fakeGlobals + 0x2800);   /* echter, freier Bereich */
        unsigned long relocatedData, relocatedCode;

        resetAll();
        setModuleField(Q9K_MH_EXEC, 0x40UL);
        setModuleField(Q9K_MH_MEM, 0x40UL);   /* 64 -- reicht fuer Datenoffset $10/$14 */
        setModuleField(Q9K_MH_INIT, 0x50UL);
        setModuleField(Q9K_MH_IDATA, 0x60UL);
        setModuleField(Q9K_MH_IREFS, 0x70UL);
        setModuleField(0x60UL, 0x10UL);    /* IData-Eintrag: Zieloffset $10 */
        setModuleField(0x64UL, 8UL);       /* IData-Eintrag: Anzahl Bytes = 8 */
        setModuleField(0x68UL, 5UL);       /* Nutzlast[0] -- spaeter Datenzeiger */
        setModuleField(0x6CUL, 3UL);       /* Nutzlast[4] -- spaeter Kodezeiger */
        putBE16((unsigned long)g_fakeModule + 0x70UL, 0);      /* Gruppe 1 (Kode): MS = 0 */
        putBE16((unsigned long)g_fakeModule + 0x72UL, 1);      /* Gruppe 1: Anzahl = 1 */
        putBE16((unsigned long)g_fakeModule + 0x74UL, 0x14);   /* Gruppe 1: Versatz $14 */
        putBE16((unsigned long)g_fakeModule + 0x76UL, 0);      /* Gruppe 1: Terminator MS = 0 */
        putBE16((unsigned long)g_fakeModule + 0x78UL, 0);      /* Gruppe 1: Terminator Anzahl = 0 */
        putBE16((unsigned long)g_fakeModule + 0x7AUL, 0);      /* Gruppe 2 (Daten): MS = 0 */
        putBE16((unsigned long)g_fakeModule + 0x7CUL, 1);      /* Gruppe 2: Anzahl = 1 */
        putBE16((unsigned long)g_fakeModule + 0x7EUL, 0x10);   /* Gruppe 2: Versatz $10 */
        putBE16((unsigned long)g_fakeModule + 0x80UL, 0);      /* Gruppe 2: Terminator MS = 0 */
        putBE16((unsigned long)g_fakeModule + 0x82UL, 0);      /* Gruppe 2: Terminator Anzahl = 0 */
        g_modDirHdr = (unsigned long)g_fakeModule;
        g_srqmemReturn = 1;
        g_srqmemOutAddr = staticMem;
        g_srqmemOutSize = 0x40UL;

        ok = Q9K_ProcTLink(5UL, 0UL, (unsigned long)(g_fakeGlobals + 0x1000),
                            &pastName, &modPtr, &execEntry, &initEntry, &staticPtr, &err);
        checkU32("F9: Erfolg", (unsigned long)ok, 1);
        checkU32("F9: staticPtr == Q9K_ProcSRqMem-Ergebnis", staticPtr, staticMem);

        relocatedData = getBE32(staticMem + 0x10);
        relocatedCode = getBE32(staticMem + 0x14);
        /* Erwartungswerte bewusst auf 32 Bit gekappt (unsigned int) --
         * Q9K_ApplyInitializedData schreibt die relozierten Werte
         * byteweise (NUR 4 Byte), auf DIESEM 64-Bit-Testhost also nicht
         * rundreisefaehig mit dem vollen Host-Zeiger, s. gleiche
         * Begruendung in test_q9kernel_firstproc.c (F5). */
        checkU32("F9: M\\$IData kopiert UND per M\\$IRefs (Datenzeiger-Gruppe) reloziert (5+staticPtr)",
                 relocatedData, (unsigned long)(unsigned int)(5UL + staticMem));
        checkU32("F9: M\\$IData kopiert UND per M\\$IRefs (Kodezeiger-Gruppe) reloziert (3+hdrAddr)",
                 relocatedCode, (unsigned long)(unsigned int)(3UL + (unsigned long)g_fakeModule));
    }

    /* Fall 10 (NACHTRAG 2026-09-13, Fortsetzung 58): Q9K_PatchCslFreelistBug
     * direkt getestet (statische Funktion, per #include sichtbar) --
     * eigenstaendiger, ausreichend grosser Fake-Modulpuffer (NICHT
     * g_fakeModule, das ist fuer diesen Versatz zu klein). */
    {
        static unsigned char bigMod[0x5700];
        unsigned long hdr = (unsigned long)bigMod;
        unsigned long patchAddr = hdr + Q9K_CSL_PATCH_OFF;
        unsigned long stubAddr;
        unsigned long expDisp;

        /* F10a: falsches Bytemuster an der Patchstelle -- bleibt unangetastet. */
        memset(bigMod, 0, sizeof(bigMod));
        setModuleFieldAt(hdr, Q9K_MH_SIZE, (unsigned long)sizeof(bigMod));
        g_allocMemCalls = 0;
        Q9K_PatchCslFreelistBug(hdr);
        checkU32("F10a: falsches Bytemuster -- Q9K_AllocMem NICHT aufgerufen",
                 (unsigned long)g_allocMemCalls, 0UL);
        checkU32("F10a: Patchstelle unveraendert (0)", Q9K_GetU8(patchAddr), 0UL);

        /* F10b: M$Size zu klein (Patch-Versatz liegt ausserhalb) -- trotz
         * korrektem Bytemuster bleibt es unangetastet. */
        memset(bigMod, 0, sizeof(bigMod));
        setModuleFieldAt(hdr, Q9K_MH_SIZE, Q9K_CSL_PATCH_OFF);   /* zu klein */
        Q9K_SetU8(patchAddr + 0, 0x62); Q9K_SetU8(patchAddr + 1, 0x00);
        Q9K_SetU8(patchAddr + 2, 0xfe); Q9K_SetU8(patchAddr + 3, 0xfc);
        g_allocMemCalls = 0;
        Q9K_PatchCslFreelistBug(hdr);
        checkU32("F10b: M\\$Size zu klein -- Q9K_AllocMem NICHT aufgerufen",
                 (unsigned long)g_allocMemCalls, 0UL);
        checkU32("F10b: Patchstelle unveraendert (Originalmuster)", Q9K_GetU8(patchAddr), 0x62UL);

        /* F10c: passendes Bytemuster + ausreichende Groesse -- patcht. */
        memset(bigMod, 0, sizeof(bigMod));
        setModuleFieldAt(hdr, Q9K_MH_SIZE, (unsigned long)sizeof(bigMod));
        Q9K_SetU8(patchAddr + 0, 0x62); Q9K_SetU8(patchAddr + 1, 0x00);
        Q9K_SetU8(patchAddr + 2, 0xfe); Q9K_SetU8(patchAddr + 3, 0xfc);
        g_allocMemCalls = 0;
        Q9K_PatchCslFreelistBug(hdr);
        checkU32("F10c: Q9K_AllocMem genau einmal aufgerufen",
                 (unsigned long)g_allocMemCalls, 1UL);
        checkU32("F10c: Patchstelle jetzt bsr.w (0x61)", Q9K_GetU8(patchAddr), 0x61UL);
        checkU32("F10c: Patchstelle Byte 2 (0x00)", Q9K_GetU8(patchAddr + 1), 0x00UL);

        stubAddr = (unsigned long)g_fakeStubBuf;
        expDisp = (unsigned long)(unsigned short)(stubAddr - (patchAddr + 2UL));
        checkU32("F10c: bsr.w-Distanz zeigt auf den Stub",
                 (Q9K_GetU8(patchAddr + 2) << 8) | Q9K_GetU8(patchAddr + 3), expDisp);

        checkU32("F10c: Stub Byte 0-1 = tst.l a4 (4A8C)",
                 (Q9K_GetU8(stubAddr) << 8) | Q9K_GetU8(stubAddr + 1), 0x4A8CUL);
        checkU32("F10c: Stub Byte 6-9 = cmp.l $4(a4),d3 (B6AC0004)",
                 ((unsigned long)Q9K_GetU8(stubAddr + 6) << 24) |
                 ((unsigned long)Q9K_GetU8(stubAddr + 7) << 16) |
                 ((unsigned long)Q9K_GetU8(stubAddr + 8) << 8) |
                 (unsigned long)Q9K_GetU8(stubAddr + 9), 0xB6AC0004UL);
        checkU32("F10c: Stub Byte 14-15 = rts (4E75)",
                 (Q9K_GetU8(stubAddr + 14) << 8) | Q9K_GetU8(stubAddr + 15), 0x4E75UL);

        expDisp = (unsigned long)(unsigned short)((hdr + Q9K_CSL_TOOSMALL_OFF) - (stubAddr + 0x14UL));
        checkU32("F10c: bra.w im Stub zeigt auf 'zu klein' (448da relativ)",
                 (Q9K_GetU8(stubAddr + 0x14) << 8) | Q9K_GetU8(stubAddr + 0x15), expDisp);
        expDisp = (unsigned long)(unsigned short)((hdr + Q9K_CSL_GROW_OFF) - (stubAddr + 0x1AUL));
        checkU32("F10c: bra.w im Stub zeigt auf 'mehr Speicher' (448e2 relativ)",
                 (Q9K_GetU8(stubAddr + 0x1A) << 8) | Q9K_GetU8(stubAddr + 0x1B), expDisp);
    }

    if (g_failures == 0) {
        printf("Alle Tests erfolgreich.\n");
        return 0;
    }
    printf("%d Test(s) fehlgeschlagen.\n", g_failures);
    return 1;
}
