#pragma once
#include <stdbool.h>
#include <stdint.h>
uint64_t platform_now_ms(void);
void     platform_sleep_ms(uint64_t ms);
bool     platform_get_terminal_size(int* width_out, int* height_out);
void     platform_winch_init(void);
bool     platform_was_resized(void);
