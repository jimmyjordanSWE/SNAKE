#include <assert.h>
#include <stdint.h>
#include "snake/utils.h"

int main(void) {
    uint32_t state1 = 0;
    uint32_t state2 = 0;
    snake_rng_seed(&state1, 12345);
    snake_rng_seed(&state2, 12345);
    for (int i = 0; i < 100; i++) {
        uint32_t a = snake_rng_next_u32(&state1);
        uint32_t b = snake_rng_next_u32(&state2);
        assert(a == b);
    }
    
    uint32_t state3 = 0; snake_rng_seed(&state3, 12345);
    for (int i = 0; i < 100; i++) {
        int r = snake_rng_range(&state3, -5, 5);
        assert(r >= -5 && r <= 5);
    }
    
    snake_rng_seed(&state3, 54321);
    for (int i = 0; i < 1000; i++) {
        int r = snake_rng_range(&state3, INT32_MIN, INT32_MAX);
        assert(r >= INT32_MIN && r <= INT32_MAX);
    }
    return 0;
}
