#pragma once
#include <stdint.h>
#include <stdbool.h>
uint64_t platform_now_ms(void);
void     platform_sleep_ms(uint64_t ms);

/* Query the current stdout terminal size in character cells. Returns true
 * and fills `width_out`/`height_out` on success, false on failure. */
bool platform_get_terminal_size(int* width_out, int* height_out);

/* Initialize SIGWINCH handling for the process. After calling this, use
 * `platform_was_resized()` to check whether a resize occurred; the function
 * clears the internal flag when observed. */
void platform_winch_init(void);
bool platform_was_resized(void);
