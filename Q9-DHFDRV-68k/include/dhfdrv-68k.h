/* dhfdrv-68k.h - Driver interface for Q9-DHF
 * Called by DHF Manager, forwards commands to simulated hardware.
 */

#ifndef DHFDRV_68K_H
#define DHFDRV_68K_H

#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>
#include "dhf_proto.h"
#include "dhf_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configuration: point driver to the shared memory window */
void dhfdrv_set_shared_mem(struct dhf_shared *mem);
struct dhf_shared *dhfdrv_get_shared_mem(void);

/* Manager-facing Driver API */
int     dhfdrv_init(const char *basepath);
int     dhfdrv_term(void);

int     dhfdrv_open(const char *path, int flags);
int     dhfdrv_create(const char *path, int flags, int mode);
int     dhfdrv_close(int fd);

ssize_t dhfdrv_read(int fd, void *buf, size_t count);
ssize_t dhfdrv_write(int fd, const void *buf, size_t count);
off_t   dhfdrv_seek(int fd, off_t offset, int whence);

int     dhfdrv_readln(int fd, char *buf, size_t maxlen);
int     dhfdrv_writeln(int fd, const char *buf, size_t len);

int     dhfdrv_getstat(const char *path, void *statbuf);
int     dhfdrv_setstat(const char *path, void *statbuf);

int     dhfdrv_chdir(const char *path);
int     dhfdrv_mkdir(const char *path, int mode);
int     dhfdrv_rmdir(const char *path);
int     dhfdrv_unlink(const char *path);
int     dhfdrv_rename(const char *oldp, const char *newp);

int     dhfdrv_opendir(const char *path);
int     dhfdrv_readdir(int dirfd, void *entry);

/* Optional hook: direct callback to emulator when running in single-process or test mode */
typedef void (*dhf_emu_trigger_fn)(void *user_data);
void dhfdrv_set_trigger_hook(dhf_emu_trigger_fn fn, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* DHFDRV_68K_H */
