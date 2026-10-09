#ifndef Q9TRACE_DESC_H
#define Q9TRACE_DESC_H

#include "q9trace.h"

/* Name field: allow up to Q9TRACE_MAX_NAME characters plus terminating NUL. */
#define Q9TRACE_DESC_NAME_LEN (Q9TRACE_MAX_NAME + 1)
/* Payload cap so header(12)+payload(<=52) <= Q9TRACE_MAX_REC (64) */
#define Q9TRACE_DESC_MAX_PAYLOAD 52

/* Copy a C string into a fixed-size field (fieldLen bytes). src==NULL is
 * treated as empty string. Ensures NUL-termination. Returns fieldLen on
 * success. */
Q9_u32 q9trace_desc_copy_str_field(Q9_u8 *dest, const char *src, Q9_u32 fieldLen);

/* All fill functions accept outMax to prevent buffer overruns. Return
 * number of bytes written to out (<= outMax) or 0 on error/insufficient
 * space. */
Q9_u32 q9trace_desc_fill_FLink(Q9_u8 *out, Q9_u32 outMax, const char *oldPath, const char *newPath);
Q9_u32 q9trace_desc_fill_FFork(Q9_u8 *out, Q9_u32 outMax, Q9_u32 flags, Q9_u16 childPid);
Q9_u32 q9trace_desc_fill_FLoad(Q9_u8 *out, Q9_u32 outMax, const char *path, Q9_u32 flags);
Q9_u32 q9trace_desc_fill_IOpen(Q9_u8 *out, Q9_u32 outMax, const char *path, Q9_u16 flags, Q9_u16 mode);
Q9_u32 q9trace_desc_fill_IRead(Q9_u8 *out, Q9_u32 outMax, Q9_u16 fd, Q9_u32 count);
Q9_u32 q9trace_desc_fill_IWrite(Q9_u8 *out, Q9_u32 outMax, Q9_u16 fd, Q9_u32 count);
Q9_u32 q9trace_desc_fill_IClose(Q9_u8 *out, Q9_u32 outMax, Q9_u16 fd);

#endif /* Q9TRACE_DESC_H */
