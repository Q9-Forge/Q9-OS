/* test_dhfdrv.c - End-to-end regression test for Q9-DHF 68k driver & emulator device
 * Simulates 68k RAM array where 68k pointers are direct offsets into emulator memory.
 */

#include "dhfdrv-68k.h"
#include "dhf_shared.h"
#include "dhf_proto.h"
#include "dhf_emu_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>

#define EMU_RAM_SIZE (64 * 1024) /* 64 KB simulated 68k RAM */
static uint8_t           g_emu_ram[EMU_RAM_SIZE];
static dhf_emu_device_t  g_test_emu;

#define ADDR_SHARED   0x0000
#define ADDR_PATH_BASE 0x1000
#define ADDR_PATH_FILE 0x1100
#define ADDR_PATH_SUB  0x1200
#define ADDR_PATH_DOT  0x1300
#define ADDR_WDATA     0x2000
#define ADDR_RDATA     0x4000
#define ADDR_DIR       0x6000

static void trigger_emu_hook(void *user_data) {
    dhf_emu_device_t *dev = (dhf_emu_device_t *)user_data;
    dhf_emu_device_process(dev);
}

static uint32_t put_guest_string(uint32_t addr, const char *str) {
    strcpy((char *)&g_emu_ram[addr], str);
    return addr;
}

int main(void) {
    char test_dir[] = "/tmp/q9_dhf_test_XXXXXX";
    if (!mkdtemp(test_dir)) {
        perror("mkdtemp");
        return 1;
    }
    printf("[TEST] Using temp dir: %s\n", test_dir);

    memset(g_emu_ram, 0, sizeof(g_emu_ram));
    struct dhf_shared *shared_area = (struct dhf_shared *)&g_emu_ram[ADDR_SHARED];

    int res = dhf_emu_device_init_local(&g_test_emu, shared_area, test_dir);
    assert(res == 0);
    dhf_emu_device_set_ram(&g_test_emu, g_emu_ram, sizeof(g_emu_ram));

    dhfdrv_set_shared_mem(shared_area);
    dhfdrv_set_trigger_hook(trigger_emu_hook, &g_test_emu);

    printf("[TEST] Initializing driver...\n");
    uint32_t base_addr = put_guest_string(ADDR_PATH_BASE, test_dir);
    res = dhfdrv_init((const char *)(uintptr_t)base_addr);
    assert(res == 0);

    /* 1. Create file */
    printf("[TEST] Creating file hello.txt...\n");
    uint32_t fname_addr = put_guest_string(ADDR_PATH_FILE, "hello.txt");
    int fd = dhfdrv_create((const char *)(uintptr_t)fname_addr, DHF_MODE_WRITE, 0644);
    assert(fd >= 0);

    /* 2. Write data directly from simulated 68k RAM buffer */
    const char *msg = "Hello Q9-DHF Zero-Copy Host Filesystem!";
    strcpy((char *)&g_emu_ram[ADDR_WDATA], msg);
    ssize_t w = dhfdrv_write(fd, (const void *)(uintptr_t)ADDR_WDATA, strlen(msg));
    assert(w == (ssize_t)strlen(msg));

    res = dhfdrv_close(fd);
    assert(res == 0);

    /* 3. Stat file */
    printf("[TEST] Getting stat for hello.txt...\n");
    res = dhfdrv_getstat((const char *)(uintptr_t)fname_addr, (void *)(uintptr_t)ADDR_RDATA);
    assert(res == 0);
    uint32_t *stat_fields = (uint32_t *)&g_emu_ram[ADDR_RDATA];
    uint32_t file_size = ntohl(stat_fields[0]);
    assert(file_size == strlen(msg));

    /* 4. Open file for reading */
    printf("[TEST] Opening file for read...\n");
    fd = dhfdrv_open((const char *)(uintptr_t)fname_addr, DHF_MODE_READ);
    assert(fd >= 0);

    /* 5. Read data back directly into 68k RAM buffer */
    memset(&g_emu_ram[ADDR_RDATA], 0, 128);
    ssize_t r = dhfdrv_read(fd, (void *)(uintptr_t)ADDR_RDATA, strlen(msg));
    assert(r == (ssize_t)strlen(msg));
    assert(strcmp((char *)&g_emu_ram[ADDR_RDATA], msg) == 0);

    /* 6. Seek test */
    off_t pos = dhfdrv_seek(fd, 6, DHF_SEEK_SET);
    assert(pos == 6);
    memset(&g_emu_ram[ADDR_RDATA], 0, 128);
    r = dhfdrv_read(fd, (void *)(uintptr_t)ADDR_RDATA, 6);
    assert(r == 6);
    assert(strncmp((char *)&g_emu_ram[ADDR_RDATA], "Q9-DHF", 6) == 0);

    res = dhfdrv_close(fd);
    assert(res == 0);

    /* 7. Mkdir and Directory listing */
    printf("[TEST] Creating subdirectory subdir...\n");
    uint32_t sub_addr = put_guest_string(ADDR_PATH_SUB, "subdir");
    res = dhfdrv_mkdir((const char *)(uintptr_t)sub_addr, 0755);
    assert(res == 0);

    uint32_t dot_addr = put_guest_string(ADDR_PATH_DOT, ".");
    int dfd = dhfdrv_opendir((const char *)(uintptr_t)dot_addr);
    assert(dfd >= 0);

    int found_hello = 0, found_subdir = 0;
    while ((r = dhfdrv_readdir(dfd, (void *)(uintptr_t)ADDR_DIR)) > 0) {
        char *entry_name = (char *)&g_emu_ram[ADDR_DIR];
        if (strcmp(entry_name, "hello.txt") == 0) found_hello = 1;
        if (strcmp(entry_name, "subdir") == 0) found_subdir = 1;
    }
    dhfdrv_close(dfd);
    assert(found_hello == 1);
    assert(found_subdir == 1);

    /* 8. Unlink file */
    printf("[TEST] Deleting hello.txt...\n");
    res = dhfdrv_unlink((const char *)(uintptr_t)fname_addr);
    assert(res == 0);

    fd = dhfdrv_open((const char *)(uintptr_t)fname_addr, DHF_MODE_READ);
    assert(fd < 0);

    /* 9. Clean up */
    dhfdrv_rmdir((const char *)(uintptr_t)sub_addr);
    dhfdrv_term();
    dhf_emu_device_cleanup(&g_test_emu);

    rmdir(test_dir);
    printf("[TEST] All DHF Driver and Emulator tests PASSED successfully!\n");
    return 0;
}
