#pragma once
#include <stdbool.h>
#include <stddef.h>
typedef struct {
bool quit, restart, pause_toggle, view_toggle;
bool any_key;
bool move_up, move_down, move_left, move_right;
bool turn_left, turn_right;
} InputState;
bool input_init(void);
void input_shutdown(void);
void input_poll(InputState* out);
void input_poll_from_buf(InputState* out, const unsigned char* buf, size_t n);
void input_set_key_bindings(char up, char down, char turn_left, char turn_right, char quit, char restart, char pause_toggle);
