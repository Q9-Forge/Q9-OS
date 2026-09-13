/*
 * test_q9kernel_setsys.c -- Regressionstest fuer q9kernel_setsys.c.
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -o test_q9kernel_setsys test_q9kernel_setsys.c && \
 *       ./test_q9kernel_setsys
 */

#include <stdio.h>

static unsigned char g_fakeGlobals[0x40];

#define Q9K_SetSysScratch_VarCode ((unsigned long)(g_fakeGlobals + 0x00))
#define Q9K_SetSysScratch_Flags   ((unsigned long)(g_fakeGlobals + 0x10))
#define Q9K_SetSysScratch_Value   ((unsigned long)(g_fakeGlobals + 0x20))

#include "q9kernel_setsys.c"

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

int main(void)
{
    unsigned long value;
    int ok;

    /* Fall 1: Lesen der bekannten csl-Malloc-Zuwachsgroesse (Var 0x7C)
     * liefert den dokumentierten Standardwert. */
    value = 0;
    ok = Q9K_ProcSetSys(0x7CUL, Q9K_SETSYS_GETFLAG, 0UL, &value);
    checkU32("F1: Erfolg", (unsigned long)ok, 1UL);
    checkU32("F1: Wert == 4096 (Standard-Zuwachsgroesse)", value, 4096UL);

    /* Fall 2: Lesen einer UNBEKANNTEN Variable liefert 0, aber trotzdem
     * Erfolg (s. Kopfkommentar -- absichtlich kein Fehlschlag). */
    value = 0xDEADBEEFUL;
    ok = Q9K_ProcSetSys(0x99UL, Q9K_SETSYS_GETFLAG, 0UL, &value);
    checkU32("F2: Erfolg", (unsigned long)ok, 1UL);
    checkU32("F2: unbekannte Variable -> 0", value, 0UL);

    /* Fall 3: Schreiben wird bestaetigt, aber NICHT gespeichert -- ein
     * anschliessendes Lesen derselben Variable liefert weiterhin den
     * Standardwert (bzw. 0 bei unbekannten Variablen), nicht den zuvor
     * "geschriebenen" Wert. */
    value = 0;
    ok = Q9K_ProcSetSys(0x7CUL, 0UL /* kein GETFLAG = schreiben */, 12345UL, &value);
    checkU32("F3: Schreiben meldet Erfolg", (unsigned long)ok, 1UL);
    checkU32("F3: Schreiben liefert den geschriebenen Wert zurueck (nur bestaetigt)", value, 12345UL);
    value = 0;
    ok = Q9K_ProcSetSys(0x7CUL, Q9K_SETSYS_GETFLAG, 0UL, &value);
    checkU32("F3: nachfolgendes Lesen unveraendert (nicht gespeichert)", value, 4096UL);

    /* Fall 4: Scratch-Bruecke Q9K_SysSetSysImpl liest/schreibt die
     * richtigen Zellen. */
    *(unsigned long *)Q9K_SetSysScratch_VarCode = 0x7CUL;
    *(unsigned long *)Q9K_SetSysScratch_Flags   = Q9K_SETSYS_GETFLAG;
    *(unsigned long *)Q9K_SetSysScratch_Value   = 0xFFFFFFFFUL;
    Q9K_SysSetSysImpl();
    checkU32("F4: Scratch-Bruecke liefert 4096 in Q9K_SetSysScratch_Value",
             *(unsigned long *)Q9K_SetSysScratch_Value, 4096UL);

    if (g_failures == 0) {
        printf("Alle Tests erfolgreich.\n");
        return 0;
    }
    printf("%d Test(s) fehlgeschlagen.\n", g_failures);
    return 1;
}
