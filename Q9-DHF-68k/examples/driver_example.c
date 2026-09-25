#include "../descriptor/dhf_descriptor.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
    dhf_descriptor_t desc;
    memset(&desc, 0, sizeof(desc));
    while (1) {
        if (desc.command == DHF_DESC_CMD_READ) {
            printf("Driver: Read command received, echoing data...\n");
            // Echo data back
            desc.status = DHF_DESC_STATUS_OK;
            // (In real use, would copy from hardware or buffer)
            // Here, just echo input
            // Set response
            desc.command = 0;
        }
        sleep(1);
    }
    return 0;
}
