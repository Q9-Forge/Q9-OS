#pragma once

#include <stdint.h>

/* Simple per-path descriptor maintained by manager. On real system this would live
 * in the path descriptor area allocated from system pool and referenced by the
 * kernel/driver. For the host test harness we store a pointer to this in the
 * manager and free it on close.
 */

typedef struct dhf_path_desc {
    int host_fd;        /* host file descriptor */
    uint32_t flags;     /* open flags */
    void *private;      /* reserved for future use */
} dhf_path_desc_t;
