#pragma once

#include <stddef.h>
#include <stdint.h>

/* Wrappers for OS-9 system pool allocation. On a real OS-9 68k system the
 * implementations should invoke the F$SRqMem/F$SRtMem system calls. Here we
 * provide weak stubs that call malloc/free when building for the host, and a
 * placeholder header for cross-compilation into 68k manager code.
 */

#ifdef __EMSCRIPTEN__
/* not used */
#endif

/* Request 'size' bytes from the system pool; on success returns 0 and sets *ptr
 * to the allocated memory. On failure returns non-zero error code.
 */
int os9_srqmem(size_t *size, void **ptr);

/* Return memory previously allocated with os9_srqmem. Returns 0 on success. */
int os9_srtmem(size_t size, void *ptr);

/* For host test harness convenience provide a simple wrapper that behaves like
 * os9_srqmem but returns a 32-bit emulated address for the allocated block.
 */
uint32_t os9_srqmem_emulated_addr(size_t size);

