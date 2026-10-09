#ifndef Q9TRACE_DESC_H
#define Q9TRACE_DESC_H

#include <stddef.h>
#include <stdint.h>

#define Q9TRACE_DESC_MAX_STR 32 /* Max bytes including null terminator */

/* Payload fill functions return payload length in bytes. All numeric
 * fields are encoded big-endian into the output buffer. Strings are
 * copied into fixed-size fields of Q9TRACE_DESC_MAX_STR bytes and are
 * guaranteed to be null-terminated; long strings are truncated. */

size_t q9trace_desc_fill_FLink(uint8_t *out, const char *oldPath, const char *newPath);
size_t q9trace_desc_fill_FFork(uint8_t *out, uint32_t flags, uint16_t childPid);
size_t q9trace_desc_fill_FLoad(uint8_t *out, const char *path, uint32_t flags);
size_t q9trace_desc_fill_IOpen(uint8_t *out, const char *path, uint16_t flags, uint16_t mode);
size_t q9trace_desc_fill_IRead(uint8_t *out, uint16_t fd, uint32_t count);
size_t q9trace_desc_fill_IWrite(uint8_t *out, uint16_t fd, uint32_t count);
size_t q9trace_desc_fill_IClose(uint8_t *out, uint16_t fd);

/* Helper: copy a C string into fixed-length field (maxlen bytes), ensure
 * null-termination and return maxlen (field size). */
size_t q9trace_desc_copy_str_field(char *dest, const char *src, size_t maxlen);

#endif /* Q9TRACE_DESC_H */
