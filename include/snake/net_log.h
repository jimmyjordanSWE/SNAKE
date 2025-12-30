#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void net_log_init(void);
void net_log_close(void);

void net_log_info(const char* fmt, ...);
void net_log_error(const char* fmt, ...);

void net_log_send(int fd, const void* buf, size_t len, const char* note);
void net_log_recv(int fd, const void* buf, size_t len, const char* note);

#ifdef __cplusplus
}
#endif
