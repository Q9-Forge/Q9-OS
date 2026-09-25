#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include "../include/dhf_shared_hostsim.h"

// Ensure path is confined under basepath
int confined_path(const char *basepath, const char *candidate, char *out_real, size_t out_sz) {
    char real_base[PATH_MAX];
    char real_cand[PATH_MAX];
    if (!realpath(basepath, real_base)) return -1;
    if (!realpath(candidate, real_cand)) return -1;
    // ensure real_cand starts with real_base and next char is / or \0
    size_t lb = strlen(real_base);
    if (strncmp(real_base, real_cand, lb) != 0) return -1;
    // allow exact match or child
    if (real_cand[lb] != '\0' && real_cand[lb] != '/') return -1;
    if (out_real && out_sz) strncpy(out_real, real_cand, out_sz-1);
    if (out_real) out_real[out_sz-1] = '\0';
    return 0;
}

// Simple open wrapper that confines to basepath and returns host fd or -1
int dhf_host_open(const char *basepath, const char *relpath, int flags, int mode) {
    char full[PATH_MAX];
    if (relpath[0] == '/') {
        // relative to basepath
        snprintf(full, sizeof(full), "%s%s", basepath, relpath);
    } else {
        snprintf(full, sizeof(full), "%s/%s", basepath, relpath);
    }
    char real_full[PATH_MAX];
    if (confined_path(basepath, full, real_full, sizeof(real_full)) != 0) {
        errno = EACCES;
        return -1;
    }
    int fd = open(real_full, flags, mode);
    return fd;
}

ssize_t dhf_host_read(int fd, void *buf, size_t count) {
    return read(fd, buf, count);
}

ssize_t dhf_host_write(int fd, const void *buf, size_t count) {
    return write(fd, buf, count);
}

int dhf_host_close(int fd) {
    return close(fd);
}
