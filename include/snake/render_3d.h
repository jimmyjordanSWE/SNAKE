#pragma once
#include "snake/game.h"
#include "snake/persist.h"
#include <stdbool.h>

/* Forward-declare the SDL3DContext type so tests can reference it without
   including the SDL-specific header in other modules. */
struct SDL3DContext;
typedef struct
{
    int   active_player;
    float fov_degrees;
    bool  show_sprite_debug;
    int   screen_width;
    int   screen_height;
    float wall_height_scale;
    float tail_height_scale;
    char  wall_texture_path[PERSIST_TEXTURE_PATH_MAX];
    char  floor_texture_path[PERSIST_TEXTURE_PATH_MAX];
} Render3DConfig;
bool render_3d_init(const GameState* game_state, const Render3DConfig* config);
void render_3d_draw(const GameState* game_state,
                    const char*      player_name,
                    const void*      scores,
                    int              score_count,
                    float            delta_seconds);
void render_3d_on_tick(const GameState* game_state);
void render_3d_set_tick_rate_ms(int ms);
void render_3d_set_active_player(int player_index);
void render_3d_set_fov(float fov_degrees);
/* Test helper: return the internal SDL display context (may be NULL). */
struct SDL3DContext* render_3d_get_display(void);
void                 render_3d_shutdown(void);

/* Console overlays rendered in the 3D view */
void render_3d_draw_death_overlay(const GameState* game,
                                  int              anim_frame,
                                  bool             show_prompt);
void render_3d_draw_congrats_overlay(int score, const char* name_entered);
/* Test helper: draw only the minimap into a provided SDL3DContext */
void render_3d_draw_minimap_into(struct SDL3DContext* ctx, const GameState* gs);

/*
 * Compute the minimap cell size in pixels given a display size and map
 * dimensions. This is useful for testing minimap scaling independently of
 * the SDL display code.
 */
int render_3d_compute_minimap_cell_px(int display_w,
                                      int display_h,
                                      int map_w,
                                      int map_h);