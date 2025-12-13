/* Feature test macro: required on some libc/tooling setups for clock_gettime/nanosleep. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "snake/platform.h"

#include <errno.h>
#include <time.h>

uint64_t platform_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

void platform_sleep_ms(uint64_t ms) {
    struct timespec req;
    req.tv_sec = (time_t)(ms / 1000u);
    req.tv_nsec = (long)((ms % 1000u) * 1000000u);
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {}
}
