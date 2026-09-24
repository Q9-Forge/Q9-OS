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
    uint8_t status;       /* result code */
    uint16_t command;     /* WORD command: 0 idle, 1..0x7FFF requests, 0x8000..0xFFFF responses */
    uint32_t seq;         /* incremented by manager for each request (BE) */

    uint32_t param[5];    /* 5 x 32-bit parameters (A0/A1/D0..D2 semantics) */

    /* NOTE: Name and Buffer are external by default; manager places them in an
       external memory block in the emulator and sets param flags accordingly.
       Legacy embedded placeholders kept for compatibility. */
    char name[DHF_SHARED_NAME_LEN];
    uint8_t buffer[DHF_SHARED_BUF_LEN];
};
#pragma pack(pop)

#endif /* DHF_SHARED_H */
