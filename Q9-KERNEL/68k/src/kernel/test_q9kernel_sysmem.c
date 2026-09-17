/*
 * test_q9kernel_sysmem.c -- Regressionstest fuer q9kernel_sysmem.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests. Die drei
 * externen Arena-Abhaengigkeiten (Q9K_AllocMem/Q9K_FreeMem/
 * Q9K_AllocLargest) werden hier durch einfache, aufrufzaehlende Stubs
 * ersetzt -- dieser Test prueft NUR die Verzweigungslogik/Rundung von
 * q9kernel_sysmem.c selbst (Q9K_AllocMem/FreeMem/AllocLargest sind
 * bereits separat in test_q9kernel_arena.c verifiziert).
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_sysmem test_q9kernel_sysmem.c && \
 *       ./test_q9kernel_sysmem
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_fakeGlobals[0x2000];

#define Q9K_SRQMEM_SCRATCH_SIZEIN  ((unsigned long)(g_fakeGlobals + 0x000))
#define Q9K_SRQMEM_SCRATCH_ADDR    ((unsigned long)(g_fakeGlobals + 0x010))
#define Q9K_SRQMEM_SCRATCH_SIZEOUT ((unsigned long)(g_fakeGlobals + 0x020))
#define Q9K_SRQMEM_SCRATCH_ERROR   ((unsigned long)(g_fakeGlobals + 0x030))
#define Q9K_SRQMEM_SCRATCH_SUCCESS ((unsigned long)(g_fakeGlobals + 0x040))
#define Q9K_SRTMEM_SCRATCH_ADDRIN  ((unsigned long)(g_fakeGlobals + 0x050))
#define Q9K_SRTMEM_SCRATCH_SIZEIN  ((unsigned long)(g_fakeGlobals + 0x060))
/* Eigentuemertabelle (32 Slots a 12 Byte + Magic = 388 Byte) -- ohne
 * diese Umlenkung greift der Test auf die echte Kerneladresse $1710 zu. */
#define Q9K_MEMOWNER_BASE          ((unsigned long)(g_fakeGlobals + 0x100))
/* Systemglobale, die q9kernel_sysmem.c fuer Eigentuemerzuordnung und
 * Speicherspur liest -- ebenfalls in den Fake-Speicher umlenken. */
#define Q9_D_PROC                  ((unsigned long)(g_fakeGlobals + 0x300))
#define Q9_D_FREEMEM               ((unsigned long)(g_fakeGlobals + 0x310))
#define Q9K_TRANS_SCRATCH_SIZE     ((unsigned long)(g_fakeGlobals + 0x400))
#define Q9K_TRANS_SCRATCH_MODE     ((unsigned long)(g_fakeGlobals + 0x420))
#define Q9K_TRANS_SCRATCH_ADDR     ((unsigned long)(g_fakeGlobals + 0x440))

/* Aufrufzaehlende Stubs. Q9_u32 ist erst NACH dem #include unten
 * verfuegbar (in q9kernel_sysmem.c definiert) -- hier bewusst
 * "unsigned long" (identischer Typ), gleiches Muster wie in den anderen
 * Kernel-Tests. */
static unsigned long g_allocMemReturn = 0;
static unsigned long g_allocMemLastRequested = 0;
static int g_allocMemCalls = 0;
unsigned long Q9K_AllocMem(unsigned long requestedSize)
{
    g_allocMemCalls++;
    g_allocMemLastRequested = requestedSize;
    return g_allocMemReturn;
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

static unsigned long g_allocLargestReturn = 0;
static unsigned long g_allocLargestOutSize = 0;
static int g_allocLargestCalls = 0;
unsigned long Q9K_AllocLargest(unsigned long *outSize)
{
    g_allocLargestCalls++;
    *outSize = g_allocLargestOutSize;
    return g_allocLargestReturn;
}

/* Speicherspur-Stubs (q9kernel_debug.c): reine Diagnose, fuer die hier
 * geprueften Verzweigungen ohne Bedeutung. */
void Q9K_MemTraceBeginCapture(void) {}
void Q9K_MemTraceEndCapture(void) {}
void Q9K_MemTraceEmit(unsigned long operation,
                      unsigned long requested,
                      unsigned long address,
                      unsigned long size,
                      unsigned long error,
                      unsigned long freeHead)
{
    (void)operation; (void)requested; (void)address;
    (void)size; (void)error; (void)freeHead;
}

#include "q9kernel_sysmem.c"

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

static void resetStubs(void)
{
    g_allocMemReturn = 0;
    g_allocMemLastRequested = 0;
    g_allocMemCalls = 0;
    g_freeMemCalls = 0;
    g_freeMemLastAddr = 0;
    g_freeMemLastSize = 0;
    g_allocLargestReturn = 0;
    g_allocLargestOutSize = 0;
    g_allocLargestCalls = 0;
}

int main(void)
{
    Q9_u32 outAddr, outSize;
    Q9_u16 outError;
    int reaped;

    memset(g_fakeGlobals, 0xCC, sizeof(g_fakeGlobals));

    /* Fall 1: normale Anfrage, Erfolg -- Groesse muss auf 16 Byte
     * aufgerundet werden (reale Konvention), Q9K_AllocMem wird mit der
     * URSPRUENGLICHEN (nicht aufgerundeten) Groesse aufgerufen (macht
     * die Rundung selbst intern, s. q9kernel_arena.c). */
    {
        resetStubs();
        g_allocMemReturn = 0x2000;

        reaped = Q9K_ProcSRqMem(17, &outAddr, &outSize, &outError);
        checkU32("F1: Erfolg (Rueckgabe 1)", (Q9_u32)reaped, 1);
        checkU32("F1: *outAddr == Q9K_AllocMem()-Ergebnis", outAddr, 0x2000);
        checkU32("F1: *outSize == 32 (17 auf 16 Byte aufgerundet)", outSize, 32);
        checkU32("F1: Q9K_AllocMem wurde mit der ROHEN Anfragegroesse (17) aufgerufen",
                 g_allocMemLastRequested, 17);
        checkU32("F1: Q9K_AllocLargest NICHT aufgerufen", (Q9_u32)g_allocLargestCalls, 0);
    }

    /* Fall 2: normale Anfrage, Fehlschlag (Arena erschoepft) -- E_MEMFUL. */
    {
        resetStubs();
        g_allocMemReturn = 0;

        reaped = Q9K_ProcSRqMem(1000, &outAddr, &outSize, &outError);
        checkU32("F2: Fehlschlag (Rueckgabe 0)", (Q9_u32)reaped, 0);
        checkU32("F2: *outError == E_MEMFUL ($CF)", outError, 0x00CFU);
    }

    /* Fall 3: d0.l == -1 (0xFFFFFFFF) -- Q9K_AllocLargest-Sonderpfad,
     * Erfolg. */
    {
        resetStubs();
        g_allocLargestReturn = 0x3000;
        g_allocLargestOutSize = 512;

        reaped = Q9K_ProcSRqMem(0xFFFFFFFFUL, &outAddr, &outSize, &outError);
        checkU32("F3: Erfolg (Rueckgabe 1)", (Q9_u32)reaped, 1);
        checkU32("F3: *outAddr == Q9K_AllocLargest()-Ergebnis", outAddr, 0x3000);
        checkU32("F3: *outSize == echte (NICHT nochmal gerundete) Blockgroesse (512)", outSize, 512);
        checkU32("F3: Q9K_AllocLargest wurde aufgerufen", (Q9_u32)g_allocLargestCalls, 1);
        checkU32("F3: Q9K_AllocMem NICHT aufgerufen (anderer Pfad)", (Q9_u32)g_allocMemCalls, 0);
    }

    /* Fall 4: d0.l == -1, Arena komplett leer -- E_NORAM (NICHT E_MEMFUL,
     * s. Kopfkommentar q9kernel_sysmem.c fuer die Begruendung). */
    {
        resetStubs();
        g_allocLargestReturn = 0;
        g_allocLargestOutSize = 0;

        reaped = Q9K_ProcSRqMem(0xFFFFFFFFUL, &outAddr, &outSize, &outError);
        checkU32("F4: Fehlschlag (Rueckgabe 0)", (Q9_u32)reaped, 0);
        checkU32("F4: *outError == E_NORAM ($ED)", outError, 0x00EDU);
    }

    /* Fall 5: Q9K_ProcSRtMem -- reine Weiterleitung an Q9K_FreeMem, mit
     * identischer Rundung wie F$SRqMem (reale Konvention). */
    {
        resetStubs();

        Q9K_ProcSRtMem(0x4000, 17);
        checkU32("F5: Q9K_FreeMem wurde aufgerufen", (Q9_u32)g_freeMemCalls, 1);
        checkU32("F5: ... mit der echten Adresse", g_freeMemLastAddr, 0x4000);
        checkU32("F5: ... mit der aufgerundeten Groesse (17 -> 32)", g_freeMemLastSize, 32);
    }

    /* Fall 6: Q9K_SysSRqMemImpl -- End-zu-Ende ueber die Scratch-Adressen
     * (Erfolgsfall). */
    {
        resetStubs();
        g_allocMemReturn = 0x5000;
        Q9K_SetU32(Q9K_SRQMEM_SCRATCH_SIZEIN, 16);

        Q9K_SysSRqMemImpl();
        checkU32("F6: Success-Flag == 1", Q9K_GetU16(Q9K_SRQMEM_SCRATCH_SUCCESS), 1);
        checkU32("F6: Scratch-Adresse == Q9K_AllocMem()-Ergebnis",
                 Q9K_GetU32(Q9K_SRQMEM_SCRATCH_ADDR), 0x5000);
        checkU32("F6: Scratch-Groesse == 16 (bereits 16-Byte-ausgerichtet)",
                 Q9K_GetU32(Q9K_SRQMEM_SCRATCH_SIZEOUT), 16);
    }

    /* Fall 7: Q9K_SysSRqMemImpl -- End-zu-Ende, Fehlschlag. */
    {
        resetStubs();
        g_allocMemReturn = 0;
        Q9K_SetU32(Q9K_SRQMEM_SCRATCH_SIZEIN, 99999);

        Q9K_SysSRqMemImpl();
        checkU32("F7: Success-Flag == 0", Q9K_GetU16(Q9K_SRQMEM_SCRATCH_SUCCESS), 0);
        checkU32("F7: Scratch-Fehlercode == E_MEMFUL ($CF)",
                 Q9K_GetU16(Q9K_SRQMEM_SCRATCH_ERROR), 0x00CFU);
    }

    /* Fall 8: Q9K_SysSRtMemImpl -- End-zu-Ende. */
    {
        resetStubs();
        Q9K_SetU32(Q9K_SRTMEM_SCRATCH_ADDRIN, 0x6000);
        Q9K_SetU32(Q9K_SRTMEM_SCRATCH_SIZEIN, 64);

        Q9K_SysSRtMemImpl();
        checkU32("F8: Q9K_FreeMem wurde aufgerufen", (Q9_u32)g_freeMemCalls, 1);
        checkU32("F8: ... mit der echten Adresse aus dem Scratch-Feld", g_freeMemLastAddr, 0x6000);
        checkU32("F8: ... mit der (bereits ausgerichteten) Groesse", g_freeMemLastSize, 64);
    }


    /* F$Trans (Callcode 0x60): auf einer Maschine ohne zweiten Bus ist
     * die Adressuebersetzung die Identitaet -- in beide Richtungen. */
    {
        Q9_u32 size = 0x1000UL;
        Q9_u32 addr = 0x00123456UL;
        Q9_u16 err = 0xFFFF;

        printf("\n--- F$Trans ---\n");
        checkU32("F$Trans nimmt Richtung 0 an (lokal -> extern)",
                 (Q9_u32)Q9K_ProcTrans(0, &size, &addr, &err), 1);
        checkU32("F$Trans laesst die Adresse unveraendert", addr, 0x00123456UL);
        checkU32("F$Trans laesst die Groesse unveraendert", size, 0x1000UL);
        checkU32("F$Trans meldet dabei keinen Fehler", (Q9_u32)err, 0);

        checkU32("F$Trans nimmt Richtung 1 an (extern -> lokal)",
                 (Q9_u32)Q9K_ProcTrans(1, &size, &addr, &err), 1);
        checkU32("auch dort bleibt die Adresse gleich", addr, 0x00123456UL);

        err = 0;
        checkU32("F$Trans weist eine unbekannte Richtung ab",
                 (Q9_u32)Q9K_ProcTrans(2, &size, &addr, &err), 0);
        checkU32("F$Trans meldet dafuer E_BPADDR", (Q9_u32)err, 0x00D2UL);

        Q9K_SetU32(Q9K_TRANS_SCRATCH_SIZE, 0x800UL);
        Q9K_SetU32(Q9K_TRANS_SCRATCH_MODE, 0UL);
        Q9K_SetU32(Q9K_TRANS_SCRATCH_ADDR, 0x00ABCDEFUL);
        Q9K_SysTransImpl();
        checkU32("F$Trans-Bridge meldet Erfolg", Q9K_GetU32(Q9K_TRANS_SCRATCH_MODE), 1);
        checkU32("F$Trans-Bridge gibt die Adresse unveraendert zurueck",
                 Q9K_GetU32(Q9K_TRANS_SCRATCH_ADDR), 0x00ABCDEFUL);
        checkU32("F$Trans-Bridge gibt die Groesse unveraendert zurueck",
                 Q9K_GetU32(Q9K_TRANS_SCRATCH_SIZE), 0x800UL);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
