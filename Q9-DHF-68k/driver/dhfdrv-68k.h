#ifndef DHFDRV_68K_H
#define DHFDRV_68K_H

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

#endif
