#ifndef DHF_SHARED_API_H
#define DHF_SHARED_API_H

#include "dhf_shared.h"
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void dhf_shared_init(struct dhf_shared *s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->version = 1;
}

static inline uint32_t dhf_shared_seq(const struct dhf_shared *s) {
    return ntohl(s->seq);
}

static inline void dhf_shared_inc_seq(struct dhf_shared *s) {
    uint32_t v = dhf_shared_seq(s);
    v++;
    s->seq = htonl(v);
}

static inline void dhf_set_command(struct dhf_shared *s, uint8_t cmd) {
    if (!s) return;
    dhf_shared_inc_seq(s);
    __sync_synchronize();
    s->command = cmd;
    __sync_synchronize();
}

static inline int dhf_wait_idle(struct dhf_shared *s, uint32_t start_seq, int timeout_ms) {
    if (!s) return -1;
    const int interval_us = 100;
    int waited_us = 0;
    int max_us = timeout_ms * 1000;
    while (1) {
        if (s->command == 0) return 0;
        if (dhf_shared_seq(s) != start_seq && s->command == 0) return 0;
        if (timeout_ms >= 0 && waited_us >= max_us) return -2;
        usleep(interval_us);
        waited_us += interval_us;
    }
    return -3;
}

static inline uint32_t dhf_get_seq_and_inc(struct dhf_shared *s) {
    uint32_t seq = dhf_shared_seq(s);
    dhf_shared_inc_seq(s);
    return seq;
}

static inline void dhf_set_path_ptr(struct dhf_shared *s, uint32_t guest_addr) {
    if (!s) return;
    s->a0 = htonl(guest_addr);
}

static inline void dhf_set_buffer_ptr(struct dhf_shared *s, uint32_t guest_addr, uint32_t len) {
    if (!s) return;
    s->a1 = htonl(guest_addr);
    s->d1 = htonl(len);
}

static inline uint32_t dhf_get_result_bytes(const struct dhf_shared *s) {
    return ntohl(s->d1);
}

static inline uint8_t dhf_get_status(const struct dhf_shared *s) {
    return s->status;
}

#ifdef __cplusplus
}
#endif

#endif /* DHF_SHARED_API_H */
