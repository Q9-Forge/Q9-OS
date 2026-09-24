/* test_dhf_socket.c - Test remote socket bridge for DHF filesystem forwarding
 * Simulates 68k RAM array and verifies forwarding across TCP socket to remote daemon.
 */

#include "dhfdrv-68k.h"
#include "dhf_shared.h"
#include "dhf_proto.h"
#include "dhf_emu_device.h"
#include "dhf_socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#define EMU_RAM_SIZE (64 * 1024)
static uint8_t           g_emu_ram[EMU_RAM_SIZE];
static dhf_emu_device_t  g_test_emu;
static volatile int      g_server_stop = 0;
static char              g_test_dir[256];

#define TEST_SOCKET_PORT 9992

#define ADDR_SHARED   0x0000
#define ADDR_PATH     0x1000
#define ADDR_WDATA    0x2000
#define ADDR_RDATA    0x4000

static void* server_thread_fn(void *arg) {
    (void)arg;
    dhf_socket_server_run(TEST_SOCKET_PORT, g_test_dir, &g_server_stop);
    return NULL;
}

static void trigger_emu_hook(void *user_data) {
    dhf_emu_device_t *dev = (dhf_emu_device_t *)user_data;
    dhf_emu_device_process(dev);
}

static uint32_t put_guest_string(uint32_t addr, const char *str) {
    strcpy((char *)&g_emu_ram[addr], str);
    return addr;
}

int main(void) {
    snprintf(g_test_dir, sizeof(g_test_dir), "/tmp/q9_dhf_sock_XXXXXX");
    if (!mkdtemp(g_test_dir)) {
        perror("mkdtemp");
        return 1;
    }
    printf("[SOCKET-TEST] Using temp dir: %s\n", g_test_dir);

    pthread_t th;
    pthread_create(&th, NULL, server_thread_fn, NULL);
    usleep(50000);

    memset(g_emu_ram, 0, sizeof(g_emu_ram));
    struct dhf_shared *shared_area = (struct dhf_shared *)&g_emu_ram[ADDR_SHARED];

    int res = dhf_emu_device_init_remote(&g_test_emu, shared_area, "127.0.0.1", TEST_SOCKET_PORT);
    assert(res == 0);
    dhf_emu_device_set_ram(&g_test_emu, g_emu_ram, sizeof(g_emu_ram));

    dhfdrv_set_shared_mem(shared_area);
    dhfdrv_set_trigger_hook(trigger_emu_hook, &g_test_emu);

    printf("[SOCKET-TEST] Initializing driver over TCP socket...\n");
    uint32_t base_addr = put_guest_string(ADDR_PATH, g_test_dir);
    res = dhfdrv_init((const char *)(uintptr_t)base_addr);
    assert(res == 0);

    printf("[SOCKET-TEST] Creating remote_test.txt over TCP...\n");
    uint32_t fname_addr = put_guest_string(ADDR_PATH, "remote_test.txt");
    int fd = dhfdrv_create((const char *)(uintptr_t)fname_addr, DHF_MODE_WRITE, 0644);
    assert(fd >= 0);

    const char *msg = "Forwarded via TCP socket to remote host filesystem!";
    strcpy((char *)&g_emu_ram[ADDR_WDATA], msg);
    ssize_t w = dhfdrv_write(fd, (const void *)(uintptr_t)ADDR_WDATA, strlen(msg));
    assert(w == (ssize_t)strlen(msg));

    res = dhfdrv_close(fd);
    assert(res == 0);

    printf("[SOCKET-TEST] Reading remote_test.txt back over TCP...\n");
    fd = dhfdrv_open((const char *)(uintptr_t)fname_addr, DHF_MODE_READ);
    assert(fd >= 0);

    memset(&g_emu_ram[ADDR_RDATA], 0, 128);
    ssize_t r = dhfdrv_read(fd, (void *)(uintptr_t)ADDR_RDATA, strlen(msg));
    assert(r == (ssize_t)strlen(msg));
    assert(strcmp((char *)&g_emu_ram[ADDR_RDATA], msg) == 0);

    dhfdrv_close(fd);

    dhfdrv_unlink((const char *)(uintptr_t)fname_addr);
    dhfdrv_term();

    g_server_stop = 1;
    dhf_emu_device_cleanup(&g_test_emu);
    pthread_join(th, NULL);

    rmdir(g_test_dir);
    printf("[SOCKET-TEST] DHF Socket Forwarding tests PASSED successfully!\n");
    return 0;
}
