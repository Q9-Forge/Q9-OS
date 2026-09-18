/*
 * test_q9kernel_blkmap.c -- Hosttest fuer F$GBlkMp (q9kernel_blkmap.c,
 *                           2026-09-18).
 *
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_blkmap test_q9kernel_blkmap.c && ./test_blkmap
 *
 * Die Freiliste wird hier in einem eigenen Feld nachgebaut, statt
 * q9kernel_arena.c mitzuziehen: geprueft wird die Uebersetzung der Liste
 * in die vom Handbuch vorgeschriebene Aussenform, nicht der Allocator.
 */

#include <stdio.h>
#include <string.h>

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;

static unsigned char g_cells[0x400];
#define CELL(off) ((unsigned long)(g_cells + (off)))

#define Q9_D_ARENA                 CELL(0x000)
#define Q9K_ARENA_HEAD             CELL(0x020)
#define Q9K_RAMSIZE_ADDR           CELL(0x040)
#define Q9K_BLKMP_SCRATCH_BEGIN    CELL(0x060)
#define Q9K_BLKMP_SCRATCH_BUFSIZE  CELL(0x080)
#define Q9K_BLKMP_SCRATCH_BUF      CELL(0x0A0)
#define Q9K_BLKMP_SCRATCH_MINALLOC CELL(0x0C0)
#define Q9K_BLKMP_SCRATCH_FRAGS    CELL(0x0E0)
#define Q9K_BLKMP_SCRATCH_TOTALRAM CELL(0x100)
#define Q9K_BLKMP_SCRATCH_FREERAM  CELL(0x120)
#define Q9K_BLKMP_SCRATCH_ERROR    CELL(0x140)
#define Q9K_BLKMP_SCRATCH_OK       CELL(0x160)

#include "q9kernel_blkmap.c"

static int failures;

/* Nachgebaute Freibloecke. Jeder Block ist so gross, dass die beiden
 * Kopffelder {next, size} hineinpassen -- auf diesem Host je 8 Byte. */
#define BLOCKS 4
static unsigned char g_blocks[BLOCKS][64];
static unsigned char g_buf[256];

#define BUFBASE ((Q9_u32)(unsigned long)g_buf)
#define BLK(i)  ((Q9_u32)(unsigned long)g_blocks[i])

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

static void reset(void)
{
    memset(g_cells, 0, sizeof g_cells);
    memset(g_blocks, 0, sizeof g_blocks);
    memset(g_buf, 0xEE, sizeof g_buf);   /* auffaellig vorbelegen */
}

/* Verkettet n Bloecke mit den angegebenen Groessen zu einer Freiliste. */
static void buildList(int n, const Q9_u32 *sizes)
{
    int i;
    for (i = 0; i < n; i++) {
        Q9K_SetU32(Q9K_FB_SIZE(BLK(i)), sizes[i]);
        Q9K_SetU32(Q9K_FB_NEXT(BLK(i)), (i + 1 < n) ? BLK(i + 1) : 0UL);
    }
    Q9K_SetU32(Q9K_ARENA_HEAD, n > 0 ? BLK(0) : 0UL);
}

/* Liest Paar i aus dem Ausgabepuffer. */
static Q9_u32 pairAddr(int i) { return Q9K_GetU32(BUFBASE + (Q9_u32)i * 2UL * sizeof(Q9_u32)); }
static Q9_u32 pairSize(int i) { return Q9K_GetU32(BUFBASE + (Q9_u32)i * 2UL * sizeof(Q9_u32) + sizeof(Q9_u32)); }

int main(void)
{
    Q9_u32 sizes[BLOCKS] = { 0x100, 0x200, 0x080, 0x400 };
    Q9_u32 frags, freeRam;
    Q9_u16 err;

    printf("== F$GBlkMp ==\n");

    /* --- Die Liste vollstaendig, mit reichlich Puffer --- */
    reset();
    buildList(4, sizes);
    err = 0;
    check("die Liste wird eingesammelt",
          (Q9_u32)Q9K_BlkMpCollect(BUFBASE, sizeof g_buf, 0, &frags, &freeRam, &err), 1);
    check("alle vier Bloecke werden gezaehlt", frags, 4);
    check("die freie Summe stimmt", freeRam, 0x100 + 0x200 + 0x080 + 0x400);
    check("das erste Paar traegt die Blockadresse", pairAddr(0), BLK(0));
    check("und die Blockgroesse", pairSize(0), 0x100);
    check("das dritte Paar ebenso", pairAddr(2), BLK(2));
    check("mit seiner Groesse", pairSize(2), 0x080);
    check("hinter dem letzten Paar steht ein Nulleintrag", pairAddr(4), 0);
    check("beide Felder genullt", pairSize(4), 0);

    /* --- Eine leere Freiliste ist kein Fehler, sondern eine Auskunft --- */
    reset();
    Q9K_SetU32(Q9K_ARENA_HEAD, 0);
    err = 0;
    check("eine leere Freiliste ist in Ordnung",
          (Q9_u32)Q9K_BlkMpCollect(BUFBASE, sizeof g_buf, 0, &frags, &freeRam, &err), 1);
    check("mit null Fragmenten", frags, 0);
    check("und nichts frei", freeRam, 0);
    check("der Puffer beginnt sofort mit dem Nulleintrag", pairAddr(0), 0);

    /* --- Die Startadresse beschraenkt den BERICHT --- */
    reset();
    buildList(4, sizes);
    err = 0;
    Q9K_BlkMpCollect(BUFBASE, sizeof g_buf, BLK(2), &frags, &freeRam, &err);
    /* Welche Bloecke oberhalb von BLK(2) liegen, entscheidet die Lage im
     * Testfeld -- die ist aufsteigend, also Block 2 und 3. */
    check("Bloecke vor der Startadresse bleiben unberichtet", frags, 2);
    check("und zaehlen nicht in die freie Summe", freeRam, 0x080 + 0x400);
    check("das erste Paar ist der erste berichtete Block", pairAddr(0), BLK(2));

    /* --- Ein zu kleiner Puffer: abschneiden, aber sauber abschliessen ---
     *
     * Die Zaehlung darf NICHT mit abgeschnitten werden: d1.l meldet die
     * Fragmente "im System", nicht die im Puffer untergebrachten. Wer das
     * verwechselt, sieht nach einem zu kleinen Puffer weniger Speicher,
     * als die Maschine hat. */
    reset();
    buildList(4, sizes);
    err = 0;
    check("ein knapper Puffer ist kein Fehler",
          (Q9_u32)Q9K_BlkMpCollect(BUFBASE, 3UL * 2UL * sizeof(Q9_u32), 0,
                                   &frags, &freeRam, &err), 1);
    check("es werden nur zwei Paare abgelegt", pairAddr(2), 0);
    check("das zweite Paar steht noch drin", pairAddr(1), BLK(1));
    check("die Fragmentzahl bleibt die des Systems", frags, 4);
    check("und die freie Summe ebenfalls", freeRam, 0x100 + 0x200 + 0x080 + 0x400);

    /* Genau ein Paar Platz: nur der Nulleintrag passt. */
    reset();
    buildList(4, sizes);
    err = 0;
    check("bei Platz fuer genau ein Paar",
          (Q9_u32)Q9K_BlkMpCollect(BUFBASE, 2UL * sizeof(Q9_u32), 0,
                                   &frags, &freeRam, &err), 1);
    check("steht dort nur der Abschluss", pairAddr(0), 0);
    check("die Auskunft stimmt trotzdem", frags, 4);

    /* --- Abweisungen --- */
    reset();
    buildList(4, sizes);
    err = 0;
    check("ein Puffer ohne Platz fuer den Abschluss wird abgewiesen",
          (Q9_u32)Q9K_BlkMpCollect(BUFBASE, 4, 0, &frags, &freeRam, &err), 0);
    check("mit E_BPADDR", (Q9_u32)err, 0xD2);
    err = 0;
    check("ein Nullzeiger wird abgewiesen",
          (Q9_u32)Q9K_BlkMpCollect(0, sizeof g_buf, 0, &frags, &freeRam, &err), 0);
    check("ebenfalls mit E_BPADDR", (Q9_u32)err, 0xD2);

    /* --- Die Bruecke --- */
    reset();
    buildList(4, sizes);
    Q9K_SetU32(Q9K_RAMSIZE_ADDR, 0x1000000);
    Q9K_SetU32(Q9K_BLKMP_SCRATCH_BUF, BUFBASE);
    Q9K_SetU32(Q9K_BLKMP_SCRATCH_BUFSIZE, sizeof g_buf);
    Q9K_SetU32(Q9K_BLKMP_SCRATCH_BEGIN, 0);
    Q9K_SysGBlkMpImpl();
    check("Bruecke meldet Erfolg", Q9K_GetU32(Q9K_BLKMP_SCRATCH_OK), 1);
    check("und die kleinste Zuteilungsgroesse",
          Q9K_GetU32(Q9K_BLKMP_SCRATCH_MINALLOC), 16);
    check("und die Fragmentzahl", Q9K_GetU32(Q9K_BLKMP_SCRATCH_FRAGS), 4);
    check("und den Gesamtspeicher", Q9K_GetU32(Q9K_BLKMP_SCRATCH_TOTALRAM), 0x1000000);
    check("und den freien Speicher",
          Q9K_GetU32(Q9K_BLKMP_SCRATCH_FREERAM), 0x100 + 0x200 + 0x080 + 0x400);

    Q9K_SetU32(Q9K_BLKMP_SCRATCH_BUF, 0);
    Q9K_SysGBlkMpImpl();
    check("Bruecke weist einen Nullpuffer ab", Q9K_GetU32(Q9K_BLKMP_SCRATCH_OK), 0);
    check("mit E_BPADDR", Q9K_GetU32(Q9K_BLKMP_SCRATCH_ERROR), 0xD2);
    check("meldet den Gesamtspeicher aber trotzdem",
          Q9K_GetU32(Q9K_BLKMP_SCRATCH_TOTALRAM), 0x1000000);

    printf("\n%s\n", failures ? "TESTS FEHLGESCHLAGEN" : "ALLE TESTS BESTANDEN");
    return failures ? 1 : 0;
}
