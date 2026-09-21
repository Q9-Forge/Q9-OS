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

static unsigned char g_fakeGlobals[0x80];

#define Q9K_SetSysScratch_VarCode ((unsigned long)(g_fakeGlobals + 0x00))
#define Q9K_SetSysScratch_Flags   ((unsigned long)(g_fakeGlobals + 0x10))
#define Q9K_SetSysScratch_Value   ((unsigned long)(g_fakeGlobals + 0x20))
#define Q9K_SetSysScratch_Error   ((unsigned long)(g_fakeGlobals + 0x30))
#define Q9K_SetSysScratch_Success ((unsigned long)(g_fakeGlobals + 0x40))

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
    unsigned long value, error;
    int ok;

    /* Fall 1: Lesen der bekannten csl-Malloc-Zuwachsgroesse (Var 0x7C)
     * liefert den dokumentierten Standardwert, Erfolg. */
    value = 0; error = 0xDEADUL;
    ok = Q9K_ProcSetSys(0x7CUL, Q9K_SETSYS_GETFLAG, 0UL, &value, &error);
    checkU32("F1: Erfolg", (unsigned long)ok, 1UL);
    checkU32("F1: Wert == 4096 (Standard-Zuwachsgroesse)", value, 4096UL);
    checkU32("F1: kein Fehlercode", error, 0UL);

    /* Fall 2 (NACHTRAG Fortsetzung 60): Lesen einer UNBEKANNTEN Variable
     * meldet jetzt einen sauberen Fehlschlag (E$UnkSvc) statt still 0 +
     * Erfolg vorzutaeuschen -- s. Kopfkommentar q9kernel_setsys.c. */
    value = 0xDEADBEEFUL; error = 0;
    ok = Q9K_ProcSetSys(0x99UL, Q9K_SETSYS_GETFLAG, 0UL, &value, &error);
    checkU32("F2: unbekannte Variable -> Fehlschlag", (unsigned long)ok, 0UL);
    checkU32("F2: Wert auf 0 gesetzt", value, 0UL);
    checkU32("F2: Fehlercode E$UnkSvc (0xD0)", error, 0xD0UL);

    /* Fall 3: Schreiben der bekannten Variable bleibt persistent und wird
     * beim anschliessenden Lesen wieder geliefert. */
    value = 0; error = 0xDEADUL;
    ok = Q9K_ProcSetSys(0x7CUL, 0UL /* kein GETFLAG = schreiben */, 12345UL, &value, &error);
    checkU32("F3: Schreiben meldet Erfolg", (unsigned long)ok, 1UL);
    checkU32("F3: Schreiben liefert den geschriebenen Wert zurueck (nur bestaetigt)", value, 12345UL);
    checkU32("F3: kein Fehlercode", error, 0UL);
    value = 0;
    ok = Q9K_ProcSetSys(0x7CUL, Q9K_SETSYS_GETFLAG, 0UL, &value, &error);
    checkU32("F3: nachfolgendes Lesen liefert den gespeicherten Wert", value, 12345UL);

    value = 0; error = 0xDEADUL;
    ok = Q9K_ProcSetSys(0x28UL, Q9K_SETSYS_GETFLAG, 0UL, &value, &error);
    checkU32("F3a: D_TckSec meldet Erfolg", (unsigned long)ok, 1UL);
    checkU32("F3a: D_TckSec startet mit 100", value, 100UL);
    ok = Q9K_ProcSetSys(0x28UL, 0UL, 200UL, &value, &error);
    checkU32("F3a: D_TckSec laesst sich setzen", (unsigned long)ok, 1UL);
    ok = Q9K_ProcSetSys(0x28UL, Q9K_SETSYS_GETFLAG, 0UL, &value, &error);
    checkU32("F3a: D_TckSec bleibt persistent", value, 200UL);

    value = 0; error = 0xDEADUL;
    ok = Q9K_ProcSetSys(0x76UL, Q9K_SETSYS_GETFLAG, 0UL, &value, &error);
    checkU32("F3b: D_TSlice meldet Erfolg", (unsigned long)ok, 1UL);
    checkU32("F3b: D_TSlice startet mit 2", value, 2UL);

    /* Fall 4: Scratch-Bruecke Q9K_SysSetSysImpl liest/schreibt die
     * richtigen Zellen -- Erfolgsfall. */
    *(unsigned long *)Q9K_SetSysScratch_VarCode = 0x7CUL;
    *(unsigned long *)Q9K_SetSysScratch_Flags   = Q9K_SETSYS_GETFLAG;
    *(unsigned long *)Q9K_SetSysScratch_Value   = 0xFFFFFFFFUL;
    *(unsigned long *)Q9K_SetSysScratch_Success = 0xFFFFFFFFUL;
    Q9K_SysSetSysImpl();
    checkU32("F4: Scratch-Bruecke liefert den gespeicherten Wert",
             *(unsigned long *)Q9K_SetSysScratch_Value, 12345UL);
    checkU32("F4: Scratch-Bruecke setzt Success=1",
             *(unsigned long *)Q9K_SetSysScratch_Success, 1UL);

    /* Fall 5 (NACHTRAG Fortsetzung 60): Scratch-Bruecke im Fehlschlagfall. */
    *(unsigned long *)Q9K_SetSysScratch_VarCode = 0x99UL;
    *(unsigned long *)Q9K_SetSysScratch_Flags   = Q9K_SETSYS_GETFLAG;
    *(unsigned long *)Q9K_SetSysScratch_Success = 0xFFFFFFFFUL;
    *(unsigned long *)Q9K_SetSysScratch_Error   = 0xFFFFFFFFUL;
    Q9K_SysSetSysImpl();
    checkU32("F5: Scratch-Bruecke setzt Success=0 bei unbekannter Variable",
             *(unsigned long *)Q9K_SetSysScratch_Success, 0UL);
    checkU32("F5: Scratch-Bruecke setzt Error=0xD0",
             *(unsigned long *)Q9K_SetSysScratch_Error, 0xD0UL);

    if (g_failures == 0) {
        printf("Alle Tests erfolgreich.\n");
        return 0;
    }
    printf("%d Test(s) fehlgeschlagen.\n", g_failures);
    return 1;
}
