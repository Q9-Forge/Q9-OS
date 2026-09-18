/*
 * q9kernel_blkmap.c -- Q9-OS eigener Kernel: F$GBlkMp (Callcode $19,
 *                      2026-09-18).
 *
 * Verifizierte ABI (68k_tech.pdf S.434):
 *
 *   EIN  d0.l = Adresse, ab der berichtet werden soll
 *        d1.l = Puffergroesse in Byte
 *        (a0) = Puffer
 *   AUS  d0.l = kleinste Zuteilungsgroesse des Systems
 *        d1.l = Zahl der Speicherfragmente im System
 *        d2.l = beim Start gefundener Gesamtspeicher
 *        d3.l = derzeit freier Speicher
 *        (a0) = Fragmentliste
 *
 * Die Fragmentliste besteht aus Paaren {Adresse, Groesse} zu je 4 Byte,
 * abgeschlossen durch einen Nulleintrag -- so steht es als Figure D-4 im
 * Handbuch.
 *
 * WOFUER: "F$GBlkMp provides a status report concerning free system memory
 * for mfree and similar utilities." Es ist ein reiner Auskunftsaufruf. Das
 * Handbuch schaerft ausdruecklich ein, dass die gemeldeten Bloecke NIE
 * direkt benutzt werden duerfen -- wer Speicher will, nimmt F$SRqMem.
 * Deshalb kopiert diese Fassung auch nur heraus und gibt nichts preis,
 * woran sich etwas festhalten liesse.
 *
 * DIE FREILISTE, aus der berichtet wird, ist die des eigenen Allocators
 * (q9kernel_arena.c): einfach verkettet, Knoten {next, size} am
 * Blockanfang, next = 0 ist das Ende. Das Format ist rein intern -- kein
 * fremdes Modul sieht es je -- und wird hier in die vom Handbuch
 * vorgeschriebene Aussenform uebersetzt.
 *
 * ZWEI EHRLICHE GRENZEN:
 *
 *   * "Total RAM found by system at startup" ist die Zahl, die das
 *     Boot-ROM beim Einsprung uebergibt; sie wird in Q9K_RamSize
 *     festgehalten (q9kernel_entry.a, direkt nach dem Zero-Fill -- davor
 *     wuerde sie mitgeloescht). Das ist der Gesamtspeicher der Maschine,
 *     nicht der je fuer die Arena verfuegbare Teil; der Kernel selbst,
 *     die Bootkette und der Stack liegen darin.
 *   * Die Arena verschmilzt beim Freigeben benachbarte Bloecke noch nicht
 *     (s. Q9K_FreeMem). Die gemeldete Fragmentzahl kann deshalb hoeher
 *     sein als die Zahl der tatsaechlich getrennten freien Bereiche. Das
 *     ist keine Falschauskunft ueber den freien Speicher -- die Summe
 *     stimmt --, aber "Fragmente" heisst hier "Listenknoten".
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;

#ifndef Q9K_E_BPADDR
#define Q9K_E_BPADDR 0xD2U
#endif

/* Kleinste Zuteilungsgroesse -- dieselbe Konstante, mit der
 * q9kernel_arena.c rundet. Lokal dupliziert, gleiche schlanke Konvention
 * wie bei allen geteilten Konstanten hier. */
#ifndef Q9K_ALLOC_GRANULARITY
#define Q9K_ALLOC_GRANULARITY 16UL
#endif

#ifndef Q9_D_ARENA
#define Q9_D_ARENA      0x3FCUL
#endif
#ifndef Q9K_ARENA_HEAD
#define Q9K_ARENA_HEAD  (Q9_D_ARENA + 0x08UL)
#endif

#ifndef Q9K_RAMSIZE_ADDR
#define Q9K_RAMSIZE_ADDR 0x1B58UL
#endif

#ifndef Q9K_BLKMP_SCRATCH_BEGIN
#define Q9K_BLKMP_SCRATCH_BEGIN    0x1B5CUL /* Q9_u32, d0.l EIN  */
#define Q9K_BLKMP_SCRATCH_BUFSIZE  0x1B60UL /* Q9_u32, d1.l EIN  */
#define Q9K_BLKMP_SCRATCH_BUF      0x1B64UL /* Q9_u32, (a0) EIN  */
#define Q9K_BLKMP_SCRATCH_MINALLOC 0x1B68UL /* Q9_u32, d0.l AUS  */
#define Q9K_BLKMP_SCRATCH_FRAGS    0x1B6CUL /* Q9_u32, d1.l AUS  */
#define Q9K_BLKMP_SCRATCH_TOTALRAM 0x1B70UL /* Q9_u32, d2.l AUS  */
#define Q9K_BLKMP_SCRATCH_FREERAM  0x1B74UL /* Q9_u32, d3.l AUS  */
#define Q9K_BLKMP_SCRATCH_ERROR    0x1B78UL /* Q9_u32, d1.w AUS  */
#define Q9K_BLKMP_SCRATCH_OK       0x1B7CUL /* Q9_u32, 0/1       */
#endif

#ifndef Q9K_CELL_ACCESSORS_PROVIDED
static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
#endif

/* Die beiden Felder eines Freiblocks. Der Abstand ist sizeof(Q9_u32) und
 * NICHT hartkodiert 4 -- auf dem 64-Bit-Testhost ist Q9_u32 acht Byte
 * breit, und genau diese Falle hat in q9kernel_arena.c schon einmal
 * zugeschlagen (dort dokumentiert, 2026-08-18). */
#define Q9K_FB_NEXT(b) ((b) + 0UL * sizeof(Q9_u32))
#define Q9K_FB_SIZE(b) ((b) + 1UL * sizeof(Q9_u32))

/* Q9K_BlkMpCollect -- die Freiliste durchgehen und in den Puffer
 * uebersetzen.
 *
 * beginAt: es wird erst ab dieser Adresse berichtet (d0.l im Aufruf).
 * Bloecke davor zaehlen weder in die Fragmentzahl noch in die freie
 * Summe -- der Aufruf heisst "Address to begin reporting segments", er
 * beschraenkt also den Bericht, nicht die Suche.
 *
 * bufSize begrenzt, wie viele Paare geschrieben werden; der abschliessende
 * Nulleintrag muss hineinpassen, sonst wird ein Paar weniger abgelegt.
 * Ein Puffer, der nicht einmal den Nulleintrag fasst, ist ein Fehler --
 * sonst bliebe die Liste ohne Ende, und der Leser liefe darueber hinaus.
 *
 * Rueckgabe 1 = in Ordnung. */
int Q9K_BlkMpCollect(Q9_u32 buf, Q9_u32 bufSize, Q9_u32 beginAt,
                     Q9_u32 *outFrags, Q9_u32 *outFree, Q9_u16 *outError)
{
    Q9_u32 block = Q9K_GetU32(Q9K_ARENA_HEAD);
    Q9_u32 pairBytes = 2UL * sizeof(Q9_u32);
    Q9_u32 written = 0UL;
    Q9_u32 capacity;

    *outFrags = 0UL;
    *outFree = 0UL;

    if (buf == 0UL) {
        *outError = Q9K_E_BPADDR;
        return 0;
    }
    if (bufSize < pairBytes) {
        /* Nicht einmal Platz fuer den Abschluss. */
        *outError = Q9K_E_BPADDR;
        return 0;
    }
    /* Ein Paar bleibt immer fuer den Nulleintrag reserviert. */
    capacity = (bufSize / pairBytes) - 1UL;

    while (block != 0UL) {
        Q9_u32 size = Q9K_GetU32(Q9K_FB_SIZE(block));

        if (block >= beginAt) {
            (*outFrags)++;
            *outFree += size;
            if (written < capacity) {
                Q9K_SetU32(buf + written * pairBytes, block);
                Q9K_SetU32(buf + written * pairBytes + sizeof(Q9_u32), size);
                written++;
            }
        }
        block = Q9K_GetU32(Q9K_FB_NEXT(block));
    }

    /* Abschluss. Das Handbuch zeigt eine einzelne 0 hinter dem letzten
     * Paar; hier wird das ganze Paar genullt, damit ein Leser, der
     * paarweise laeuft, in jedem Fall sauber endet. */
    Q9K_SetU32(buf + written * pairBytes, 0UL);
    Q9K_SetU32(buf + written * pairBytes + sizeof(Q9_u32), 0UL);
    return 1;
}

/* Q9K_SysGBlkMpImpl -- Bruecke fuer F$GBlkMp. */
void Q9K_SysGBlkMpImpl(void)
{
    Q9_u32 frags = 0UL, freeRam = 0UL;
    Q9_u16 err = 0U;

    Q9K_SetU32(Q9K_BLKMP_SCRATCH_OK, 0UL);
    Q9K_SetU32(Q9K_BLKMP_SCRATCH_MINALLOC, Q9K_ALLOC_GRANULARITY);
    Q9K_SetU32(Q9K_BLKMP_SCRATCH_TOTALRAM, Q9K_GetU32(Q9K_RAMSIZE_ADDR));

    if (!Q9K_BlkMpCollect(Q9K_GetU32(Q9K_BLKMP_SCRATCH_BUF),
                          Q9K_GetU32(Q9K_BLKMP_SCRATCH_BUFSIZE),
                          Q9K_GetU32(Q9K_BLKMP_SCRATCH_BEGIN),
                          &frags, &freeRam, &err)) {
        Q9K_SetU32(Q9K_BLKMP_SCRATCH_ERROR, (Q9_u32)err);
        return;
    }

    Q9K_SetU32(Q9K_BLKMP_SCRATCH_FRAGS, frags);
    Q9K_SetU32(Q9K_BLKMP_SCRATCH_FREERAM, freeRam);
    Q9K_SetU32(Q9K_BLKMP_SCRATCH_OK, 1UL);
}
