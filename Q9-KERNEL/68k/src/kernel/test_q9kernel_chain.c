/*
 * test_q9kernel_chain.c -- Hosttest fuer F$Chain (q9kernel_chain.c,
 *                          2026-09-18).
 *
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_chain test_q9kernel_chain.c && ./test_chain
 *
 * Der Schwerpunkt liegt auf der REIHENFOLGE: dass im Fehlerfall nichts
 * abgebaut wird, und dass der alte Speicherblock erst im zweiten Schritt
 * freikommt. Beides laesst sich auf dem Host praezise pruefen, weil
 * Modulverzeichnis und Allocator hier als mitzaehlende Stubs vorliegen.
 */

#include <stdio.h>
#include <string.h>

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

static unsigned char g_cells[0x400];
#define CELL(off) ((unsigned long)(g_cells + (off)))

#define Q9_D_PROC                  CELL(0x000)
#define Q9K_CHAIN_SCRATCH_TYPELANG CELL(0x020)
#define Q9K_CHAIN_SCRATCH_ADDMEM   CELL(0x040)
#define Q9K_CHAIN_SCRATCH_PARAMSZ  CELL(0x060)
#define Q9K_CHAIN_SCRATCH_NUMPATHS CELL(0x080)
#define Q9K_CHAIN_SCRATCH_PRIORITY CELL(0x0A0)
#define Q9K_CHAIN_SCRATCH_NAME     CELL(0x0C0)
#define Q9K_CHAIN_SCRATCH_PARAM    CELL(0x0E0)
#define Q9K_CHAIN_SCRATCH_ERROR    CELL(0x100)
#define Q9K_CHAIN_SCRATCH_OK       CELL(0x120)
#define Q9K_CHAIN_SCRATCH_OLDBLOCK CELL(0x140)
#define Q9K_CHAIN_SCRATCH_OLDSIZE  CELL(0x160)
#define Q9K_CHAIN_SCRATCH_OLDMOD   CELL(0x180)

/* Deskriptor-Abstaende auf Testbreite -- Q9_u32 ist hier acht Byte. */
#define Q9K_PROCDESC_SAVEDSP_OFF   0x08UL
#define Q9K_PROCDESC_PRIORITY_OFF  0x18UL
#define Q9K_PROCDESC_SIGNAL_OFF    0x20UL
#define Q9K_PROCDESC_SIGVEC_OFF    0x28UL
#define Q9K_PROCDESC_SIGDAT_OFF    0x30UL
#define Q9K_PROCDESC_MODHDR_OFF    0x38UL
#define Q9K_PROCDESC_ALLOCBASE_OFF 0x40UL
#define Q9K_PROCDESC_ALLOCSIZE_OFF 0x48UL
#define Q9K_PROCDESC_ENTRYPC_OFF   0x50UL
#define Q9K_PROCDESC_PATH_OFF      0x60UL

/* --- Stubs, die mitzaehlen --------------------------------------- */
static unsigned char g_module[64];         /* "neues" Modul */
static unsigned char g_oldModule[64];      /* altes Primaermodul */
static unsigned char g_newBlock[512];
static int g_linkFails, g_allocFails;
static int g_linkCalls, g_unlinkCalls, g_allocCalls, g_freeCalls;
static unsigned long g_unlinkLast, g_freeLastAddr, g_freeLastSize;
static unsigned long g_appliedTo;

#define MODULE    ((Q9_u32)(unsigned long)g_module)
#define OLDMODULE ((Q9_u32)(unsigned long)g_oldModule)
#define NEWBLOCK  ((Q9_u32)(unsigned long)g_newBlock)
#define OLDBLOCK  0x99000000UL             /* nie dereferenziert */
#define OLDSIZE   0x400UL

Q9_u32 Q9K_ModDirLinkByName(Q9_u16 typeLang, const char *name)
{
    (void)typeLang; (void)name;
    g_linkCalls++;
    return g_linkFails ? 0UL : MODULE;
}

void Q9K_ModDirUnlinkByHeader(Q9_u32 hdrAddr)
{
    g_unlinkCalls++;
    g_unlinkLast = hdrAddr;
}

Q9_u32 Q9K_AllocMem(Q9_u32 size)
{
    (void)size;
    g_allocCalls++;
    return g_allocFails ? 0UL : NEWBLOCK;
}

void Q9K_FreeMem(Q9_u32 addr, Q9_u32 size)
{
    g_freeCalls++;
    g_freeLastAddr = addr;
    g_freeLastSize = size;
}

void Q9K_ApplyInitializedData(Q9_u32 hdrAddr, Q9_u32 block)
{
    (void)hdrAddr;
    g_appliedTo = block;
}

/* Modulkopffelder im Stub-Modul: Daten 0x40, Stack 0x80, Einstieg 0x10.
 *
 * Die Offsets sind die ECHTEN aus q9kernel_firstproc.c. Das ist hier kein
 * Detail: ein erster Entwurf hatte sie geraten (0x30/0x34/0x28), der
 * Hosttest lief damit gruen -- er benutzte ja dieselben falschen Werte --
 * und erst der Emulator zeigte den Absturz. Ein Test, der die Annahme des
 * Getesteten teilt, prueft nichts. */
Q9_u32 Q9K_ReadHdrU32BE(Q9_u32 addr)
{
    Q9_u32 off = addr - MODULE;
    if (off == 0x38UL) return 0x40UL;   /* M$Mem   */
    if (off == 0x3CUL) return 0x80UL;   /* M$Stack */
    if (off == 0x30UL) return 0x10UL;   /* M$Exec  */
    return 0UL;
}

static unsigned long g_frameRegs[16];
static unsigned long g_frameBase;
void Q9K_SetFrameReg(Q9_u32 frameBase, Q9_u32 index, Q9_u32 value)
{
    g_frameBase = frameBase;
    if (index < 16) g_frameRegs[index] = value;
}

Q9_u16 Q9K_ProcIdForDesc(Q9_u32 desc) { (void)desc; return 7; }

#include "q9kernel_chain.c"

static int failures;
static unsigned char g_desc[256];
#define DESC ((Q9_u32)(unsigned long)g_desc)

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
    memset(g_desc, 0, sizeof g_desc);
    memset(g_newBlock, 0, sizeof g_newBlock);
    memset(g_frameRegs, 0, sizeof g_frameRegs);
    g_linkFails = g_allocFails = 0;
    g_linkCalls = g_unlinkCalls = g_allocCalls = g_freeCalls = 0;
    g_unlinkLast = g_freeLastAddr = g_freeLastSize = 0; g_appliedTo = 0;

    /* Ein laufender Prozess mit altem Programm und altem Speicher. */
    Q9K_SetU32(Q9_D_PROC, DESC);
    Q9K_SetU32(DESC + Q9K_PROCDESC_MODHDR_OFF, OLDMODULE);
    Q9K_SetU32(DESC + Q9K_PROCDESC_ALLOCBASE_OFF, OLDBLOCK);
    Q9K_SetU32(DESC + Q9K_PROCDESC_ALLOCSIZE_OFF, OLDSIZE);
    Q9K_SetU8(DESC + Q9K_PROCDESC_PRIORITY_OFF, 42);
    /* Ein anstehendes Signal und ein Intercept des alten Programms. */
    Q9K_SetU16(DESC + Q9K_PROCDESC_SIGNAL_OFF, 3);
    Q9K_SetU32(DESC + Q9K_PROCDESC_SIGVEC_OFF, 0xCAFEUL);
    Q9K_SetU32(DESC + Q9K_PROCDESC_SIGDAT_OFF, 0xBEEFUL);
    /* Ein offener Pfad -- er muss den Chain ueberleben. */
    Q9K_SetU16(DESC + Q9K_PROCDESC_PATH_OFF, 5);
}

int main(void)
{
    Q9_u16 err;

    printf("== F$Chain ==\n");

    /* --- Der Erfolgsfall --- */
    reset();
    err = 0;
    check("der Aufbau gelingt",
          (Q9_u32)Q9K_ProcChain(1, 0, 0, 0, 0, 0, 0, &err), 1);
    check("das neue Modul wurde gelinkt", (Q9_u32)g_linkCalls, 1);
    check("Speicher wurde angefordert", (Q9_u32)g_allocCalls, 1);
    check("und die initialisierten Daten dorthin gelegt", g_appliedTo, NEWBLOCK);
    check("der Deskriptor zeigt aufs neue Modul",
          Q9K_GetU32(DESC + Q9K_PROCDESC_MODHDR_OFF), MODULE);
    check("und auf den neuen Speicher",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ALLOCBASE_OFF), NEWBLOCK);
    check("mit der Groesse aus dem neuen Modulkopf",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ALLOCSIZE_OFF), 0x40 + 0x80);

    /* Der springende Punkt der Reihenfolge: das ALTE ist noch da. */
    check("das alte Modul ist noch NICHT freigegeben", (Q9_u32)g_unlinkCalls, 0);
    check("und der alte Speicher auch nicht", (Q9_u32)g_freeCalls, 0);
    check("beides liegt fuer den zweiten Schritt bereit",
          Q9K_GetU32(Q9K_CHAIN_SCRATCH_OLDBLOCK), OLDBLOCK);
    check("samt Groesse", Q9K_GetU32(Q9K_CHAIN_SCRATCH_OLDSIZE), OLDSIZE);
    check("und altem Modul", Q9K_GetU32(Q9K_CHAIN_SCRATCH_OLDMOD), OLDMODULE);

    /* Intercepts und anstehende Signale sind geloescht -- sie gehoerten
     * zum alten Programm. */
    check("ein anstehendes Signal ist geloescht",
          (Q9_u32)Q9K_GetU16(DESC + Q9K_PROCDESC_SIGNAL_OFF), 0);
    check("der Intercept-Vektor ebenfalls",
          Q9K_GetU32(DESC + Q9K_PROCDESC_SIGVEC_OFF), 0);
    check("und dessen Datenzeiger", Q9K_GetU32(DESC + Q9K_PROCDESC_SIGDAT_OFF), 0);

    /* Offene Pfade bleiben -- genau dafuer gibt es F$Chain. */
    check("ein offener Pfad ueberlebt den Chain",
          (Q9_u32)Q9K_GetU16(DESC + Q9K_PROCDESC_PATH_OFF), 5);

    /* Die Prozess-ID bleibt dieselbe: es ist derselbe Prozess. */
    check("das neue Programm sieht dieselbe Prozess-ID", g_frameRegs[0], 7);
    check("und den Einstiegspunkt des neuen Moduls",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ENTRYPC_OFF), MODULE + 0x10);
    check("a3 zeigt auf das neue Primaermodul", g_frameRegs[11], MODULE);
    check("die Prioritaet bleibt erhalten, wenn keine genannt wird",
          (Q9_u32)Q9K_GetU8(DESC + Q9K_PROCDESC_PRIORITY_OFF), 42);

    /* --- Der zweite Schritt raeumt auf --- */
    Q9K_SysChainReleaseImpl();
    check("jetzt wird das alte Modul freigegeben", (Q9_u32)g_unlinkCalls, 1);
    check("und zwar das richtige", g_unlinkLast, OLDMODULE);
    check("der alte Speicher ebenfalls", (Q9_u32)g_freeCalls, 1);
    check("mit der richtigen Adresse", g_freeLastAddr, OLDBLOCK);
    check("und der richtigen Groesse", g_freeLastSize, OLDSIZE);
    check("die Merkzellen sind danach leer",
          Q9K_GetU32(Q9K_CHAIN_SCRATCH_OLDBLOCK), 0);

    /* Ein zweiter Aufruf darf nicht noch einmal freigeben. */
    Q9K_SysChainReleaseImpl();
    check("ein zweiter Aufruf gibt nichts doppelt frei", (Q9_u32)g_freeCalls, 1);

    /* --- Fehlerfaelle: der Aufrufer MUSS unversehrt bleiben --- */
    reset();
    g_linkFails = 1;
    err = 0;
    check("ein unbekanntes Modul laesst den Chain scheitern",
          (Q9_u32)Q9K_ProcChain(1, 0, 0, 0, 0, 0, 0, &err), 0);
    check("mit E_MNF ($DD)", (Q9_u32)err, 0xDD);
    check("es wurde kein Speicher angefordert", (Q9_u32)g_allocCalls, 0);
    check("nichts wurde freigegeben", (Q9_u32)g_freeCalls, 0);
    check("der Prozess behaelt sein altes Modul",
          Q9K_GetU32(DESC + Q9K_PROCDESC_MODHDR_OFF), OLDMODULE);
    check("und seinen alten Speicher",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ALLOCBASE_OFF), OLDBLOCK);
    check("sein anstehendes Signal bleibt unangetastet",
          (Q9_u32)Q9K_GetU16(DESC + Q9K_PROCDESC_SIGNAL_OFF), 3);

    reset();
    g_allocFails = 1;
    err = 0;
    check("fehlender Speicher laesst den Chain scheitern",
          (Q9_u32)Q9K_ProcChain(1, 0, 0, 0, 0, 0, 0, &err), 0);
    check("mit E_MEMFUL", (Q9_u32)err, 0xCF);
    /* Das frisch gelinkte Modul muss wieder losgelassen werden, sonst
     * bleibt sein Linkzaehler fuer immer zu hoch. */
    check("das neu gelinkte Modul wird zurueckgegeben", (Q9_u32)g_unlinkCalls, 1);
    check("und zwar das NEUE, nicht das alte", g_unlinkLast, MODULE);
    check("der Prozess behaelt sein altes Modul",
          Q9K_GetU32(DESC + Q9K_PROCDESC_MODHDR_OFF), OLDMODULE);
    check("und seinen alten Speicher",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ALLOCBASE_OFF), OLDBLOCK);

    reset();
    Q9K_SetU32(Q9_D_PROC, 0);
    err = 0;
    check("ohne laufenden Prozess wird abgewiesen",
          (Q9_u32)Q9K_ProcChain(1, 0, 0, 0, 0, 0, 0, &err), 0);
    check("mit E_BPADDR", (Q9_u32)err, 0xD2);

    /* --- Parameter und Prioritaet --- */
    reset();
    {
        static unsigned char params[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        err = 0;
        Q9K_ProcChain(1, 0, 8, 0, (Q9_u32)(unsigned long)params, 9, 3, &err);
        /* HOST-ARTEFAKT, hier einmal ausgemessen und festgehalten: auf
         * diesem Testhost ist Q9_u32 acht Byte breit, die drei
         * Schreibzugriffe auf den Exception-Rahmen reichen deshalb bis in
         * die ersten Parameterbytes hinein. Auf dem echten 32-Bit-Ziel
         * endet der Rahmen genau an der Parametergrenze. Geprueft werden
         * darum die Bytes DAHINTER -- dieselbe Vorsichtsmassnahme wie in
         * q9kernel_arena.c, wo derselbe Breitenunterschied schon einmal
         * einen echten Fehler verdeckt hat. */
        check("die Parameter landen am oberen Ende des Blocks",
              (Q9_u32)g_newBlock[0x40 + 0x80 + 3], 4);
        check("vollstaendig bis zum letzten Byte",
              (Q9_u32)g_newBlock[0x40 + 0x80 + 7], 8);
        check("d5 nennt ihre Groesse", g_frameRegs[5], 8);
        check("eine genannte Prioritaet setzt sich durch",
              (Q9_u32)Q9K_GetU8(DESC + Q9K_PROCDESC_PRIORITY_OFF), 9);
        check("d3 traegt die Pfadzahl weiter", g_frameRegs[3], 3);
    }

    /* Zusaetzlicher Speicher wird dazugerechnet. */
    reset();
    err = 0;
    Q9K_ProcChain(1, 0x100, 0, 0, 0, 0, 0, &err);
    check("zusaetzlicher Speicher wird eingerechnet",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ALLOCSIZE_OFF), 0x40 + 0x80 + 0x100);

    /* --- Die Bruecke --- */
    reset();
    Q9K_SetU32(Q9K_CHAIN_SCRATCH_TYPELANG, 1);
    Q9K_SysChainImpl();
    check("Bruecke meldet Erfolg", Q9K_GetU32(Q9K_CHAIN_SCRATCH_OK), 1);

    reset();
    g_linkFails = 1;
    Q9K_SetU32(Q9K_CHAIN_SCRATCH_TYPELANG, 1);
    Q9K_SysChainImpl();
    check("Bruecke meldet den Fehlschlag", Q9K_GetU32(Q9K_CHAIN_SCRATCH_OK), 0);
    check("mit dem Fehlercode E_MNF ($DD)", Q9K_GetU32(Q9K_CHAIN_SCRATCH_ERROR), 0xDD);

    printf("\n%s\n", failures ? "TESTS FEHLGESCHLAGEN" : "ALLE TESTS BESTANDEN");
    return failures ? 1 : 0;
}
