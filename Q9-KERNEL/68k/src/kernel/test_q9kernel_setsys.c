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

static unsigned char g_fakeGlobals[0x2000];

#define Q9K_SETSYS_ADDR(offset) ((unsigned long)(g_fakeGlobals + (offset)))

#define Q9K_SetSysScratch_VarCode ((unsigned long)(g_fakeGlobals + 0x00))
#define Q9K_SetSysScratch_Flags   ((unsigned long)(g_fakeGlobals + 0x10))
#define Q9K_SetSysScratch_Value   ((unsigned long)(g_fakeGlobals + 0x20))
#define Q9K_SetSysScratch_Error   ((unsigned long)(g_fakeGlobals + 0x30))
#define Q9K_SetSysScratch_Success ((unsigned long)(g_fakeGlobals + 0x40))

/* C5: D_MinPty/D_MaxAge leben im Scheduler; hier einfache Ersatzzellen. */
static unsigned long g_minPty = 0, g_maxAge = 0;
unsigned long Q9K_SchedGetMinPty(void) { return g_minPty; }
void Q9K_SchedSetMinPty(unsigned long v) { g_minPty = v & 0xFFFFUL; }
unsigned long Q9K_SchedGetMaxAge(void) { return g_maxAge; }
void Q9K_SchedSetMaxAge(unsigned long v) { g_maxAge = v & 0xFFFFUL; }

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
    checkU32("F3: Schreiben liefert den VORHERIGEN Wert (Handbuch: original value)", value, 4096UL);
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

    /* Fall 3c: verifizierte System-Globals sind lesbar. */
    *(unsigned long *)(g_fakeGlobals + 0x6CUL) = 0x00800000UL;
    value = 0; error = 0xDEADUL;
    ok = Q9K_ProcSetSys(0x6CUL, Q9K_SETSYS_GETFLAG, 0UL, &value, &error);
    checkU32("F3c: D_TotRAM ist lesbar", (unsigned long)ok, 1UL);
    checkU32("F3c: D_TotRAM liefert Globalwert", value, 0x00800000UL);

    *(unsigned long *)(g_fakeGlobals + 0x4CUL) = 0x00123456UL;
    value = 0;
    ok = Q9K_ProcSetSys(0x4CUL, Q9K_SETSYS_GETFLAG, 0UL, &value, &error);
    checkU32("F3d: D_Proc ist lesbar", (unsigned long)ok, 1UL);
    checkU32("F3d: D_Proc liefert Zeigerwert", value, 0x00123456UL);

    *(unsigned long *)(g_fakeGlobals + 0x1BE0UL) = 9876UL;
    value = 0;
    ok = Q9K_ProcSetSys(0x54UL, Q9K_SETSYS_GETFLAG, 0UL, &value, &error);
    checkU32("F3e: D_Ticks ist lesbar", (unsigned long)ok, 1UL);
    checkU32("F3e: D_Ticks liefert Q9-Zaehler", value, 9876UL);

    *(unsigned long *)(g_fakeGlobals + 0x4CUL) = 0UL;      /* kein laufender Prozess (Kernelkontext): darf schreiben */
    /* Schreibzugriffe auf bekannte, nicht veraenderbare Globals: E$Param (C5; frueher E$UnkSvc). */
    value = 0xDEADUL; error = 0;
    ok = Q9K_ProcSetSys(0x6CUL, 0UL, 1UL, &value, &error);
    checkU32("F3f: D_TotRAM bleibt schreibgeschuetzt", (unsigned long)ok, 0UL);
    checkU32("F3f: D_TotRAM Schreibfehler E$Param (0xE1)", error, 0xE1UL);

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

    /* ---- C5: Groessen, Rechte, D_MinPty/D_MaxAge, D_ProcSz ---- */
    {
        static unsigned char fakePd[0x40];
        unsigned long *userCell = (unsigned long *)(fakePd + 0x14UL);

        *(unsigned long *)(g_fakeGlobals + 0x4CUL) = (unsigned long)fakePd;
        *userCell = 0x00000007UL;                             /* Gruppe 0, Benutzer 7: Super-User */

        value = 0xDEADUL; error = 0;
        ok = Q9K_ProcSetSys(0x55EUL, Q9K_SETSYS_GETFLAG | 2UL, 0UL, &value, &error);
        checkU32("C5-a: D_MinPty lesen (Wort)", (unsigned long)ok, 1UL);
        checkU32("C5-a: D_MinPty startet mit 0", value, 0UL);
        ok = Q9K_ProcSetSys(0x55EUL, 2UL, 5UL, &value, &error);
        checkU32("C5-a: Super-User darf D_MinPty setzen", (unsigned long)ok, 1UL);
        checkU32("C5-a: Schreiben liefert den vorherigen Wert (0)", value, 0UL);
        checkU32("C5-a: der Scheduler-Wert ist gesetzt", g_minPty, 5UL);
        ok = Q9K_ProcSetSys(0x55EUL, Q9K_SETSYS_GETFLAG | 2UL, 0UL, &value, &error);
        checkU32("C5-a: Lesen danach liefert 5", value, 5UL);
        ok = Q9K_ProcSetSys(0x55EUL, 2UL, 0UL, &value, &error);
        checkU32("C5-a: D_MinPty zurueck auf 0, vorheriger Wert 5", value, 5UL);

        ok = Q9K_ProcSetSys(0x560UL, 2UL, 7UL, &value, &error);
        checkU32("C5-b: D_MaxAge setzen", (unsigned long)ok, 1UL);
        checkU32("C5-b: Scheduler-Wert", g_maxAge, 7UL);
        ok = Q9K_ProcSetSys(0x560UL, 2UL, 0UL, &value, &error);

        /* Groessen */
        ok = Q9K_ProcSetSys(0x55EUL, 2UL, 0x12345UL, &value, &error);
        checkU32("C5-c: Wort-Schreiben schneidet auf 16 Bit ab", g_minPty, 0x2345UL);
        ok = Q9K_ProcSetSys(0x55EUL, 2UL, 0UL, &value, &error);
        g_minPty = 0x1234UL;
        ok = Q9K_ProcSetSys(0x55EUL, Q9K_SETSYS_GETFLAG | 1UL, 0UL, &value, &error);
        checkU32("C5-c: Byte-Lesen liefert nur das untere Byte", value, 0x34UL);
        ok = Q9K_ProcSetSys(0x55EUL, Q9K_SETSYS_GETFLAG | 3UL, 0UL, &value, &error);
        checkU32("C5-c: Groesse 3 -> Fehlschlag", (unsigned long)ok, 0UL);
        checkU32("C5-c: Groesse 3 -> E$Param (0xE1)", error, 0xE1UL);
        ok = Q9K_ProcSetSys(0x55EUL, Q9K_SETSYS_GETFLAG | 8UL, 0UL, &value, &error);
        checkU32("C5-c: Groesse 8 -> E$Param", error, 0xE1UL);
        g_minPty = 0;

        /* D_ProcSz */
        ok = Q9K_ProcSetSys(0x3E2UL, Q9K_SETSYS_GETFLAG | 2UL, 0UL, &value, &error);
        checkU32("C5-d: D_ProcSz lesbar", (unsigned long)ok, 1UL);
        checkU32("C5-d: D_ProcSz = 0x400", value, 0x400UL);
        ok = Q9K_ProcSetSys(0x3E2UL, 2UL, 1UL, &value, &error);
        checkU32("C5-d: D_ProcSz nicht veraenderbar (E$Param)", error, 0xE1UL);
        /* D_TSlice unter dem Kopfzeilen-Offset ist dieselbe Zelle wie $76 */
        ok = Q9K_ProcSetSys(0x42EUL, Q9K_SETSYS_GETFLAG | 2UL, 0UL, &value, &error);
        checkU32("C5-d: D_TSlice ($42E) liest denselben Wert wie $76", value, 2UL);

        /* Rechte: Gruppe ungleich 0 darf nicht schreiben, aber lesen */
        *userCell = 0x00030007UL;                             /* Gruppe 3 */
        ok = Q9K_ProcSetSys(0x55EUL, 2UL, 9UL, &value, &error);
        checkU32("C5-e: Nicht-Super-User schreibt nicht", (unsigned long)ok, 0UL);
        checkU32("C5-e: Fehlercode E$Permit (0xA4)", error, 0xA4UL);
        checkU32("C5-e: Wert blieb unveraendert", g_minPty, 0UL);
        ok = Q9K_ProcSetSys(0x55EUL, Q9K_SETSYS_GETFLAG | 2UL, 0UL, &value, &error);
        checkU32("C5-e: Nicht-Super-User darf lesen", (unsigned long)ok, 1UL);
        ok = Q9K_ProcSetSys(0x99UL, 2UL, 1UL, &value, &error);
        checkU32("C5-e: unbekannte Variable schreiben: kein Recht -> E$Permit zuerst nur fuer bekannte", (unsigned long)ok, 0UL);
        *(unsigned long *)(g_fakeGlobals + 0x4CUL) = 0UL;
    }

    if (g_failures == 0) {
        printf("Alle Tests erfolgreich.\n");
        return 0;
    }
    printf("%d Test(s) fehlgeschlagen.\n", g_failures);
    return 1;
}
