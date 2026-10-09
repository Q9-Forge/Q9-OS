/* trace.c -- simple OS-9 command to control the kernel trace via F$Q9Dbg (0x58)
 * Host-testable command wrapper: provides a main(argc,argv) that parses
 * 'trace on|off|clear|show' and maps to the corresponding subcodes.
 * For the host tests, the actual syscall is simulated via a weak
 * "q9syscall" function which can be overridden/mocked in tests. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Syscall number */
#define F_Q9DBG 0x58

/* Subcodes */
#define Q9DBG_SUB_ONOFF 1
#define Q9DBG_SUB_MASK_SET 2
#define Q9DBG_SUB_MASK_GET 3
#define Q9DBG_SUB_RING_READ 6

/* Prototype of low-level syscall function (to be provided by OS or test) */
int q9syscall(int callcode, int subcode, void *a0, int d1, int d2, int d3);

static void usage(void)
{
    printf("usage: trace on|off|clear|show [args]\n");
}

int cmd_trace(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return 1;
    }

    if (strcmp(argv[1], "on") == 0) {
        /* subcode 1, d1=1 */
        int r = q9syscall(F_Q9DBG, Q9DBG_SUB_ONOFF, NULL, 1, 0, 0);
        printf("trace on -> rc=%d\n", r);
        return 0;
    }

    if (strcmp(argv[1], "off") == 0) {
        int r = q9syscall(F_Q9DBG, Q9DBG_SUB_ONOFF, NULL, 0, 0, 0);
        printf("trace off -> rc=%d\n", r);
        return 0;
    }

    if (strcmp(argv[1], "clear") == 0) {
        /* set mask to zero: subcode 2 with 32 zero bytes in a0 */
        unsigned char mask[32] = {0};
        int r = q9syscall(F_Q9DBG, Q9DBG_SUB_MASK_SET, mask, 0, 0, 0);
        printf("trace clear -> rc=%d\n", r);
        return 0;
    }

    if (strcmp(argv[1], "show") == 0) {
        /* Read ring buffer: subcode 6. We'll pass d1=0 (start), d2=max bytes
         * and a0 is a buffer provided by host; simulate reading into a local buffer. */
        unsigned char buf[256];
        int max = (int)sizeof(buf);
        int r = q9syscall(F_Q9DBG, Q9DBG_SUB_RING_READ, buf, 0, max, 0);
        if (r <= 0) {
            printf("trace show: read returned %d\n", r);
            return 0;
        }
        /* parse records (12-byte header + payload). Print PID, code, depth and if return */
        int off = 0;
        while (off + 12 <= r) {
            unsigned char *rec = &buf[off];
            unsigned char recType = rec[0];
            unsigned char recLen = rec[1];
            unsigned char code = rec[2];
            unsigned char flags = rec[3];
            unsigned short pid = (rec[4] << 8) | rec[5];
            unsigned int tick = (rec[6]<<24)|(rec[7]<<16)|(rec[8]<<8)|rec[9];
            unsigned short fine = (rec[10]<<8)|rec[11];
            unsigned depth = flags & 0x0f;
            int isReturn = (recType == 2 || recType == 4);
            printf("rec off=%d type=%u len=%u pid=%u code=%u depth=%u return=%d\n",
                   off, (unsigned)recType, (unsigned)recLen, (unsigned)pid, (unsigned)code, depth, isReturn);
            if (recLen == 0) break; /* defensive */
            off += recLen;
        }
        return 0;
    }

    usage();
    return 1;
}

/* No main here for linking with test harness; TEST_MAIN may be defined in test build to provide a main. */
