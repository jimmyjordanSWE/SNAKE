#include "env.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int env_bool(const char* name, int default_val) {
    if (name == NULL)
        return default_val;
    const char* v = getenv(name);
    if (!v)
        return default_val;
    /* Limit length to avoid pathological env values */
    size_t len = strlen(v);
    if (len == 0)
        return default_val;
    if (len >= 64)
        return default_val;
    char buf[65];
    for (size_t i = 0; i < len; ++i)
        buf[i] = (char)tolower((unsigned char)v[i]);
    buf[len] = '\0';
    if (strcmp(buf, "1") == 0 || strcmp(buf, "true") == 0 || strcmp(buf, "yes") == 0 || strcmp(buf, "on") == 0)
        return 1;
    if (strcmp(buf, "0") == 0 || strcmp(buf, "false") == 0 || strcmp(buf, "no") == 0 || strcmp(buf, "off") == 0)
        return 0;
    /* Fallback: check first char */
    if (buf[0] == '1' || buf[0] == 'y' || buf[0] == 't' || buf[0] == 'o')
        return 1;
    return 0;
}
