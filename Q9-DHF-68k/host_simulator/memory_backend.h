#ifndef MEMORY_BACKEND_H
#define MEMORY_BACKEND_H

#include <stddef.h>

void *memmap_file_readonly(const char *path, size_t *out_size);
int memunmap(void *addr, size_t size);

#endif
