#ifndef SNAKE_RENDER_3D_H
#define SNAKE_RENDER_3D_H

#include "snake/game.h"
#include <stdbool.h>

/**
 * Render3D - Main entry point and orchestration for pseudo-3D renderer.
 *
 * Manages:
 * - Rendering pipeline orchestration
 * - Mode switching (2D vs 3D)
 * - Camera following active player
 * - HUD overlay in 3D mode
 */

typedef struct {
    int active_player; /* Which player camera follows */
    float fov_degrees; /* Field of view */
    bool show_minimap; /* Whether to show minimap overlay */
    bool show_stats;   /* Whether to show stats (FPS, distance, etc.) */
    int screen_width;  /* Screen width in pixels */
    int screen_height; /* Screen height in pixels */
} Render3DConfig;

/**
 * Initialize 3D rendering system.
 *
 * Must be called before any 3D rendering.
 *
 * @param game_state    Pointer to GameState for read-only access
 * @param config        Configuration (can be NULL for defaults)
 * @return              true on success, false on failure
 */
bool render_3d_init(const GameState* game_state, const Render3DConfig* config);

/**
 * Render current game frame in 3D mode.
 *
 * Main entry point for rendering. Called from render_draw() when in 3D mode.
 *
 * @param game_state    Current GameState
 * @param player_name   Player name for HUD
 * @param scores        High scores (can be NULL)
 * @param score_count   Number of scores
 */
void render_3d_draw(const GameState* game_state, const char* player_name, const void* scores, int score_count);

/**
 * Toggle active player camera.
 *
 * In multiplayer, switches which player's perspective is shown.
 */
void render_3d_set_active_player(int player_index);

/**
 * Configure FOV.
 *
 * @param fov_degrees   Field of view in degrees (60-120 typical)
 */
void render_3d_set_fov(float fov_degrees);

/**
 * Toggle HUD elements.
 */
void render_3d_toggle_minimap(void);
void render_3d_toggle_stats(void);

/**
 * Shutdown 3D rendering system and free resources.
 */
void render_3d_shutdown(void);

#endif /* SNAKE_RENDER_3D_H */
