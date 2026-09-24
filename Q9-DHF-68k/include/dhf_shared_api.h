#ifndef DHF_SHARED_API_H
#define DHF_SHARED_API_H

#include "dhf_shared.h"
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

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

static inline void dhf_set_command(struct dhf_shared *s, uint16_t cmd) {
    if (!s) return;
    /* increment seq to mark new request */
    dhf_shared_inc_seq(s);
    __sync_synchronize(); /* memory barrier */
    s->command = htons(cmd);
}

/* Wait until driver sets command back to 0 (idle) or timeout_ms elapses. */
static inline int dhf_wait_idle(struct dhf_shared *s, uint32_t start_seq, int timeout_ms) {
    if (!s) return -1;
    const int interval_us = 1000; /* 1ms poll */
    int waited = 0;
    while (1) {
        if (ntohs(s->command) == 0) return 0;
        /* if seq changed and command cleared, treat as completed */
        if (dhf_shared_seq(s) != start_seq && ntohs(s->command) == 0) return 0;
        if (timeout_ms >= 0 && waited >= timeout_ms) return -2; /* timeout */
        usleep(interval_us);
        waited += interval_us/1000;
    }
    return -3;
}

static inline uint32_t dhf_get_seq_and_inc(struct dhf_shared *s) {
    uint32_t seq = dhf_shared_seq(s);
    dhf_shared_inc_seq(s);
    return seq;
}

static inline void dhf_set_param(struct dhf_shared *s, int idx, uint32_t v) {
    if (!s || idx<0 || idx>=5) return;
    s->param[idx] = htonl(v);
}

static inline uint32_t dhf_get_param(const struct dhf_shared *s, int idx) {
    if (!s || idx<0 || idx>=5) return 0;
    return ntohl(s->param[idx]);
}

static inline void dhf_set_name_external_flag(struct dhf_shared *s) {
    if (!s) return;
    s->param[0] = htonl(1);
}

static inline void dhf_set_buffer_external_flag(struct dhf_shared *s) {
    if (!s) return;
    s->param[1] = htonl(1);
}

static inline uint32_t dhf_get_result_bytes(const struct dhf_shared *s) {
    return ntohl(s->param[1]);
}

static inline uint8_t dhf_get_status(const struct dhf_shared *s) {
    return s->status;
}

static inline int dhf_is_response(const struct dhf_shared *s) {
    return (ntohs(s->command) & 0x8000) != 0;
}

#ifdef __cplusplus
}
#endif

#endif /* DHF_SHARED_API_H */
