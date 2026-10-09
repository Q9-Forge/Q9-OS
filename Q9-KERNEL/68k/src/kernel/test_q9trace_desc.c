#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "q9trace_desc.h"

int main(void)
{
    Q9_u8 buf[128];
    Q9_u32 len;

    const Q9_u32 NAME = Q9TRACE_DESC_NAME_LEN;

    /* FLink layout and basic copy */
    memset(buf, 0xAA, sizeof(buf));
    len = q9trace_desc_fill_FLink(buf, sizeof(buf), "old", "new");
    assert(len == NAME * 2);
    assert(buf[0] == 'o');
    assert(buf[1] == 'l');
    assert(buf[2] == 'd');
    assert(buf[3] == '\0');
    assert(buf[NAME] == 'n');
    assert(buf[NAME + 1] == 'e');
    assert(buf[NAME + 2] == 'w');
    assert(buf[NAME + (NAME - 1)] == '\0');

    /* FFork numeric BE encoding */
    len = q9trace_desc_fill_FFork(buf, sizeof(buf), 0x11223344u, 0x5566u);
    assert(len == 6);
    assert(buf[0] == 0x11);
    assert(buf[1] == 0x22);
    assert(buf[2] == 0x33);
    assert(buf[3] == 0x44);
    assert(buf[4] == 0x55);
    assert(buf[5] == 0x66);

    /* FLoad path + flags */
    memset(buf, 0, sizeof(buf));
    len = q9trace_desc_fill_FLoad(buf, sizeof(buf), "path/to/module", 0xDEADBEEFu);
    assert(len == NAME + 4);
    assert(strcmp((char*)buf, "path/to/module") == 0);
    assert(buf[NAME + 0] == 0xDE);
    assert(buf[NAME + 1] == 0xAD);
    assert(buf[NAME + 2] == 0xBE);
    assert(buf[NAME + 3] == 0xEF);

    /* IOpen flags/mode BE */
    memset(buf, 0xFF, sizeof(buf));
    len = q9trace_desc_fill_IOpen(buf, sizeof(buf), "file.txt", 0x1122u, 0x3344u);
    assert(len == NAME + 4);
    assert(buf[NAME + 0] == 0x11);
    assert(buf[NAME + 1] == 0x22);
    assert(buf[NAME + 2] == 0x33);
    assert(buf[NAME + 3] == 0x44);

    /* IRead/IWrite layout */
    len = q9trace_desc_fill_IRead(buf, sizeof(buf), 0x7F7Fu, 0x01020304u);
    assert(len == 6);
    assert(buf[0] == 0x7F);
    assert(buf[1] == 0x7F);
    assert(buf[2] == 0x01);
    assert(buf[3] == 0x02);
    assert(buf[4] == 0x03);
    assert(buf[5] == 0x04);

    len = q9trace_desc_fill_IWrite(buf, sizeof(buf), 0x0102u, 0xAABBCCDDu);
    assert(len == 6);
    assert(buf[0] == 0x01);
    assert(buf[1] == 0x02);

    /* IClose */
    len = q9trace_desc_fill_IClose(buf, sizeof(buf), 0x1234u);
    assert(len == 2);
    assert(buf[0] == 0x12);
    assert(buf[1] == 0x34);

    /* NULL and empty string handling */
    memset(buf, 0xFF, sizeof(buf));
    len = q9trace_desc_fill_FLink(buf, sizeof(buf), NULL, "ok");
    assert(len == NAME * 2);
    assert(buf[0] == 0x00);

    memset(buf, 0xFF, sizeof(buf));
    len = q9trace_desc_fill_FLink(buf, sizeof(buf), "", "ok");
    assert(len == NAME * 2);
    assert(buf[0] == 0x00);

    /* long string truncation */
    char longstr[64];
    for (int i = 0; i < 63; i++) longstr[i] = 'X';
    longstr[63] = '\0';
    memset(buf, 0xFF, sizeof(buf));
    len = q9trace_desc_fill_FLink(buf, sizeof(buf), longstr, "ok");
    /* first field should end with NUL at index NAME-1 */
    assert(buf[NAME - 1] == 0x00);
    assert(buf[NAME - 2] == 'X');
    assert(buf[NAME] == 'o');
    assert(buf[NAME + 1] == 'k');

    /* exact-fit string: 15 chars (maximum), NUL at position NAME-1 */
    char exact[Q9TRACE_MAX_NAME + 1];
    for (int i = 0; i < (int)Q9TRACE_MAX_NAME; i++) exact[i] = (char)('A' + (i % 26));
    exact[Q9TRACE_MAX_NAME] = '\0';
    memset(buf, 0xEE, sizeof(buf));
    len = q9trace_desc_fill_FLink(buf, sizeof(buf), exact, "b");
    assert(len == NAME * 2);
    assert(buf[NAME - 1] == '\0');
    assert(buf[NAME - 2] == exact[Q9TRACE_MAX_NAME - 1]);

    printf("OK\n");
    return 0;
}
