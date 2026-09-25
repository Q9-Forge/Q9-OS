#pragma once

#include "../descriptor/dhf_descriptor.h"
#include "../driver/dhfdrv-68k.h"
#include <stddef.h>
#include <sys/types.h>

int dhf_manager_init(void);
int dhf_manager_open(const char *path, int flags);
int dhf_manager_close(int fd);
ssize_t dhf_manager_read(int fd, void *buf, size_t count);
ssize_t dhf_manager_write(int fd, const void *buf, size_t count);
int dhf_manager_getstat(const char *path, void *statbuf);
int dhf_manager_setstat(const char *path, void *statbuf);
int dhf_manager_chdir(const char *path);
int dhf_manager_mkdir(const char *path, int mode);
int dhf_manager_rmdir(const char *path);
int dhf_manager_unlink(const char *path);
int dhf_manager_rename(const char *oldp, const char *newp);
int dhf_manager_opendir(const char *path);
int dhf_manager_readdir(int dirfd, void *entry);

/* Manager allocation helpers - on real OS-9 these should allocate from system pool */
void *dhf_manager_alloc(size_t size);
void dhf_manager_free(void *p);

/* Helpers for tests that mimic emulator-address handoff */
uint32_t dhf_manager_alloc_emulated_addr(size_t size);
void *dhf_manager_resolve_emulated_addr(uint32_t addr);

