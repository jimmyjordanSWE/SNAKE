#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "snake/tty.h"

static int resize_called = 0;
static void on_resize(tty_context* ctx, int old_w, int old_h, int new_w, int new_h, void* ud) {
    (void)ctx; (void)ud;
    resize_called++;
}

int main(void) {
    /* Force a deterministic terminal size small enough to be invalid for min */
    tty_set_test_size(10, 5);
    tty_context* ctx = tty_open(NULL, 20, 10);
    assert(ctx != NULL);
    /* Because min > size, size_valid should be false */
    assert(!tty_size_valid(ctx));
    tty_set_resize_callback(ctx, on_resize, NULL);
    /* Now simulate a resize to larger size */
    tty_set_test_size(80, 25);
    tty_simulate_winch();
    assert(tty_check_resize(ctx));
    assert(resize_called == 1);
    tty_clear_test_size();
    tty_close(ctx);
    return 0;
}
