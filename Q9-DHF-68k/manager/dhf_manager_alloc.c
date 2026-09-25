#include "dhf_manager_hostsim.h"
#include <stdlib.h>
#include <stdio.h>

/* Manager-side allocation helpers.
 * On real OS-9 68K this should call F$SRqMem/F$SRtMem to allocate from the system
 * pool. Here provide stubs that use malloc/free so tests can run in the host.
 *
 * Additionally export helpers that mimic returning a pointer to be stored in
 * the OS-9 path descriptor (pd->pd_opt) so the driver can access per-path state.
 */

#include <stdint.h>
#include "os9_sysalloc.h"
#include <stdio.h>
#include <stdlib.h>

void *dhf_manager_alloc(size_t size) {
    void *p = NULL;
    size_t s = size;
    if (os9_srqmem(&s, &p) != 0) {
        fprintf(stderr, "dhf_manager_alloc: os9_srqmem failed (%zu)\n", size);
        return NULL;
    }
    return p;
}

void dhf_manager_free(void *p) {
    if (!p) return;
    /* os9_srtmem expects size to track internal accounting; we don't track it here, pass 0 */
    os9_srtmem(0, p);
}

/* For host tests, return the pointer value as a 32-bit "emulator address" which
 * manager would have placed in the emulator path descriptor. This helps exercising
 * the driver-manager pointer handoff in tests.
 */
uint32_t dhf_manager_alloc_emulated_addr(size_t size) {
    return os9_srqmem_emulated_addr(size);
}

void *dhf_manager_resolve_emulated_addr(uint32_t addr) {
    if (addr == 0) return NULL;
    return (void *)(uintptr_t)addr;
}
