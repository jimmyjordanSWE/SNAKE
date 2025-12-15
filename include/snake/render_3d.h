#pragma once
#include "snake/game.h"
#include "snake/persist.h"
#include <stdbool.h>
typedef struct {
int active_player;
float fov_degrees;
bool show_sprite_debug;
int screen_width;
int screen_height;
/* optional asset paths and scaling */
float wall_height_scale;
float tail_height_scale;
char wall_texture_path[PERSIST_TEXTURE_PATH_MAX];
char floor_texture_path[PERSIST_TEXTURE_PATH_MAX];
} Render3DConfig;
bool render_3d_init(const GameState* game_state, const Render3DConfig* config);
/* delta_seconds: time elapsed since last frame in seconds (used for
 * interpolation) */
void render_3d_draw(const GameState* game_state, const char* player_name, const void* scores, int score_count, float delta_seconds);
void render_3d_on_tick(const GameState* game_state);
void render_3d_set_tick_rate_ms(int ms);
void render_3d_set_active_player(int player_index);
void render_3d_set_fov(float fov_degrees);
/* minimap/stats toggles removed (no rendering implementation) */
void render_3d_shutdown(void);