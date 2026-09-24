#include "qioman_kernel.h"

Q9IOMAN_u32 q9ioman_frame_read32(const unsigned char *frame,
                                 Q9IOMAN_u16 offset)
{
    const unsigned char *byte;
    if (frame == 0)
        return 0;
    byte = frame + offset;
    return ((Q9IOMAN_u32)byte[0] << 24) |
           ((Q9IOMAN_u32)byte[1] << 16) |
           ((Q9IOMAN_u32)byte[2] << 8) |
           (Q9IOMAN_u32)byte[3];
}

void q9ioman_frame_write32(unsigned char *frame,
                           Q9IOMAN_u16 offset,
                           Q9IOMAN_u32 value)
{
    unsigned char *byte;
    if (frame == 0)
        return;
    byte = frame + offset;
    byte[0] = (unsigned char)((value >> 24) & 0xffUL);
    byte[1] = (unsigned char)((value >> 16) & 0xffUL);
    byte[2] = (unsigned char)((value >> 8) & 0xffUL);
    byte[3] = (unsigned char)(value & 0xffUL);
}

Q9IOMAN_u16 q9ioman_frame_read16(const unsigned char *frame,
                                 Q9IOMAN_u16 offset)
{
    const unsigned char *byte;
    if (frame == 0)
        return 0;
    byte = frame + offset;
    return (Q9IOMAN_u16)(((Q9IOMAN_u16)byte[0] << 8) | byte[1]);
}

void q9ioman_frame_write16(unsigned char *frame,
                           Q9IOMAN_u16 offset,
                           Q9IOMAN_u16 value)
{
    unsigned char *byte;
    if (frame == 0)
        return;
    byte = frame + offset;
    byte[0] = (unsigned char)((value >> 8) & 0xffU);
    byte[1] = (unsigned char)(value & 0xffU);
}

Q9IOMAN_u16 q9ioman_status_to_os9_error(Q9IOMAN_Status status)
{
    switch (status) {
    case Q9IOMAN_OK:
        return 0;
    case Q9IOMAN_E_INVALID_ARGUMENT:
        return Q9IOMAN_OS9_E_PARAM;
    case Q9IOMAN_E_INVALID_PATH:
        return Q9IOMAN_OS9_E_BPNUM;
    case Q9IOMAN_E_NO_PATH_SLOTS:
    case Q9IOMAN_E_REGISTRY_FULL:
        return Q9IOMAN_OS9_E_PTHFUL;
    case Q9IOMAN_E_NOT_FOUND:
        return Q9IOMAN_OS9_E_MNF;
    case Q9IOMAN_E_UNSUPPORTED_OPERATION:
    default:
        return Q9IOMAN_OS9_E_UNKSVC;
    }
}
