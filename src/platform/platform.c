
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "snake/platform.h"
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
uint64_t platform_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}
void platform_sleep_ms(uint64_t ms) {
    struct timespec req;
    req.tv_sec = (time_t)(ms / 1000u);
    req.tv_nsec = (long)((ms % 1000u) * 1000000u);
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
    }
}
static volatile sig_atomic_t platform_terminal_resized = 0;
static void platform_sigwinch_handler(int sig) {
    (void)sig;
    platform_terminal_resized = 1;
}
bool platform_get_terminal_size(int* width_out, int* height_out) {
    if (!width_out || !height_out)
        return false;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
        return false;
    if (ws.ws_col == 0 || ws.ws_row == 0)
        return false;
    *width_out = (int)ws.ws_col;
    *height_out = (int)ws.ws_row;
    return true;
}
void platform_winch_init(void) {
    signal(SIGWINCH, platform_sigwinch_handler);
}
bool platform_was_resized(void) {
    if (platform_terminal_resized) {
        platform_terminal_resized = 0;
        return true;
    }
    return false;
}
