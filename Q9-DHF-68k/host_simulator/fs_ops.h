#ifndef FS_OPS_H
#define FS_OPS_H

#include <sys/types.h>
#include <stddef.h>

int confined_path(const char *basepath, const char *candidate, char *out_real, size_t out_sz);
int dhf_host_open(const char *basepath, const char *relpath, int flags, int mode);
ssize_t dhf_host_read(int fd, void *buf, size_t count);
ssize_t dhf_host_write(int fd, const void *buf, size_t count);
int dhf_host_close(int fd);
int dhf_host_getstat(const char *basepath, const char *relpath, struct stat *st);

#endif
