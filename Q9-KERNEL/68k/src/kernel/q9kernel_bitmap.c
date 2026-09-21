/*
 * q9kernel_bitmap.c -- Q9-OS eigener Kernel: die drei Bitmap-Aufrufe
 *                      F$SchBit ($12), F$AllBit ($13), F$DelBit ($14)
 *                      (2026-09-18).
 *
 * Verifizierte ABI (68k_tech.pdf S.369, 394, 484):
 *
 *   F$SchBit  d0.w = Startbit der Suche, d1.w = benoetigte Bitzahl,
 *             (a0) = Bitmapanfang, (a1) = Bitmapende+1
 *             AUS: d0.w = gefundenes Startbit, d1.w = gefundene Bitzahl
 *   F$AllBit  d0.w = erstes Bit, d1.w = Anzahl, (a0) = Bitmapanfang
 *   F$DelBit  wie F$AllBit, loescht statt setzt
 *
 * Gesetztes Bit = belegt, geloeschtes Bit = frei; Bitnummern laufen von 0
 * bis n-1. Die drei bilden zusammen einen Zyklus: F$SchBit findet einen
 * freien Bereich, F$AllBit belegt ihn, F$DelBit gibt ihn zurueck. RBF
 * verwaltet damit die Clusterbelegung einer Platte.
 *
 * DER FEHLERFALL VON F$SchBit IST UNGEWOEHNLICH und hier bewusst genau so
 * nachgebaut: findet sich kein ausreichend grosser Bereich, setzt der
 * Aufruf zwar das Carry-Bit, liefert in d0.w/d1.w aber KEINEN Fehlercode,
 * sondern Anfang und Groesse des groessten gefundenen Blocks ("it returns
 * with the carry set, beginning bit number, and size of the largest block
 * found"). Ein Aufrufer kann danach also entscheiden, ob ihm weniger
 * genuegt. Wer hier stattdessen d1.w als Fehlernummer liest, bekommt
 * stillen Unsinn -- deshalb steht es hier so ausfuehrlich.
 *
 * ZUR BITREIHENFOLGE -- eine begruendete Annahme, kein Beleg:
 *
 * Das Handbuch sagt nur "bit numbers range from 0 to n-1" und laesst
 * offen, ob Bit 0 das hoechst- oder niederwertigste Bit von Byte 0 ist.
 * Diese Fassung nimmt MSB zuerst: Bit 0 ist $80 in Byte 0, Bit 7 ist $01,
 * Bit 8 ist $80 in Byte 1. Das ist die Reihenfolge, in der eine Bitmap
 * gelesen wie geschrieben dieselbe Reihenfolge wie der Datenstrom hat,
 * und die ueblichere Wahl bei OS-9-Plattenformaten.
 *
 * BELEGT IST SIE NICHT. Zwei Versuche sind gescheitert und stehen hier,
 * damit sie niemand wiederholt:
 *
 *   * Die Allokationsbitmap eines echten Q9-Abbilds liess sich nicht
 *     eindeutig lesen -- DD_MAP und DD_DIR widersprechen sich dort
 *     (Wurzelverzeichnis bei LSN 65, waehrend die Bitmap nach DD_MAP bis
 *     LSN 128 reichen muesste).
 *   * Eine Messung am laufenden System (F$AllBit auf einen genullten
 *     Puffer, Ergebnisbyte ausgegeben) lieferte $00 bei gemeldetem
 *     Erfolg. Die Kontrollprobe mit einem garantiert unbelegten Callcode
 *     lieferte korrekt Carry, der Messaufbau war also in Ordnung.
 *
 * WER DIESE AUFRUFE IM LAUFENDEN SYSTEM BEDIENT -- gemessen, nicht
 * vermutet: der echte Microware-IOMan registriert sich per F$SSvc fuer
 * $12 und $13; die Marker-Tabelle des Kernels ($1400 + Callcode) steht
 * dort auf 1. Das deckt sich mit dem Handbuch ("The IOMan module
 * implements F$SchBit/F$AllBit/F$DelBit"). Solange dieser IOMan geladen
 * ist, laeuft ALSO SEIN Handler und nicht der hier -- und zwar, wie die
 * Messung zeigt, ohne die Bitmap zu veraendern.
 *
 * Diese Datei ist deshalb der kernel-eigene Pfad fuer den Betrieb OHNE
 * fremden IOMan, den dieses Projekt anstrebt. Im aktuellen Aufbau ist sie
 * per Emulator nicht pruefbar: ein Live-Test wuerde IOMans Handler messen
 * und nicht diesen. Der Hosttest deckt sie vollstaendig ab; ein
 * Emulator-Nachweis steht aus, bis der Kernel ohne den fremden IOMan
 * bootet. Ehrlicher, als einen gruenen Marker zu erzeugen, der etwas
 * anderes prueft als er behauptet.
 *
 * Wer die Bitreihenfolge entscheiden will, disassembliert IOMans
 * F$AllBit-Handler -- so wurde auch das Datumsformat entschieden (s.
 * Q9K_SysFTime in q9kernel_entry.a). Solange das aussteht, gilt: die
 * Reihenfolge ist in allen drei Aufrufen dieselbe, ein Aufrufer, der nur
 * ueber diese Aufrufe auf die Bitmap zugreift, bekommt also in jedem Fall
 * stimmige Ergebnisse. Falsch waere sie nur fuer jemanden, der dieselbe
 * Bitmap ausserdem selbst interpretiert. Der Hosttest haelt sie fest,
 * damit sie sich nicht unbemerkt aendert.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

#ifndef Q9K_E_BPADDR
#define Q9K_E_BPADDR 0xD2U
#endif

/* Scratch-Bruecke, hinter der Systemuhr ($1B20-$1B3F). */
#ifndef Q9K_BITMAP_SCRATCH_START
#define Q9K_BITMAP_SCRATCH_START 0x1B40UL /* Q9_u32, d0.w EIN / AUS         */
#define Q9K_BITMAP_SCRATCH_COUNT 0x1B44UL /* Q9_u32, d1.w EIN / AUS         */
#define Q9K_BITMAP_SCRATCH_BASE  0x1B48UL /* Q9_u32, (a0) EIN               */
#define Q9K_BITMAP_SCRATCH_END   0x1B4CUL /* Q9_u32, (a1) EIN, nur F$SchBit */
#define Q9K_BITMAP_SCRATCH_ERROR 0x1B50UL /* Q9_u32, d1.w AUS bei Fehler    */
#define Q9K_BITMAP_SCRATCH_OK    0x1B54UL /* Q9_u32, 0/1                    */
#endif

#ifndef Q9K_CELL_ACCESSORS_PROVIDED
static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
#endif

static Q9_u8 Q9K_BitGetByte(Q9_u32 base, Q9_u32 index)
{
    return *(volatile Q9_u8 *)(base + index);
}

static void Q9K_BitSetByte(Q9_u32 base, Q9_u32 index, Q9_u8 value)
{
    *(volatile Q9_u8 *)(base + index) = value;
}

/* Die Maske eines Bits innerhalb seines Bytes -- die einzige Stelle, an
 * der die Reihenfolge festgelegt ist (s. Kopfkommentar). Wer sie aendern
 * muss, aendert sie hier und nirgends sonst. */
static Q9_u8 Q9K_BitMask(Q9_u32 bit)
{
    return (Q9_u8)(0x80U >> (bit & 7U));
}

/* Q9K_BitmapTest -- 1, wenn das Bit gesetzt (also belegt) ist. */
int Q9K_BitmapTest(Q9_u32 base, Q9_u32 bit)
{
    return (Q9K_BitGetByte(base, bit >> 3) & Q9K_BitMask(bit)) ? 1 : 0;
}

/* Q9K_BitmapAllocate -- F$AllBit: Bits als belegt markieren.
 *
 * Eine Anzahl von 0 ist kein Fehler, sondern schlicht nichts zu tun --
 * das Handbuch kennt keinen Fehlerfall fuer diesen Aufruf ausser dem
 * allgemeinen. */
void Q9K_BitmapAllocate(Q9_u32 base, Q9_u32 firstBit, Q9_u32 count)
{
    Q9_u32 bit;

    for (bit = firstBit; bit < firstBit + count; bit++) {
        Q9_u32 index = bit >> 3;
        Q9K_BitSetByte(base, index,
                       (Q9_u8)(Q9K_BitGetByte(base, index) | Q9K_BitMask(bit)));
    }
}

/* Q9K_BitmapDeallocate -- F$DelBit: Bits wieder freigeben. */
void Q9K_BitmapDeallocate(Q9_u32 base, Q9_u32 firstBit, Q9_u32 count)
{
    Q9_u32 bit;

    for (bit = firstBit; bit < firstBit + count; bit++) {
        Q9_u32 index = bit >> 3;
        Q9K_BitSetByte(base, index,
                       (Q9_u8)(Q9K_BitGetByte(base, index) & (Q9_u8)~Q9K_BitMask(bit)));
    }
}

/* Q9K_BitmapSearch -- F$SchBit: den ersten freien Bereich der gewuenschten
 * Laenge suchen, ab Bit firstBit.
 *
 * Rueckgabe 1 = gefunden; dann stehen in den beiden Ausgabezeigern Anfang und
 * die angeforderte Laenge. Rueckgabe 0 = nichts Passendes; dann tragen
 * dieselben beiden Ausgaben den GROESSTEN gefundenen freien Block, so wie
 * das Handbuch es fuer den Carry-Fall beschreibt. Sind ueberhaupt keine
 * freien Bits da, ist die Groesse 0.
 *
 * endExclusive ist die Adresse HINTER dem letzten Bitmapbyte, genau wie
 * (a1) im Aufruf. */
int Q9K_BitmapSearch(Q9_u32 base, Q9_u32 endExclusive, Q9_u32 firstBit,
                     Q9_u32 count, Q9_u32 *outStart, Q9_u32 *outCount)
{
    Q9_u32 totalBits;
    Q9_u32 bit;
    Q9_u32 runStart = 0UL, runLength = 0UL;
    Q9_u32 bestStart = 0UL, bestLength = 0UL;

    *outStart = 0UL;
    *outCount = 0UL;

    /* The ABI supplies raw pointers.  A null start must be rejected before
     * the first bitmap byte is read; an inverted/equal range is likewise an
     * empty bitmap.  F$SchBit reports both cases through its documented
     * carry-only failure result (zero largest run), not an error number. */
    if (base == 0UL || endExclusive <= base)
        return 0;
    totalBits = (endExclusive - base) * 8UL;
    if (firstBit >= totalBits)
        return 0;

    for (bit = firstBit; bit < totalBits; bit++) {
        if (Q9K_BitmapTest(base, bit)) {
            runLength = 0UL;
            continue;
        }
        if (runLength == 0UL)
            runStart = bit;
        runLength++;
        if (runLength > bestLength) {
            bestStart = runStart;
            bestLength = runLength;
        }
        /* Sobald die geforderte Laenge zusammenhaengend erreicht ist,
         * ist die Suche fertig -- das Handbuch verlangt den ERSTEN
         * passenden Block, nicht den besten. */
        if (count != 0UL && runLength >= count) {
            *outStart = runStart;
            *outCount = count;
            return 1;
        }
    }

    /* Nichts Passendes: der groesste gefundene Block, wie beschrieben. */
    *outStart = bestStart;
    *outCount = bestLength;
    return 0;
}

/* Q9K_SysBitmapImpl -- Bruecke fuer alle drei Aufrufe. Welcher gemeint
 * ist, sagt der Funktionscode in der Startzelle nicht; die ASM-Seite ruft
 * deshalb drei verschiedene Einstiege. */
void Q9K_SysSchBitImpl(void)
{
    Q9_u32 start = 0UL, found = 0UL;
    int ok = Q9K_BitmapSearch(Q9K_GetU32(Q9K_BITMAP_SCRATCH_BASE),
                              Q9K_GetU32(Q9K_BITMAP_SCRATCH_END),
                              Q9K_GetU32(Q9K_BITMAP_SCRATCH_START),
                              Q9K_GetU32(Q9K_BITMAP_SCRATCH_COUNT),
                              &start, &found);
    /* In BEIDEN Faellen zurueckgeschrieben -- im Fehlerfall tragen sie
     * den groessten gefundenen Block, nicht einen Fehlercode. */
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_START, start);
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_COUNT, found);
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_OK, (Q9_u32)ok);
}

void Q9K_SysAllBitImpl(void)
{
    Q9_u32 base = Q9K_GetU32(Q9K_BITMAP_SCRATCH_BASE);

    if (base == 0UL) {
        Q9K_SetU32(Q9K_BITMAP_SCRATCH_ERROR, Q9K_E_BPADDR);
        Q9K_SetU32(Q9K_BITMAP_SCRATCH_OK, 0UL);
        return;
    }
    Q9K_BitmapAllocate(base, Q9K_GetU32(Q9K_BITMAP_SCRATCH_START),
                       Q9K_GetU32(Q9K_BITMAP_SCRATCH_COUNT));
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_OK, 1UL);
}

void Q9K_SysDelBitImpl(void)
{
    Q9_u32 base = Q9K_GetU32(Q9K_BITMAP_SCRATCH_BASE);

    if (base == 0UL) {
        Q9K_SetU32(Q9K_BITMAP_SCRATCH_ERROR, Q9K_E_BPADDR);
        Q9K_SetU32(Q9K_BITMAP_SCRATCH_OK, 0UL);
        return;
    }
    Q9K_BitmapDeallocate(base, Q9K_GetU32(Q9K_BITMAP_SCRATCH_START),
                         Q9K_GetU32(Q9K_BITMAP_SCRATCH_COUNT));
    Q9K_SetU32(Q9K_BITMAP_SCRATCH_OK, 1UL);
}
