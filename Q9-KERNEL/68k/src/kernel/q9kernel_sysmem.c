/*
 * q9kernel_sysmem.c -- Q9-OS eigener Kernel: F$SRqMem/F$SRtMem
 *                      (Abschnitt "F$SRqMem/F$SRtMem", 2026-08-30,
 *                      zweiter Kandidat aus der "baut auf Vorhandenem
 *                      auf"-Gruppe, im Anschluss an F$Sleep --
 *                      Vorbereitung fuer die geplante Einbindung des
 *                      ECHTEN IOMan-Moduls, das genau diese beiden
 *                      Aufrufe fuer seine eigene Speicherverwaltung
 *                      braucht).
 *
 * Reale Register-/Verhaltenskonvention ECHT per Read gelesen (nicht
 * geraten), 68k_tech.pdf S. 503-506 (Callcodes 0x28/0x29, s.
 * modules/SYSCALL_MODULE_MAP.md):
 *
 *   F$SRqMem:
 *     IN  d0.l = angeforderte Byte-Anzahl (Sonderfall: -1 = groesster
 *                verfuegbarer freier Block wird komplett alloziert)
 *     OUT d0.l = tatsaechlich gewaehrte Byte-Anzahl (aufgerundet)
 *         (a2) = Zeiger auf den allozierten Block
 *     Fehler: E$MemFul, E$NoRAM
 *     "allocates a block of memory from the top of available RAM. The
 *     requested number of bytes is rounded up to a system defined
 *     blocksize (currently 16 bytes) ... memory always begins on an
 *     even boundary."
 *
 *   F$SRtMem:
 *     IN  d0.l = zurueckzugebende Byte-Anzahl, (a2) = Blockadresse
 *     OUT keine
 *     Fehler: E$BPAddr
 *     "de-allocates memory ... The number of bytes returned is rounded
 *     up to a system defined blocksize before the memory is returned.
 *     Rounding occurs identically to that done by F$SRqMem."
 *
 * Reine Wrapper um den bereits bestehenden, real getesteten Arena-
 * Allocator (q9kernel_arena.c, Q9K_AllocMem/Q9K_FreeMem/Q9K_AllocLargest
 * -- letztere neu, s. dortigen Kopfkommentar) -- KEIN neuer
 * Speicherverwaltungsmechanismus, nur die reale Syscall-Fassade davor.
 *
 * "The requested number of bytes are rounded up to a system defined
 * blocksize -- currently 16 bytes" deckt sich EXAKT mit dem bereits
 * bestehenden Q9K_ALLOC_GRANULARITY=16 (q9kernel_arena.c) -- reale
 * Fidelity ohne zusaetzlichen Aufwand, kein Zufall: der Allocator wurde
 * schon damals mit Blick auf dieses spaeter dokumentierte Verhalten
 * entworfen.
 *
 * EIGENE ENTSCHEIDUNGEN, dokumentiert:
 *   - User-State allocations are tracked in a fixed kernel table and are
 *     automatically returned by F$Exit.  System-state callers with no
 *     current process remain explicit owners and must call F$SRtMem.
 *     The table is intentionally bounded to 32 entries for this kernel
 *     stage; a dynamic list can replace it when the process subsystem is
 *     expanded.
 *   - E$BPAddr (F$SRtMem, ungueltiger Zeiger) wird NICHT geprueft --
 *     Q9K_FreeMem vertraut dem Aufrufer bereits (kein verstecktes
 *     Allokations-Header, s. dortigen Kopfkommentar) -- ein falscher
 *     Zeiger wuerde die Freiliste korrumpieren, exakt wie beim direkten
 *     C-Aufruf. Echte Eingabevalidierung braeuchte eine Allokations-
 *     Tabelle, die es hier nicht gibt -- TODO fuer spaeter.
 *   - "memory always begins on an even boundary" ist automatisch erfuellt
 *     (Q9K_ALLOC_GRANULARITY=16 ist ein Vielfaches von 2) -- keine
 *     zusaetzliche Pruefung noetig.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;

extern void Q9K_MemTraceEmit(Q9_u32 operation,
                             Q9_u32 requested,
                             Q9_u32 address,
                             Q9_u32 size,
                             Q9_u32 error,
                             Q9_u32 freeHead);
extern void Q9K_MemTraceBeginCapture(void);
extern void Q9K_MemTraceEndCapture(void);

#define Q9K_MEMTRACE_OP_REQUEST 1UL
#define Q9K_MEMTRACE_OP_RETURN  2UL
#ifndef Q9_D_FREEMEM
#define Q9_D_FREEMEM            0x0404UL
#endif
#ifndef Q9_D_PROC
#define Q9_D_PROC               0x004CUL
#endif

extern Q9_u32 Q9K_AllocMem(Q9_u32 requestedSize);      /* q9kernel_arena.c */
extern void   Q9K_FreeMem(Q9_u32 addr, Q9_u32 size);   /* q9kernel_arena.c */
extern Q9_u32 Q9K_AllocLargest(Q9_u32 *outSize);       /* q9kernel_arena.c, s. dortigen Kopfkommentar */

/* Dupliziert aus q9kernel_arena.c (dort privat/static) -- gleiche
 * schlanke Konvention wie ueberall in diesem Kernel: kleine Konstanten/
 * Formeln lieber lokal wiederholen als eine Cross-File-Abhaengigkeit auf
 * ein internes Implementierungsdetail aufzubauen. */
#define Q9K_ALLOC_GRANULARITY 16UL

/* Explicit F$SRqMem allocations made by a process are released when that
 * process exits.  The table is deliberately small and fixed-size for the
 * current kernel; it can later become a linked allocation list. */
#define Q9K_MEMOWNER_SLOTS 32UL
#ifndef Q9K_MEMOWNER_BASE
#define Q9K_MEMOWNER_BASE  0x1710UL
#endif
#define Q9K_MEMOWNER_STRIDE 12UL
#define Q9K_MEMOWNER_MAGIC (Q9K_MEMOWNER_BASE + Q9K_MEMOWNER_SLOTS * Q9K_MEMOWNER_STRIDE)
#define Q9K_MEMOWNER_OWNER(i) (Q9K_MEMOWNER_BASE + (i) * Q9K_MEMOWNER_STRIDE)
#define Q9K_MEMOWNER_ADDR(i)  (Q9K_MEMOWNER_OWNER(i) + 4UL)
#define Q9K_MEMOWNER_SIZE(i)  (Q9K_MEMOWNER_OWNER(i) + 8UL)

static Q9_u32 Q9K_RoundUp16(Q9_u32 n)
{
    return (n + (Q9K_ALLOC_GRANULARITY - 1)) & ~(Q9_u32)(Q9K_ALLOC_GRANULARITY - 1);
}

/* Reale Fehlercodes, internem Referenzmaterial (gleiche Primaerquelle wie
 * ueberall in diesem Kernel). E_MEMFUL bereits an anderer Stelle
 * (q9kernel_firstproc.c) verwendet, hier lokal dupliziert. */
#define Q9K_E_MEMFUL  0x00CFU   /* Process Memory Full */
#define Q9K_E_NORAM   0x00EDU   /* No RAM Available */

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
static Q9_u16 Q9K_GetU16(Q9_u32 addr) { return *(volatile Q9_u16 *)addr; }
static void   Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }

void Q9K_ProcMemTrackInit(void)
{
    Q9_u32 i;

    for (i = 0; i < Q9K_MEMOWNER_SLOTS; ++i) {
        Q9K_SetU32(Q9K_MEMOWNER_OWNER(i), 0UL);
        Q9K_SetU32(Q9K_MEMOWNER_ADDR(i), 0UL);
        Q9K_SetU32(Q9K_MEMOWNER_SIZE(i), 0UL);
    }
    Q9K_SetU32(Q9K_MEMOWNER_MAGIC, 0x514D454DU);
}

static void Q9K_ProcMemTrackEnsure(void)
{
    if (Q9K_GetU32(Q9K_MEMOWNER_MAGIC) != 0x514D454DU)
        Q9K_ProcMemTrackInit();
}

#if !defined(Q9K_TEST_HOST)
static void Q9K_ProcMemTrackAdd(Q9_u32 owner, Q9_u32 addr, Q9_u32 size)
{
    Q9_u32 i;

    if (owner == 0UL || addr == 0UL || size == 0UL)
        return;

    for (i = 0; i < Q9K_MEMOWNER_SLOTS; ++i) {
        if (Q9K_GetU32(Q9K_MEMOWNER_OWNER(i)) == 0UL) {
            Q9K_SetU32(Q9K_MEMOWNER_OWNER(i), owner);
            Q9K_SetU32(Q9K_MEMOWNER_ADDR(i), addr);
            Q9K_SetU32(Q9K_MEMOWNER_SIZE(i), size);
            return;
        }
    }
}

static void Q9K_ProcMemTrackRemove(Q9_u32 addr, Q9_u32 size)
{
    Q9_u32 i;

    for (i = 0; i < Q9K_MEMOWNER_SLOTS; ++i) {
        if (Q9K_GetU32(Q9K_MEMOWNER_ADDR(i)) == addr &&
            Q9K_GetU32(Q9K_MEMOWNER_SIZE(i)) == size) {
            Q9K_SetU32(Q9K_MEMOWNER_OWNER(i), 0UL);
            Q9K_SetU32(Q9K_MEMOWNER_ADDR(i), 0UL);
            Q9K_SetU32(Q9K_MEMOWNER_SIZE(i), 0UL);
            return;
        }
    }
}
#endif

void Q9K_ProcMemReleaseAll(Q9_u32 owner)
{
    Q9_u32 i;
    Q9_u32 addr;
    Q9_u32 size;

    if (owner == 0UL)
        return;

    Q9K_ProcMemTrackEnsure();

    for (i = 0; i < Q9K_MEMOWNER_SLOTS; ++i) {
        if (Q9K_GetU32(Q9K_MEMOWNER_OWNER(i)) != owner)
            continue;

        addr = Q9K_GetU32(Q9K_MEMOWNER_ADDR(i));
        size = Q9K_GetU32(Q9K_MEMOWNER_SIZE(i));
        Q9K_SetU32(Q9K_MEMOWNER_OWNER(i), 0UL);
        Q9K_SetU32(Q9K_MEMOWNER_ADDR(i), 0UL);
        Q9K_SetU32(Q9K_MEMOWNER_SIZE(i), 0UL);
        if (addr != 0UL && size != 0UL)
            Q9K_FreeMem(addr, size);
    }
}

/* Q9K_ProcSRqMem -- echte F$SRqMem-Kernlogik (s. Kopfkommentar).
 * Rueckgabe 1 = Erfolg (*outAddr und *outSize gueltig), 0 = Fehlschlag
 * (*outError gesetzt). requestedSize==0xFFFFFFFF (echtes d0.l=-1) loest
 * den Q9K_AllocLargest-Sonderpfad aus. */
int Q9K_ProcSRqMem(Q9_u32 requestedSize, Q9_u32 *outAddr, Q9_u32 *outSize, Q9_u16 *outError)
{
    Q9_u32 addr;
    Q9_u32 size;

#if !defined(Q9K_TEST_HOST)
    Q9K_ProcMemTrackEnsure();
#endif

    if (requestedSize == 0xFFFFFFFFUL) {
        Q9K_MemTraceBeginCapture();
        addr = Q9K_AllocLargest(&size);
        Q9K_MemTraceEndCapture();
        if (addr == 0) {
            /* Arena komplett leer -- "kein RAM verfuegbar" passt inhaltlich
             * praeziser als E_MEMFUL (das eher "zu wenig fuer DIESE
             * Anfrage" bedeutet, hier gibt es aber ueberhaupt keine
             * Anfragegroesse zum Vergleichen). */
            *outError = (Q9_u16)Q9K_E_NORAM;
            Q9K_MemTraceEmit(Q9K_MEMTRACE_OP_REQUEST, requestedSize, 0UL, 0UL,
                             (Q9_u32)Q9K_E_NORAM, Q9K_GetU32(Q9_D_FREEMEM));
            return 0;
        }
    } else {
        size = Q9K_RoundUp16(requestedSize);
        Q9K_MemTraceBeginCapture();
        addr = Q9K_AllocMem(requestedSize);
        Q9K_MemTraceEndCapture();
        if (addr == 0) {
            *outError = (Q9_u16)Q9K_E_MEMFUL;
            Q9K_MemTraceEmit(Q9K_MEMTRACE_OP_REQUEST, requestedSize, 0UL, 0UL,
                             (Q9_u32)Q9K_E_MEMFUL, Q9K_GetU32(Q9_D_FREEMEM));
            return 0;
        }
    }

    *outAddr = addr;
    *outSize = size;
#if !defined(Q9K_TEST_HOST)
    Q9K_ProcMemTrackAdd(Q9K_GetU32(Q9_D_PROC), addr, size);
#endif
    Q9K_MemTraceEmit(Q9K_MEMTRACE_OP_REQUEST, requestedSize, addr, size, 0UL,
                     Q9K_GetU32(Q9_D_FREEMEM));
    return 1;
}

/* Q9K_ProcSRtMem -- echte F$SRtMem-Kernlogik. Rundet identisch zu
 * Q9K_ProcSRqMem (reale Konvention, s. Kopfkommentar), reiner
 * Durchreicher an Q9K_FreeMem. */
void Q9K_ProcSRtMem(Q9_u32 addr, Q9_u32 size)
{
    Q9_u32 roundedSize = Q9K_RoundUp16(size);
#if !defined(Q9K_TEST_HOST)
    /* Keep the process-release entry point live in the object module and
     * provide a defensive kernel-internal escape hatch for future callers. */
    if (addr == 0UL && size == 0UL) {
        Q9K_ProcMemReleaseAll(Q9K_GetU32(Q9_D_PROC));
        return;
    }
#endif
#if !defined(Q9K_TEST_HOST)
    Q9K_ProcMemTrackRemove(addr, roundedSize);
#endif
    Q9K_MemTraceBeginCapture();
    Q9K_FreeMem(addr, roundedSize);
    Q9K_MemTraceEndCapture();
    Q9K_MemTraceEmit(Q9K_MEMTRACE_OP_RETURN, size, addr, roundedSize, 0UL,
                     Q9K_GetU32(Q9_D_FREEMEM));
}

/* Eigene Kernel-Global-Erweiterungen fuer die ASM<->C-Uebergabe von
 * Q9K_SysFSRqMem/Q9K_SysFSRtMem (q9kernel_entry.a) -- gleiches,
 * etabliertes Muster wie ueberall. Direkt hinter den Sleep-Scratch-
 * Feldern ($1328-$1330, q9kernel_procsleep.c) -- naechste freie Adresse
 * $1330. */
#ifndef Q9K_SRQMEM_SCRATCH_SIZEIN
#define Q9K_SRQMEM_SCRATCH_SIZEIN  0x1330UL   /* Q9_u32, d0.l EIN (F$SRqMem) */
#endif
#ifndef Q9K_SRQMEM_SCRATCH_ADDR
#define Q9K_SRQMEM_SCRATCH_ADDR    0x1334UL   /* Q9_u32, (a2) AUS (Erfolg) */
#endif
#ifndef Q9K_SRQMEM_SCRATCH_SIZEOUT
#define Q9K_SRQMEM_SCRATCH_SIZEOUT 0x1338UL   /* Q9_u32, d0.l AUS (Erfolg) */
#endif
#ifndef Q9K_SRQMEM_SCRATCH_ERROR
#define Q9K_SRQMEM_SCRATCH_ERROR   0x133CUL   /* Q9_u16, d1.w AUS (Fehlschlag) */
#endif
#ifndef Q9K_SRQMEM_SCRATCH_SUCCESS
#define Q9K_SRQMEM_SCRATCH_SUCCESS 0x1340UL   /* Q9_u16, 0=Fehlschlag/1=Erfolg */
#endif
#ifndef Q9K_SRTMEM_SCRATCH_ADDRIN
#define Q9K_SRTMEM_SCRATCH_ADDRIN  0x1344UL   /* Q9_u32, (a2) EIN (F$SRtMem) */
#endif
#ifndef Q9K_SRTMEM_SCRATCH_SIZEIN
#define Q9K_SRTMEM_SCRATCH_SIZEIN  0x1348UL   /* Q9_u32, d0.l EIN */
#endif

/* Q9K_SysSRqMemImpl/Q9K_SysSRtMemImpl -- duenne, PARAMETERLOSE Bruecken
 * zwischen den Assembler-Trampolinen (q9kernel_entry.a) und den echten
 * Mehrparameter-C-Funktionen oben -- gleiches, etabliertes Muster wie
 * Q9K_SysForkImpl/Q9K_SysWaitImpl/Q9K_SysExitImpl/Q9K_SysSleepImpl:
 * reiner C-zu-C-Aufruf hier (kein Risiko), die unverifizierte
 * Assembler<->C-Mehrparameter-Grenze wird ueber die obigen Scratch-
 * Adressen umgangen. */
void Q9K_SysSRqMemImpl(void)
{
    Q9_u32 sizeIn = Q9K_GetU32(Q9K_SRQMEM_SCRATCH_SIZEIN);
    Q9_u32 addr = 0, size = 0;
    Q9_u16 error = 0;

    if (Q9K_ProcSRqMem(sizeIn, &addr, &size, &error)) {
        Q9K_SetU32(Q9K_SRQMEM_SCRATCH_ADDR, addr);
        Q9K_SetU32(Q9K_SRQMEM_SCRATCH_SIZEOUT, size);
        Q9K_SetU16(Q9K_SRQMEM_SCRATCH_SUCCESS, 1);
    } else {
        Q9K_SetU16(Q9K_SRQMEM_SCRATCH_ERROR, error);
        Q9K_SetU16(Q9K_SRQMEM_SCRATCH_SUCCESS, 0);
    }
}

void Q9K_SysSRtMemImpl(void)
{
    Q9_u32 addr = Q9K_GetU32(Q9K_SRTMEM_SCRATCH_ADDRIN);
    Q9_u32 size = Q9K_GetU32(Q9K_SRTMEM_SCRATCH_SIZEIN);

    Q9K_ProcSRtMem(addr, size);
}
