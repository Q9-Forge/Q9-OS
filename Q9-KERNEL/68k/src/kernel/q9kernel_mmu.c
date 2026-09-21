/*
 * q9kernel_mmu.c -- explicit MMU build-mode gate.
 *
 * The flat kernel is the only runnable mode until the 68030 PMMU backend is
 * complete.  Hardware mode deliberately fails initialization instead of
 * entering the scheduler with an uninitialized CRP/SRP or page tables.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9K_MmuU32;
typedef unsigned short Q9K_MmuU16;

#define Q9K_MMU_PAGE_SIZE   0x1000UL
#define Q9K_MMU_PAGE_MASK   (Q9K_MMU_PAGE_SIZE - 1UL)
#define Q9K_MMU_MAX_REGIONS 128
#define Q9K_MMU_READ        0x01U
#define Q9K_MMU_WRITE       0x02U
#define Q9K_MMU_EXEC        0x04U

typedef struct {
    Q9K_MmuU32 base;
    Q9K_MmuU32 size;
    Q9K_MmuU16 pid;
    unsigned char rights;
    unsigned char used;
} Q9K_MmuRegion;

static Q9K_MmuRegion q9k_mmu_regions[Q9K_MMU_MAX_REGIONS];

void Q9K_MmuReset(void)
{
    Q9K_MmuU16 i;
    for (i = 0; i < Q9K_MMU_MAX_REGIONS; ++i)
        q9k_mmu_regions[i].used = 0;
}

/* Convert a byte range to page boundaries without allowing 32-bit wrap. */
static int Q9K_MmuRoundRange(Q9K_MmuU32 address, Q9K_MmuU32 size,
                             Q9K_MmuU32 *outBase, Q9K_MmuU32 *outSize)
{
    Q9K_MmuU32 end;
    Q9K_MmuU32 roundedEnd;

    if (size == 0 || address > 0xFFFFFFFFUL - size)
        return 0;
    end = address + size;
    roundedEnd = (end + Q9K_MMU_PAGE_MASK) & ~Q9K_MMU_PAGE_MASK;
    if (roundedEnd < end || roundedEnd < address)
        return 0;
    *outBase = address & ~Q9K_MMU_PAGE_MASK;
    *outSize = roundedEnd - *outBase;
    return *outSize != 0;
}

static int Q9K_MmuOverlap(Q9K_MmuU32 a, Q9K_MmuU32 asize,
                          Q9K_MmuU32 b, Q9K_MmuU32 bsize)
{
    return a < b + bsize && b < a + asize;
}

/* Add one page-aligned region to a process map.  Return 0 on success,
 * 1=invalid range/rights, 2=overlap, 3=region table full. */
int Q9K_MmuPermit(Q9K_MmuU16 pid, Q9K_MmuU32 address, Q9K_MmuU32 size,
                  unsigned char rights)
{
    Q9K_MmuU32 base, roundedSize;
    Q9K_MmuU16 i;
    int freeSlot = -1;

    if (pid == 0 || (rights & (Q9K_MMU_READ | Q9K_MMU_WRITE | Q9K_MMU_EXEC)) == 0
        || !Q9K_MmuRoundRange(address, size, &base, &roundedSize))
        return 1;
    for (i = 0; i < Q9K_MMU_MAX_REGIONS; ++i) {
        if (!q9k_mmu_regions[i].used) {
            if (freeSlot < 0)
                freeSlot = i;
            continue;
        }
        if (q9k_mmu_regions[i].pid == pid
            && Q9K_MmuOverlap(base, roundedSize,
                              q9k_mmu_regions[i].base,
                              q9k_mmu_regions[i].size))
            return 2;
    }
    if (freeSlot < 0)
        return 3;
    q9k_mmu_regions[freeSlot].base = base;
    q9k_mmu_regions[freeSlot].size = roundedSize;
    q9k_mmu_regions[freeSlot].pid = pid;
    q9k_mmu_regions[freeSlot].rights = rights
        & (Q9K_MMU_READ | Q9K_MMU_WRITE | Q9K_MMU_EXEC);
    q9k_mmu_regions[freeSlot].used = 1;
    return 0;
}

/* Remove one exact page-aligned region.  Partial unmaps deliberately fail
 * until the PMMU table splitter is connected, avoiding silent over-unmap. */
int Q9K_MmuProtect(Q9K_MmuU16 pid, Q9K_MmuU32 address, Q9K_MmuU32 size)
{
    Q9K_MmuU32 base, roundedSize;
    Q9K_MmuU16 i;

    if (pid == 0 || !Q9K_MmuRoundRange(address, size, &base, &roundedSize))
        return 1;
    for (i = 0; i < Q9K_MMU_MAX_REGIONS; ++i) {
        if (q9k_mmu_regions[i].used && q9k_mmu_regions[i].pid == pid
            && q9k_mmu_regions[i].base == base
            && q9k_mmu_regions[i].size == roundedSize) {
            q9k_mmu_regions[i].used = 0;
            return 0;
        }
    }
    return 2;
}

/* Verify that one complete range has the requested access rights. */
int Q9K_MmuCheck(Q9K_MmuU16 pid, Q9K_MmuU32 address, Q9K_MmuU32 size,
                 unsigned char rights)
{
    Q9K_MmuU32 base, roundedSize;
    Q9K_MmuU16 i;

    if (pid == 0 || !Q9K_MmuRoundRange(address, size, &base, &roundedSize))
        return 0;
    for (i = 0; i < Q9K_MMU_MAX_REGIONS; ++i) {
        if (q9k_mmu_regions[i].used && q9k_mmu_regions[i].pid == pid
            && base >= q9k_mmu_regions[i].base
            && roundedSize <= q9k_mmu_regions[i].size
            && (rights & q9k_mmu_regions[i].rights) == rights)
            return 1;
    }
    return 0;
}

int Q9K_MmuInit(void)
{
    Q9K_MmuReset();
#if defined(Q9K_MMU_FLAT)
    return 0;
#else
    /* PMOVE/PTEST/PLOAD/PFLUSH and per-process roots are not implemented. */
    return 1;
#endif
}
