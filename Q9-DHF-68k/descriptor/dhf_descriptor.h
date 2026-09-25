#ifndef DHF_DESCRIPTOR_H
#define DHF_DESCRIPTOR_H

const char* dhf_descriptor_get_basepath(void);
int dhf_descriptor_set_basepath(const char *p);

// Socket/flags API additions

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dhf_descriptor {
    int socket_fd;
    int socket_flags;
    unsigned int flags;
    unsigned int command;
    unsigned int status;
    unsigned int data_len;
    unsigned char data[256];
} dhf_descriptor_t;

// Socket field accessors
int dhf_descriptor_get_socket_fd(const dhf_descriptor_t *desc);
void dhf_descriptor_set_socket_fd(dhf_descriptor_t *desc, int fd);
int dhf_descriptor_get_socket_flags(const dhf_descriptor_t *desc);
void dhf_descriptor_set_socket_flags(dhf_descriptor_t *desc, int flags);

// Flags accessors
unsigned int dhf_descriptor_get_flags(const dhf_descriptor_t *desc);
void dhf_descriptor_set_flags(dhf_descriptor_t *desc, unsigned int flags);

// Command/Status/Constants
#define DHF_DESC_CMD_READ 1
#define DHF_DESC_STATUS_OK 0
#define DHF_DESC_FLAG_READ_PENDING 0x01

#ifdef __cplusplus
}
#endif

#endif
