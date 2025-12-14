#pragma once
#include <stdbool.h>
#include <stdint.h>
void snake_rng_seed(uint32_t* state, uint32_t seed);
uint32_t snake_rng_next_u32(uint32_t* state);
int snake_rng_range(uint32_t* state, int lo, int hi_inclusive);
bool in_bounds(int x, int y, int width, int height);
uint64_t platform_now_ms(void);
void platform_sleep_ms(uint32_t ms);
bool platform_get_terminal_size(int* width_out, int* height_out);
bool snake_in_bounds(int x, int y, int width, int height);
uint64_t platform_now_ms(void);
void platform_sleep_ms(uint32_t ms);
bool platform_get_terminal_size(int* width_out, int* height_out);
