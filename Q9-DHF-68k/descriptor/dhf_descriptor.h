#ifndef DHF_DESCRIPTOR_H
#define DHF_DESCRIPTOR_H

#include <stdint.h>

#define DHF_DESC_FLAG_READ_PENDING 0x01

typedef struct dhf_descriptor {
    char basepath[1024];
    uint8_t socket_enabled;
    char socket_address[128];
    uint16_t socket_port;
    int socket_fd;
    uint32_t flags;

    /* small example command area for example programs */
    uint8_t command;
    uint8_t status;
    uint32_t data_len;
    uint8_t data[256];
} dhf_descriptor_t;

const char* dhf_descriptor_get_basepath(void);
int dhf_descriptor_set_basepath(const char *p);

/* socket helpers */
int dhf_descriptor_set_socket_enabled(dhf_descriptor_t *d, uint8_t v);
int dhf_descriptor_set_socket_address(dhf_descriptor_t *d, const char *addr);
int dhf_descriptor_set_socket_port(dhf_descriptor_t *d, uint16_t port);
int dhf_descriptor_set_socket_fd(dhf_descriptor_t *d, int fd);
int dhf_descriptor_get_socket_fd(const dhf_descriptor_t *d);
int dhf_descriptor_set_socket_flags(dhf_descriptor_t *d, uint32_t flags);
int dhf_descriptor_get_socket_flags(const dhf_descriptor_t *d);

/* flags */
int dhf_descriptor_set_flags(dhf_descriptor_t *d, uint32_t f);
uint32_t dhf_descriptor_get_flags(const dhf_descriptor_t *d);

/* example commands/status values */
enum { DHF_DESC_CMD_IDLE=0, DHF_DESC_CMD_READ=1 };
enum { DHF_DESC_STATUS_OK=0, DHF_DESC_STATUS_ERROR=1 };

#endif
