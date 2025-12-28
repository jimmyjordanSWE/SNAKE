#pragma once
#include <stdint.h>

/* Fast inverse square root (Carmack) - returns approx 1/sqrt(x).
 * Behavior: for x <= 0 returns 0.0f. Uses one Newton-Raphson iteration for speed.
 */
static inline float fast_inv_sqrt(float x) {
    if (!(x > 0.0f))
        return 0.0f;
    union { float f; uint32_t i; } conv;
    conv.f = x;
    /* Magic constant 0x5f3759df - classic; one NR iteration for reasonable accuracy */
    conv.i = 0x5f3759df - (conv.i >> 1);
    float y = conv.f;
    /* One Newton-Raphson iteration */
    y = y * (1.5f - 0.5f * x * y * y);
    return y;
}
