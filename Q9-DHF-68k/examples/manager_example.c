#include "../descriptor/dhf_descriptor.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
    // Example: set/get socket fields, flags, and issue a Read command
    dhf_descriptor_t desc;
    memset(&desc, 0, sizeof(desc));
    dhf_descriptor_set_socket_fd(&desc, 42);
    dhf_descriptor_set_socket_flags(&desc, 0xA5);
    printf("Socket FD: %d\n", dhf_descriptor_get_socket_fd(&desc));
    printf("Socket Flags: 0x%X\n", dhf_descriptor_get_socket_flags(&desc));
    dhf_descriptor_set_flags(&desc, DHF_DESC_FLAG_READ_PENDING);
    printf("Flags: 0x%X\n", dhf_descriptor_get_flags(&desc));
    // Issue a Read command (simulate)
    desc.command = DHF_DESC_CMD_READ;
    desc.data_len = 8;
    strcpy((char*)desc.data, "ABCDEFGH");
    printf("Issued Read command, waiting for response...\n");
    // Simulate wait for response
    sleep(1);
    // Simulate response
    desc.status = DHF_DESC_STATUS_OK;
    printf("Response status: %d, data: %s\n", desc.status, desc.data);
    return 0;
}
