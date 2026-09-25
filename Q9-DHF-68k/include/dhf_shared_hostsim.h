#ifndef DHF_SHARED_H
#define DHF_SHARED_H

#include <stdint.h>

// Shared command area between manager (emulated 68k) and host simulator.
// All fields are big-endian when interpreted by 68k; host reads/writes using helpers

typedef struct dhf_shared {
    uint16_t version;    // protocol version
    uint16_t command;    // 0 = idle, 0x0001-0x000D manager commands, 0x0020 control, 0x0080+ responses
    uint32_t seq;        // sequence counter
    uint32_t param[5];   // parameters (emulator addresses / lengths / flags)
    uint32_t result_code; // result / errno mapping
    uint32_t result_len;  // optional length for read/write results
    uint8_t  reserved[64];
} dhf_shared_t;

#endif
