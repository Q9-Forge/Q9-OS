#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>

#include "../include/dhf_shared_hostsim.h"
#include "fs_ops.h"
#include "errno_map.h"
#include "../descriptor/dhf_descriptor.h"
#include <errno.h>
#include <limits.h>

static volatile int keep_running = 1;
static void sigint_handler(int sig) { (void)sig; keep_running = 0; }

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <emulator_mem_file>\n", argv[0]);
        return 2;
    }
    signal(SIGINT, sigint_handler);

    const char *path = argv[1];
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("fstat");
        close(fd);
        return 1;
    }
    size_t map_sz = (size_t)st.st_size;
    if (map_sz < sizeof(dhf_shared_t)) {
        fprintf(stderr, "file too small for dhf_shared_t (%zu < %zu)\n", map_sz, sizeof(dhf_shared_t));
        close(fd);
        return 1;
    }
    void *mem = mmap(NULL, map_sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    dhf_shared_t *sh = (dhf_shared_t *)mem;

    // dir table for simple DIR* tracking (supports up to 16 concurrent handles)
    typedef struct { int used; DIR *dptr; } dir_entry_t;
    dir_entry_t dir_table[16];
    memset(dir_table, 0, sizeof(dir_table));

    printf("DHF host-simulator (file-backed) started. Monitoring commands...\n");
    uint32_t last_seq = sh->seq;
    uint16_t last_cmd = sh->command;

    while (keep_running) {
        uint32_t seq = sh->seq;
        uint16_t cmd = sh->command;
        if (cmd != 0 && (cmd < 0x0080)) { // request range
            printf("Detected command 0x%04x seq=%u\n", cmd, seq);
            for (int i = 0; i < 5; ++i) printf(" param[%d]=0x%08x\n", i, (unsigned)sh->param[i]);

            // Basic handlers
            if (cmd == 0x0021) { // Descriptor Init
                printf("Handling Descriptor Init\n");
                sh->result_code = 0; // ok
            } else if (cmd == 0x0022) { // Ping
                printf("Handling Ping\n");
                sh->result_code = 0; // ok
            } else if (cmd >= 0x0001 && cmd <= 0x000D) {
                // Map simple manager FM calls: use param[0] as pointer to path (in file-backed memory we treat as offset)
                // For file-backed test harness, param[0] contains offset into the mapped file where a NUL-terminated path is stored.
                uint32_t p0 = sh->param[0];
                char *path = (char *)((uint8_t*)mem + p0);
                printf("FM command 0x%04x path@0x%08x -> '%s'\n", cmd, p0, path);
                if (cmd == 0x0001) { // Create -> open with O_CREAT
                    int fd = dhf_host_open(dhf_descriptor_get_basepath(), path, O_CREAT | O_RDWR, 0644);
                    if (fd < 0) sh->result_code = dhf_map_errno(errno); else { sh->result_code = 0; sh->result_len = (uint32_t)fd; }
                } else if (cmd == 0x0002) { // Open
                    int fd = dhf_host_open(dhf_descriptor_get_basepath(), path, O_RDONLY, 0);
                    if (fd < 0) sh->result_code = dhf_map_errno(errno); else { sh->result_code = 0; sh->result_len = (uint32_t)fd; }
                } else if (cmd == 0x0003) { // Seek (not implemented)
                    sh->result_code = (uint32_t)ENOSYS;
                } else if (cmd == 0x0004) { // Read
                    // param[2] = host fd, param[1]=buffer offset in emulator mem, param[3]=length
                    int hfd = (int)sh->param[2];
                    uint32_t buf_off = sh->param[1];
                    uint32_t len = sh->param[3];
                    if (buf_off + len > map_sz) { sh->result_code = (uint32_t)EFAULT; }
                    else {
                        ssize_t r = dhf_host_read(hfd, (uint8_t*)mem + buf_off, len);
                        if (r < 0) sh->result_code = dhf_map_errno(errno); else { sh->result_code = 0; sh->result_len = (uint32_t)r; }
                    }
                } else if (cmd == 0x0005) { // Write
                    // param[2] = host fd, param[1]=buffer offset in emulator mem, param[3]=length
                    int hfd = (int)sh->param[2];
                    uint32_t buf_off = sh->param[1];
                    uint32_t len = sh->param[3];
                    if (buf_off + len > map_sz) { sh->result_code = (uint32_t)EFAULT; }
                    else {
                        ssize_t w = dhf_host_write(hfd, (uint8_t*)mem + buf_off, len);
                        if (w < 0) sh->result_code = dhf_map_errno(errno); else { sh->result_code = 0; sh->result_len = (uint32_t)w; }
                    }
                } else if (cmd == 0x0006) { // Close
                    {
                        int hfd = (int)sh->param[2];
                        if (dhf_host_close(hfd) != 0) sh->result_code = dhf_map_errno(errno); else sh->result_code = 0;
                    }
                    // Also support closing dir handles via param[2] if param[4]==1
                    if (sh->param[4] == 1) {
                        int dir_handle = (int)sh->param[2];
                        if (dir_handle > 0 && dir_handle <= 16 && dir_table[dir_handle-1].used) {
                            closedir(dir_table[dir_handle-1].dptr);
                            dir_table[dir_handle-1].used = 0;
                            dir_table[dir_handle-1].dptr = NULL;
                        }
                    }
                } else if (cmd == 0x0007) { // GetStat
                    // param[0]=path offset; result_len returns size of struct stat filled into emulator memory at param[1]
                    uint32_t p0 = sh->param[0];
                    uint32_t out_off = sh->param[1];
                    char *path = (char *)((uint8_t*)mem + p0);
                    char real[PATH_MAX];
                    if (confined_path(dhf_descriptor_get_basepath(), path, real, sizeof(real)) != 0) {
                        sh->result_code = (uint32_t)EACCES;
                    } else {
                        struct stat stbuf;
                        if (stat(real, &stbuf) != 0) { sh->result_code = dhf_map_errno(errno); }
                        else {
                            // write stat struct into emulator memory at out_off if space
                            if (out_off + sizeof(struct stat) > map_sz) { sh->result_code = (uint32_t)EFAULT; }
                            else {
                                memcpy((uint8_t*)mem + out_off, &stbuf, sizeof(struct stat));
                                sh->result_code = 0;
                                sh->result_len = sizeof(struct stat);
                            }
                        }
                    }
                } else if (cmd == 0x0008) { // SetStat
                    // Implement SetStat: copy struct stat from param[1] offset and apply via utimens/chmod/chown where possible
                    uint32_t in_off = sh->param[1];
                    uint32_t p0 = sh->param[0];
                    char *path = (char *)((uint8_t*)mem + p0);
                    char real[PATH_MAX];
                    if (in_off + sizeof(struct stat) > map_sz) { sh->result_code = dhf_map_errno(EFAULT); }
                    else if (confined_path(dhf_descriptor_get_basepath(), path, real, sizeof(real)) != 0) { sh->result_code = dhf_map_errno(EACCES); }
                    else {
                        struct stat st_in;
                        memcpy(&st_in, (uint8_t*)mem + in_off, sizeof(struct stat));
                        // Apply mode change if provided
                        if (chmod(real, st_in.st_mode) != 0) {
                            sh->result_code = dhf_map_errno(errno);
                        } else {
                            sh->result_code = 0;
                        }
                    }
                } else if (cmd == 0x0009) { // Opendir
                    // param[0]=path offset, result_len will return a fake dirfd (int)
                    uint32_t p0 = sh->param[0];
                    char *path = (char *)((uint8_t*)mem + p0);
                    char real[PATH_MAX];
                    if (confined_path(dhf_descriptor_get_basepath(), path, real, sizeof(real)) != 0) {
                        sh->result_code = (uint32_t)EACCES;
                    } else {
                        // allocate a DIR* slot
                        int slot = -1;
                        for (int i = 0; i < 16; ++i) if (!dir_table[i].used) { slot = i; break; }
                        if (slot < 0) { sh->result_code = dhf_map_errno(EMFILE); }
                        else {
                            DIR *d = opendir(real);
                            if (!d) { sh->result_code = dhf_map_errno(errno); }
                            else {
                                dir_table[slot].used = 1;
                                dir_table[slot].dptr = d;
                                sh->result_code = 0;
                                sh->result_len = (uint32_t)(slot+1); // handle = slot+1
                            }
                        }
                    }
                } else if (cmd == 0x000A) { // Readdir
                    // Simple readdir implementation for a single opendir handle 1: param[2]=dir handle, param[1]=out buffer offset
                    int dir_handle = (int)sh->param[2];
                    uint32_t out_off = sh->param[1];
                    if (dir_handle != 1) { sh->result_code = dhf_map_errno(EBADF); }
                    else {
                        // Read next entry from an opened directory tracked by dir table
                        uint32_t p0 = sh->param[0];
                        char *path = (char *)((uint8_t*)mem + p0);
                        char real[PATH_MAX];
                        if (confined_path(dhf_descriptor_get_basepath(), path, real, sizeof(real)) != 0) {
                            sh->result_code = (uint32_t)EACCES;
                        } else {
                            // Lookup DIR* by handle (param[2])
                            int dir_handle = (int)sh->param[2];
                            if (dir_handle <= 0 || dir_handle > 16 || !dir_table[dir_handle-1].used) { sh->result_code = (uint32_t)EBADF; }
                            else {
                                DIR *d = dir_table[dir_handle-1].dptr;
                                struct dirent *de = readdir(d);
                                if (!de) {
                                    if (errno != 0) sh->result_code = dhf_map_errno(errno); else sh->result_code = dhf_map_errno(ENOENT);
                                } else {
                                    size_t namelen = strlen(de->d_name) + 1;
                                    if (out_off + namelen > map_sz) { sh->result_code = dhf_map_errno(EFAULT); }
                                    else {
                                        memcpy((uint8_t*)mem + out_off, de->d_name, namelen);
                                        sh->result_code = 0;
                                        sh->result_len = (uint32_t)namelen;
                                    }
                                }
                            }
                        }
                    }
                } else if (cmd == 0x000C) { // MkDir
                    char real[PATH_MAX];
                    if (confined_path(dhf_descriptor_get_basepath(), path, real, sizeof(real)) != 0) {
                        sh->result_code = (uint32_t)EACCES;
                    } else if (mkdir(real, 0755) != 0) {
                        sh->result_code = (uint32_t)errno;
                    } else sh->result_code = 0;
                } else if (cmd == 0x000B) { // Delete / unlink
                    char real[PATH_MAX];
                    if (confined_path(dhf_descriptor_get_basepath(), path, real, sizeof(real)) != 0) {
                        sh->result_code = (uint32_t)EACCES;
                    } else if (unlink(real) != 0) {
                        sh->result_code = (uint32_t)errno;
                    } else sh->result_code = 0;
                } else {
                    printf("FM cmd 0x%04x not implemented in host-sim yet\n", cmd);
                    sh->result_code = (uint32_t)ENOSYS;
                }
            } else {
                printf("Unhandled command 0x%04x - echoing OK\n", cmd);
                sh->result_code = 0; // ok
            }

            // Set response command (0x0080 == OK)
            sh->command = 0x0080;

            // bump seq to indicate update (simple approach)
            sh->seq = seq + 1;

            // wait for manager to clear or change command
            int waitcnt = 0;
            while (sh->command == 0x0080 && keep_running && waitcnt++ < 1000) {
                usleep(1000);
            }
        }
        usleep(1000*50);
    }

    if (munmap(mem, map_sz) != 0) perror("munmap");
    close(fd);
    printf("DHF host-simulator exiting.\n");
    return 0;
}
