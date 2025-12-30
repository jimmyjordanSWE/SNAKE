#include "net_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

/* Simple thread-safe logger for network I/O. Default file is ./net_io.log
 * Override path with environment variable SNAKE_NET_LOG. Limits payload dump to
 * MAX_DUMP bytes. */

#define MAX_DUMP 256

static FILE* g_log = NULL;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/* forward */
static void timestr(char* buf, size_t buflen);

#include <stdarg.h>
#include <errno.h>

static void open_log_if_needed(void) {
    if (g_log) return;
    static int tried = 0;
    if (tried) return;
    tried = 1;
    const char* path = getenv("SNAKE_NET_LOG");
    if (!path) path = "./net_io.log";

    /* Only write the initial header when the file did not previously exist to avoid
       noisy duplicate lines across multiple runs. Include PID for traceability. */
    int existed = (access(path, F_OK) == 0);
    FILE* f = fopen(path, "a");
    if (!f) {
        /* try fallback silently */
        const char* fallback = "/tmp/net_io.log";
        int fb_existed = (access(fallback, F_OK) == 0);
        f = fopen(fallback, "a");
        if (!f) {
            return;
        }
        g_log = f;
        if (!fb_existed) {
            char ts[64]; timestr(ts, sizeof(ts));
            fprintf(g_log, "[%s] INFO net_log: logging to fallback '%s' pid=%d\n", ts, fallback, (int)getpid());
            fflush(g_log);
        }
        return;
    }
    g_log = f;
    if (!existed) {
        char ts[64]; timestr(ts, sizeof(ts));
        fprintf(g_log, "[%s] INFO net_log: logging to '%s' pid=%d\n", ts, path, (int)getpid());
        fflush(g_log);
    }
}

void net_log_info(const char* fmt, ...) {
    pthread_mutex_lock(&g_lock);
    open_log_if_needed();
    if (!g_log) { pthread_mutex_unlock(&g_lock); return; }
    char ts[64]; timestr(ts, sizeof(ts));
    fprintf(g_log, "[%s] INFO ", ts);
    va_list ap; va_start(ap, fmt); vfprintf(g_log, fmt, ap); va_end(ap);
    fprintf(g_log, "\n");
    fflush(g_log);
    pthread_mutex_unlock(&g_lock);
}

void net_log_error(const char* fmt, ...) {
    pthread_mutex_lock(&g_lock);
    open_log_if_needed();
    if (!g_log) { pthread_mutex_unlock(&g_lock); return; }
    char ts[64]; timestr(ts, sizeof(ts));
    fprintf(g_log, "[%s] ERROR ", ts);
    va_list ap; va_start(ap, fmt); vfprintf(g_log, fmt, ap); va_end(ap);
    fprintf(g_log, "\n");
    fflush(g_log);
    pthread_mutex_unlock(&g_lock);
}

static void timestr(char* buf, size_t buflen) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);
    int ms = (int)(tv.tv_usec / 1000);
    snprintf(buf, buflen, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
}

static void hex_dump_to_file(FILE* f, const unsigned char* data, size_t len) {
    size_t i;
    for (i = 0; i < len; ++i) {
        fprintf(f, "%02x", data[i]);
        if (i + 1 < len) fprintf(f, " ");
    }
}

static void log_io(const char* dir, int fd, const void* buf, size_t len, const char* note) {
    if (!buf) return;
    pthread_mutex_lock(&g_lock);
    open_log_if_needed();
    if (!g_log) { pthread_mutex_unlock(&g_lock); return; }
    char ts[64]; timestr(ts, sizeof(ts));
    fprintf(g_log, "[%s] %s fd=%d len=%zu %s\n", ts, dir, fd, len, note ? note : "");
    size_t dump_len = len < MAX_DUMP ? len : MAX_DUMP;
    if (dump_len > 0) {
        fprintf(g_log, "data(hex,%zu): ", dump_len);
        hex_dump_to_file(g_log, (const unsigned char*)buf, dump_len);
        if (dump_len < len) fprintf(g_log, " ... (truncated)");
        fprintf(g_log, "\n");
    }
    fflush(g_log);
    pthread_mutex_unlock(&g_lock);
}

void net_log_send(int fd, const void* buf, size_t len, const char* note) {
    log_io("SEND", fd, buf, len, note);
}

void net_log_recv(int fd, const void* buf, size_t len, const char* note) {
    log_io("RECV", fd, buf, len, note);
}

/* Public init/close so the program can create the log file at startup */
void net_log_init(void) {
    pthread_mutex_lock(&g_lock);
    open_log_if_needed();
    pthread_mutex_unlock(&g_lock);
}

void net_log_close(void) {
    pthread_mutex_lock(&g_lock);
    if (g_log) {
        fclose(g_log);
        g_log = NULL;
    }
    pthread_mutex_unlock(&g_lock);
}

