#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "q9trace_desc.h"

int main(void)
{
    uint8_t buf[128];
    size_t len;

    /* FLink layout and basic copy */
    memset(buf, 0xAA, sizeof(buf));
    len = q9trace_desc_fill_FLink(buf, "old", "new");
    assert(len == 64);
    assert(buf[0] == 'o');
    assert(buf[1] == 'l');
    assert(buf[2] == 'd');
    assert(buf[3] == '\0');
    assert(buf[32] == 'n');
    assert(buf[33] == 'e');
    assert(buf[34] == 'w');
    assert(buf[63] == '\0');

    /* FFork numeric BE encoding */
    len = q9trace_desc_fill_FFork(buf, 0x11223344u, 0x5566u);
    assert(len == 6);
    assert(buf[0] == 0x11);
    assert(buf[1] == 0x22);
    assert(buf[2] == 0x33);
    assert(buf[3] == 0x44);
    assert(buf[4] == 0x55);
    assert(buf[5] == 0x66);

    /* FLoad path + flags */
    memset(buf, 0, sizeof(buf));
    len = q9trace_desc_fill_FLoad(buf, "path/to/module", 0xDEADBEEFu);
    assert(len == Q9TRACE_DESC_MAX_STR + 4);
    assert(strcmp((char*)buf, "path/to/module") == 0);
    /* flags at offset 32 in big-endian */
    assert(buf[32] == 0xDE);
    assert(buf[33] == 0xAD);
    assert(buf[34] == 0xBE);
    assert(buf[35] == 0xEF);

    /* IOpen flags/mode BE */
    memset(buf, 0xFF, sizeof(buf));
    len = q9trace_desc_fill_IOpen(buf, "file.txt", 0x1122u, 0x3344u);
    assert(len == Q9TRACE_DESC_MAX_STR + 4);
    assert(buf[Q9TRACE_DESC_MAX_STR + 0] == 0x11);
    assert(buf[Q9TRACE_DESC_MAX_STR + 1] == 0x22);
    assert(buf[Q9TRACE_DESC_MAX_STR + 2] == 0x33);
    assert(buf[Q9TRACE_DESC_MAX_STR + 3] == 0x44);

    /* IRead/IWrite layout */
    len = q9trace_desc_fill_IRead(buf, 0x7F7Fu, 0x01020304u);
    assert(len == 6);
    assert(buf[0] == 0x7F);
    assert(buf[1] == 0x7F);
    assert(buf[2] == 0x01);
    assert(buf[3] == 0x02);
    assert(buf[4] == 0x03);
    assert(buf[5] == 0x04);

    len = q9trace_desc_fill_IWrite(buf, 0x0102u, 0xAABBCCDDu);
    assert(len == 6);
    assert(buf[0] == 0x01);
    assert(buf[1] == 0x02);

    /* IClose */
    len = q9trace_desc_fill_IClose(buf, 0x1234u);
    assert(len == 2);
    assert(buf[0] == 0x12);
    assert(buf[1] == 0x34);

    /* String truncation and null-termination tests */
    char longstr[64];
    for (int i = 0; i < 63; i++) longstr[i] = 'X';
    longstr[63] = '\0';
    memset(buf, 0xFF, sizeof(buf));
    len = q9trace_desc_fill_FLink(buf, longstr, "ok");
    /* first field should be 32 bytes with final byte == '\0' */
    assert(buf[31] == 0x00);
    assert(buf[30] == 'X');
    /* second field should contain "ok" */
    assert(buf[32] == 'o');
    assert(buf[33] == 'k');

    /* exact-fit string: 31 chars -> null at position 31 */
    char exact[Q9TRACE_DESC_MAX_STR];
    for (int i = 0; i < (int)Q9TRACE_DESC_MAX_STR - 1; i++) exact[i] = (char)('A' + (i % 26));
    exact[Q9TRACE_DESC_MAX_STR - 1] = '\0';
    memset(buf, 0xEE, sizeof(buf));
    len = q9trace_desc_fill_FLink(buf, exact, "b");
    assert(buf[31] == '\0');
    assert(buf[30] == exact[30]);

    printf("OK\n");
    return 0;
}
