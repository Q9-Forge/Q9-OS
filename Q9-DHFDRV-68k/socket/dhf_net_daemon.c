/* dhf_net_daemon.c - Standalone DHF Remote Filesystem Daemon
 * Run on a remote host to serve files to Q9-Flux over TCP socket.
 */

#include "dhf_socket.h"
#include "../include/dhf_proto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static volatile int g_stop = 0;

static void handle_sig(int sig) {
    (void)sig;
    g_stop = 1;
}

int main(int argc, char *argv[]) {
    int port = DHF_DEFAULT_PORT;
    const char *basepath = ".";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 < argc) basepath = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [-p port] [-d root_directory]\n", argv[0]);
            printf("Default port: %d, default dir: .\n", DHF_DEFAULT_PORT);
            return 0;
        }
    }

    signal(SIGINT, handle_sig);
    signal(SIGTERM, handle_sig);

    printf("[dhf-net-daemon] Starting on port %d serving '%s'...\n", port, basepath);
    int res = dhf_socket_server_run(port, basepath, &g_stop);
    printf("[dhf-net-daemon] Stopped (code %d).\n", res);
    return res;
}
