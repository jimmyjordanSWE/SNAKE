#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "tty.h"

int main(void) {
    tty_context* ctx = tty_open(NULL, 1, 1);
    if (ctx) {
        size_t cap = tty_get_write_buffer_size(ctx);
        /* Must be non-zero and reasonably capped (<= 10MB) */
        assert(cap > 0 && cap <= 10 * 1024 * 1024);
        tty_close(ctx);
    }
    printf("test_tty_buffer_cap: OK\n");
    return 0;
}
