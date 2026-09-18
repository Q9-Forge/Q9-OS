/*
 * test_q9kernel_bitmap.c -- Hosttest fuer F$SchBit/F$AllBit/F$DelBit
 *                           (q9kernel_bitmap.c, 2026-09-18).
 *
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_bitmap test_q9kernel_bitmap.c && ./test_bitmap
 */

#include <stdio.h>
#include <string.h>

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

static unsigned char g_cells[0x200];
#define CELL(off) ((unsigned long)(g_cells + (off)))

#define Q9K_BITMAP_SCRATCH_START CELL(0x000)
#define Q9K_BITMAP_SCRATCH_COUNT CELL(0x020)
#define Q9K_BITMAP_SCRATCH_BASE  CELL(0x040)
#define Q9K_BITMAP_SCRATCH_END   CELL(0x060)
#define Q9K_BITMAP_SCRATCH_ERROR CELL(0x080)
#define Q9K_BITMAP_SCRATCH_OK    CELL(0x0A0)

#include "q9kernel_bitmap.c"

static int failures;
static unsigned char g_map[8];
#define MAPBASE ((Q9_u32)(unsigned long)g_map)
#define MAPEND  (MAPBASE + sizeof g_map)

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
    memset(g_map, 0, sizeof g_map);
}

int main(void)
{
    Q9_u32 start, count;

    printf("== Bitmap-Aufrufe F$SchBit / F$AllBit / F$DelBit ==\n");

    /* --- Die Bitreihenfolge. Sie ist eine begruendete Annahme, kein
     *     Handbuchbeleg (s. Kopfkommentar von q9kernel_bitmap.c) --
     *     deshalb steht sie hier als ausdrueckliche Pruefung: aendert
     *     jemand Q9K_BitMask, faellt es hier auf und nicht erst auf einer
     *     fremd beschriebenen Platte. --- */
    reset();
    Q9K_BitmapAllocate(MAPBASE, 0, 1);
    check("Bit 0 ist das hoechstwertige Bit von Byte 0", g_map[0], 0x80);
    reset();
    Q9K_BitmapAllocate(MAPBASE, 7, 1);
    check("Bit 7 ist das niederwertigste Bit von Byte 0", g_map[0], 0x01);
    reset();
    Q9K_BitmapAllocate(MAPBASE, 8, 1);
    check("Bit 8 ist das hoechstwertige Bit von Byte 1", g_map[1], 0x80);
    check("und Byte 0 bleibt unberuehrt", g_map[0], 0x00);

    /* --- F$AllBit --- */
    reset();
    Q9K_BitmapAllocate(MAPBASE, 4, 8);
    check("acht Bits ab 4 fuellen den Rest von Byte 0", g_map[0], 0x0F);
    check("und die obere Haelfte von Byte 1", g_map[1], 0xF0);
    check("Byte 2 bleibt frei", g_map[2], 0x00);

    reset();
    Q9K_BitmapAllocate(MAPBASE, 3, 0);
    check("eine Anzahl von 0 aendert nichts", g_map[0], 0x00);

    /* Bereits belegte Bits bleiben belegt -- Belegen ist nicht Umschalten. */
    reset();
    Q9K_BitmapAllocate(MAPBASE, 0, 2);
    Q9K_BitmapAllocate(MAPBASE, 1, 2);
    check("ueberlappendes Belegen setzt, statt umzuschalten", g_map[0], 0xE0);

    /* --- F$DelBit --- */
    reset();
    memset(g_map, 0xFF, sizeof g_map);
    Q9K_BitmapDeallocate(MAPBASE, 4, 8);
    check("Freigeben loescht genau dieselben Bits", g_map[0], 0xF0);
    check("auch ueber die Bytegrenze", g_map[1], 0x0F);
    check("und laesst den Rest belegt", g_map[2], 0xFF);

    /* --- F$SchBit: der Normalfall --- */
    reset();
    check("in einer leeren Bitmap wird sofort gefunden",
          (Q9_u32)Q9K_BitmapSearch(MAPBASE, MAPEND, 0, 4, &start, &count), 1);
    check("und zwar ganz am Anfang", start, 0);
    check("in der angeforderten Laenge", count, 4);

    /* Belegte Bits am Anfang muessen uebersprungen werden. */
    reset();
    Q9K_BitmapAllocate(MAPBASE, 0, 5);
    check("belegte Bits werden uebersprungen",
          (Q9_u32)Q9K_BitmapSearch(MAPBASE, MAPEND, 0, 4, &start, &count), 1);
    check("der Bereich beginnt hinter ihnen", start, 5);

    /* Die Suche beginnt bei firstBit, auch wenn davor Platz waere. */
    reset();
    check("die Suche beachtet das Startbit",
          (Q9_u32)Q9K_BitmapSearch(MAPBASE, MAPEND, 20, 4, &start, &count), 1);
    check("und beginnt nicht davor", start, 20);

    /* Ein zu kurzer Zwischenraum wird nicht genommen. */
    reset();
    Q9K_BitmapAllocate(MAPBASE, 3, 1);      /* Luecke 0..2 ist nur 3 Bit lang */
    check("eine zu kurze Luecke wird uebergangen",
          (Q9_u32)Q9K_BitmapSearch(MAPBASE, MAPEND, 0, 4, &start, &count), 1);
    check("der Bereich liegt dahinter", start, 4);

    /* Das Handbuch verlangt den ERSTEN passenden Block, nicht den
     * groessten -- hier gibt es beides, und der erste muss gewinnen. */
    reset();
    Q9K_BitmapAllocate(MAPBASE, 4, 1);      /* frei: 0..3 (4 Bit), dann 5..63 */
    check("der erste passende Block gewinnt, nicht der groesste",
          (Q9_u32)Q9K_BitmapSearch(MAPBASE, MAPEND, 0, 4, &start, &count), 1);
    check("also der vordere", start, 0);

    /* --- F$SchBit: der ungewoehnliche Fehlerfall ---
     *
     * Kein Fehlercode, sondern Anfang und Groesse des groessten
     * gefundenen Blocks. Genau das unterscheidet diesen Aufruf von jedem
     * anderen im Kernel. */
    reset();
    memset(g_map, 0xFF, sizeof g_map);
    Q9K_BitmapDeallocate(MAPBASE, 10, 3);   /* groesster freier Block: 3 Bit ab 10 */
    Q9K_BitmapDeallocate(MAPBASE, 40, 2);
    check("ohne passenden Block meldet die Suche Fehlschlag",
          (Q9_u32)Q9K_BitmapSearch(MAPBASE, MAPEND, 0, 8, &start, &count), 0);
    check("nennt aber den groessten gefundenen Block", start, 10);
    check("samt seiner Groesse -- keinen Fehlercode", count, 3);

    reset();
    memset(g_map, 0xFF, sizeof g_map);
    check("in einer vollen Bitmap ist der groesste Block leer",
          (Q9_u32)Q9K_BitmapSearch(MAPBASE, MAPEND, 0, 1, &start, &count), 0);
    check("mit Groesse 0", count, 0);

    /* Die Suche darf nicht ueber das Bitmapende hinauslaufen. */
    reset();
    check("eine Anforderung groesser als die Bitmap scheitert",
          (Q9_u32)Q9K_BitmapSearch(MAPBASE, MAPEND, 0, 1000, &start, &count), 0);
    check("und meldet die Gesamtgroesse als groessten Block",
          count, 8UL * 8UL);
    check("ein Startbit hinter dem Ende scheitert",
          (Q9_u32)Q9K_BitmapSearch(MAPBASE, MAPEND, 64, 1, &start, &count), 0);
    check("eine leere Bitmap scheitert",
          (Q9_u32)Q9K_BitmapSearch(MAPBASE, MAPBASE, 0, 1, &start, &count), 0);

    /* --- Der Zyklus, fuer den die drei gedacht sind --- */
    reset();
    Q9K_BitmapSearch(MAPBASE, MAPEND, 0, 6, &start, &count);
    Q9K_BitmapAllocate(MAPBASE, start, count);
    check("nach Suchen und Belegen ist der Bereich belegt",
          (Q9_u32)Q9K_BitmapTest(MAPBASE, start), 1);
    check("und das Bit dahinter noch frei",
          (Q9_u32)Q9K_BitmapTest(MAPBASE, start + count), 0);
    Q9K_BitmapSearch(MAPBASE, MAPEND, 0, 6, &start, &count);
    check("die naechste Suche liefert den Bereich dahinter", start, 6);
    Q9K_BitmapDeallocate(MAPBASE, 0, 6);
    Q9K_BitmapSearch(MAPBASE, MAPEND, 0, 6, &start, &count);
    check("nach dem Freigeben wieder den vorderen", start, 0);

    /* --- Die Bruecken --- */
    reset();
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_BASE, MAPBASE);
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_START, 2);
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_COUNT, 3);
    Q9K_SysAllBitImpl();
    check("Bruecke F$AllBit meldet Erfolg", Q9K_GetU32(Q9K_BITMAP_SCRATCH_OK), 1);
    check("und hat wirklich gesetzt", g_map[0], 0x38);

    Q9K_SysDelBitImpl();
    check("Bruecke F$DelBit meldet Erfolg", Q9K_GetU32(Q9K_BITMAP_SCRATCH_OK), 1);
    check("und hat wirklich geloescht", g_map[0], 0x00);

    /* Ein Nullzeiger ist der einzige Fall, den diese Bruecken abweisen --
     * ohne ihn wuerde auf Adresse 0 geschrieben. */
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_BASE, 0);
    Q9K_SysAllBitImpl();
    check("Bruecke F$AllBit weist einen Nullzeiger ab",
          Q9K_GetU32(Q9K_BITMAP_SCRATCH_OK), 0);
    check("mit E_BPADDR", Q9K_GetU32(Q9K_BITMAP_SCRATCH_ERROR), 0xD2);
    Q9K_SysDelBitImpl();
    check("Bruecke F$DelBit ebenso", Q9K_GetU32(Q9K_BITMAP_SCRATCH_OK), 0);

    reset();
    memset(g_map, 0xFF, sizeof g_map);
    Q9K_BitmapDeallocate(MAPBASE, 16, 4);
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_BASE, MAPBASE);
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_END, MAPEND);
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_START, 0);
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_COUNT, 4);
    Q9K_SysSchBitImpl();
    check("Bruecke F$SchBit findet den Bereich", Q9K_GetU32(Q9K_BITMAP_SCRATCH_OK), 1);
    check("und gibt sein Startbit zurueck", Q9K_GetU32(Q9K_BITMAP_SCRATCH_START), 16);
    check("und die Laenge", Q9K_GetU32(Q9K_BITMAP_SCRATCH_COUNT), 4);

    Q9K_SetU32(Q9K_BITMAP_SCRATCH_START, 0);
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_COUNT, 9);
    Q9K_SysSchBitImpl();
    check("bei zu grosser Anforderung meldet die Bruecke Fehlschlag",
          Q9K_GetU32(Q9K_BITMAP_SCRATCH_OK), 0);
    check("gibt aber den groessten Block zurueck, keinen Fehlercode",
          Q9K_GetU32(Q9K_BITMAP_SCRATCH_COUNT), 4);
    check("samt seinem Startbit", Q9K_GetU32(Q9K_BITMAP_SCRATCH_START), 16);

    printf("\n%s\n", failures ? "TESTS FEHLGESCHLAGEN" : "ALLE TESTS BESTANDEN");
    return failures ? 1 : 0;
}
