/*
 * test_q9kernel_mem.c -- Regressionstest fuer q9kernel_mem.c (F$Mem).
 *
 * Gleiche #include-Konvention wie die anderen Kernel-Tests.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -o test_q9kernel_mem test_q9kernel_mem.c && \
 *       ./test_q9kernel_mem
 */

#include <stdio.h>

/* Fake-Prozessdeskriptor und Scratch-Block.
 *
 * WICHTIG -- gleiche Falle wie in test_q9kernel_chain.c: auf dem Host
 * ist `unsigned long` 64 Bit, im 68k-Kernel 32 Bit. Die echten
 * Deskriptor-Offsets $1B0 und $1B4 liegen nur VIER Byte auseinander;
 * ein 64-Bit-Schreibzugriff auf $1B4 wuerde die oberen vier Byte des
 * Feldes bei $1B0 mit ueberschreiben und den Test scheitern lassen,
 * obwohl die Implementierung stimmt. Deshalb werden die Offsets hier
 * -- nur fuer den Hosttest -- auf acht Byte Abstand gelegt. Die
 * echten Werte bleiben in q9kernel_mem.c unveraendert gueltig. */
#define Q9K_PROCDESC_ALLOCBASE_OFF 0x40UL
#define Q9K_PROCDESC_ALLOCSIZE_OFF 0x48UL

static unsigned char g_fakeDesc[0x200];
static unsigned char g_fakeScratch[0x40];

#define Q9K_MemScratch_Request  ((unsigned long)(g_fakeScratch + 0x00))
#define Q9K_MemScratch_OutSize  ((unsigned long)(g_fakeScratch + 0x08))
#define Q9K_MemScratch_OutUpper ((unsigned long)(g_fakeScratch + 0x10))
#define Q9K_MemScratch_Error    ((unsigned long)(g_fakeScratch + 0x18))
#define Q9K_MemScratch_Ok       ((unsigned long)(g_fakeScratch + 0x20))

/* Q9_D_PROC zeigt im echten Kernel auf ein Systemglobal, das den
 * aktuellen Deskriptor enthaelt -- hier eine eigene Zelle. */
static unsigned long g_dProcCell;
#define Q9_D_PROC ((unsigned long)&g_dProcCell)

#include "q9kernel_mem.c"

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
    unsigned long size, upper, error;
    int ok;
    unsigned long desc = (unsigned long)g_fakeDesc;

    /* Prozessblock: Basis $00040000, Groesse $2000 -> obere Grenze
     * $00042000. Werte frei gewaehlt, nur die Arithmetik zaehlt. */
    *(volatile unsigned long *)(desc + Q9K_PROCDESC_ALLOCBASE_OFF) = 0x00040000UL;
    *(volatile unsigned long *)(desc + Q9K_PROCDESC_ALLOCSIZE_OFF) = 0x00002000UL;

    /* Fall 1: Informationsabfrage (d0 = 0) -- der einzige Weg, der ab
     * OS-9/68K V2.3 laut Handbuch (S.466) noch funktioniert. */
    size = 0; upper = 0; error = 0xDEADUL;
    ok = Q9K_ProcMem(desc, 0UL, &size, &upper, &error);
    checkU32("F1: Erfolg", (unsigned long)ok, 1UL);
    checkU32("F1: d0.l = Groesse des Datenbereichs", size, 0x2000UL);
    checkU32("F1: a1 = obere Grenze (Basis + Groesse)", upper, 0x42000UL);
    checkU32("F1: kein Fehlercode", error, 0UL);

    /* Fall 2: Vergroesserung wird abgelehnt -- E$NoRAM, derselbe Code,
     * den auch der Referenzkernel an dieser Stelle liefert ($61da). */
    size = 0xAAUL; upper = 0xBBUL; error = 0;
    ok = Q9K_ProcMem(desc, 0x4000UL, &size, &upper, &error);
    checkU32("F2: Vergroesserung -> Fehlschlag", (unsigned long)ok, 0UL);
    checkU32("F2: Fehlercode E$NoRAM (0xED)", error, 0xEDUL);
    checkU32("F2: Groesse auf 0 zurueckgesetzt", size, 0UL);
    checkU32("F2: obere Grenze auf 0 zurueckgesetzt", upper, 0UL);

    /* Fall 3: auch eine VERKLEINERUNG wird abgelehnt. Das Original
     * setzt sie zwar noch fort, dieser Kernel lehnt sie bewusst ab
     * (keine Teilrueckgabe an die Arena) -- s. Kopfkommentar. */
    size = 0xAAUL; upper = 0xBBUL; error = 0;
    ok = Q9K_ProcMem(desc, 0x1000UL, &size, &upper, &error);
    checkU32("F3: Verkleinerung -> Fehlschlag", (unsigned long)ok, 0UL);
    checkU32("F3: Fehlercode E$NoRAM (0xED)", error, 0xEDUL);

    /* Fall 4: exakt die aktuelle Groesse ist ebenfalls eine
     * Groessenaenderungs-Anfrage (d0 != 0) und schlaegt fehl. Nur d0=0
     * ist die Informationsabfrage. */
    error = 0;
    ok = Q9K_ProcMem(desc, 0x2000UL, &size, &upper, &error);
    checkU32("F4: gleiche Groesse (d0 != 0) -> Fehlschlag", (unsigned long)ok, 0UL);
    checkU32("F4: Fehlercode E$NoRAM (0xED)", error, 0xEDUL);

    /* Fall 5: kein aktueller Prozess -> E$IPrcID, gleiche Wahl wie in
     * q9kernel_procsleep.c fuer denselben Zustand. */
    size = 0xAAUL; upper = 0xBBUL; error = 0;
    ok = Q9K_ProcMem(0UL, 0UL, &size, &upper, &error);
    checkU32("F5: kein Prozess -> Fehlschlag", (unsigned long)ok, 0UL);
    checkU32("F5: Fehlercode E$IPrcID (0xE0)", error, 0xE0UL);

    /* Fall 6: Nullgrosser Block -- Basis wird unveraendert als obere
     * Grenze gemeldet, kein Sonderfall, kein Fehler. */
    *(volatile unsigned long *)(desc + Q9K_PROCDESC_ALLOCBASE_OFF) = 0x00050000UL;
    *(volatile unsigned long *)(desc + Q9K_PROCDESC_ALLOCSIZE_OFF) = 0UL;
    size = 0xAAUL; upper = 0xBBUL; error = 0xDEADUL;
    ok = Q9K_ProcMem(desc, 0UL, &size, &upper, &error);
    checkU32("F6: Erfolg bei leerem Block", (unsigned long)ok, 1UL);
    checkU32("F6: Groesse 0", size, 0UL);
    checkU32("F6: obere Grenze = Basis", upper, 0x50000UL);

    /* Fall 7: die Scratch-Bruecke selbst -- schreibt sie alle vier
     * Ausgabezellen richtig? Deskriptor wieder mit echten Werten. */
    *(volatile unsigned long *)(desc + Q9K_PROCDESC_ALLOCBASE_OFF) = 0x00060000UL;
    *(volatile unsigned long *)(desc + Q9K_PROCDESC_ALLOCSIZE_OFF) = 0x00000800UL;
    g_dProcCell = desc;
    *(volatile unsigned long *)Q9K_MemScratch_Request = 0UL;
    Q9K_SysFMemImpl();
    checkU32("F7: Bruecke meldet Erfolg",
             *(volatile unsigned long *)Q9K_MemScratch_Ok, 1UL);
    checkU32("F7: Bruecke schreibt Groesse",
             *(volatile unsigned long *)Q9K_MemScratch_OutSize, 0x800UL);
    checkU32("F7: Bruecke schreibt obere Grenze",
             *(volatile unsigned long *)Q9K_MemScratch_OutUpper, 0x60800UL);
    checkU32("F7: Bruecke meldet keinen Fehler",
             *(volatile unsigned long *)Q9K_MemScratch_Error, 0UL);

    /* Fall 8: Bruecke im Fehlerfall -- Groessenaenderung ueber die
     * Scratch-Zelle. */
    *(volatile unsigned long *)Q9K_MemScratch_Request = 0x9000UL;
    Q9K_SysFMemImpl();
    checkU32("F8: Bruecke meldet Fehlschlag",
             *(volatile unsigned long *)Q9K_MemScratch_Ok, 0UL);
    checkU32("F8: Bruecke schreibt E$NoRAM",
             *(volatile unsigned long *)Q9K_MemScratch_Error, 0xEDUL);

    if (g_failures == 0) {
        printf("\nAlle F$Mem-Testfaelle bestanden.\n");
        return 0;
    }
    printf("\n%d Testfall/Testfaelle FEHLGESCHLAGEN.\n", g_failures);
    return 1;
}
