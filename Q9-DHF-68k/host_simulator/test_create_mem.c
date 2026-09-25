#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../include/dhf_shared_hostsim.h"

int main(int argc, char **argv) {
    const char *out = "/tmp/dhf_mem.bin";
    size_t sz = 65536;
    FILE *f = fopen(out, "wb");
    if (!f) { perror("fopen"); return 1; }
    // zero buffer
    void *buf = calloc(1, sz);
    if (!buf) { perror("calloc"); fclose(f); return 1; }
    // populate dhf_shared at offset 0
    dhf_shared_t *sh = (dhf_shared_t*)buf;
    sh->version = 1;
    sh->seq = 1;
    sh->command = 0x0002; // Open
    // param[0] = path offset, param[1]=buffer offset, param[2]=fd placeholder
    sh->param[0] = 256;
    sh->param[1] = 512;
    sh->param[2] = 0;
    sh->param[3] = 0;
    sh->param[4] = 0;
    // path string at offset 256
    const char *path = "/tmp/dhf_test.txt";
    strncpy((char*)buf + 256, path, 256);
    // write file
    fwrite(buf, 1, sz, f);
    fclose(f);
    free(buf);
    printf("Wrote test memory file: %s (size=%zu)\n", out, sz);
    return 0;
}
