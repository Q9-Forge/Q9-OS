/* Q9-DHF manager skeleton (Q9-DHF-68k)
 * Forwards Q9 filemanager calls to dhfdrv-68k which maps to the host filesystem.
 * Minimal stubs to be implemented further.
 */

#include "../descriptor/dhf_descriptor.h"
#include "../driver/dhfdrv-68k.h"
#include "dhf_manager_hostsim.h"
#include "path_desc.h"
#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dhf_manager_init(void) {
    return dhfdrv_init(dhf_descriptor_get_basepath());
}

/* For host-testing we keep a small table of active path descriptors indexed by
 * manager fd (which for simplicity we mirror to host fd + 1000 offset). This is
 * a test harness only; manager in OS-9 would use kernel path descriptors.
 */
#define MANAGER_FD_OFFSET 1000
#define MAX_PATH_DESC 64
static dhf_path_desc_t *g_path_table[MAX_PATH_DESC];

static int alloc_manager_fd(dhf_path_desc_t *pd) {
    for (int i = 0; i < MAX_PATH_DESC; ++i) {
        if (!g_path_table[i]) {
            g_path_table[i] = pd;
            return MANAGER_FD_OFFSET + i;
        }
    }
    return -1;
}

static dhf_path_desc_t *lookup_pd(int manager_fd) {
    int idx = manager_fd - MANAGER_FD_OFFSET;
    if (idx < 0 || idx >= MAX_PATH_DESC) return NULL;
    return g_path_table[idx];
}

static void free_manager_fd(int manager_fd) {
    int idx = manager_fd - MANAGER_FD_OFFSET;
    if (idx < 0 || idx >= MAX_PATH_DESC) return;
    if (g_path_table[idx]) {
        dhf_manager_free(g_path_table[idx]);
        g_path_table[idx] = NULL;
    }
}

int dhf_manager_open(const char *path, int flags) {
    dhf_path_desc_t *pd = dhf_manager_alloc(sizeof(*pd));
    if (!pd) return -1;
    pd->flags = flags;
    pd->host_fd = dhfdrv_open(path, flags);
    if (pd->host_fd < 0) {
        dhf_manager_free(pd);
        return -1;
    }
    int mfd = alloc_manager_fd(pd);
    if (mfd < 0) {
        dhfdrv_close(pd->host_fd);
        dhf_manager_free(pd);
        return -1;
    }
    /* Simulate writing the emulated address (as 32-bit value) into the OS-9 path descriptor
     * so the driver can read it. In a real OS-9 manager this would write to pd->pd_opt.
     */
    uint32_t emu_addr = dhf_manager_alloc_emulated_addr(sizeof(dhf_path_desc_t));
    /* Copy our local pd contents into the allocated emulated block so driver can read it */
    void *resolved = dhf_manager_resolve_emulated_addr(emu_addr);
    if (resolved) memcpy(resolved, pd, sizeof(dhf_path_desc_t));

    /* For host tests also store the emu_addr in pd->private for reference */
    pd->private = (void*)(uintptr_t)emu_addr;

    return mfd;
}

int dhf_manager_close(int fd) {
    dhf_path_desc_t *pd = lookup_pd(fd);
    if (!pd) return -1;
    /* Use PD-based driver close so driver resolves PD pointer itself */
    uint32_t emu_addr = (uint32_t)(uintptr_t)pd->private;
    int res = dhfdrv_close_pd(emu_addr);
    free_manager_fd(fd);
    return res;
}

ssize_t dhf_manager_read(int fd, void *buf, size_t count) {
    dhf_path_desc_t *pd = lookup_pd(fd);
    if (!pd) return -1;
    uint32_t emu_addr = (uint32_t)(uintptr_t)pd->private;
    return dhfdrv_read_pd(emu_addr, buf, count);
}

ssize_t dhf_manager_write(int fd, const void *buf, size_t count) {
    dhf_path_desc_t *pd = lookup_pd(fd);
    if (!pd) return -1;
    uint32_t emu_addr = (uint32_t)(uintptr_t)pd->private;
    return dhfdrv_write_pd(emu_addr, buf, count);
}

int dhf_manager_getstat(const char *path, void *statbuf) { return dhfdrv_getstat(path, statbuf); }
int dhf_manager_setstat(const char *path, void *statbuf) { return dhfdrv_setstat(path, statbuf); }
int dhf_manager_chdir(const char *path) { return dhfdrv_chdir(path); }
int dhf_manager_mkdir(const char *path, int mode) { return dhfdrv_mkdir(path, mode); }
int dhf_manager_rmdir(const char *path) { return dhfdrv_rmdir(path); }
int dhf_manager_unlink(const char *path) { return dhfdrv_unlink(path); }
int dhf_manager_rename(const char *oldp, const char *newp) { return dhfdrv_rename(oldp,newp); }
int dhf_manager_opendir(const char *path) { return dhfdrv_opendir(path); }
int dhf_manager_readdir(int dirfd, void *entry) { return dhfdrv_readdir(dirfd, entry); }

