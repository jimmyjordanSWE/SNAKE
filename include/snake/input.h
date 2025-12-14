#pragma once
#include <stdbool.h>
#include <stddef.h>
typedef struct {
bool quit, restart, pause_toggle, view_toggle;
bool any_key;
bool move_up, move_down, move_left, move_right;
/* strafing relative to view (e.g. 'a'/'d' like in Doom) */
bool move_strafe_left, move_strafe_right;
} InputState;
bool input_init(void);
void input_shutdown(void);
void input_poll(InputState* out);
/* Parse a supplied input buffer (useful for tests). */
void input_poll_from_buf(InputState* out, const unsigned char* buf, size_t n);
