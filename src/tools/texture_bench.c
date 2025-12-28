#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "render_3d_texture.h"

static double timespec_diff_ms(const struct timespec* a, const struct timespec* b) {
    return (double)(b->tv_sec - a->tv_sec) * 1000.0 + (double)(b->tv_nsec - a->tv_nsec) / 1e6;
}

int main(void) {
    const int w = 128, h = 128;
    Texture3D* tex = texture_create_procedural(w, h);
    if (!tex) {
        fprintf(stderr, "texture_bench: texture_create_procedural failed\n");
        return 2;
    }
    const int iters = 2000000; /* 2M samples */
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    volatile uint32_t acc = 0;
    for (int i = 0; i < iters; i++) {
        float u = (float)(i % w) / (float)w;
        float v = (float)((i / w) % h) / (float)h;
        uint32_t c = texture_sample(tex, u, v, true);
        acc ^= c;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms = timespec_diff_ms(&t0, &t1);
    printf("texture_bench: iters=%d total_ms=%.3f avg_ns=%.3f acc=0x%08x\n", iters, ms, (ms*1e6)/(double)iters, (unsigned int)acc);
    texture_destroy(tex);
    return 0;
}
