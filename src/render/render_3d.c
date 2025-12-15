#include "snake/render_3d.h"
#include "snake/game_internal.h"
#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_raycast.h"
#include "snake/render_3d_sdl.h"
#include "snake/render_3d_sprite.h"
#include "snake/render_3d_texture.h"
#include "snake/types.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
typedef enum { RENDER_MODE_2D= 0, RENDER_MODE_3D, RENDER_MODE_COUNT } RenderMode;
typedef struct {
const GameState* game_state;
Camera3D camera;
Raycaster3D raycaster;
Projection3D projector;
Texture3D texture; /* fallback */
Texture3D wall_texture;
Texture3D floor_texture;
SpriteRenderer3D sprite_renderer;
SDL3DContext display;
Render3DConfig config;
bool initialized;
float* column_depths;
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
g_render_3d.config.show_sprite_debug= false;
g_render_3d.config.screen_width= 800;
g_render_3d.config.screen_height= 600;
g_render_3d.config.wall_height_scale= (float)PERSIST_CONFIG_DEFAULT_WALL_SCALE;
g_render_3d.config.tail_height_scale= (float)PERSIST_CONFIG_DEFAULT_TAIL_SCALE;
g_render_3d.config.wall_texture_path[0]= '\0';
g_render_3d.config.floor_texture_path[0]= '\0';
}
/* Log requested SDL size so config values can be traced during startup */
fprintf(stderr, "render_3d_init: requested size %dx%d\n", g_render_3d.config.screen_width, g_render_3d.config.screen_height);
if(!render_3d_sdl_init(g_render_3d.config.screen_width, g_render_3d.config.screen_height, &g_render_3d.display)) return false;
camera_init(&g_render_3d.camera, g_render_3d.config.fov_degrees, g_render_3d.config.screen_width, 0.5f);
raycast_init(&g_render_3d.raycaster, game_state->width, game_state->height, NULL);
/* projection with configurable wall height scale */
projection_init(&g_render_3d.projector, g_render_3d.config.screen_width, g_render_3d.config.screen_height, g_render_3d.config.fov_degrees * 3.14159265359f / 180.0f, g_render_3d.config.wall_height_scale);
/* initialize textures */
texture_init(&g_render_3d.texture);
texture_init(&g_render_3d.wall_texture);
texture_init(&g_render_3d.floor_texture);
/* try to load configured textures (optional). Use configured paths if set,
     * otherwise fall back to built-in defaults. */
if(g_render_3d.config.wall_texture_path[0]) {
if(!texture_load_from_file(&g_render_3d.wall_texture, g_render_3d.config.wall_texture_path)) {
fprintf(stderr,
    "render_3d_init: failed to load %s (using procedural "
    "fallback)\n",
    g_render_3d.config.wall_texture_path);
}
} else {
if(!texture_load_from_file(&g_render_3d.wall_texture, PERSIST_CONFIG_DEFAULT_WALL_TEXTURE)) {
fprintf(stderr,
    "render_3d_init: failed to load %s (using procedural "
    "fallback)\n",
    PERSIST_CONFIG_DEFAULT_WALL_TEXTURE);
}
}
if(g_render_3d.config.floor_texture_path[0]) {
if(!texture_load_from_file(&g_render_3d.floor_texture, g_render_3d.config.floor_texture_path)) { fprintf(stderr, "render_3d_init: failed to load %s (using flat floor color)\n", g_render_3d.config.floor_texture_path); }
} else {
if(!texture_load_from_file(&g_render_3d.floor_texture, PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE)) { fprintf(stderr, "render_3d_init: failed to load %s (using flat floor color)\n", PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE); }
}
/* allocate per-column perpendicular depth buffer for sprite occlusion */
g_render_3d.column_depths= calloc((size_t)g_render_3d.display.width, sizeof(float));
sprite_init(&g_render_3d.sprite_renderer, 100, &g_render_3d.camera, &g_render_3d.projector);
g_render_3d.initialized= true;
return true;
}
void render_3d_draw(const GameState* game_state, const char* player_name, const void* scores, int score_count, float delta_seconds) {
(void)player_name;
(void)scores;
(void)score_count;
if(!g_render_3d.initialized || !game_state) return;
/* Note: camera/player previous/current updates occur on tick via
     * `render_3d_on_tick()` so we don't reset interpolation every frame here.
     */
/* advance interpolation timer by frame delta (seconds) */
camera_update_interpolation(&g_render_3d.camera, delta_seconds);
uint32_t sky_color= 0xFF87CEEB;
render_3d_sdl_clear(&g_render_3d.display, sky_color);
uint32_t floor_color= 0xFF8B4513;
uint32_t ceiling_color= 0xFF4169E1;
int horizon= g_render_3d.display.height / 2;
/* Optional debug overlay: small texture samples when SNAKE_DEBUG_TEXTURES=1
     */
const char* dbg= getenv("SNAKE_DEBUG_TEXTURES");
if(dbg && dbg[0] == '1') {
/* draw small 16x16 sample of wall texture at (8,8) and floor at (8,28)
         */
if(g_render_3d.wall_texture.pixels) {
for(int yy= 0; yy < 16; yy++) {
for(int xx= 0; xx < 16; xx++) {
uint32_t c= texture_sample(&g_render_3d.wall_texture, (float)xx / 16.0f, (float)yy / 16.0f, true);
if(8 + xx >= 0 && 8 + xx < g_render_3d.display.width && 8 + yy >= 0 && 8 + yy < g_render_3d.display.height) g_render_3d.display.pixels[(8 + yy) * g_render_3d.display.width + (8 + xx)]= c;
}
}
}
if(g_render_3d.floor_texture.pixels) {
for(int yy= 0; yy < 16; yy++) {
for(int xx= 0; xx < 16; xx++) {
uint32_t c= texture_sample(&g_render_3d.floor_texture, (float)xx / 16.0f, (float)yy / 16.0f, true);
if(8 + xx >= 0 && 8 + xx < g_render_3d.display.width && 28 + yy >= 0 && 28 + yy < g_render_3d.display.height) g_render_3d.display.pixels[(28 + yy) * g_render_3d.display.width + (8 + xx)]= c;
}
}
}
fprintf(stderr, "render_3d: debug texture overlay drawn\n");
}
/* use interpolated camera position & angle for rendering */
float interp_cam_x, interp_cam_y;
camera_get_interpolated_position(&g_render_3d.camera, &interp_cam_x, &interp_cam_y);
float interp_cam_angle= camera_get_interpolated_angle(&g_render_3d.camera);
for(int x= 0; x < g_render_3d.display.width; x++) {
float ray_angle;
camera_get_ray_angle(&g_render_3d.camera, x, &ray_angle);
RayHit hit;
float cos_a= cosf(ray_angle);
float sin_a= sinf(ray_angle);
/* nudge origin slightly forward and perpendicular to ray to avoid
           exact grid-line intersection artifacts (vertical seam when aligned)
        */
float eps_fwd= 0.0002f;
float eps_perp= 0.0002f;
/* Nudge origin slightly forward along the ray and a small amount
            perpendicular to the ray to avoid grid-line intersection artifacts.
            Perpendicular vector to (cos_a, sin_a) is (-sin_a, cos_a). */
float origin_x= interp_cam_x + cos_a * eps_fwd - sin_a * eps_perp;
float origin_y= interp_cam_y + sin_a * eps_fwd + cos_a * eps_perp;
if(raycast_cast_ray(&g_render_3d.raycaster, origin_x, origin_y, ray_angle, &hit)) {
WallProjection proj;
/* Diagnostic: if a very short hit occurs, log for debugging */
if(hit.distance < 0.5f) {
if(g_render_3d.config.show_sprite_debug) {
fprintf(stderr,
    "[ray-debug] col=%d dist=%.4f hit=(%.3f,%.3f) "
    "vert=%d\n",
    x, hit.distance, hit.hit_x, hit.hit_y, hit.is_vertical);
}
}
/* perpendicular distance to avoid fisheye */
projection_project_wall_perp(&g_render_3d.projector, hit.distance, ray_angle, interp_cam_angle, &proj);
render_3d_sdl_draw_column(&g_render_3d.display, x, 0, proj.draw_start - 1, ceiling_color);
Texel texel;
float pd= hit.distance * cosf(ray_angle - interp_cam_angle);
if(pd <= 0.1f) pd= 0.1f;
/* record perpendicular depth for this screen column */
if(g_render_3d.column_depths) g_render_3d.column_depths[x]= pd;
float tex_coord= raycast_get_texture_coord(&hit, hit.is_vertical) * (float)TEXTURE_SCALE;
/* Draw textured wall column per-row to map V coordinate to wall
             * height */
int wall_h= proj.draw_end - proj.draw_start + 1;
if(wall_h <= 0) wall_h= 1;
/* fast column write: get base pointer */
uint32_t* pix= g_render_3d.display.pixels;
int w= g_render_3d.display.width;
int h= g_render_3d.display.height;
for(int yy= proj.draw_start; yy <= proj.draw_end; yy++) {
float v= (float)(yy - proj.draw_start) / (float)wall_h; /* 0..1 */
float tex_v= v * (float)TEXTURE_SCALE;
uint32_t col= 0;
if(g_render_3d.wall_texture.pixels && g_render_3d.wall_texture.img_w > 0 && g_render_3d.wall_texture.img_h > 0) {
col= texture_sample(&g_render_3d.wall_texture, tex_coord, tex_v, true);
} else {
texture_get_texel(&g_render_3d.texture, pd, hit.is_vertical, tex_coord, &texel);
col= texel.color;
}
if(pix && x >= 0 && x < w && yy >= 0 && yy < h) pix[yy * w + x]= col;
}
/* floor: Draw solid brown color below walls */
render_3d_sdl_draw_column(&g_render_3d.display, x, proj.draw_end, g_render_3d.display.height - 1, floor_color);
} else {
/* no hit -> mark as infinite distance so sprites are never occluded
             */
if(g_render_3d.column_depths) g_render_3d.column_depths[x]= INFINITY;
render_3d_sdl_draw_column(&g_render_3d.display, x, 0, horizon - 1, ceiling_color);
/* Draw floor in solid brown below horizon */
render_3d_sdl_draw_column(&g_render_3d.display, x, horizon, g_render_3d.display.height - 1, floor_color);
}
}
/* collect sprites for this frame */
sprite_clear(&g_render_3d.sprite_renderer);
for(int i= 0; i < game_state->food_count; i++) {
/* place food slightly above floor, small sprite */
sprite_add(&g_render_3d.sprite_renderer, (float)game_state->food[i].x + 0.5f, (float)game_state->food[i].y + 0.5f, 0.25f, 0.0f, true, (int)i, 0);
}
for(int p= 0; p < game_state->num_players; p++) {
if(p == g_render_3d.config.active_player) continue;
const PlayerState* player= &game_state->players[p];
if(!player->active || player->length == 0) continue;
/* interpolate head position between previous and current tick */
float t= 0.0f;
if(g_render_3d.camera.update_interval > 0.0f) {
t= g_render_3d.camera.interp_time / g_render_3d.camera.update_interval;
if(t < 0.0f) t= 0.0f;
if(t > 1.0f) t= 1.0f;
}
float head_x= player->prev_head_x + (((float)player->body[0].x + 0.5f) - player->prev_head_x) * t;
float head_y= player->prev_head_y + (((float)player->body[0].y + 0.5f) - player->prev_head_y) * t;
sprite_add(&g_render_3d.sprite_renderer, head_x, head_y, 1.0f, 0.0f, true, (int)p, 0);
}
/* Render tail segments as short gray walls (sprites) for all players */
for(int p= 0; p < game_state->num_players; p++) {
const PlayerState* player= &game_state->players[p];
if(!player->active || player->length <= 1) continue;
/* Start from 1 to skip head (0) */
for(int bi= 1; bi < player->length; bi++) {
float seg_x= (float)player->body[bi].x + 0.5f;
float seg_y= (float)player->body[bi].y + 0.5f;
/* use configured tail height (fraction of wall height) */
float tail_h= g_render_3d.config.tail_height_scale;
uint32_t gray= render_3d_sdl_color(128, 128, 128, 255);
sprite_add_color(&g_render_3d.sprite_renderer, seg_x, seg_y, tail_h, 0.0f, true, -1, 0, gray);
}
}
/* Optional tail debug */
{
const char* dbg_tail= getenv("SNAKE_DEBUG_TAIL");
if(dbg_tail && dbg_tail[0] == '1') { fprintf(stderr, "[tail-dbg] sprite_count=%d\n", g_render_3d.sprite_renderer.count); }
}
/* project, sort and draw using per-column occlusion */
sprite_project_all(&g_render_3d.sprite_renderer);
sprite_sort_by_depth(&g_render_3d.sprite_renderer);
sprite_draw(&g_render_3d.sprite_renderer, &g_render_3d.display, g_render_3d.column_depths);
if(!render_3d_sdl_present(&g_render_3d.display)) {}
}
void render_3d_set_active_player(int player_index) {
if(g_render_3d.initialized) g_render_3d.config.active_player= player_index;
}
void render_3d_set_fov(float fov_degrees) {
if(g_render_3d.initialized) g_render_3d.config.fov_degrees= fov_degrees;
}
void render_3d_on_tick(const GameState* game_state) {
if(!g_render_3d.initialized || !game_state) return;
if(g_render_3d.config.active_player < game_state->num_players) {
const PlayerState* player= &game_state->players[g_render_3d.config.active_player];
if(player->length > 0) camera_set_from_player(&g_render_3d.camera, player->body[0].x, player->body[0].y, player->current_dir);
}
}
void render_3d_set_tick_rate_ms(int ms) {
if(!g_render_3d.initialized) return;
if(ms <= 0) ms= 1;
g_render_3d.camera.update_interval= (float)ms / 1000.0f;
/* clamp interp_time to new interval */
if(g_render_3d.camera.interp_time > g_render_3d.camera.update_interval) g_render_3d.camera.interp_time= g_render_3d.camera.update_interval;
}
/* minimap/stats toggles intentionally removed (no rendering implementation) */
void render_3d_shutdown(void) {
if(!g_render_3d.initialized) return;
render_3d_sdl_shutdown(&g_render_3d.display);
if(g_render_3d.column_depths) {
free(g_render_3d.column_depths);
g_render_3d.column_depths= NULL;
}
/* clean up sprite renderer storage */
sprite_shutdown(&g_render_3d.sprite_renderer);
/* free textures */
texture_free_image(&g_render_3d.wall_texture);
texture_free_image(&g_render_3d.floor_texture);
g_render_3d.initialized= false;
g_render_3d.game_state= NULL;
}
