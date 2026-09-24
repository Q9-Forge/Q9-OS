/* dhfdrv-68k: driver skeleton mapping to host filesystem
 * Implement platform-specific host FS operations and confinement here.
 */

#include "dhfdrv-68k.h"
#include <string.h>
#include <stdio.h>

int dhfdrv_init(const char *basepath) { (void)basepath; /* TODO: init host FS mapping */ return 0; }
int dhfdrv_open(const char *path, int flags) { (void)path; (void)flags; return -1; }
int dhfdrv_close(int fd) { (void)fd; return -1; }
ssize_t dhfdrv_read(int fd, void *buf, size_t count) { (void)fd; (void)buf; (void)count; return -1; }
ssize_t dhfdrv_write(int fd, const void *buf, size_t count) { (void)fd; (void)buf; (void)count; return -1; }
int dhfdrv_getstat(const char *path, void *statbuf) { (void)path; (void)statbuf; return -1; }
int dhfdrv_setstat(const char *path, void *statbuf) { (void)path; (void)statbuf; return -1; }
int dhfdrv_chdir(const char *path) { (void)path; return -1; }
int dhfdrv_mkdir(const char *path, int mode) { (void)path; (void)mode; return -1; }
int dhfdrv_rmdir(const char *path) { (void)path; return -1; }
int dhfdrv_unlink(const char *path) { (void)path; return -1; }
int dhfdrv_rename(const char *oldp, const char *newp) { (void)oldp; (void)newp; return -1; }
int dhfdrv_opendir(const char *path) { (void)path; return -1; }
int dhfdrv_readdir(int dirfd, void *entry) { (void)dirfd; (void)entry; return -1; }

