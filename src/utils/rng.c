#include "snake/utils.h"
void snake_rng_seed(uint32_t* state, uint32_t seed) {
if(!state) return;
*state= (seed == 0u) ? 0xA341316Cu : seed;
}
uint32_t snake_rng_next_u32(uint32_t* state) {
if(!state) return 0u;
uint32_t x= *state;
x^= x << 13;
x^= x >> 17;
x^= x << 5;
*state= x;
return x;
}
int snake_rng_range(uint32_t* state, int lo, int hi_inclusive) {
if(hi_inclusive < lo) {
int tmp= lo;
lo= hi_inclusive;
hi_inclusive= tmp;
}
uint32_t span= (uint32_t)(hi_inclusive - lo + 1);
uint32_t r= snake_rng_next_u32(state);
if(span == 0u) { return (int32_t)r; }
return lo + (int)(r % span);
}
