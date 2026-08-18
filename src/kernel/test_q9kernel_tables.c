/*
 * test_q9kernel_tables.c -- Regressionstest fuer q9kernel_tables.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. Q9_D_SYSDIS/
 * USRDIS/MODDIR und die eigenen Pool-Erweiterungsfelder werden per
 * #define VOR dem #include auf einen echten Testpuffer umgebogen.
 * Q9K_AllocMem wird hier NICHT aus der echten Arena verlinkt (die ist
 * bereits separat getestet, s. test_q9kernel_arena.c) -- stattdessen ein
 * simpler Bump-Allocator als Fake-Implementierung, damit dieser Test nur
 * die Slicing-/Freiliste-Logik von q9kernel_tables.c selbst prueft.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_tables test_q9kernel_tables.c && \
 *       ./test_q9kernel_tables
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x2000];  /* deckt SYSDIS/USRDIS/MODDIR + Pool-Erweiterungsfelder ab */
static unsigned char g_fakePool[1 << 20];    /* 1 MB "Arena"-Ruecklage fuer den Fake-Allokator */
static unsigned long g_fakePoolNext;

/* Testfeld-Layout bewusst NICHT an den echten q9sysglob.h-Offsets
 * (0x3A4/0x3A8/... liegen dort nur 4 Byte auseinander) -- auf diesem
 * 64-Bit-Testhost ist Q9_u32 8 Byte breit, ein Q9K_SetU32-Schreibzugriff
 * wuerde direkt benachbarte Felder ueberschreiben (gleicher Fund wie
 * beim Arena-Test). Da diese Testdatei nur die Slicing-/Freiliste-Logik
 * von q9kernel_tables.c prueft (die echten Offsets sind bereits per
 * Disassemblierung verifiziert, nicht Gegenstand dieses Tests), einfach
 * grosszuegig auf 0x10-Schritten platziert. */
#define Q9_D_SYSDIS             ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9_D_USRDIS             ((unsigned long)(g_fakeGlobals + 0x010))
#define Q9_D_MODDIR             ((unsigned long)(g_fakeGlobals + 0x020))
#define Q9_D_MODDIR_END         ((unsigned long)(g_fakeGlobals + 0x030))
#define Q9K_PROCPOOL_BASE_ADDR  ((unsigned long)(g_fakeGlobals + 0x040))
#define Q9K_PROCPOOL_FREE_ADDR  ((unsigned long)(g_fakeGlobals + 0x050))
#define Q9K_PATHPOOL_BASE_ADDR  ((unsigned long)(g_fakeGlobals + 0x060))
#define Q9K_PATHPOOL_FREE_ADDR  ((unsigned long)(g_fakeGlobals + 0x070))

/* Fake-Allokator: einfacher Bump-Allocator, kein Freigeben noetig fuer
 * diesen Test (jeder Testfall alloziert einmal, komplett unabhaengig von
 * der echten Arena-Implementierung). */
unsigned long Q9K_AllocMem(unsigned long requestedSize)
{
    unsigned long addr;
    if (g_fakePoolNext + requestedSize > sizeof(g_fakePool))
        return 0;
    addr = (unsigned long)(g_fakePool + g_fakePoolNext);
    g_fakePoolNext += requestedSize;
    return addr;
}

#include "q9kernel_tables.c"

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

/* Baut ein minimales, synthetisches Init-Modul-Fragment: nur die drei
 * Felder, die Q9K_SetupTables tatsaechlich liest (M$Procs/M$Paths/
 * M$MDirSz), alles andere bleibt 0 (irrelevant fuer diesen Test). */
static void buildFakeInit(unsigned char *buf, unsigned short procs, unsigned short paths, unsigned short mdirSz)
{
    memset(buf, 0, 0x80);
    buf[Q9K_INIT_OFF_PROCS]     = (unsigned char)(procs >> 8);
    buf[Q9K_INIT_OFF_PROCS + 1] = (unsigned char)(procs & 0xFF);
    buf[Q9K_INIT_OFF_PATHS]     = (unsigned char)(paths >> 8);
    buf[Q9K_INIT_OFF_PATHS + 1] = (unsigned char)(paths & 0xFF);
    buf[Q9K_INIT_OFF_MDIRSZ]     = (unsigned char)(mdirSz >> 8);
    buf[Q9K_INIT_OFF_MDIRSZ + 1] = (unsigned char)(mdirSz & 0xFF);
}

int main(void)
{
    static unsigned char fakeInit[0x80];

    memset(g_fakeGlobals, 0xCC, sizeof(g_fakeGlobals));
    buildFakeInit(fakeInit, /*procs=*/4, /*paths=*/2, /*mdirSz=*/3);

    checkU32("Q9K_SetupTables() Rueckgabewert (0 = Erfolg)", Q9K_SetupTables(fakeInit), 0);

    /* Fall: SYSDIS/USRDIS liegen direkt hintereinander, 0x800 Byte auseinander */
    {
        Q9_u32 sysdis = *(Q9_u32 *)Q9_D_SYSDIS;
        Q9_u32 usrdis = *(Q9_u32 *)Q9_D_USRDIS;
        checkU32("USRDIS liegt exakt 0x800 Byte nach SYSDIS", usrdis - sysdis, Q9K_SYSDIS_SIZE);
    }

    /* Fall: Modulverzeichnis Start-/Ende-Zeiger passen zu mdirSz=3 (3*16=48 Byte) */
    {
        Q9_u32 moddirStart = *(Q9_u32 *)Q9_D_MODDIR;
        Q9_u32 moddirEnd   = *(Q9_u32 *)Q9_D_MODDIR_END;
        checkU32("Modulverzeichnis-Groesse == mdirSz*16", moddirEnd - moddirStart, 3 * Q9K_MODDIR_ENTRY_SIZE);
    }

    /* Fall: Proc-Pool hat 4 Slots -- Freiliste muss 4 verkettete, dann
     * terminierte Eintraege liefern */
    {
        Q9_u32 base = *(Q9_u32 *)Q9K_PROCPOOL_BASE_ADDR;
        Q9_u32 cur = *(Q9_u32 *)Q9K_PROCPOOL_FREE_ADDR;
        int count = 0;
        checkU32("Proc-Freiliste startet am Pool-Anfang", cur, base);
        while (cur != 0 && count < 10) {
            cur = *(Q9_u32 *)cur;
            count++;
        }
        checkU32("Proc-Freiliste hat genau 4 Eintraege", (Q9_u32)count, 4);
    }

    /* Fall: Path-Pool hat 2 Slots */
    {
        Q9_u32 cur = *(Q9_u32 *)Q9K_PATHPOOL_FREE_ADDR;
        int count = 0;
        while (cur != 0 && count < 10) {
            cur = *(Q9_u32 *)cur;
            count++;
        }
        checkU32("Path-Freiliste hat genau 2 Eintraege", (Q9_u32)count, 2);
    }

    /* Fall: Allokationsfehlschlag (Pool zu klein) wird sauber gemeldet,
     * nicht ignoriert -- eigener Fake-Allokator mit winzigem Rest-Pool */
    {
        unsigned char tinyInit[0x80];
        unsigned long savedNext = g_fakePoolNext;
        g_fakePoolNext = sizeof(g_fakePool) - 4; /* fast erschoepft */
        buildFakeInit(tinyInit, 1000, 1000, 1000); /* garantiert zu gross fuer den Rest */
        checkU32("Fehlschlag bei Arena-Erschoepfung wird gemeldet (1)", Q9K_SetupTables(tinyInit), 1);
        g_fakePoolNext = savedNext;
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
