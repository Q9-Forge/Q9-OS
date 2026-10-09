#include "q9trace_desc.h"
#include <string.h>

static void write_u16_be(Q9_u8 *out, Q9_u16 v)
{
    out[0] = (Q9_u8)(v >> 8);
    out[1] = (Q9_u8)(v & 0xff);
}

static void write_u32_be(Q9_u8 *out, Q9_u32 v)
{
    out[0] = (Q9_u8)(v >> 24);
    out[1] = (Q9_u8)(v >> 16);
    out[2] = (Q9_u8)(v >> 8);
    out[3] = (Q9_u8)(v & 0xff);
}

Q9_u32 q9trace_desc_copy_str_field(Q9_u8 *dest, const char *src, Q9_u32 fieldLen)
{
    Q9_u32 i = 0;
    if (fieldLen == 0)
        return 0;
    if (src == NULL)
        src = "";
    /* copy up to fieldLen-1 bytes, leave space for NUL */
    while (i + 1 < fieldLen && src[i] != '\0') {
        dest[i] = (Q9_u8)src[i];
        i++;
    }
    /* NUL-terminate */
    dest[i] = 0;
    /* fill remainder with zeros */
    for (i = i + 1; i < fieldLen; i++)
        dest[i] = 0;
    return fieldLen;
}

Q9_u32 q9trace_desc_fill_FLink(Q9_u8 *out, Q9_u32 outMax, const char *oldPath, const char *newPath)
{
    const Q9_u32 need = Q9TRACE_DESC_NAME_LEN * 2;
    if (outMax < need || need > Q9TRACE_DESC_MAX_PAYLOAD)
        return 0;
    q9trace_desc_copy_str_field(&out[0], oldPath, Q9TRACE_DESC_NAME_LEN);
    q9trace_desc_copy_str_field(&out[Q9TRACE_DESC_NAME_LEN], newPath, Q9TRACE_DESC_NAME_LEN);
    return need;
}

Q9_u32 q9trace_desc_fill_FFork(Q9_u8 *out, Q9_u32 outMax, Q9_u32 flags, Q9_u16 childPid)
{
    const Q9_u32 need = 4 + 2;
    if (outMax < need || need > Q9TRACE_DESC_MAX_PAYLOAD)
        return 0;
    write_u32_be(&out[0], flags);
    write_u16_be(&out[4], childPid);
    return need;
}

Q9_u32 q9trace_desc_fill_FLoad(Q9_u8 *out, Q9_u32 outMax, const char *path, Q9_u32 flags)
{
    const Q9_u32 need = Q9TRACE_DESC_NAME_LEN + 4;
    if (outMax < need || need > Q9TRACE_DESC_MAX_PAYLOAD)
        return 0;
    q9trace_desc_copy_str_field(&out[0], path, Q9TRACE_DESC_NAME_LEN);
    write_u32_be(&out[Q9TRACE_DESC_NAME_LEN], flags);
    return need;
}

Q9_u32 q9trace_desc_fill_IOpen(Q9_u8 *out, Q9_u32 outMax, const char *path, Q9_u16 flags, Q9_u16 mode)
{
    const Q9_u32 need = Q9TRACE_DESC_NAME_LEN + 2 + 2;
    if (outMax < need || need > Q9TRACE_DESC_MAX_PAYLOAD)
        return 0;
    q9trace_desc_copy_str_field(&out[0], path, Q9TRACE_DESC_NAME_LEN);
    write_u16_be(&out[Q9TRACE_DESC_NAME_LEN], flags);
    write_u16_be(&out[Q9TRACE_DESC_NAME_LEN + 2], mode);
    return need;
}

Q9_u32 q9trace_desc_fill_IRead(Q9_u8 *out, Q9_u32 outMax, Q9_u16 fd, Q9_u32 count)
{
    const Q9_u32 need = 2 + 4;
    if (outMax < need || need > Q9TRACE_DESC_MAX_PAYLOAD)
        return 0;
    write_u16_be(&out[0], fd);
    write_u32_be(&out[2], count);
    return need;
}

Q9_u32 q9trace_desc_fill_IWrite(Q9_u8 *out, Q9_u32 outMax, Q9_u16 fd, Q9_u32 count)
{
    return q9trace_desc_fill_IRead(out, outMax, fd, count);
}

Q9_u32 q9trace_desc_fill_IClose(Q9_u8 *out, Q9_u32 outMax, Q9_u16 fd)
{
    const Q9_u32 need = 2;
    if (outMax < need || need > Q9TRACE_DESC_MAX_PAYLOAD)
        return 0;
    write_u16_be(&out[0], fd);
    return need;
}
