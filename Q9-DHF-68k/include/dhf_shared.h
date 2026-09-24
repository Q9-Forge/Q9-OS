/* dhf_shared.h - Shared Command Area struct for Q9-DHF-68k
 * Packed, big-endian layout. Manager fills and sets command; driver
 * processes and sets command back to 0 (idle) when done.
 */

#ifndef DHF_SHARED_H
#define DHF_SHARED_H

#include <stdint.h>

#define DHF_SHARED_NAME_LEN 256
#define DHF_SHARED_BUF_LEN  512

#pragma pack(push,1)
struct dhf_shared {
    uint8_t version;      /* protocol version */
    uint8_t command;      /* 0 idle, others commands, 255 = return */
    uint8_t status;       /* result code */
    uint8_t flags;        /* reserved flags */
    uint32_t seq;         /* incremented by manager for each request (BE) */

    uint32_t a0;          /* filename/name offset/flag (BE) */
    uint32_t a1;          /* buffer offset/flag (BE) */
    uint32_t d0;          /* descriptor/pathnum (BE) */
    uint32_t d1;          /* status/bytecount/maxlength (BE) */
    uint32_t d2;          /* attributes/flags (BE) */

    char name[DHF_SHARED_NAME_LEN];
    uint8_t buffer[DHF_SHARED_BUF_LEN];
};
#pragma pack(pop)

#endif /* DHF_SHARED_H */
