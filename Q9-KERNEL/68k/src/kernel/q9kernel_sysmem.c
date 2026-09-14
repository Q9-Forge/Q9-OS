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
 *   - "In user state, the system keeps track of memory allocated to a
 *     process ... automatically de-allocated ... when a process
 *     terminates" (F$SRtMem-Manual) ist NICHT implementiert -- kein
 *     Allokations-Tracking pro Prozess vorhanden (gleiches, bereits bei
 *     F$Exit dokumentiertes TODO: "kein echtes Speicher-Zurueckgeben").
 *     F$SRtMem selbst funktioniert trotzdem VOLLSTAENDIG fuer den
 *     expliziten Aufruf (im System-Zustand ohnehin PFLICHT laut Manual:
 *     "In system state, the process must explicitly return its
 *     memory") -- nur der automatische User-State-Fall beim
 *     Prozessende fehlt.
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

extern Q9_u32 Q9K_AllocMem(Q9_u32 requestedSize);      /* q9kernel_arena.c */
extern void   Q9K_FreeMem(Q9_u32 addr, Q9_u32 size);   /* q9kernel_arena.c */
extern Q9_u32 Q9K_AllocLargest(Q9_u32 *outSize);       /* q9kernel_arena.c, s. dortigen Kopfkommentar */

/* Dupliziert aus q9kernel_arena.c (dort privat/static) -- gleiche
 * schlanke Konvention wie ueberall in diesem Kernel: kleine Konstanten/
 * Formeln lieber lokal wiederholen als eine Cross-File-Abhaengigkeit auf
 * ein internes Implementierungsdetail aufzubauen. */
#define Q9K_ALLOC_GRANULARITY 16UL

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

/* Q9K_ProcSRqMem -- echte F$SRqMem-Kernlogik (s. Kopfkommentar).
 * Rueckgabe 1 = Erfolg (*outAddr und *outSize gueltig), 0 = Fehlschlag
 * (*outError gesetzt). requestedSize==0xFFFFFFFF (echtes d0.l=-1) loest
 * den Q9K_AllocLargest-Sonderpfad aus. */
int Q9K_ProcSRqMem(Q9_u32 requestedSize, Q9_u32 *outAddr, Q9_u32 *outSize, Q9_u16 *outError)
{
    Q9_u32 addr;
    Q9_u32 size;

    if (requestedSize == 0xFFFFFFFFUL) {
        addr = Q9K_AllocLargest(&size);
        if (addr == 0) {
            /* Arena komplett leer -- "kein RAM verfuegbar" passt inhaltlich
             * praeziser als E_MEMFUL (das eher "zu wenig fuer DIESE
             * Anfrage" bedeutet, hier gibt es aber ueberhaupt keine
             * Anfragegroesse zum Vergleichen). */
            *outError = (Q9_u16)Q9K_E_NORAM;
            return 0;
        }
    } else {
        size = Q9K_RoundUp16(requestedSize);
        addr = Q9K_AllocMem(requestedSize);
        if (addr == 0) {
            *outError = (Q9_u16)Q9K_E_MEMFUL;
            return 0;
        }
    }

    *outAddr = addr;
    *outSize = size;
    return 1;
}

/* Q9K_ProcSRtMem -- echte F$SRtMem-Kernlogik. Rundet identisch zu
 * Q9K_ProcSRqMem (reale Konvention, s. Kopfkommentar), reiner
 * Durchreicher an Q9K_FreeMem. */
void Q9K_ProcSRtMem(Q9_u32 addr, Q9_u32 size)
{
    Q9K_FreeMem(addr, Q9K_RoundUp16(size));
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
