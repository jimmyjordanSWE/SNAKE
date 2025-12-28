#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "tty.h"

static double timespec_diff_ms(const struct timespec* a, const struct timespec* b) {
    return (double)(b->tv_sec - a->tv_sec) * 1000.0 + (double)(b->tv_nsec - a->tv_nsec) / 1e6;
}

int main(void) {
    const int width = 80;
    const int height = 24;
    tty_context* ctx = tty_open(NULL, width, height);
    if (!ctx) {
        fprintf(stderr, "tty_open failed\n");
        return 2;
    }
    struct ascii_pixel* buf = tty_get_buffer(ctx);
    if (!buf) {
        fprintf(stderr, "tty_get_buffer failed\n");
        tty_close(ctx);
        return 2;
    }

    /* warm up */
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            tty_put_pixel(ctx, x, y, PIXEL_MAKE(' ', COLOR_DEFAULT_FG, COLOR_DEFAULT_BG));
    tty_flip(ctx);

    const int iters = 2000;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < iters; i++) {
        /* toggle a single char to create dirty region */
        int x = i % width;
        int y = (i / width) % height;
        tty_put_pixel(ctx, x, y, PIXEL_MAKE((char)('A' + (i % 26)), (i % 16), ((i + 5) % 16)));
        tty_flip(ctx);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms = timespec_diff_ms(&t0, &t1);
    printf("tty_bench: iters=%d total_ms=%.3f avg_ms=%.6f\n", iters, ms, ms / iters);
    tty_close(ctx);
    return 0;
}
