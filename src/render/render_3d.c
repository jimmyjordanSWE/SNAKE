#include "snake/render_3d.h"
#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_raycast.h"
#include "snake/render_3d_sdl.h"
#include "snake/render_3d_sprite.h"
#include <math.h>
#include "snake/render_3d_texture.h"
#include "snake/types.h"
#include <stdio.h>
#include <stdlib.h>
typedef enum { RENDER_MODE_2D= 0, RENDER_MODE_3D, RENDER_MODE_COUNT } RenderMode;
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
static Render3DContext g_render_3d= {0};
bool render_3d_init(const GameState* game_state, const Render3DConfig* config) {
if(g_render_3d.initialized) return true;
if(!game_state) return false;
g_render_3d.game_state= game_state;
if(config) {
g_render_3d.config= *config;
} else {
g_render_3d.config.active_player= 0;
g_render_3d.config.fov_degrees= 90.0f;
g_render_3d.config.show_minimap= false;
g_render_3d.config.show_stats= false;
g_render_3d.config.screen_width= 800;
g_render_3d.config.screen_height= 600;
}
if(!render_3d_sdl_init(g_render_3d.config.screen_width, g_render_3d.config.screen_height, &g_render_3d.display)) return false;
camera_init(&g_render_3d.camera, g_render_3d.config.fov_degrees, g_render_3d.config.screen_width, 0.5f);
raycast_init(&g_render_3d.raycaster, game_state->width, game_state->height, NULL);
projection_init(&g_render_3d.projector, g_render_3d.config.screen_width, g_render_3d.config.screen_height, g_render_3d.config.fov_degrees * 3.14159265359f / 180.0f);
texture_init(&g_render_3d.texture);
sprite_init(&g_render_3d.sprite_renderer, 100, &g_render_3d.camera);
g_render_3d.initialized= true;
return true;
}
void render_3d_draw(const GameState* game_state, const char* player_name, const void* scores, int score_count) {
(void)player_name;
(void)scores;
(void)score_count;
if(!g_render_3d.initialized || !game_state) return;
if(g_render_3d.config.active_player < game_state->num_players) {
const PlayerState* player= &game_state->players[g_render_3d.config.active_player];
if(player->length > 0) camera_set_from_player(&g_render_3d.camera, player->body[0].x, player->body[0].y, player->current_dir);
}
camera_update_interpolation(&g_render_3d.camera, 1.0f / 60.0f);
uint32_t sky_color= 0xFF87CEEB;
render_3d_sdl_clear(&g_render_3d.display, sky_color);
uint32_t floor_color= 0xFF8B4513;
uint32_t ceiling_color= 0xFF4169E1;
int horizon= g_render_3d.display.height / 2;
for(int x= 0; x < g_render_3d.display.width; x++) {
float ray_angle;
camera_get_ray_angle(&g_render_3d.camera, x, &ray_angle);
RayHit hit;
if(raycast_cast_ray(&g_render_3d.raycaster, g_render_3d.camera.x, g_render_3d.camera.y, ray_angle, &hit)) {
WallProjection proj;
/* perpendicular distance to avoid fisheye */
projection_project_wall_perp(&g_render_3d.projector, hit.distance, ray_angle, g_render_3d.camera.angle, &proj);
render_3d_sdl_draw_column(&g_render_3d.display, x, 0, proj.draw_start - 1, ceiling_color);
Texel texel;
float pd = hit.distance * cosf(ray_angle - g_render_3d.camera.angle);
if(pd <= 0.1f) pd = 0.1f;
texture_get_texel(&g_render_3d.texture, pd, hit.is_vertical, 0.0f, &texel);
render_3d_sdl_draw_column(&g_render_3d.display, x, proj.draw_start, proj.draw_end, texel.color);
render_3d_sdl_draw_column(&g_render_3d.display, x, proj.draw_end + 1, g_render_3d.display.height - 1, floor_color);
} else {
render_3d_sdl_draw_column(&g_render_3d.display, x, 0, horizon - 1, ceiling_color);
render_3d_sdl_draw_column(&g_render_3d.display, x, horizon, g_render_3d.display.height - 1, floor_color);
}
}
sprite_clear(&g_render_3d.sprite_renderer);
for(int i= 0; i < game_state->food_count; i++) sprite_add(&g_render_3d.sprite_renderer, (float)game_state->food[i].x, (float)game_state->food[i].y, 0, (uint8_t)i);
for(int p= 0; p < game_state->num_players; p++) {
if(p == g_render_3d.config.active_player) continue;
const PlayerState* player= &game_state->players[p];
if(!player->active || player->length == 0) continue;
sprite_add(&g_render_3d.sprite_renderer, (float)player->body[0].x, (float)player->body[0].y, 1, (uint8_t)p);
}
sprite_sort_by_depth(&g_render_3d.sprite_renderer);
for(int i= 0; i < sprite_get_count(&g_render_3d.sprite_renderer); i++) {
const SpriteProjection* spr= sprite_get(&g_render_3d.sprite_renderer, i);
if(!spr || !spr->is_visible) continue;
int center_x= (int)((spr->screen_x + 1.0f) * 0.5f * (float)g_render_3d.display.width);
int center_y= g_render_3d.display.height / 2;
int half_width= (int)(spr->screen_width * 0.5f * (float)g_render_3d.display.width);
int half_height= (int)(spr->screen_height * 0.5f * (float)g_render_3d.display.height);
int x1= center_x - half_width;
int x2= center_x + half_width;
int y1= center_y - half_height;
int y2= center_y + half_height;
if(x1 < 0) x1= 0;
if(x2 >= g_render_3d.display.width) x2= g_render_3d.display.width - 1;
if(y1 < 0) y1= 0;
if(y2 >= g_render_3d.display.height) y2= g_render_3d.display.height - 1;
uint32_t sprite_color= 0xFF00FF00;
int radius= (half_width < half_height) ? half_width : half_height;
if(radius > 0) render_3d_sdl_draw_filled_circle(&g_render_3d.display, center_x, center_y, radius, sprite_color);
}
if(!render_3d_sdl_present(&g_render_3d.display)) {}
}
void render_3d_set_active_player(int player_index) {
if(g_render_3d.initialized) g_render_3d.config.active_player= player_index;
}
void render_3d_set_fov(float fov_degrees) {
if(g_render_3d.initialized) g_render_3d.config.fov_degrees= fov_degrees;
}
void render_3d_toggle_minimap(void) {
if(g_render_3d.initialized) g_render_3d.config.show_minimap= !g_render_3d.config.show_minimap;
}
void render_3d_toggle_stats(void) {
if(g_render_3d.initialized) g_render_3d.config.show_stats= !g_render_3d.config.show_stats;
}
void render_3d_shutdown(void) {
if(!g_render_3d.initialized) return;
render_3d_sdl_shutdown(&g_render_3d.display);
g_render_3d.initialized= false;
g_render_3d.game_state= NULL;
}
