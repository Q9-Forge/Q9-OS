/* dhf_descriptor: stores base path for root filesystem mapping
 * Provides getter/setter for driver/manager to use.
 */

#include "dhf_descriptor.h"
#include <string.h>

static char basepath[1024] = "/tmp";

const char* dhf_descriptor_get_basepath(void) { return basepath; }
int dhf_descriptor_set_basepath(const char *p) {
    if (!p) return -1;
    strncpy(basepath, p, sizeof(basepath)-1);
    basepath[sizeof(basepath)-1] = '\0';
    return 0;
}

// Socket field accessors
int dhf_descriptor_get_socket_fd(const dhf_descriptor_t *desc) {
    return desc ? desc->socket_fd : -1;
}
void dhf_descriptor_set_socket_fd(dhf_descriptor_t *desc, int fd) {
    if (desc) desc->socket_fd = fd;
}
int dhf_descriptor_get_socket_flags(const dhf_descriptor_t *desc) {
    return desc ? desc->socket_flags : 0;
}
void dhf_descriptor_set_socket_flags(dhf_descriptor_t *desc, int flags) {
    if (desc) desc->socket_flags = flags;
}

// Flags accessors
unsigned int dhf_descriptor_get_flags(const dhf_descriptor_t *desc) {
    return desc ? desc->flags : 0;
}
void dhf_descriptor_set_flags(dhf_descriptor_t *desc, unsigned int flags) {
    if (desc) desc->flags = flags;
}

