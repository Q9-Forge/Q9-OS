#include "q9trace_desc.h"
#include <string.h>

static void write_u16_be(uint8_t *out, uint16_t v)
{
    out[0] = (uint8_t)(v >> 8);
    out[1] = (uint8_t)(v & 0xff);
}

static void write_u32_be(uint8_t *out, uint32_t v)
{
    out[0] = (uint8_t)(v >> 24);
    out[1] = (uint8_t)(v >> 16);
    out[2] = (uint8_t)(v >> 8);
    out[3] = (uint8_t)(v & 0xff);
}

size_t q9trace_desc_copy_str_field(char *dest, const char *src, size_t maxlen)
{
    if (maxlen == 0) return 0;
    /* copy at most maxlen-1 bytes and ensure null termination */
    size_t i;
    for (i = 0; i + 1 < maxlen && src[i] != '\0'; i++)
        dest[i] = src[i];
    /* fill remainder with zeros */
    dest[i] = '\0';
    for (i = i + 1; i < maxlen; i++)
        dest[i] = '\0';
    return maxlen;
}

size_t q9trace_desc_fill_FLink(uint8_t *out, const char *oldPath, const char *newPath)
{
    /* two fixed 32-byte string fields */
    q9trace_desc_copy_str_field((char *)&out[0], oldPath, Q9TRACE_DESC_MAX_STR);
    q9trace_desc_copy_str_field((char *)&out[Q9TRACE_DESC_MAX_STR], newPath, Q9TRACE_DESC_MAX_STR);
    return Q9TRACE_DESC_MAX_STR * 2;
}

size_t q9trace_desc_fill_FFork(uint8_t *out, uint32_t flags, uint16_t childPid)
{
    write_u32_be(&out[0], flags);
    write_u16_be(&out[4], childPid);
    return 6;
}

size_t q9trace_desc_fill_FLoad(uint8_t *out, const char *path, uint32_t flags)
{
    q9trace_desc_copy_str_field((char *)&out[0], path, Q9TRACE_DESC_MAX_STR);
    write_u32_be(&out[Q9TRACE_DESC_MAX_STR], flags);
    return Q9TRACE_DESC_MAX_STR + 4;
}

size_t q9trace_desc_fill_IOpen(uint8_t *out, const char *path, uint16_t flags, uint16_t mode)
{
    q9trace_desc_copy_str_field((char *)&out[0], path, Q9TRACE_DESC_MAX_STR);
    write_u16_be(&out[Q9TRACE_DESC_MAX_STR], flags);
    write_u16_be(&out[Q9TRACE_DESC_MAX_STR + 2], mode);
    return Q9TRACE_DESC_MAX_STR + 4;
}

size_t q9trace_desc_fill_IRead(uint8_t *out, uint16_t fd, uint32_t count)
{
    write_u16_be(&out[0], fd);
    write_u32_be(&out[2], count);
    return 6;
}

size_t q9trace_desc_fill_IWrite(uint8_t *out, uint16_t fd, uint32_t count)
{
    /* same layout as IRead */
    return q9trace_desc_fill_IRead(out, fd, count);
}

size_t q9trace_desc_fill_IClose(uint8_t *out, uint16_t fd)
{
    write_u16_be(&out[0], fd);
    return 2;
}
