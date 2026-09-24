/* dhf_socket_server.c - Server side for remote DHF filesystem daemon
 */

#include "dhf_socket.h"
#include "../emulator/dhf_host_fs.h"
#include "../include/dhf_proto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>

static int send_all(int fd, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = (const char*)buf;
    while (total < len) {
        ssize_t n = send(fd, p + total, len - total, 0);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

static int recv_all(int fd, void *buf, size_t len) {
    size_t total = 0;
    char *p = (char*)buf;
    while (total < len) {
        ssize_t n = recv(fd, p + total, len - total, 0);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

static int handle_client_request(dhf_host_fs_t *fs, int client_fd) {
    dhf_net_header_t hdr;
    if (recv_all(client_fd, &hdr, sizeof(hdr)) != 0 || ntohl(hdr.magic) != DHF_NET_MAGIC) {
        return -1;
    }

    uint8_t cmd = hdr.command;
    uint32_t d0 = ntohl(hdr.d0);
    uint32_t d1 = ntohl(hdr.d1);
    uint32_t d2 = ntohl(hdr.d2);
    uint32_t plen = ntohl(hdr.path_len);
    uint32_t dlen = ntohl(hdr.data_len);

    char path[DHF_PATH_MAX] = {0};
    if (plen > 0) {
        size_t rplen = plen < sizeof(path) ? plen : sizeof(path) - 1;
        if (recv_all(client_fd, path, plen) != 0) return -1;
        path[rplen] = '\0';
    }

    char in_data[65536];
    if (dlen > 0) {
        if (dlen > sizeof(in_data)) return -1;
        if (recv_all(client_fd, in_data, dlen) != 0) return -1;
    }

    uint8_t status = DHF_ERR_OK;
    char out_data[65536];
    uint32_t out_len = 0;

    switch (cmd) {
        case DHF_CMD_INIT:
            if (path[0]) dhf_host_fs_init(fs, path);
            status = DHF_ERR_OK;
            break;

        case DHF_CMD_TERM:
            dhf_host_fs_cleanup(fs);
            status = DHF_ERR_OK;
            break;

        case DHF_CMD_OPEN: {
            int h = dhf_host_fs_open(fs, path, (int)d2, &status);
            if (h >= 0) d0 = (uint32_t)h;
            break;
        }

        case DHF_CMD_CREATE: {
            int h = dhf_host_fs_create(fs, path, (int)d2, (int)d1, &status);
            if (h >= 0) d0 = (uint32_t)h;
            break;
        }

        case DHF_CMD_CLOSE:
            dhf_host_fs_close(fs, (int)d0, &status);
            break;

        case DHF_CMD_READ: {
            size_t req = d1 < sizeof(out_data) ? d1 : sizeof(out_data);
            ssize_t r = dhf_host_fs_read(fs, (int)d0, out_data, req, &status);
            if (r >= 0) {
                out_len = (uint32_t)r;
                d1 = out_len;
            } else {
                d1 = 0;
            }
            break;
        }

        case DHF_CMD_WRITE: {
            ssize_t w = dhf_host_fs_write(fs, (int)d0, in_data, dlen, &status);
            d1 = (w >= 0) ? (uint32_t)w : 0;
            break;
        }

        case DHF_CMD_SEEK: {
            off_t pos = dhf_host_fs_seek(fs, (int)d0, (off_t)d1, (int)d2, &status);
            if (pos != (off_t)-1) d1 = (uint32_t)pos;
            break;
        }

        case DHF_CMD_READLN: {
            size_t req = d1 < sizeof(out_data) ? d1 : sizeof(out_data);
            int r = dhf_host_fs_readln(fs, (int)d0, out_data, req, &status);
            if (r >= 0) {
                out_len = (uint32_t)r;
                d1 = out_len;
            } else {
                d1 = 0;
            }
            break;
        }

        case DHF_CMD_WRITELN: {
            int w = dhf_host_fs_writeln(fs, (int)d0, in_data, dlen, &status);
            d1 = (w >= 0) ? (uint32_t)w : 0;
            break;
        }

        case DHF_CMD_GETSTT: {
            size_t stlen = 0;
            dhf_host_fs_getstat(fs, path, out_data, &stlen, &status);
            out_len = (uint32_t)stlen;
            d1 = out_len;
            break;
        }

        case DHF_CMD_SETSTT:
            dhf_host_fs_setstat(fs, path, in_data, &status);
            break;

        case DHF_CMD_CHDIR:
            dhf_host_fs_chdir(fs, path, &status);
            break;

        case DHF_CMD_MKDIR:
            dhf_host_fs_mkdir(fs, path, (int)d1, &status);
            break;

        case DHF_CMD_RMDIR:
            dhf_host_fs_rmdir(fs, path, &status);
            break;

        case DHF_CMD_DELETE:
            dhf_host_fs_unlink(fs, path, &status);
            break;

        case DHF_CMD_RENAME:
            dhf_host_fs_rename(fs, path, in_data, &status);
            break;

        case DHF_CMD_OPENDIR: {
            int h = dhf_host_fs_opendir(fs, path, &status);
            if (h >= 0) d0 = (uint32_t)h;
            break;
        }

        case DHF_CMD_READDIR: {
            char entry_name[DHF_PATH_MAX];
            uint32_t fsize = 0, fmode = 0;
            int ret = dhf_host_fs_readdir(fs, (int)d0, entry_name, sizeof(entry_name), &fsize, &fmode, &status);
            if (ret > 0) {
                size_t nlen = strlen(entry_name) + 1;
                memcpy(out_data, entry_name, nlen);
                uint32_t meta[2] = { htonl(fsize), htonl(fmode) };
                memcpy(out_data + nlen, meta, sizeof(meta));
                out_len = (uint32_t)(nlen + sizeof(meta));
                d1 = out_len;
            } else {
                d1 = 0;
            }
            break;
        }

        case DHF_CMD_PING:
            status = DHF_ERR_OK;
            break;

        default:
            status = DHF_ERR_UNSUPPORTED;
            break;
    }

    /* Send response */
    dhf_net_header_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.magic = htonl(DHF_NET_MAGIC);
    resp.command = cmd;
    resp.status = status;
    resp.d0 = htonl(d0);
    resp.d1 = htonl(d1);
    resp.d2 = htonl(d2);
    resp.path_len = 0;
    resp.data_len = htonl(out_len);

    if (send_all(client_fd, &resp, sizeof(resp)) != 0 ||
        (out_len > 0 && send_all(client_fd, out_data, out_len) != 0)) {
        return -1;
    }

    return 0;
}

int dhf_socket_server_run(int port, const char *basepath, volatile int *stop_flag) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) return -1;

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((uint16_t)(port > 0 ? port : DHF_DEFAULT_PORT));
    sin.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr*)&sin, sizeof(sin)) != 0) {
        close(listen_fd);
        return -1;
    }

    if (listen(listen_fd, 5) != 0) {
        close(listen_fd);
        return -1;
    }

    while (!stop_flag || !*stop_flag) {
        struct sockaddr_in cin;
        socklen_t clen = sizeof(cin);
        int client_fd = accept(listen_fd, (struct sockaddr*)&cin, &clen);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            break;
        }

        int flag = 1;
        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

        dhf_host_fs_t fs;
        dhf_host_fs_init(&fs, basepath);

        while (!stop_flag || !*stop_flag) {
            if (handle_client_request(&fs, client_fd) != 0) {
                break;
            }
        }

        dhf_host_fs_cleanup(&fs);
        close(client_fd);
    }

    close(listen_fd);
    return 0;
}
