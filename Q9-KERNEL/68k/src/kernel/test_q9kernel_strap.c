/*
 * test_q9kernel_strap.c -- Hosttest fuer F$STrap (q9kernel_strap.c,
 *                          2026-09-19).
 *
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_strap test_q9kernel_strap.c && ./test_strap
 *
 * Schwerpunkt ist die Auswertung der Initialisierungstabelle: die
 * PC-relative Adressrechnung und die Zuordnung Vektor -> Platz. Beides
 * laesst sich still falsch machen, und beides fuehrt dann dazu, dass im
 * Fehlerfall an eine beliebige Adresse gesprungen wird.
 */

#include <stdio.h>
#include <string.h>

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

static unsigned char g_cells[0x200];
#define CELL(off) ((unsigned long)(g_cells + (off)))

#define Q9_D_PROC               CELL(0x000)
#define Q9K_STRAP_SCRATCH_STACK CELL(0x020)
#define Q9K_STRAP_SCRATCH_TABLE CELL(0x040)
#define Q9K_STRAP_SCRATCH_ERROR CELL(0x060)
#define Q9K_STRAP_SCRATCH_OK    CELL(0x080)
#define Q9K_STRAP_SCRATCH_COUNT CELL(0x0A0)
#define Q9K_STRAP_SCRATCH_VEC   CELL(0x0C0)
#define Q9K_STRAP_SCRATCH_JUMP  CELL(0x0E0)

/* Die Behandlertabelle liegt hier an eigener Basis, damit sie nicht mit
 * Q9_D_PROC kollidiert -- und mit verbreitertem Platzabstand: Q9K_SetU32
 * schreibt auf diesem Host acht statt vier Byte und griffe bei echtem
 * Abstand in den naechsten Platz. Genau das ist beim ersten Lauf
 * passiert (ein Eintrag fuer Vektor 4 loeschte den fuer Vektor 5). */
#define Q9K_PROCDESC_EXCEPT_OFF 0x100UL
#define Q9K_PROCDESC_EXSTK_OFF  0x140UL
#define Q9K_EXCEPT_STRIDE       8UL   /* Q9_u32 ist hier acht Byte breit */

#include "q9kernel_strap.c"

static int failures;
static unsigned char g_desc[0x200];
static unsigned char g_table[64];

#define DESC  ((Q9_u32)(unsigned long)g_desc)
#define TABLE ((Q9_u32)(unsigned long)g_table)

static void check(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-62s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-62s = %lu (erwartet %lu)\n", label,
               (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

static void checkHex(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-62s = 0x%lx\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-62s = 0x%lx (erwartet 0x%lx)\n", label,
               (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

static void reset(void)
{
    memset(g_cells, 0, sizeof g_cells);
    memset(g_desc, 0, sizeof g_desc);
    memset(g_table, 0, sizeof g_table);
    Q9K_SetU32(Q9_D_PROC, DESC);
}

/* Ein Tabellenpaar schreiben, so wie der Assembler es erzeugt:
 * dc.w <code>, <ziel-*-4>  --  der Abstand ist relativ zum Paaranfang. */
static void putEntry(int idx, Q9_u16 code, long target)
{
    Q9_u32 at = TABLE + (Q9_u32)idx * 4UL;
    long rel = target - (long)(at + 4UL);
    *(volatile Q9_u16 *)at = code;
    *(volatile Q9_u16 *)(at + 2UL) = (Q9_u16)(short)rel;
}

static void putEnd(int idx)
{
    *(volatile Q9_u16 *)(TABLE + (Q9_u32)idx * 4UL) = 0xFFFFU;
}

static Q9_u32 slotOf(Q9_u32 vector)
{
    return Q9K_GetU32(DESC + Q9K_PROCDESC_EXCEPT_OFF + (vector - 2UL) * Q9K_EXCEPT_STRIDE);
}

int main(void)
{
    printf("== F$STrap ==\n");

    /* --- Die PC-relative Adressrechnung ---
     *
     * "Routine-*-4": die Routine liegt vier Byte hinter dem Paaranfang
     * plus dem eingetragenen Abstand. Ein Vorzeichenfehler hier laesst
     * den Behandler im Nirgendwo landen, und zwar erst im Fehlerfall --
     * also genau dann, wenn niemand mehr zusieht. */
    reset();
    putEntry(0, 16, (long)TABLE + 0x40);   /* T_IllIns = 16 -> Vektor 4 */
    putEnd(1);
    check("ein Eintrag wird uebernommen",
          (Q9_u32)Q9K_StrapInstall(DESC, TABLE, 0), 1);
    checkHex("die Behandleradresse ist PC-relativ aufgeloest",
             slotOf(4), TABLE + 0x40);

    /* Ein Ziel VOR der Tabelle -- negativer Abstand. */
    reset();
    putEntry(0, 16, (long)TABLE - 0x20);
    putEnd(1);
    Q9K_StrapInstall(DESC, TABLE, 0);
    checkHex("auch ein negativer Abstand wird richtig gerechnet",
             slotOf(4), TABLE - 0x20);

    /* --- Die Zuordnung Vektor -> Platz --- */
    reset();
    putEntry(0, 8,  (long)TABLE + 0x100);  /* T_BusErr -> Vektor 2, Platz 0 */
    putEntry(1, 44, (long)TABLE + 0x200);  /* T_1111   -> Vektor 11, Platz 9 */
    putEnd(2);
    check("beide Eintraege werden uebernommen",
          (Q9_u32)Q9K_StrapInstall(DESC, TABLE, 0), 2);
    checkHex("Bus Error liegt auf dem ersten Platz", slotOf(2), TABLE + 0x100);
    checkHex("Line 1111 auf dem letzten", slotOf(11), TABLE + 0x200);
    check("dazwischen bleibt alles leer", slotOf(6), 0);

    /* --- Der Stackzeiger wird mitgefuehrt --- */
    reset();
    putEntry(0, 16, (long)TABLE + 0x40);
    putEnd(1);
    Q9K_StrapInstall(DESC, TABLE, 0x12340000UL);
    checkHex("der Ausnahmestack wird zum Eintrag vermerkt",
             Q9K_GetU32(DESC + Q9K_PROCDESC_EXSTK_OFF + (4UL - 2UL) * Q9K_EXCEPT_STRIDE),
             0x12340000UL);

    /* --- Ersetzen statt verdoppeln ---
     *
     * "If an entry for a particular routine already exists, it is
     * replaced." */
    reset();
    putEntry(0, 16, (long)TABLE + 0x40);
    putEnd(1);
    Q9K_StrapInstall(DESC, TABLE, 0);
    putEntry(0, 16, (long)TABLE + 0x80);
    Q9K_StrapInstall(DESC, TABLE, 0);
    checkHex("ein zweiter Eintrag ersetzt den ersten", slotOf(4), TABLE + 0x80);

    /* --- Abweisungen --- */
    reset();
    putEntry(0, 4, (long)TABLE + 0x40);    /* Vektor 1 -- nicht abfangbar */
    putEnd(1);
    check("ein Vektor unterhalb des Bereichs wird abgewiesen",
          (Q9_u32)Q9K_StrapInstall(DESC, TABLE, 0), (Q9_u32)-1L);

    reset();
    putEntry(0, 48, (long)TABLE + 0x40);   /* Vektor 12 -- nicht abfangbar */
    putEnd(1);
    check("ein Vektor oberhalb des Bereichs ebenso",
          (Q9_u32)Q9K_StrapInstall(DESC, TABLE, 0), (Q9_u32)-1L);

    reset();
    putEntry(0, 17, (long)TABLE + 0x40);   /* kein Vielfaches von 4 */
    putEnd(1);
    check("ein Eintrag, der kein Vektoroffset sein kann, wird abgewiesen",
          (Q9_u32)Q9K_StrapInstall(DESC, TABLE, 0), (Q9_u32)-1L);

    reset();
    check("ein Nullzeiger auf die Tabelle wird abgewiesen",
          (Q9_u32)Q9K_StrapInstall(DESC, 0, 0), (Q9_u32)-1L);
    check("ein Nulldeskriptor ebenso",
          (Q9_u32)Q9K_StrapInstall(0, TABLE, 0), (Q9_u32)-1L);

    /* Eine leere Tabelle ist kein Fehler -- sie traegt nur nichts ein. */
    reset();
    putEnd(0);
    check("eine leere Tabelle ist in Ordnung",
          (Q9_u32)Q9K_StrapInstall(DESC, TABLE, 0), 0);

    /* Eine Tabelle ohne Endmarkierung darf nicht ins Blaue laufen. */
    reset();
    {
        int i;
        for (i = 0; i < 16; i++)
            putEntry(i, 16, (long)TABLE + 0x40);
        /* Die Tabelle ist nur 64 Byte gross; die Begrenzung laeuft also
         * in den Speicher dahinter, findet dort keinen gueltigen Eintrag
         * und bricht sauber ab. Geprueft wird hier genau das: ein
         * definiertes Ende statt eines Weiterlesens ohne Halt. */
        check("eine Tabelle ohne Ende wird begrenzt, nicht endlos gelesen",
              (Q9_u32)Q9K_StrapInstall(DESC, TABLE, 0), (Q9_u32)-1L);
    }

    /* --- Nachschlagen im Fehlerfall --- */
    reset();
    putEntry(0, 16, (long)TABLE + 0x40);   /* Illegal Instruction */
    putEnd(1);
    Q9K_StrapInstall(DESC, TABLE, 0);
    checkHex("der Behandler wird zur Vektornummer gefunden",
             Q9K_StrapHandlerFor(DESC, 4), TABLE + 0x40);
    check("eine Ausnahme ohne Behandler liefert 0",
          Q9K_StrapHandlerFor(DESC, 5), 0);
    check("ein Vektor ausserhalb des Bereichs liefert 0",
          Q9K_StrapHandlerFor(DESC, 32), 0);
    check("ein Nulldeskriptor liefert 0", Q9K_StrapHandlerFor(0, 4), 0);

    /* --- Die Bruecken --- */
    reset();
    putEntry(0, 16, (long)TABLE + 0x40);
    putEnd(1);
    Q9K_SetU32(Q9K_STRAP_SCRATCH_TABLE, TABLE);
    Q9K_SetU32(Q9K_STRAP_SCRATCH_STACK, 0);
    Q9K_SysSTrapImpl();
    check("Bruecke F$STrap meldet Erfolg", Q9K_GetU32(Q9K_STRAP_SCRATCH_OK), 1);
    check("und die Zahl der Eintraege", Q9K_GetU32(Q9K_STRAP_SCRATCH_COUNT), 1);

    Q9K_SetU32(Q9K_STRAP_SCRATCH_VEC, 4);
    Q9K_SysExcDispatchImpl();
    checkHex("die Ausnahmebruecke nennt das Sprungziel",
             Q9K_GetU32(Q9K_STRAP_SCRATCH_JUMP), TABLE + 0x40);

    Q9K_SetU32(Q9K_STRAP_SCRATCH_VEC, 5);
    Q9K_SysExcDispatchImpl();
    check("und 0, wo kein Behandler eingetragen ist",
          Q9K_GetU32(Q9K_STRAP_SCRATCH_JUMP), 0);

    Q9K_SetU32(Q9K_STRAP_SCRATCH_TABLE, 0);
    Q9K_SysSTrapImpl();
    check("Bruecke F$STrap weist einen Nullzeiger ab",
          Q9K_GetU32(Q9K_STRAP_SCRATCH_OK), 0);
    check("mit E_BPADDR", Q9K_GetU32(Q9K_STRAP_SCRATCH_ERROR), 0xD2);

    printf("\n%s\n", failures ? "TESTS FEHLGESCHLAGEN" : "ALLE TESTS BESTANDEN");
    return failures ? 1 : 0;
}
