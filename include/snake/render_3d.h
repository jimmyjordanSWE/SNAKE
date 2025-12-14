#pragma once
#include "snake/game.h"
#include <stdbool.h>
typedef struct {
int active_player;
float fov_degrees;
bool show_minimap;
bool show_stats;
int screen_width;
int screen_height;
} Render3DConfig;
bool render_3d_init(const GameState* game_state, const Render3DConfig* config);
void render_3d_draw(const GameState* game_state, const char* player_name, const void* scores, int score_count);
void render_3d_set_active_player(int player_index);
void render_3d_set_fov(float fov_degrees);
void render_3d_toggle_minimap(void);
void render_3d_toggle_stats(void);
void render_3d_shutdown(void);