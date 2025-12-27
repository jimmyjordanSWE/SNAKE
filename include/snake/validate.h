#ifndef SNAKE_VALIDATE_H
#define SNAKE_VALIDATE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool safe_snprintf(char* buf, size_t buf_size, const char* fmt, ...);

/* Copy src (len bytes) into dst ensuring null termination and bounds. Returns true on success. */
bool safe_copy_and_null(char* dst, size_t dst_size, const char* src, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SNAKE_VALIDATE_H */