/* dhfdrv.c - Q9-DHF 68k Driver implementation
 * Passes guest 68k pointers (A0 for paths, A1 for buffers) to the
 * simulated hardware device in the shared command area.
 * The driver never dereferences user pointers; it only passes them to hardware.
 */

#include "dhfdrv-68k.h"
#include "dhf_shared.h"
#include "dhf_shared_api.h"
#include "dhf_proto.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

static struct dhf_shared *g_shared = NULL;
static dhf_emu_trigger_fn g_trigger_fn = NULL;
static void              *g_trigger_data = NULL;
static int                g_timeout_ms = 3000;

void dhfdrv_set_shared_mem(struct dhf_shared *mem) {
    g_shared = mem;
}

struct dhf_shared *dhfdrv_get_shared_mem(void) {
    return g_shared;
}

void dhfdrv_set_trigger_hook(dhf_emu_trigger_fn fn, void *user_data) {
    g_trigger_fn = fn;
    g_trigger_data = user_data;
}

static int dhf_exec_cmd(uint8_t cmd) {
    if (!g_shared) {
        errno = ENXIO;
        return -1;
    }

    uint32_t seq = dhf_get_seq_and_inc(g_shared);
    __sync_synchronize();
    g_shared->status = 0;
    g_shared->command = cmd;
    __sync_synchronize();

    if (g_trigger_fn) {
        g_trigger_fn(g_trigger_data);
    }

    int ret = dhf_wait_idle(g_shared, seq, g_timeout_ms);
    if (ret != 0) {
        errno = ETIMEDOUT;
        return -1;
    }

    uint8_t st = dhf_get_status(g_shared);
    if (st != DHF_ERR_OK) {
        switch (st) {
            case DHF_ERR_NOT_FOUND:     errno = ENOENT; break;
            case DHF_ERR_NO_PERMISSION: errno = EACCES; break;
            case DHF_ERR_FILE_EXISTS:   errno = EEXIST; break;
            case DHF_ERR_BAD_PATH:      errno = EBADF; break;
            case DHF_ERR_DISK_FULL:     errno = ENOSPC; break;
            case DHF_ERR_IS_DIR:        errno = EISDIR; break;
            case DHF_ERR_NOT_DIR:       errno = ENOTDIR; break;
            default:                    errno = EIO; break;
        }
        return -1;
    }
    return 0;
}

int dhfdrv_init(const char *basepath) {
    if (!g_shared) {
        g_shared = (struct dhf_shared *)((uintptr_t)DHF_DEFAULT_HW_BASE);
    }
    dhf_shared_init(g_shared);
    if (basepath) {
        g_shared->a0 = htonl((uint32_t)(uintptr_t)basepath);
    }
    return dhf_exec_cmd(DHF_CMD_INIT);
}

int dhfdrv_term(void) {
    if (!g_shared) return 0;
    return dhf_exec_cmd(DHF_CMD_TERM);
}

int dhfdrv_open(const char *path, int flags) {
    if (!g_shared || !path) {
        errno = EINVAL;
        return -1;
    }
    g_shared->a0 = htonl((uint32_t)(uintptr_t)path);
    g_shared->d2 = htonl((uint32_t)flags);
    if (dhf_exec_cmd(DHF_CMD_OPEN) != 0) return -1;
    return (int)ntohl(g_shared->d0);
}

int dhfdrv_create(const char *path, int flags, int mode) {
    if (!g_shared || !path) {
        errno = EINVAL;
        return -1;
    }
    g_shared->a0 = htonl((uint32_t)(uintptr_t)path);
    g_shared->d1 = htonl((uint32_t)mode);
    g_shared->d2 = htonl((uint32_t)flags);
    if (dhf_exec_cmd(DHF_CMD_CREATE) != 0) return -1;
    return (int)ntohl(g_shared->d0);
}

int dhfdrv_close(int fd) {
    if (!g_shared || fd < 0) {
        errno = EBADF;
        return -1;
    }
    g_shared->d0 = htonl((uint32_t)fd);
    return dhf_exec_cmd(DHF_CMD_CLOSE);
}

ssize_t dhfdrv_read(int fd, void *buf, size_t count) {
    if (!g_shared || fd < 0 || !buf) {
        errno = EINVAL;
        return -1;
    }
    g_shared->d0 = htonl((uint32_t)fd);
    g_shared->a1 = htonl((uint32_t)(uintptr_t)buf);
    g_shared->d1 = htonl((uint32_t)count);
    if (dhf_exec_cmd(DHF_CMD_READ) != 0) return -1;
    return (ssize_t)ntohl(g_shared->d1);
}

ssize_t dhfdrv_write(int fd, const void *buf, size_t count) {
    if (!g_shared || fd < 0 || !buf) {
        errno = EINVAL;
        return -1;
    }
    g_shared->d0 = htonl((uint32_t)fd);
    g_shared->a1 = htonl((uint32_t)(uintptr_t)buf);
    g_shared->d1 = htonl((uint32_t)count);
    if (dhf_exec_cmd(DHF_CMD_WRITE) != 0) return -1;
    return (ssize_t)ntohl(g_shared->d1);
}

off_t dhfdrv_seek(int fd, off_t offset, int whence) {
    if (!g_shared || fd < 0) {
        errno = EBADF;
        return -1;
    }
    g_shared->d0 = htonl((uint32_t)fd);
    g_shared->d1 = htonl((uint32_t)offset);
    g_shared->d2 = htonl((uint32_t)whence);
    if (dhf_exec_cmd(DHF_CMD_SEEK) != 0) return -1;
    return (off_t)ntohl(g_shared->d1);
}

int dhfdrv_readln(int fd, char *buf, size_t maxlen) {
    if (!g_shared || fd < 0 || !buf || maxlen == 0) {
        errno = EINVAL;
        return -1;
    }
    g_shared->d0 = htonl((uint32_t)fd);
    g_shared->a1 = htonl((uint32_t)(uintptr_t)buf);
    g_shared->d1 = htonl((uint32_t)maxlen);
    if (dhf_exec_cmd(DHF_CMD_READLN) != 0) return -1;
    return (int)ntohl(g_shared->d1);
}

int dhfdrv_writeln(int fd, const char *buf, size_t len) {
    if (!g_shared || fd < 0 || !buf) {
        errno = EINVAL;
        return -1;
    }
    g_shared->d0 = htonl((uint32_t)fd);
    g_shared->a1 = htonl((uint32_t)(uintptr_t)buf);
    g_shared->d1 = htonl((uint32_t)len);
    if (dhf_exec_cmd(DHF_CMD_WRITELN) != 0) return -1;
    return (int)ntohl(g_shared->d1);
}

int dhfdrv_getstat(const char *path, void *statbuf) {
    if (!g_shared || !path || !statbuf) {
        errno = EINVAL;
        return -1;
    }
    g_shared->a0 = htonl((uint32_t)(uintptr_t)path);
    g_shared->a1 = htonl((uint32_t)(uintptr_t)statbuf);
    return dhf_exec_cmd(DHF_CMD_GETSTT);
}

int dhfdrv_setstat(const char *path, void *statbuf) {
    if (!g_shared || !path) {
        errno = EINVAL;
        return -1;
    }
    g_shared->a0 = htonl((uint32_t)(uintptr_t)path);
    g_shared->a1 = htonl((uint32_t)(uintptr_t)statbuf);
    return dhf_exec_cmd(DHF_CMD_SETSTT);
}

int dhfdrv_chdir(const char *path) {
    if (!g_shared || !path) {
        errno = EINVAL;
        return -1;
    }
    g_shared->a0 = htonl((uint32_t)(uintptr_t)path);
    return dhf_exec_cmd(DHF_CMD_CHDIR);
}

int dhfdrv_mkdir(const char *path, int mode) {
    if (!g_shared || !path) {
        errno = EINVAL;
        return -1;
    }
    g_shared->a0 = htonl((uint32_t)(uintptr_t)path);
    g_shared->d1 = htonl((uint32_t)mode);
    return dhf_exec_cmd(DHF_CMD_MKDIR);
}

int dhfdrv_rmdir(const char *path) {
    if (!g_shared || !path) {
        errno = EINVAL;
        return -1;
    }
    g_shared->a0 = htonl((uint32_t)(uintptr_t)path);
    return dhf_exec_cmd(DHF_CMD_RMDIR);
}

int dhfdrv_unlink(const char *path) {
    if (!g_shared || !path) {
        errno = EINVAL;
        return -1;
    }
    g_shared->a0 = htonl((uint32_t)(uintptr_t)path);
    return dhf_exec_cmd(DHF_CMD_DELETE);
}

int dhfdrv_rename(const char *oldp, const char *newp) {
    if (!g_shared || !oldp || !newp) {
        errno = EINVAL;
        return -1;
    }
    g_shared->a0 = htonl((uint32_t)(uintptr_t)oldp);
    g_shared->a1 = htonl((uint32_t)(uintptr_t)newp);
    return dhf_exec_cmd(DHF_CMD_RENAME);
}

int dhfdrv_opendir(const char *path) {
    if (!g_shared || !path) {
        errno = EINVAL;
        return -1;
    }
    g_shared->a0 = htonl((uint32_t)(uintptr_t)path);
    if (dhf_exec_cmd(DHF_CMD_OPENDIR) != 0) return -1;
    return (int)ntohl(g_shared->d0);
}

int dhfdrv_readdir(int dirfd, void *entry) {
    if (!g_shared || dirfd < 0 || !entry) {
        errno = EINVAL;
        return -1;
    }
    g_shared->d0 = htonl((uint32_t)dirfd);
    g_shared->a1 = htonl((uint32_t)(uintptr_t)entry);
    if (dhf_exec_cmd(DHF_CMD_READDIR) != 0) return -1;
    return (int)ntohl(g_shared->d1);
}
