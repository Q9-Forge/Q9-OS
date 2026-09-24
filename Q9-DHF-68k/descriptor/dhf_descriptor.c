/* dhf_descriptor: stores base path for root filesystem mapping
 * Provides getter/setter for driver/manager to use. Extended with
 * socket configuration fields for remote shares.
 */

#include "dhf_descriptor.h"
#include <string.h>

static char global_basepath[1024] = "/";

const char* dhf_descriptor_get_basepath(void) { return global_basepath; }
int dhf_descriptor_set_basepath(const char *p) {
    if (!p) return -1;
    strncpy(global_basepath, p, sizeof(global_basepath)-1);
    global_basepath[sizeof(global_basepath)-1] = '\0';
    return 0;
}

int dhf_descriptor_set_socket_enabled(dhf_descriptor_t *d, uint8_t v) {
    if (!d) return -1;
    d->socket_enabled = v ? 1 : 0;
    return 0;
}
int dhf_descriptor_set_socket_address(dhf_descriptor_t *d, const char *addr) {
    if (!d || !addr) return -1;
    strncpy(d->socket_address, addr, sizeof(d->socket_address)-1);
    d->socket_address[sizeof(d->socket_address)-1] = '\0';
    return 0;
}
int dhf_descriptor_set_socket_port(dhf_descriptor_t *d, uint16_t port) {
    if (!d) return -1;
    d->socket_port = port;
    return 0;
}
int dhf_descriptor_set_socket_fd(dhf_descriptor_t *d, int fd) {
    if (!d) return -1;
    d->socket_fd = fd;
    return 0;
}
int dhf_descriptor_get_socket_fd(const dhf_descriptor_t *d) {
    if (!d) return -1;
    return d->socket_fd;
}
int dhf_descriptor_set_socket_flags(dhf_descriptor_t *d, uint32_t flags) {
    if (!d) return -1;
    d->flags = (d->flags & ~0xFFFF0000) | (flags & 0xFFFF);
    return 0;
}
int dhf_descriptor_get_socket_flags(const dhf_descriptor_t *d) {
    if (!d) return 0;
    return d->flags & 0xFFFF;
}

int dhf_descriptor_set_flags(dhf_descriptor_t *d, uint32_t f) {
    if (!d) return -1;
    d->flags = f;
    return 0;
}
uint32_t dhf_descriptor_get_flags(const dhf_descriptor_t *d) {
    if (!d) return 0;
    return d->flags;
}

