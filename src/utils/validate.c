#include "validate.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
bool safe_snprintf(char* buf, size_t buf_size, const char* fmt, ...) {
    if (!buf || buf_size == 0 || !fmt)
        return false;
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, buf_size, fmt, ap);
    va_end(ap);
    if (r < 0)
        return false;
    if ((size_t)r >= buf_size)
        return false; /* truncated */
    return true;
}
bool safe_copy_and_null(char* dst, size_t dst_size, const char* src, size_t len) {
    if (!dst || dst_size == 0)
        return false;
    if (!src) {
        dst[0] = '\0';
        return true;
    }
    if (len >= dst_size)
        return false;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return true;
}
