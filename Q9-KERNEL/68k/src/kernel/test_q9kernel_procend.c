/*
 * test_q9kernel_procend.c -- Regressionstest fuer q9kernel_procend.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. Alle externen
 * Abhaengigkeiten (Q9K_SchedInsert/SchedFirstPick/WaitQRemove/
 * ModDirUnlinkByHeader) werden hier durch einfache, aufrufzaehlende
 * Stubs ersetzt -- dieser Test prueft NUR die Pool-Scan-/Zombie-/
 * Reaktivierungslogik von q9kernel_procend.c selbst.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_procend test_q9kernel_procend.c && \
 *       ./test_q9kernel_procend
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x2000];

/* Grosszuegige, getrennte Testadressen -- gleiche Begruendung wie in
 * den anderen Kernel-Tests (Q9_u32 = 8 Byte auf diesem Host). */
#define Q9K_PROCPOOL_BASE_ADDR  ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9K_PROCPOOL_FREE_ADDR  ((unsigned long)(g_fakeGlobals + 0x010))
#define Q9K_PROCPOOL_COUNT_ADDR ((unsigned long)(g_fakeGlobals + 0x020))
#define Q9_D_PROC               ((unsigned long)(g_fakeGlobals + 0x030))

/* Real nur wenige Byte auseinander (s. q9kernel_firstproc.c Kopf-
 * kommentar) -- hier grosszuegig auf 8-Byte-Schritte gelegt, gleiches
 * Muster wie ueberall. Q9K_TEST_DESC_SIZE muss gross genug fuer
 * SAVEDSP_OFF+Rahmen sein. */
#define Q9K_PROCDESC_STATE_OFF      0x00UL   /* bleibt real -- nur 1 Byte, keine Ueberlappungsgefahr */
#define Q9K_PROCDESC_PARENT_OFF     0x08UL
#define Q9K_PROCDESC_MODHDR_OFF     0x10UL
#define Q9K_PROCDESC_EXITSTATUS_OFF 0x18UL
#define Q9K_PROCDESC_SAVEDSP_OFF    0x20UL
#define Q9K_PROCDESC_ALLOCBASE_OFF  0x28UL
#define Q9K_PROCDESC_ALLOCSIZE_OFF  0x30UL

#define Q9K_TEST_DESC_SIZE 64UL
#define Q9K_TEST_POOL_COUNT 4UL
/* MUSS mit Q9K_TEST_DESC_SIZE uebereinstimmen -- q9kernel_procend.c
 * benutzt Q9K_PROCDESC_SIZE selbst als Pool-Scan-Schrittweite (Default
 * 128, s. dortigen #ifndef); ohne diesen Override wuerde der Scan mit
 * falscher Schrittweite ueber den (nur 64 Byte/Slot grossen) Testpool
 * hinauslaufen (real per AddressSanitizer gefunden: global-buffer-
 * overflow in Q9K_GetU32 <- Q9K_ProcWaitTryReap). */
#define Q9K_PROCDESC_SIZE Q9K_TEST_DESC_SIZE

/* Aufrufzaehlende Stubs fuer alle externen Abhaengigkeiten. */
static int g_schedInsertCalls = 0;
static unsigned long g_schedInsertLastDesc = 0;
void Q9K_SchedInsert(unsigned long desc)
{
    g_schedInsertCalls++;
    g_schedInsertLastDesc = desc;
}

static unsigned long g_schedFirstPickReturn = 0xDEADBEEFUL;
unsigned long Q9K_SchedFirstPick(void)
{
    return g_schedFirstPickReturn;
}

static int g_waitQRemoveCalls = 0;
static unsigned long g_waitQRemoveLastDesc = 0;
void Q9K_WaitQRemove(unsigned long desc)
{
    g_waitQRemoveCalls++;
    g_waitQRemoveLastDesc = desc;
}

static int g_modDirUnlinkCalls = 0;
static unsigned long g_modDirUnlinkLastHdr = 0;
unsigned long Q9K_ModDirUnlinkByHeader(unsigned long hdrAddr)
{
    g_modDirUnlinkCalls++;
    g_modDirUnlinkLastHdr = hdrAddr;
    return 0;
}

static int g_freeMemCalls = 0;
static unsigned long g_freeMemLastAddr = 0;
static unsigned long g_freeMemLastSize = 0;
void Q9K_FreeMem(unsigned long addr, unsigned long size)
{
    g_freeMemCalls++;
    g_freeMemLastAddr = addr;
    g_freeMemLastSize = size;
}

/* Q9K_ProcMemReleaseAll lebt in q9kernel_sysmem.c (per F$SRqMem
 * getrackte Zusatzbloecke). Hier aufrufzaehlender Stub -- dieser Test
 * prueft nur q9kernel_procend.c selbst. */
static int g_memReleaseAllCalls = 0;
static unsigned long g_memReleaseAllLastOwner = 0;
void Q9K_ProcMemReleaseAll(unsigned long owner)
{
    g_memReleaseAllCalls++;
    g_memReleaseAllLastOwner = owner;
}

#include "q9kernel_procend.c"

static int failures = 0;

static void checkU32(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-65s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-65s = %lu (erwartet %lu)\n", label, (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

/* Getreu der byteweisen Big-Endian-Schreibkonvention von Q9K_SetFrameReg
 * -- Lesen hier ebenso byteweise, gleiches Muster wie test_q9kernel_firstproc.c. */
static Q9_u32 getBE32(Q9_u32 addr)
{
    const Q9_u8 *p = (const Q9_u8 *)addr;
    return ((Q9_u32)p[0] << 24) | ((Q9_u32)p[1] << 16) | ((Q9_u32)p[2] << 8) | (Q9_u32)p[3];
}

static void resetPool(unsigned char *pool, Q9_u32 poolBase)
{
    memset(pool, 0, Q9K_TEST_POOL_COUNT * Q9K_TEST_DESC_SIZE);
    Q9K_SetU32(Q9K_PROCPOOL_BASE_ADDR, poolBase);
    Q9K_SetU32(Q9K_PROCPOOL_FREE_ADDR, 0);   /* in diesem Test nicht benutzt/geprueft */
    Q9K_SetU32(Q9K_PROCPOOL_COUNT_ADDR, Q9K_TEST_POOL_COUNT);
}

int main(void)
{
    static unsigned char pool[Q9K_TEST_POOL_COUNT * Q9K_TEST_DESC_SIZE];
    Q9_u32 poolBase = (Q9_u32)(unsigned long)pool;
    Q9_u32 d0 = poolBase + 0 * Q9K_TEST_DESC_SIZE;
    Q9_u32 d1 = poolBase + 1 * Q9K_TEST_DESC_SIZE;
    Q9_u32 d2 = poolBase + 2 * Q9K_TEST_DESC_SIZE;
    Q9_u32 d3 = poolBase + 3 * Q9K_TEST_DESC_SIZE;

    memset(g_fakeGlobals, 0, sizeof(g_fakeGlobals));

    /* Fall 1: Q9K_ProcWaitTryReap -- Aufrufer (d0) hat UEBERHAUPT keine
     * Kinder -- E_NOCHLD. */
    {
        Q9_u32 childPid = 0xCC;
        Q9_u16 exitStatus = 0xCC;
        Q9_u16 error = 0;
        int reaped;

        resetPool(pool, poolBase);
        /* d1/d2/d3 bleiben ohne Elternbezug (ParentDesc==0) -- s. resetPool memset(0) */

        reaped = Q9K_ProcWaitTryReap(d0, &childPid, &exitStatus, &error);
        checkU32("F1: Q9K_ProcWaitTryReap ohne jedes Kind -> 0", (Q9_u32)reaped, 0);
        checkU32("F1: *outError == E_NOCHLD ($E2)", error, Q9K_E_NOCHLD);
    }

    /* Fall 2: Aufrufer hat ein lebendes Kind (State='a'), aber keins
     * Zombie -- muss blockieren (Rueckgabe 0, *outError bleibt 0). */
    {
        Q9_u32 childPid = 0;
        Q9_u16 exitStatus = 0;
        Q9_u16 error = 0;
        int reaped;

        resetPool(pool, poolBase);
        Q9K_SetU32(d1 + Q9K_PROCDESC_PARENT_OFF, d0);
        Q9K_SetU8(d1 + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);

        reaped = Q9K_ProcWaitTryReap(d0, &childPid, &exitStatus, &error);
        checkU32("F2: lebendes Kind, kein Zombie -> 0 (blockieren)", (Q9_u32)reaped, 0);
        checkU32("F2: *outError bleibt 0 (kein Fehler, nur blockieren)", error, 0);
    }

    /* Fall 3: Aufrufer hat ein Zombie-Kind (d2) -- sofortiges Abholen. */
    {
        Q9_u32 childPid = 0;
        Q9_u16 exitStatus = 0;
        Q9_u16 error = 0;
        int reaped;

        resetPool(pool, poolBase);
        Q9K_SetU32(d1 + Q9K_PROCDESC_PARENT_OFF, d0);
        Q9K_SetU8(d1 + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);
        Q9K_SetU32(d2 + Q9K_PROCDESC_PARENT_OFF, d0);
        Q9K_SetU8(d2 + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ZOMBIE);
        Q9K_SetU16(d2 + Q9K_PROCDESC_EXITSTATUS_OFF, 42);
        Q9K_SetU32(Q9K_PROCPOOL_FREE_ADDR, 0x99999999UL);   /* Marker -- muss nach dem Freigeben auf d2 zeigen */

        reaped = Q9K_ProcWaitTryReap(d0, &childPid, &exitStatus, &error);
        checkU32("F3: Zombie-Kind gefunden -> 1", (Q9_u32)reaped, 1);
        checkU32("F3: *outChildPid == 3 (d2 = dritter Slot, 1-basiert)", childPid, 3);
        checkU32("F3: *outExitStatus == 42", exitStatus, 42);
        checkU32("F3: d2 zurueck in die Freiliste gehaengt (Q9K_PROCPOOL_FREE_ADDR == d2)",
                 Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR), d2);
        checkU32("F3: d2.ParentDesc auf 0 zurueckgesetzt (Freigabe-Invariante)",
                 Q9K_GetU32(d2 + Q9K_PROCDESC_PARENT_OFF), 0);
    }

    /* Fall 4: Q9K_ProcExit -- Elternprozess (d1) wartet NICHT (State='a')
     * -- Aufrufer (d0) wird Zombie, kein Reaktivieren/Freigeben. */
    {
        Q9_u32 next;

        resetPool(pool, poolBase);
        g_modDirUnlinkCalls = 0;
        g_schedInsertCalls = 0;
        g_waitQRemoveCalls = 0;
        g_schedFirstPickReturn = d3;   /* naechster Prozess, frei erfunden fuer diesen Test */

        Q9K_SetU32(d0 + Q9K_PROCDESC_PARENT_OFF, d1);
        Q9K_SetU32(d0 + Q9K_PROCDESC_MODHDR_OFF, 0x12345678UL);
        Q9K_SetU8(d1 + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);   /* Elternprozess laeuft noch, wartet nicht */

        next = Q9K_ProcExit(d0, 7);
        checkU32("F4: Rueckgabe == Q9K_SchedFirstPick()-Ergebnis", next, d3);
        checkU32("F4: Q9K_ModDirUnlinkByHeader wurde EINMAL aufgerufen", (Q9_u32)g_modDirUnlinkCalls, 1);
        checkU32("F4: ... mit dem echten ModuleHdr-Wert", g_modDirUnlinkLastHdr, 0x12345678UL);
        checkU32("F4: d0.ModuleHdr auf 0 zurueckgesetzt", Q9K_GetU32(d0 + Q9K_PROCDESC_MODHDR_OFF), 0);
        checkU32("F4: d0.State == 'z' (Zombie)", (Q9_u32)Q9K_GetU8(d0 + Q9K_PROCDESC_STATE_OFF), (Q9_u32)'z');
        checkU32("F4: d0.ExitStatus == 7", Q9K_GetU16(d0 + Q9K_PROCDESC_EXITSTATUS_OFF), 7);
        checkU32("F4: Elternprozess NICHT angefasst (kein SchedInsert)", (Q9_u32)g_schedInsertCalls, 0);
        checkU32("F4: kein WaitQRemove (Elternprozess wartet nicht)", (Q9_u32)g_waitQRemoveCalls, 0);
    }

    /* Fall 5: Q9K_ProcExit -- Elternprozess (d1) WARTET (State='w') --
     * sofortiges Reaktivieren, Kind-Deskriptor wird SOFORT freigegeben
     * (kein Zombie-Zwischenzustand). */
    {
        Q9_u32 next;
        Q9_u32 parentSP = poolBase + 1000UL;   /* eigener, ausserhalb des Pools liegender Fake-Rahmen */
        static unsigned char parentFrame[64];

        resetPool(pool, poolBase);
        memset(parentFrame, 0, sizeof(parentFrame));
        parentSP = (Q9_u32)(unsigned long)parentFrame;

        g_modDirUnlinkCalls = 0;
        g_schedInsertCalls = 0;
        g_schedInsertLastDesc = 0;
        g_waitQRemoveCalls = 0;
        g_waitQRemoveLastDesc = 0;
        g_schedFirstPickReturn = d3;

        Q9K_SetU32(d1 + Q9K_PROCDESC_PARENT_OFF, d2);              /* d1 = Kind von d2 (Elternprozess) */
        Q9K_SetU8(d2 + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_WAITING);  /* d2 blockiert in F$Wait */
        Q9K_SetU32(d2 + Q9K_PROCDESC_SAVEDSP_OFF, parentSP);

        next = Q9K_ProcExit(d1, 99);
        checkU32("F5: Rueckgabe == Q9K_SchedFirstPick()-Ergebnis", next, d3);
        checkU32("F5: Q9K_WaitQRemove wurde mit dem Elternprozess (d2) aufgerufen",
                 (Q9_u32)g_waitQRemoveCalls, 1);
        checkU32("F5: ... genau mit d2", g_waitQRemoveLastDesc, d2);
        checkU32("F5: Elternprozess-Rahmen D0 == Kind-PID (2, d1 = zweiter Slot)",
                 getBE32(parentSP + 0 * 4), 2);
        checkU32("F5: Elternprozess-Rahmen D1 == Exit-Status (99)",
                 getBE32(parentSP + 1 * 4), 99);
        checkU32("F5: d2.State zurueck auf 'a' (aktiv)",
                 (Q9_u32)Q9K_GetU8(d2 + Q9K_PROCDESC_STATE_OFF), (Q9_u32)'a');
        checkU32("F5: Q9K_SchedInsert wurde mit dem Elternprozess (d2) aufgerufen",
                 (Q9_u32)g_schedInsertCalls, 1);
        checkU32("F5: ... genau mit d2", g_schedInsertLastDesc, d2);
        checkU32("F5: Kind-Deskriptor (d1) SOFORT freigegeben (ParentDesc==0)",
                 Q9K_GetU32(d1 + Q9K_PROCDESC_PARENT_OFF), 0);
        checkU32("F5: d1 zurueck in die Freiliste gehaengt (Q9K_PROCPOOL_FREE_ADDR == d1)",
                 Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR), d1);
        /* d1.State (Offset 0x00) wird von Q9K_ProcPoolFree NICHT gezielt
         * gesetzt -- es kollidiert absichtlich mit dem Freilisten-
         * "naechster frei"-Zeiger (s. Kopfkommentar q9kernel_firstproc.c),
         * hier zufaellig 0 (Q9K_PROCPOOL_FREE_ADDR war vor dem Freigeben
         * selbst 0, s. resetPool). Auf diese konkrete Byte-Folge ist keine
         * echte Kernel-Logik angewiesen -- Zombie-Scan/F$Wait beruhen
         * ausschliesslich auf ParentDesc, s. F3 oben. */
        checkU32("F5: d1.State nach Freigabe == 0 (Freilisten-Kollision, s. Kommentar)",
                 (Q9_u32)Q9K_GetU8(d1 + Q9K_PROCDESC_STATE_OFF), 0);
    }

    /* Fall 6: Q9K_ProcExitFreeDeadChildren (ueber Q9K_ProcExit) -- der
     * beendende Prozess (d0) hat SELBST ein Zombie-Kind (d3) haengen, das
     * niemand mehr je abholen kann -- muss beim Beenden von d0 mit
     * freigegeben werden ("Free process descriptor of any dead child
     * processes", 68k_tech.pdf S. 424). */
    {
        resetPool(pool, poolBase);
        g_schedFirstPickReturn = 0;

        Q9K_SetU32(d0 + Q9K_PROCDESC_PARENT_OFF, 0);   /* d0 selbst hat keinen Elternprozess */
        Q9K_SetU32(d3 + Q9K_PROCDESC_PARENT_OFF, d0);  /* d3 ist Zombie-Kind von d0 */
        Q9K_SetU8(d3 + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ZOMBIE);

        Q9K_ProcExit(d0, 0);
        checkU32("F6: eigenes Zombie-Kind (d3) beim Beenden mit freigegeben (ParentDesc==0)",
                 Q9K_GetU32(d3 + Q9K_PROCDESC_PARENT_OFF), 0);
        checkU32("F6: d3 landet in der Freiliste (Q9K_PROCPOOL_FREE_ADDR == d3)",
                 Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR), d3);
    }

    /* Fall 7: Der eigene Prozessblock wird bereits beim F$Exit
     * freigegeben, bleibt aber im Zombie-Zustand deskriptorseitig bis
     * F$Wait. Das nachfolgende Reap darf NICHT doppelt freigeben. */
    {
        Q9_u32 childPid = 0;
        Q9_u16 exitStatus = 0;
        Q9_u16 error = 0;

        resetPool(pool, poolBase);
        g_freeMemCalls = 0;
        Q9K_SetU32(d1 + Q9K_PROCDESC_PARENT_OFF, d0);
        Q9K_SetU32(d1 + Q9K_PROCDESC_ALLOCBASE_OFF, 0xA000UL);
        Q9K_SetU32(d1 + Q9K_PROCDESC_ALLOCSIZE_OFF, 0x400UL);
        Q9K_SetU8(d0 + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);

        Q9K_ProcExit(d1, 12);
        checkU32("F7: F$Exit gibt den Prozessblock sofort einmal frei", (Q9_u32)g_freeMemCalls, 1);
        checkU32("F7: Freigabe mit korrekter Basis", g_freeMemLastAddr, 0xA000UL);
        checkU32("F7: Freigabe mit korrekter Groesse", g_freeMemLastSize, 0x400UL);
        checkU32("F7: AllocBase wird nach Freigabe genullt",
                 Q9K_GetU32(d1 + Q9K_PROCDESC_ALLOCBASE_OFF), 0);
        checkU32("F7: Zombie-Reap findet das Kind", (Q9_u32)Q9K_ProcWaitTryReap(d0, &childPid, &exitStatus, &error), 1);
        checkU32("F7: Zombie-Reap gibt denselben Block nicht doppelt frei", (Q9_u32)g_freeMemCalls, 1);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
