/* Host regression for the MMU region ledger. */
#include <stdio.h>
#include "q9kernel_config.h"
#include "q9kernel_mmu.c"

static int failures;

static void check(const char *name, int condition)
{
    if (condition)
        printf("[OK]   %s\n", name);
    else {
        printf("[FAIL] %s\n", name);
        ++failures;
    }
}

int main(void)
{
    Q9K_MmuReset();
    check("page-aligned Permit succeeds",
          Q9K_MmuPermit(1, 0x1203UL, 0x100UL, Q9K_MMU_READ | Q9K_MMU_WRITE) == 0);
    check("rounded region permits a full page read",
          Q9K_MmuCheck(1, 0x1000UL, 0x1000UL, Q9K_MMU_READ));
    check("missing write permission is rejected",
          !Q9K_MmuCheck(1, 0x1000UL, 0x1000UL, Q9K_MMU_EXEC));
    check("overlap is rejected",
          Q9K_MmuPermit(1, 0x1800UL, 0x100UL, Q9K_MMU_READ) == 2);
    check("different processes have independent maps",
          Q9K_MmuPermit(2, 0x1000UL, 0x1000UL, Q9K_MMU_EXEC) == 0
          && Q9K_MmuCheck(2, 0x1000UL, 0x1000UL, Q9K_MMU_EXEC));
    check("exact Protect removes the region",
          Q9K_MmuProtect(1, 0x1000UL, 0x1000UL) == 0
          && !Q9K_MmuCheck(1, 0x1000UL, 1, Q9K_MMU_READ));
    check("range wraparound is rejected",
          Q9K_MmuPermit(3, 0xFFFFFFF0UL, 0x40UL, Q9K_MMU_READ) == 1);
    check("partial Protect is rejected rather than over-unmapping",
          Q9K_MmuPermit(3, 0x4000UL, 0x2000UL, Q9K_MMU_READ) == 0
          && Q9K_MmuProtect(3, 0x5000UL, 0x1000UL) == 2
          && Q9K_MmuCheck(3, 0x4000UL, 0x2000UL, Q9K_MMU_READ));

    if (failures == 0)
        printf("ALLE TESTS BESTANDEN\n");
    return failures != 0;
}
