#pragma once

#include <errno.h>

/* Minimal mapping from host errno to Q9 DHF result codes. For now pass-through
 * host errno values, but provide a function to centralize mapping for later
 * adjustments.
 */
static inline uint32_t dhf_map_errno(int host_errno) {
    if (host_errno == 0) return 0;
    return (uint32_t)host_errno;
}
