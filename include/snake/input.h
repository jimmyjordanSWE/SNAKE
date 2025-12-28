#pragma once
#include <stdbool.h>
#include <stddef.h>
typedef struct {
bool quit, restart, pause_toggle, view_toggle;
bool any_key;
bool move_up, move_down, move_left, move_right;
bool turn_left, turn_right;
} InputState;

/* Forward declaration to avoid including persist.h here. */
struct GameConfig;

/* Copy bindings from `cfg` into the input subsystem; `cfg` is not owned by this function. */
void input_set_bindings_from_config(const struct GameConfig* cfg);

bool input_init(void);
void input_shutdown(void);
void input_poll(InputState* out);
void input_poll_from_buf(InputState* out, const unsigned char* buf, size_t n);
/* Set per-player left/right bindings only */
void input_set_player_key_bindings(int player_idx, char left, char right);
void input_poll_all(InputState* outs, int max_players);
void input_poll_all_from_buf(InputState* outs, int max_players, const unsigned char* buf, size_t n);
