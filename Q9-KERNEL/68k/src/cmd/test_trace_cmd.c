#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Provide a mock q9syscall for testing */
int last_call = -1, last_sub = -1;
int q9syscall(int callcode, int subcode, void *a0, int d1, int d2, int d3)
{
    last_call = callcode; last_sub = subcode;
    if (subcode == 6) {
        /* simulate reading two records: each 12-byte header + 2-byte payload */
        unsigned char *buf = (unsigned char *)a0;
        if (d2 < 28) return -1;
        /* rec1 */
        buf[0] = 1; buf[1] = 14; buf[2] = 5; buf[3] = 0; buf[4] = 0; buf[5] = 1; /* pid=1 */
        /* tick/fine */ buf[6]=buf[7]=buf[8]=buf[9]=0; buf[10]=buf[11]=0;
        buf[12]=0xAA; buf[13]=0xBB; /* payload */
        /* rec2 */
        buf[14] = 2; buf[15]=14; buf[16]=7; buf[17]=0x11; buf[18]=0x00; buf[19]=0x02; /* pid=2 */
        buf[20]=buf[21]=buf[22]=buf[23]=0; buf[24]=buf[25]=0;
        buf[26]=0xCC; buf[27]=0xDD;
        return 28;
    }
    return 0;
}

/* Declare the command under test */
int cmd_trace(int argc, char **argv);

int main(void)
{
    char *argv_on[] = {"trace","on", NULL};
    cmd_trace(2, argv_on);
    assert(last_call == 0x58 && last_sub == 1);

    char *argv_off[] = {"trace","off", NULL};
    cmd_trace(2, argv_off);
    assert(last_call == 0x58 && last_sub == 1);

    char *argv_clear[] = {"trace","clear", NULL};
    cmd_trace(2, argv_clear);
    assert(last_call == 0x58 && last_sub == 2);

    char *argv_show[] = {"trace","show", NULL};
    cmd_trace(2, argv_show);
    assert(last_call == 0x58 && last_sub == 6);

    printf("TEST_OK\n");
    return 0;
}
