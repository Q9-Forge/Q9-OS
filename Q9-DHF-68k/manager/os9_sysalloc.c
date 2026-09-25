#include "os9_sysalloc.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

int os9_srqmem(size_t *size, void **ptr) {
    if (!size || !ptr) return -1;
    void *p = malloc(*size);
    if (!p) return -1;
    *ptr = p;
    return 0;
}

int os9_srtmem(size_t size, void *ptr) {
    (void)size;
    free(ptr);
    return 0;
}

uint32_t os9_srqmem_emulated_addr(size_t size) {
    void *p = NULL;
    if (os9_srqmem(&size, &p) != 0) return 0;
    uintptr_t v = (uintptr_t)p;
    return (uint32_t)(v & 0xffffffff);
}
