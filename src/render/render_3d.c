#include "snake/render_3d.h"
#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_raycast.h"
#include "snake/render_3d_sdl.h"
#include "snake/render_3d_sprite.h"
#include "snake/render_3d_texture.h"
#include "snake/types.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum { RENDER_MODE_2D = 0, RENDER_MODE_3D, RENDER_MODE_COUNT } RenderMode;

extern void render_set_mode(RenderMode mode);

typedef struct {
    const GameState* game_state;
    Camera3D camera;
    Raycaster3D raycaster;
    Projection3D projector;
    Texture3D texture;
    SpriteRenderer3D sprite_renderer;
    SDL3DContext display;
    Render3DConfig config;
    bool initialized;
} Render3DContext;

static Render3DContext g_render_3d = {0};

bool render_3d_init(const GameState* game_state, const Render3DConfig* config) {
    if (g_render_3d.initialized) { return true; /* Already initialized */ }

    if (!game_state) { return false; }

    g_render_3d.game_state = game_state;

    /* Set default config if not provided */
    if (config) {
        g_render_3d.config = *config;
    } else {
        g_render_3d.config.active_player = 0;
        g_render_3d.config.fov_degrees = 90.0f;
        g_render_3d.config.show_minimap = false;
        g_render_3d.config.show_stats = false;
        g_render_3d.config.screen_width = 800;
        g_render_3d.config.screen_height = 600;
    }

    /* Initialize SDL display */
    if (!render_3d_sdl_init(g_render_3d.config.screen_width, g_render_3d.config.screen_height, &g_render_3d.display)) { return false; }

    /* Initialize sub-modules */
    camera_init(&g_render_3d.camera, g_render_3d.config.fov_degrees, g_render_3d.config.screen_width, 0.5f); /* 0.5 = 2 Hz game update */
    raycast_init(&g_render_3d.raycaster, game_state->width, game_state->height, NULL);                       /* No internal walls */
    projection_init(&g_render_3d.projector, g_render_3d.config.screen_width, g_render_3d.config.screen_height,
                    g_render_3d.config.fov_degrees * 3.14159265359f / 180.0f);
    texture_init(&g_render_3d.texture);
    sprite_init(&g_render_3d.sprite_renderer, 100, &g_render_3d.camera); /* Max 100 sprites */

    g_render_3d.initialized = true;
    return true;
}

void render_3d_draw(const GameState* game_state, const char* player_name, const void* scores, int score_count) {
    (void)player_name;
    (void)scores;
    (void)score_count; /* TODO: Use for HUD */

    if (!g_render_3d.initialized || !game_state) { return; }

    /* Update camera position to follow active player */
    if (g_render_3d.config.active_player < game_state->num_players) {
        const PlayerState* player = &game_state->players[g_render_3d.config.active_player];
        if (player->length > 0) { camera_set_from_player(&g_render_3d.camera, player->body[0].x, player->body[0].y, player->current_dir); }
    }

    /* Update interpolation based on frame delta time (assume 60 Hz target = 16.67 ms per frame) */
    camera_update_interpolation(&g_render_3d.camera, 1.0f / 60.0f);

    /* Clear to sky blue background */
    uint32_t sky_color = 0xFF87CEEB; /* Sky blue */
    render_3d_sdl_clear(&g_render_3d.display, sky_color);

    /* Define colors */
    uint32_t floor_color = 0xFF8B4513;   /* Brown */
    uint32_t ceiling_color = 0xFF4169E1; /* Royal blue */
    int horizon = g_render_3d.display.height / 2;

    /* Raycast and draw walls, floor, and ceiling per column */
    for (int x = 0; x < g_render_3d.display.width; x++) {
        float ray_angle;
        camera_get_ray_angle(&g_render_3d.camera, x, &ray_angle);

        RayHit hit;
        if (raycast_cast_ray(&g_render_3d.raycaster, g_render_3d.camera.x, g_render_3d.camera.y, ray_angle, &hit)) {
            WallProjection proj;
            projection_project_wall(&g_render_3d.projector, hit.distance, &proj);

            /* Draw ceiling from top to wall start */
            render_3d_sdl_draw_column(&g_render_3d.display, x, 0, proj.draw_start - 1, ceiling_color);

            /* Draw wall */
            Texel texel;
            texture_get_texel(&g_render_3d.texture, hit.distance, hit.is_vertical, 0.0f, &texel);
            render_3d_sdl_draw_column(&g_render_3d.display, x, proj.draw_start, proj.draw_end, texel.color);

            /* Draw floor from wall end to bottom */
            render_3d_sdl_draw_column(&g_render_3d.display, x, proj.draw_end + 1, g_render_3d.display.height - 1, floor_color);
        } else {
            /* No wall hit - draw full ceiling and floor */
            render_3d_sdl_draw_column(&g_render_3d.display, x, 0, horizon - 1, ceiling_color);
            render_3d_sdl_draw_column(&g_render_3d.display, x, horizon, g_render_3d.display.height - 1, floor_color);
        }
    }

    /* Collect sprites */
    sprite_clear(&g_render_3d.sprite_renderer);

    /* Add food sprites */
    for (int i = 0; i < game_state->food_count; i++) {
        sprite_add(&g_render_3d.sprite_renderer, (float)game_state->food[i].x, (float)game_state->food[i].y, 0, (uint8_t)i); /* entity_type 0 = food */
    }

    /* Add snake sprites (other players) */
    for (int p = 0; p < game_state->num_players; p++) {
        if (p == g_render_3d.config.active_player) continue; /* Don't render self as sprite */

        const PlayerState* player = &game_state->players[p];
        if (!player->active || player->length == 0) continue;

        /* Add sprite for snake head */
        sprite_add(&g_render_3d.sprite_renderer, (float)player->body[0].x, (float)player->body[0].y, 1, (uint8_t)p); /* entity_type 1 = snake */
    }

    /* Sort and render sprites */
    sprite_sort_by_depth(&g_render_3d.sprite_renderer);
    for (int i = 0; i < sprite_get_count(&g_render_3d.sprite_renderer); i++) {
        const SpriteProjection* spr = sprite_get(&g_render_3d.sprite_renderer, i);
        if (!spr || !spr->is_visible) continue;

        /* Convert screen coordinates to pixel coordinates */
        int center_x = (int)((spr->screen_x + 1.0f) * 0.5f * (float)g_render_3d.display.width);
        int center_y = g_render_3d.display.height / 2; /* Center vertically */

        int half_width = (int)(spr->screen_width * 0.5f * (float)g_render_3d.display.width);
        int half_height = (int)(spr->screen_height * 0.5f * (float)g_render_3d.display.height);

        int x1 = center_x - half_width;
        int x2 = center_x + half_width;
        int y1 = center_y - half_height;
        int y2 = center_y + half_height;

        /* Clamp to screen bounds */
        if (x1 < 0) x1 = 0;
        if (x2 >= g_render_3d.display.width) x2 = g_render_3d.display.width - 1;
        if (y1 < 0) y1 = 0;
        if (y2 >= g_render_3d.display.height) y2 = g_render_3d.display.height - 1;

        /* Choose color based on entity type */
        uint32_t sprite_color = 0xFF00FF00; /* Green for all sprites */

        /* Draw sprite as filled circle */
        int radius = (half_width < half_height) ? half_width : half_height;
        if (radius > 0) { render_3d_sdl_draw_filled_circle(&g_render_3d.display, center_x, center_y, radius, sprite_color); }
    }

    /* Present frame */
    if (!render_3d_sdl_present(&g_render_3d.display)) {
        /* Window was closed or ESC pressed, switch back to 2D mode */
        extern void render_set_mode(RenderMode mode);
        render_set_mode(RENDER_MODE_2D);
    }
}

void render_3d_set_active_player(int player_index) {
    if (g_render_3d.initialized) { g_render_3d.config.active_player = player_index; }
}

void render_3d_set_fov(float fov_degrees) {
    if (g_render_3d.initialized) { g_render_3d.config.fov_degrees = fov_degrees; }
}

void render_3d_toggle_minimap(void) {
    if (g_render_3d.initialized) { g_render_3d.config.show_minimap = !g_render_3d.config.show_minimap; }
}

void render_3d_toggle_stats(void) {
    if (g_render_3d.initialized) { g_render_3d.config.show_stats = !g_render_3d.config.show_stats; }
}

void render_3d_shutdown(void) {
    if (!g_render_3d.initialized) { return; }

    /* Shutdown SDL */
    render_3d_sdl_shutdown(&g_render_3d.display);

    /* TODO: Shutdown other sub-modules if needed */

    g_render_3d.initialized = false;
    g_render_3d.game_state = NULL;
}
