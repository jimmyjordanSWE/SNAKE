#pragma once
#include <stdbool.h>
typedef struct {
bool quit, restart, pause_toggle, view_toggle;
bool any_key;
bool move_up, move_down, move_left, move_right;
} InputState;
bool input_init(void);
void input_shutdown(void);
void input_poll(InputState* out);
