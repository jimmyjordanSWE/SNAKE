#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tty.h"

int main(void) {
    /* Too long path should be rejected */
    size_t long_len = 1024;
    char *longpath = malloc(long_len + 1);
    if (!longpath) return 1;
    for (size_t i = 0; i < long_len; ++i) longpath[i] = 'a';
    longpath[long_len] = '\0';
    tty_context* ctx = tty_open(longpath, 10, 10);
    assert(ctx == NULL);
    free(longpath);

    /* NULL path should open stdout fallback (may succeed in CI) */
    tty_context* ctx2 = tty_open(NULL, 1, 1);
    if (ctx2) {
        tty_close(ctx2);
    }

    printf("test_tty_path: OK\n");
    return 0;
}
