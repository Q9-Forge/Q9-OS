/* Q9-DHF manager skeleton (Q9-DHF-68k)
 * Forwards Q9 filemanager calls to dhfdrv-68k which maps to the host filesystem.
 * Minimal stubs to be implemented further.
 */

#include "../descriptor/dhf_descriptor.h"
#include "../driver/dhfdrv-68k.h"
#include <stddef.h>
#include <sys/types.h>

int dhf_manager_init(void) {
    return dhfdrv_init(dhf_descriptor_get_basepath());
}

int dhf_manager_open(const char *path, int flags) { return dhfdrv_open(path, flags); }
int dhf_manager_close(int fd) { return dhfdrv_close(fd); }
ssize_t dhf_manager_read(int fd, void *buf, size_t count) { return dhfdrv_read(fd, buf, count); }
ssize_t dhf_manager_write(int fd, const void *buf, size_t count) { return dhfdrv_write(fd, buf, count); }
int dhf_manager_getstat(const char *path, void *statbuf) { return dhfdrv_getstat(path, statbuf); }
int dhf_manager_setstat(const char *path, void *statbuf) { return dhfdrv_setstat(path, statbuf); }
int dhf_manager_chdir(const char *path) { return dhfdrv_chdir(path); }
int dhf_manager_mkdir(const char *path, int mode) { return dhfdrv_mkdir(path, mode); }
int dhf_manager_rmdir(const char *path) { return dhfdrv_rmdir(path); }
int dhf_manager_unlink(const char *path) { return dhfdrv_unlink(path); }
int dhf_manager_rename(const char *oldp, const char *newp) { return dhfdrv_rename(oldp,newp); }
int dhf_manager_opendir(const char *path) { return dhfdrv_opendir(path); }
int dhf_manager_readdir(int dirfd, void *entry) { return dhfdrv_readdir(dirfd, entry); }

