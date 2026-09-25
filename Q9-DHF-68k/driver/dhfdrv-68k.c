/* dhfdrv-68k: driver skeleton mapping to host filesystem
 * Implement platform-specific host FS operations and confinement here.
 */

#include "dhfdrv-68k.h"
#include <string.h>
#include <stdio.h>

int dhfdrv_init(const char *basepath) { (void)basepath; /* TODO: init host FS mapping */ return 0; }

/* Attach a manager-allocated per-path control block emulated-address to driver
 * state. In the real system manager will store a pointer in the path descriptor
 * (pd->pd_opt) which the driver can read. For our tests the manager will call
 * dhf_manager_alloc_emulated_addr() and pass the 32-bit value into the driver
 * via higher-level wiring; the driver can resolve it below.
 */

#include <stdint.h>
#include "../manager/path_desc.h"

void *dhfdrv_resolve_pd_from_emulated_addr(uint32_t emu_addr) {
    void *p = dhf_manager_resolve_emulated_addr(emu_addr);
    /* Basic validation: ensure pointer points to expected dhf_path_desc_t layout by checking non-zero host_fd */
    if (!p) return NULL;
    dhf_path_desc_t *pd = (dhf_path_desc_t *)p;
    if (pd->host_fd == 0) return NULL;
    return p;
}

#include "../host_simulator/fs_ops.h"

int dhfdrv_open(const char *path, int flags) {
    const char *base = dhf_descriptor_get_basepath();
    if (!base) return -1;
    // map flags roughly: manager passes O_RDONLY/O_WRONLY/O_RDWR|O_CREAT etc.
    int host_fd = dhf_host_open(base, path, flags, 0666);
    return host_fd;
}

int dhfdrv_close(int fd) {
    return dhf_host_close(fd);
}

ssize_t dhfdrv_read(int fd, void *buf, size_t count) {
    return dhf_host_read(fd, buf, count);
}

ssize_t dhfdrv_write(int fd, const void *buf, size_t count) {
    return dhf_host_write(fd, buf, count);
}

/* PD-based helpers */
int dhfdrv_close_pd(uint32_t emu_pd_addr) {
    void *p = dhfdrv_resolve_pd_from_emulated_addr(emu_pd_addr);
    if (!p) return -1;
    dhf_path_desc_t *pd = (dhf_path_desc_t*)p;
    return dhfdrv_close(pd->host_fd);
}

ssize_t dhfdrv_read_pd(uint32_t emu_pd_addr, void *buf, size_t count) {
    void *p = dhfdrv_resolve_pd_from_emulated_addr(emu_pd_addr);
    if (!p) return -1;
    dhf_path_desc_t *pd = (dhf_path_desc_t*)p;
    return dhfdrv_read(pd->host_fd, buf, count);
}

ssize_t dhfdrv_write_pd(uint32_t emu_pd_addr, const void *buf, size_t count) {
    void *p = dhfdrv_resolve_pd_from_emulated_addr(emu_pd_addr);
    if (!p) return -1;
    dhf_path_desc_t *pd = (dhf_path_desc_t*)p;
    return dhfdrv_write(pd->host_fd, buf, count);
}

off_t dhfdrv_seek_pd(uint32_t emu_pd_addr, off_t offset, int whence) {
    void *p = dhfdrv_resolve_pd_from_emulated_addr(emu_pd_addr);
    if (!p) return -1;
    dhf_path_desc_t *pd = (dhf_path_desc_t*)p;
    return dhfdrv_seek(pd->host_fd, offset, whence);
}
int dhfdrv_getstat(const char *path, void *statbuf) { (void)path; (void)statbuf; return -1; }
int dhfdrv_setstat(const char *path, void *statbuf) { (void)path; (void)statbuf; return -1; }
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int dhfdrv_chdir(const char *path) {
    char base[PATH_MAX];
    const char *bp = dhf_descriptor_get_basepath();
    if (!bp) return -1;
    strncpy(base, bp, sizeof(base)-1);
    base[sizeof(base)-1] = '\0';

    // Build candidate path under base
    char candidate[PATH_MAX];
    if (path[0] == '/')
        snprintf(candidate, sizeof(candidate), "%s%s", base, path);
    else
        snprintf(candidate, sizeof(candidate), "%s/%s", base, path);

    // Resolve realpaths and ensure confinement
    char real_base[PATH_MAX];
    char real_cand[PATH_MAX];
    if (!realpath(base, real_base)) return -1;
    if (!realpath(candidate, real_cand)) return -1;
    size_t lb = strlen(real_base);
    if (strncmp(real_base, real_cand, lb) != 0) {
        errno = EACCES;
        return -1;
    }

    // perform chdir to the real candidate
    if (chdir(real_cand) != 0) return -1;
    return 0;
}
#include <sys/stat.h>
#include <dirent.h>

int dhfdrv_mkdir(const char *path, int mode) {
    const char *base = dhf_descriptor_get_basepath();
    if (!base) return -1;
    char full[PATH_MAX];
    if (path[0] == '/') snprintf(full, sizeof(full), "%s%s", base, path); else snprintf(full, sizeof(full), "%s/%s", base, path);
    char real[PATH_MAX];
    if (confined_path(base, full, real, sizeof(real)) != 0) { errno = EACCES; return -1; }
    if (mkdir(real, mode) != 0) return -1;
    return 0;
}

int dhfdrv_rmdir(const char *path) {
    const char *base = dhf_descriptor_get_basepath();
    if (!base) return -1;
    char full[PATH_MAX];
    if (path[0] == '/') snprintf(full, sizeof(full), "%s%s", base, path); else snprintf(full, sizeof(full), "%s/%s", base, path);
    char real[PATH_MAX];
    if (confined_path(base, full, real, sizeof(real)) != 0) { errno = EACCES; return -1; }
    if (rmdir(real) != 0) return -1;
    return 0;
}

int dhfdrv_unlink(const char *path) {
    const char *base = dhf_descriptor_get_basepath();
    if (!base) return -1;
    char full[PATH_MAX];
    if (path[0] == '/') snprintf(full, sizeof(full), "%s%s", base, path); else snprintf(full, sizeof(full), "%s/%s", base, path);
    char real[PATH_MAX];
    if (confined_path(base, full, real, sizeof(real)) != 0) { errno = EACCES; return -1; }
    if (unlink(real) != 0) return -1;
    return 0;
}

int dhfdrv_rename(const char *oldp, const char *newp) {
    const char *base = dhf_descriptor_get_basepath();
    if (!base) return -1;
    char fullold[PATH_MAX], fullnew[PATH_MAX];
    if (oldp[0] == '/') snprintf(fullold, sizeof(fullold), "%s%s", base, oldp); else snprintf(fullold, sizeof(fullold), "%s/%s", base, oldp);
    if (newp[0] == '/') snprintf(fullnew, sizeof(fullnew), "%s%s", base, newp); else snprintf(fullnew, sizeof(fullnew), "%s/%s", base, newp);
    char realold[PATH_MAX], realnew[PATH_MAX];
    if (confined_path(base, fullold, realold, sizeof(realold)) != 0) { errno = EACCES; return -1; }
    if (confined_path(base, fullnew, realnew, sizeof(realnew)) != 0) { errno = EACCES; return -1; }
    if (rename(realold, realnew) != 0) return -1;
    return 0;
}

int dhfdrv_opendir(const char *path) {
    const char *base = dhf_descriptor_get_basepath();
    if (!base) return -1;
    char full[PATH_MAX];
    if (path[0] == '/') snprintf(full, sizeof(full), "%s%s", base, path); else snprintf(full, sizeof(full), "%s/%s", base, path);
    char real[PATH_MAX];
    if (confined_path(base, full, real, sizeof(real)) != 0) { errno = EACCES; return -1; }
    DIR *d = opendir(real);
    if (!d) return -1;
    // return pointer value as int handle (test-harness only)
    return (int)(uintptr_t)d;
}

int dhfdrv_readdir(int dirfd, void *entry) {
    if (!entry) return -1;
    DIR *d = (DIR *)(uintptr_t)dirfd;
    struct dirent *de = readdir(d);
    if (!de) return -1;
    strncpy((char*)entry, de->d_name, 256);
    return 0;
}

off_t dhfdrv_seek(int fd, off_t offset, int whence) {
    return lseek(fd, offset, whence);
}

int dhfdrv_truncate(const char *path, off_t length) {
    const char *base = dhf_descriptor_get_basepath();
    if (!base) return -1;
    char full[PATH_MAX];
    if (path[0] == '/') snprintf(full, sizeof(full), "%s%s", base, path); else snprintf(full, sizeof(full), "%s/%s", base, path);
    char real[PATH_MAX];
    if (confined_path(base, full, real, sizeof(real)) != 0) { errno = EACCES; return -1; }
    if (truncate(real, length) != 0) return -1;
    return 0;
}

