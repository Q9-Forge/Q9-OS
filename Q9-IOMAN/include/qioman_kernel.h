#ifndef Q9_IOMAN_KERNEL_H
#define Q9_IOMAN_KERNEL_H

#include "qioman.h"

/* Byte offsets in the 72-byte Q9 OS-9-compatible trap register frame. */
#define Q9IOMAN_R_D0 0x00
#define Q9IOMAN_R_D1 0x04
#define Q9IOMAN_R_A0 0x20
#define Q9IOMAN_R_A3 0x2c
#define Q9IOMAN_R_A4 0x30
#define Q9IOMAN_R_A5 0x34
#define Q9IOMAN_R_A7 0x3c
#define Q9IOMAN_R_SR 0x40
#define Q9IOMAN_R_PC 0x42
#define Q9IOMAN_R_FMT 0x46
#define Q9IOMAN_R_SIZE 0x48

/* Q9 kernel error numbers used by the IOMan's initial error adapter. */
#define Q9IOMAN_OS9_E_PTHFUL 0x00c8
#define Q9IOMAN_OS9_E_BPNUM  0x00c9
#define Q9IOMAN_OS9_E_BMODE  0x00cb
#define Q9IOMAN_OS9_E_UNKSVC 0x00d0
#define Q9IOMAN_OS9_E_MNF    0x00dd
#define Q9IOMAN_OS9_E_PARAM  0x00e1

typedef enum {
    Q9IOMAN_KERNEL_NONE = 0,
    Q9IOMAN_KERNEL_OPEN,
    Q9IOMAN_KERNEL_READ,
    Q9IOMAN_KERNEL_CLOSE
} Q9IOMAN_KernelRequestType;

typedef struct {
    Q9IOMAN_KernelRequestType type;
    Q9IOMAN_u16 path;
    Q9IOMAN_u16 mode;
    Q9IOMAN_u32 buffer;
    Q9IOMAN_u32 length;
} Q9IOMAN_KernelRequest;

/* Platform bridge must validate and map a NUL-terminated Q9 path string. */
typedef Q9IOMAN_Status (*Q9IOMAN_PathAddressFn)(
    void *context,
    Q9IOMAN_u32 address,
    const char **path,
    Q9IOMAN_u32 *bytes_including_nul);

Q9IOMAN_u32 q9ioman_frame_read32(const unsigned char *frame,
                                 Q9IOMAN_u16 offset);
void q9ioman_frame_write32(unsigned char *frame,
                           Q9IOMAN_u16 offset,
                           Q9IOMAN_u32 value);
Q9IOMAN_u16 q9ioman_frame_read16(const unsigned char *frame,
                                 Q9IOMAN_u16 offset);
void q9ioman_frame_write16(unsigned char *frame,
                           Q9IOMAN_u16 offset,
                           Q9IOMAN_u16 value);
Q9IOMAN_u16 q9ioman_status_to_os9_error(Q9IOMAN_Status status);
Q9IOMAN_Status q9ioman_decode_kernel_request(
    Q9IOMAN_u16 callcode,
    const unsigned char *frame,
    Q9IOMAN_KernelRequest *request);
Q9IOMAN_Status q9ioman_dispatch_kernel_request(
    Q9IOMAN_u16 callcode,
    Q9IOMAN_Manager *manager,
    unsigned char *frame,
    Q9IOMAN_PathAddressFn resolve_path,
    void *address_context);

#endif
