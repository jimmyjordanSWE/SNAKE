#include "render_input.h"
#include <ctype.h>
#include <string.h>
int render_sanitize_player_name(const char* input, char* out, size_t out_len) {
    if (!out || out_len == 0)
        return 0;
    const char* start = input ? input : "";
    while (*start && isspace((unsigned char)*start))
        start++;
    const char* end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1]))
        end--;
    size_t len = (size_t)(end - start);
    if (len > 0 && len < out_len) {
        memcpy(out, start, len);
        out[len] = '\0';
    } else {
        /* default name */
        if (out_len > 0) {
            const char* def = "You";
            size_t dlen = strlen(def);
            if (dlen + 1 <= out_len) {
                memcpy(out, def, dlen + 1);
            } else {
                /* truncate defensively */
                if (out_len > 1) {
                    memcpy(out, def, out_len - 1);
                    out[out_len - 1] = '\0';
                } else {
                    out[0] = '\0';
                }
            }
        }
    }
    return 1;
}
