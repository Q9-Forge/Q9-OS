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

static inline void dhf_set_command(struct dhf_shared *s, uint8_t cmd) {
    if (!s) return;
    /* increment seq to mark new request */
    dhf_shared_inc_seq(s);
    __sync_synchronize(); /* memory barrier */
    s->command = cmd;
}

/* Wait until driver sets command back to 0 (idle) or timeout_ms elapses. */
static inline int dhf_wait_idle(struct dhf_shared *s, uint32_t start_seq, int timeout_ms) {
    if (!s) return -1;
    const int interval_us = 1000; /* 1ms poll */
    int waited = 0;
    while (1) {
        if (s->command == 0) return 0;
        /* if seq changed and command cleared, treat as completed */
        if (dhf_shared_seq(s) != start_seq && s->command == 0) return 0;
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

static inline void dhf_set_name(struct dhf_shared *s, const char *name) {
    if (!s || !name) return;
    strncpy(s->name, name, sizeof(s->name)-1);
    s->name[sizeof(s->name)-1] = '\0';
    /* mark A0 to indicate name present: use 1 */
    s->a0 = htonl(1);
}

static inline void dhf_set_buffer(struct dhf_shared *s, const void *buf, uint32_t len) {
    if (!s || !buf) return;
    if (len > sizeof(s->buffer)) len = sizeof(s->buffer);
    memcpy(s->buffer, buf, len);
    s->a1 = htonl(1);
    s->d1 = htonl(len);
}

static inline uint32_t dhf_get_result_bytes(const struct dhf_shared *s) {
    return ntohl(s->d1);
}

static inline uint8_t dhf_get_status(const struct dhf_shared *s) {
    return s->status;
}

static inline int dhf_is_response(const struct dhf_shared *s) {
    return (s->command & 0x80) != 0;
}

#ifdef __cplusplus
}
#endif

#endif /* DHF_SHARED_API_H */
