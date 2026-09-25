#ifndef DHFDRV_68K_H
#define DHFDRV_68K_H

#include <sys/types.h>
#include <stdint.h>

const char* dhf_descriptor_get_basepath(void);
int dhfdrv_init(const char *basepath);
int dhfdrv_open(const char *path, int flags);
int dhfdrv_close(int fd);
ssize_t dhfdrv_read(int fd, void *buf, size_t count);
ssize_t dhfdrv_write(int fd, const void *buf, size_t count);
int dhfdrv_getstat(const char *path, void *statbuf);
int dhfdrv_setstat(const char *path, void *statbuf);
int dhfdrv_chdir(const char *path);
int dhfdrv_mkdir(const char *path, int mode);
int dhfdrv_rmdir(const char *path);
int dhfdrv_unlink(const char *path);
int dhfdrv_rename(const char *oldp, const char *newp);
int dhfdrv_opendir(const char *path);
int dhfdrv_readdir(int dirfd, void *entry);

off_t dhfdrv_seek(int fd, off_t offset, int whence);
int dhfdrv_truncate(const char *path, off_t length);

/* PD-based variants: accept a 32-bit emulated pd pointer, resolve pd, and use
 * the host_fd stored in the pd for operations. These are test-harness helpers
 * allowing driver to operate based on manager-provided path descriptor pointers.
 */
int dhfdrv_close_pd(uint32_t emu_pd_addr);
ssize_t dhfdrv_read_pd(uint32_t emu_pd_addr, void *buf, size_t count);
ssize_t dhfdrv_write_pd(uint32_t emu_pd_addr, const void *buf, size_t count);
off_t dhfdrv_seek_pd(uint32_t emu_pd_addr, off_t offset, int whence);

/* Manager/Driver handoff helpers for emulated pd pointer (test harness)
 * The manager will allocate a per-path control block and place an emulator
 * address in the path descriptor; driver can resolve this emulated address
 * with the following helper in tests.
 */
uint32_t dhf_manager_alloc_emulated_addr(size_t size);
void *dhf_manager_resolve_emulated_addr(uint32_t addr);

/* Driver helper to resolve a manager-emulated pd pointer */
void *dhfdrv_resolve_pd_from_emulated_addr(uint32_t emu_addr);

#endif
