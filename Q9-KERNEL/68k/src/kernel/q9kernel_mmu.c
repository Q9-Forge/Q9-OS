/*
 * q9kernel_mmu.c -- explicit MMU build-mode gate.
 *
 * The flat kernel is the only runnable mode until the 68030 PMMU backend is
 * complete.  Hardware mode deliberately fails initialization instead of
 * entering the scheduler with an uninitialized CRP/SRP or page tables.
 */

#include "q9kernel_config.h"

int Q9K_MmuInit(void)
{
#if defined(Q9K_MMU_FLAT)
    return 0;
#else
    /* PMOVE/PTEST/PLOAD/PFLUSH and per-process roots are not implemented. */
    return 1;
#endif
}
