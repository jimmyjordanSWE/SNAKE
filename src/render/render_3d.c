#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "render_3d.h"
#include "env.h"
#include "game_internal.h"
#include "render_3d_camera.h"
#include "render_3d_projection.h"
#include "render_3d_raycast.h"
#include "render_3d_sdl.h"
#include "render_3d_sprite.h"
#include "render_3d_texture.h"
#include "types.h"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MINIMAP_TARGET_OCCUPANCY_PERCENT 40
#define MINIMAP_MIN_TARGET_SIZE 160
#define MINIMAP_MIN_CELL_PIXELS 3
#define MINIMAP_PADDING 8
typedef enum { RENDER_MODE_2D= 0, RENDER_MODE_3D, RENDER_MODE_COUNT } RenderMode;
typedef struct {
float x, y, radius, factor;
float radius_sq;
int factor_256;
} Decal;
typedef struct {
short count;
short ids[16];
} TileBucket;
typedef struct {
const GameState* game_state;
Camera3D* camera;
Raycaster3D* raycaster;
Projection3D* projector;
Texture3D* texture;
Texture3D* wall_texture;
Texture3D* floor_texture;
SpriteRenderer3D* sprite_renderer;
SDL3DContext* display;
Render3DConfig config;
bool initialized;
float* column_depths;
float frame_times[60];
int frame_time_idx;
float current_fps;
float* cos_offsets;
float* sin_offsets;
int offsets_cap;
/* Cached env_bool flags (avoid per-frame getenv) */
int cached_debug_timing;
int cached_fast_wall_tex;
int cached_fast_floor_tex;
int cached_debug_textures;
bool env_cached;
/* Pre-allocated decal/bucket arrays for floor shadows */
Decal* decal_pool;
int decal_pool_cap;
TileBucket* bucket_pool;
int bucket_pool_cap;
} Render3DContext;
static Render3DContext g_render_3d= {0};
/* Return a darker version of `col` by `pct` percent (pct in 0..100). */
static uint32_t render_3d_shade_color(uint32_t col, int pct) {
uint8_t a= (uint8_t)(col >> 24);
uint8_t r= (uint8_t)((col >> 16) & 0xFF);
uint8_t g= (uint8_t)((col >> 8) & 0xFF);
uint8_t b= (uint8_t)(col & 0xFF);
r= (uint8_t)((r * pct) / 100);
g= (uint8_t)((g * pct) / 100);
b= (uint8_t)((b * pct) / 100);
return render_3d_sdl_color(r, g, b, a);
}
static void render_3d_draw_char(SDL3DContext* disp, int x, int y, char c, uint32_t col, int scale);
static void render_3d_log(const char* fmt, ...) {
char buf[512];
time_t t= time(NULL);
struct tm tm;
if(localtime_r(&t, &tm) == NULL) tm= (struct tm){0};
int n= snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d] ", 1900 + tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
va_list ap;
va_start(ap, fmt);
size_t rem;
if(n < 0 || n >= (int)sizeof(buf)) {
rem= 0;
n= (int)sizeof(buf) - 1;
} else {
rem= sizeof(buf) - (size_t)n;
}
vsnprintf(buf + (n > 0 ? n : 0), rem, fmt, ap);
va_end(ap);
FILE* f= fopen("logs/render_debug.log", "a");
if(!f) return;
fprintf(f, "%s", buf);
fclose(f);
}
static double render_3d_now(void) {
struct timespec ts;
clock_gettime(CLOCK_MONOTONIC, &ts);
return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}
static void render_3d_draw_minimap(Render3DContext* r, float interp_t) {
if(!r || !r->game_state) {
render_3d_log("minimap: context or game_state NULL\n");
return;
}
const GameState* gs= r->game_state;
int map_w= gs->width;
int map_h= gs->height;
if(map_w <= 0 || map_h <= 0) {
render_3d_log("minimap: map size invalid %dx%d\n", map_w, map_h);
return;
}
int cell_px= render_3d_compute_minimap_cell_px(render_3d_sdl_get_width(r->display), render_3d_sdl_get_height(r->display), map_w, map_h);
int map_px_w= cell_px * map_w;
int map_px_h= cell_px * map_h;
int padding= MINIMAP_PADDING;
int x0= render_3d_sdl_get_width(r->display) - padding - map_px_w;
int y0= padding; /* Top instead of bottom */
if(x0 < padding) x0= padding;
if(y0 < padding) y0= padding;
/* debug: early getenv removed to avoid unused-variable warning; use env_bool() for checks */
if(env_bool("SNAKE_DEBUG_MINIMAP", 0)) {
render_3d_log("minimap: called gs=%p map=%dx%d display=%dx%d x0=%d "
              "y0=%d cell_px=%d\n",
    (void*)gs, map_w, map_h, render_3d_sdl_get_width(r->display), render_3d_sdl_get_height(r->display), x0, y0, cell_px);
}
uint32_t bg= render_3d_sdl_color(0, 0, 0, 200);
render_3d_sdl_draw_filled_rect(r->display, x0, y0, map_px_w, map_px_h, bg);
uint32_t border= render_3d_sdl_color(255, 255, 255, 255);
render_3d_sdl_draw_filled_rect(r->display, x0, y0, map_px_w, 1, border);
render_3d_sdl_draw_filled_rect(r->display, x0, y0 + map_px_h - 1, map_px_w, 1, border);
render_3d_sdl_draw_filled_rect(r->display, x0, y0, 1, map_px_h, border);
render_3d_sdl_draw_filled_rect(r->display, x0 + map_px_w - 1, y0, 1, map_px_h, border);
if(env_bool("SNAKE_DEBUG_MINIMAP", 0)) {
uint32_t dbgcol= render_3d_sdl_color(255, 0, 255, 255);
render_3d_sdl_draw_filled_rect(r->display, x0, y0, map_px_w, 1, dbgcol);
render_3d_sdl_draw_filled_rect(r->display, x0, y0 + map_px_h - 1, map_px_w, 1, dbgcol);
render_3d_sdl_draw_filled_rect(r->display, x0, y0, 1, map_px_h, dbgcol);
render_3d_sdl_draw_filled_rect(r->display, x0 + map_px_w - 1, y0, 1, map_px_h, dbgcol);
render_3d_log("minimap: debug border drawn at %d,%d size %dx%d (display %dx%d)\n", x0, y0, map_px_w, map_px_h, render_3d_sdl_get_width(r->display), render_3d_sdl_get_height(r->display));
}
uint32_t food_col= render_3d_sdl_color(255, 64, 64, 255);
for(int i= 0; i < gs->food_count; i++) {
int fx= x0 + gs->food[i].x * cell_px + cell_px / 2;
int fy= y0 + gs->food[i].y * cell_px + cell_px / 2;
int radius= cell_px > 2 ? (cell_px / 3) : 1;
render_3d_sdl_draw_filled_circle(r->display, fx, fy, radius, food_col);
}
for(int p= 0; p < gs->num_players; p++) {
const PlayerState* pl= &gs->players[p];
if(!pl->active || pl->length <= 0) continue;
/* Use player-configured color (0 means fallback). Tail is a darker shaded variant. */
uint32_t pcol= pl->color ? pl->color : render_3d_sdl_color(0, 128, 0, 255);
uint32_t tail_col_local= render_3d_shade_color(pcol, 60);
for(int bi= 1; bi < pl->length; bi++) {
float seg_x_f, seg_y_f;
seg_x_f= pl->prev_segment[bi].x + (((float)pl->body[bi].x + 0.5f) - pl->prev_segment[bi].x) * interp_t;
seg_y_f= pl->prev_segment[bi].y + (((float)pl->body[bi].y + 0.5f) - pl->prev_segment[bi].y) * interp_t;
int tx= x0 + (int)(seg_x_f * (float)cell_px + 0.5f);
int ty= y0 + (int)(seg_y_f * (float)cell_px + 0.5f);
int bw= cell_px > 2 ? (cell_px * 3 / 4) : 1;
int radius= bw / 2;
if(radius <= 0) radius= 1;
render_3d_sdl_draw_filled_circle(r->display, tx, ty, radius, tail_col_local);
(void)bi;
}
float head_x= pl->prev_head.x + (((float)pl->body[0].x + 0.5f) - pl->prev_head.x) * interp_t;
float head_y= pl->prev_head.y + (((float)pl->body[0].y + 0.5f) - pl->prev_head.y) * interp_t;
int hx= x0 + (int)(head_x * (float)cell_px + 0.5f);
int hy= y0 + (int)(head_y * (float)cell_px + 0.5f);
int hr= cell_px > 2 ? (cell_px / 2) : 1;
render_3d_sdl_draw_filled_circle(r->display, hx, hy, hr, pcol);
int dir_off_x= 0, dir_off_y= 0;
int off= (cell_px / 2) + 1;
switch(pl->current_dir) {
case SNAKE_DIR_UP: dir_off_y= -off; break;
case SNAKE_DIR_DOWN: dir_off_y= off; break;
case SNAKE_DIR_LEFT: dir_off_x= -off; break;
case SNAKE_DIR_RIGHT: dir_off_x= off; break;
default: break;
}
render_3d_sdl_draw_filled_circle(r->display, hx + dir_off_x, hy + dir_off_y, hr > 1 ? hr / 2 : 1, pcol);
}
}
void render_3d_draw_minimap_into(struct SDL3DContext* ctx, const GameState* gs) {
Render3DContext tmp= {0};
tmp.display= ctx;
tmp.game_state= gs;
render_3d_draw_minimap(&tmp, 0.0f);
}
static void render_3d_update_fps(float delta_seconds) {
if(delta_seconds <= 0.0f) return;
g_render_3d.frame_times[g_render_3d.frame_time_idx]= delta_seconds;
g_render_3d.frame_time_idx= (g_render_3d.frame_time_idx + 1) % 60;
float avg_time= 0.0f;
for(int i= 0; i < 60; i++) { avg_time+= g_render_3d.frame_times[i]; }
avg_time/= 60.0f;
g_render_3d.current_fps= (avg_time > 0.0f) ? (1.0f / avg_time) : 0.0f;
}
static void render_3d_draw_fps_counter(void) {
if(!g_render_3d.display) return;
char fps_buf[32];
snprintf(fps_buf, sizeof(fps_buf), "FPS: %.1f", g_render_3d.current_fps);
uint32_t fps_color= render_3d_sdl_color(0, 255, 0, 255);
render_3d_draw_char(g_render_3d.display, 4, 4, 'F', fps_color, 1);
render_3d_draw_char(g_render_3d.display, 10, 4, 'P', fps_color, 1);
render_3d_draw_char(g_render_3d.display, 16, 4, 'S', fps_color, 1);
render_3d_draw_char(g_render_3d.display, 22, 4, ':', fps_color, 1);
int x= 28;
int int_part= (int)g_render_3d.current_fps;
int frac_part= (int)((g_render_3d.current_fps - (float)int_part) * 10.0f);
if(int_part >= 100) {
render_3d_draw_char(g_render_3d.display, x, 4, (char)('0' + (int_part / 100)), fps_color, 1);
x+= 6;
int_part%= 100;
}
if(int_part >= 10 || (int)g_render_3d.current_fps >= 10) {
render_3d_draw_char(g_render_3d.display, x, 4, (char)('0' + (int_part / 10)), fps_color, 1);
x+= 6;
}
render_3d_draw_char(g_render_3d.display, x, 4, (char)('0' + (int_part % 10)), fps_color, 1);
x+= 6;
render_3d_draw_char(g_render_3d.display, x, 4, '.', fps_color, 1);
x+= 6;
render_3d_draw_char(g_render_3d.display, x, 4, (char)('0' + frac_part), fps_color, 1);
}
static const uint8_t font5x7_A_Z[][7]= {
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x0A, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    {0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},
};
static const uint8_t font5x7_digits[][7]= {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
    {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
};
static const uint8_t font5x7_exclaim[7]= {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04};
static void render_3d_draw_char(SDL3DContext* disp, int x, int y, char c, uint32_t col, int scale) {
if(!disp) return;
if(c == ' ') return;
const uint8_t* glyph= NULL;
if(c >= 'A' && c <= 'Z') {
glyph= font5x7_A_Z[c - 'A'];
} else if(c >= '0' && c <= '9') {
glyph= font5x7_digits[c - '0'];
} else if(c == '!') {
glyph= font5x7_exclaim;
} else {
return;
}
uint32_t* pix= render_3d_sdl_get_pixels(disp);
int w= render_3d_sdl_get_width(disp);
int h= render_3d_sdl_get_height(disp);
if(!pix) return;
for(int row= 0; row < 7; row++) {
uint8_t bits= glyph[row];
for(int colbit= 0; colbit < 5; colbit++) {
if(bits & (1 << (4 - colbit))) {
int px= x + colbit * scale;
int py= y + row * scale;
for(int yy= 0; yy < scale; yy++) {
if(py + yy < 0 || py + yy >= h) continue;
uint32_t* row_pix= &pix[(py + yy) * w];
for(int xx= 0; xx < scale; xx++) {
if(px + xx < 0 || px + xx >= w) continue;
row_pix[px + xx]= col;
}
}
}
}
}
}
static void render_3d_draw_text_centered(SDL3DContext* disp, int y, const char* text, uint32_t col, int scale) {
if(!disp || !text) return;
int char_w= 5 * scale;
int spacing= scale * 2;
int text_px_w= (int)strlen(text) * (char_w + spacing) - spacing;
int x= (render_3d_sdl_get_width(disp) - text_px_w) / 2;
for(const char* p= text; *p; ++p) {
char c= (char)toupper((unsigned char)*p);
render_3d_draw_char(disp, x, y, c, col, scale);
x+= char_w + spacing;
}
}
void render_3d_draw_death_overlay(const GameState* game, int anim_frame, bool show_prompt) {
(void)anim_frame;
(void)game;
if(!g_render_3d.initialized || !g_render_3d.display) return;
SDL3DContext* d= g_render_3d.display;
int w= render_3d_sdl_get_width(d) / 2;
int h= render_3d_sdl_get_height(d) / 4;
int x= (render_3d_sdl_get_width(d) - w) / 2;
int y= (render_3d_sdl_get_height(d) - h) / 2;
uint32_t bg= render_3d_sdl_color(0, 0, 0, 200);
render_3d_sdl_draw_filled_rect(d, x, y, w, h, bg);
int ly= y + 8;
render_3d_draw_text_centered(d, ly, "YOU DIED", render_3d_sdl_color(255, 255, 255, 255), 4);
ly+= 4 * 7 + 4 * 7;
if(show_prompt) {
render_3d_draw_text_centered(d, ly, "PRESS ANY KEY TO RESTART", render_3d_sdl_color(160, 255, 160, 255), 2);
ly+= 2 * 7 + 2 * 7;
render_3d_draw_text_centered(d, ly, "OR Q TO QUIT", render_3d_sdl_color(160, 255, 160, 255), 2);
}
(void)render_3d_sdl_present(d);
}
void render_3d_draw_winner_overlay(const GameState* game, int winner, int score) {
if(!g_render_3d.initialized || !g_render_3d.display) return;
SDL3DContext* d= g_render_3d.display;
int w= render_3d_sdl_get_width(d) * 2 / 3;
int h= render_3d_sdl_get_height(d) / 3;
int x= (render_3d_sdl_get_width(d) - w) / 2;
int y= (render_3d_sdl_get_height(d) - h) / 2;
uint32_t bg= render_3d_sdl_color(0, 0, 0, 200);
render_3d_sdl_draw_filled_rect(d, x, y, w, h, bg);
int ly= y + 6;
char title[64];
if(game && game->players[winner].name[0])
snprintf(title, sizeof(title), "PLAYER %d (%.*s) WON", winner + 1, (int)sizeof(game->players[winner].name) - 1, game->players[winner].name);
else
snprintf(title, sizeof(title), "PLAYER %d WON", winner + 1);
render_3d_draw_text_centered(d, ly, title, render_3d_sdl_color(255, 255, 0, 255), 3);
ly+= 3 * 7 + 3 * 7;
char buf[64];
snprintf(buf, sizeof(buf), "SCORE: %d", score);
render_3d_draw_text_centered(d, ly, buf, render_3d_sdl_color(200, 200, 255, 255), 2);
ly+= 2 * 7 + 2 * 7;
render_3d_draw_text_centered(d, ly, "PRESS ANY KEY TO CONTINUE", render_3d_sdl_color(160, 255, 160, 255), 2);
(void)render_3d_sdl_present(d);
}
void render_3d_draw_congrats_overlay(int score, const char* name_entered) {
if(!g_render_3d.initialized || !g_render_3d.display) return;
SDL3DContext* d= g_render_3d.display;
int w= render_3d_sdl_get_width(d) * 2 / 3;
int h= render_3d_sdl_get_height(d) / 3;
int x= (render_3d_sdl_get_width(d) - w) / 2;
int y= (render_3d_sdl_get_height(d) - h) / 2;
uint32_t bg= render_3d_sdl_color(0, 0, 0, 200);
render_3d_sdl_draw_filled_rect(d, x, y, w, h, bg);
int ly= y + 6;
render_3d_draw_text_centered(d, ly, "CONGRATULATIONS!", render_3d_sdl_color(255, 255, 0, 255), 3);
ly+= 3 * 7 + 3 * 7;
char buf[64];
snprintf(buf, sizeof(buf), "SCORE: %d", score);
render_3d_draw_text_centered(d, ly, buf, render_3d_sdl_color(200, 200, 255, 255), 2);
ly+= 2 * 7 + 2 * 7;
if(name_entered && name_entered[0]) {
char nb[32];
snprintf(nb, sizeof(nb), "NAME: %.*s", 8, name_entered);
render_3d_draw_text_centered(d, ly, nb, render_3d_sdl_color(160, 255, 160, 255), 2);
} else {
render_3d_draw_text_centered(d, ly, "ENTER A SHORT NAME", render_3d_sdl_color(160, 255, 160, 255), 2);
}
(void)render_3d_sdl_present(d);
}
int render_3d_compute_minimap_cell_px(int d_w, int d_h, int m_w, int m_h) {
if(d_w <= 0 || d_h <= 0 || m_w <= 0 || m_h <= 0) return 0;
int max_dim= m_w > m_h ? m_w : m_h;
int smaller= d_w < d_h ? d_w : d_h;
int target= (smaller * MINIMAP_TARGET_OCCUPANCY_PERCENT) / 100;
if(target < MINIMAP_MIN_TARGET_SIZE) target= MINIMAP_MIN_TARGET_SIZE;
int cell_px= target / max_dim;
if(cell_px < MINIMAP_MIN_CELL_PIXELS) cell_px= MINIMAP_MIN_CELL_PIXELS;
return cell_px;
}
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
{
FILE* f= fopen("logs/debug_init.log", "a");
if(f) {
fprintf(f, "DEBUG: reached render_3d_init point 1\n");
fprintf(f, "DEBUG: reached render_3d_init point 2 (display=%p)\n", (void*)g_render_3d.display);
fclose(f);
}
}
g_render_3d.display= render_3d_sdl_create(g_render_3d.config.screen_width, g_render_3d.config.screen_height, g_render_3d.config.vsync);
if(!g_render_3d.display) return false;
g_render_3d.camera= camera_create(g_render_3d.config.fov_degrees, g_render_3d.config.screen_width, 0.5f);
if(!g_render_3d.camera) return false;
g_render_3d.raycaster= raycaster_create(game_state->width, game_state->height, NULL);
if(!g_render_3d.raycaster) return false;
g_render_3d.projector= projection_create(g_render_3d.config.screen_width, g_render_3d.config.screen_height, g_render_3d.config.fov_degrees * 3.14159265359f / 180.0f, g_render_3d.config.wall_height_scale);
if(!g_render_3d.projector) return false;
g_render_3d.texture= texture_create();
g_render_3d.wall_texture= texture_create();
g_render_3d.floor_texture= texture_create();
if(!g_render_3d.texture || !g_render_3d.wall_texture || !g_render_3d.floor_texture) return false;
if(g_render_3d.config.wall_texture_path[0]) {
if(!texture_load_from_file(g_render_3d.wall_texture, g_render_3d.config.wall_texture_path)) {
fprintf(stderr,
    "render_3d_init: failed to load %s (using procedural "
    "fallback)\n",
    g_render_3d.config.wall_texture_path);
}
} else {
if(!texture_load_from_file(g_render_3d.wall_texture, PERSIST_CONFIG_DEFAULT_WALL_TEXTURE)) {
fprintf(stderr,
    "render_3d_init: failed to load %s (using procedural "
    "fallback)\n",
    PERSIST_CONFIG_DEFAULT_WALL_TEXTURE);
}
}
if(g_render_3d.config.floor_texture_path[0]) {
if(!texture_load_from_file(g_render_3d.floor_texture, g_render_3d.config.floor_texture_path)) { fprintf(stderr, "render_3d_init: failed to load %s (using flat floor color)\n", g_render_3d.config.floor_texture_path); }
} else {
if(!texture_load_from_file(g_render_3d.floor_texture, PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE)) { fprintf(stderr, "render_3d_init: failed to load %s (using flat floor color)\n", PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE); }
}
g_render_3d.column_depths= calloc((size_t)render_3d_sdl_get_width(g_render_3d.display), sizeof(float));
if(!g_render_3d.column_depths) return false;
g_render_3d.sprite_renderer= sprite_create(100, g_render_3d.camera, g_render_3d.projector);
if(!g_render_3d.sprite_renderer) return false;
g_render_3d.initialized= true;
return true;
}
static void render_3d_draw_debug_overlays(Render3DContext* r) {
if(!r->cached_debug_textures) return;
const uint32_t* wp= texture_get_pixels(r->wall_texture);
if(wp) {
for(int yy= 0; yy < 16; yy++) {
for(int xx= 0; xx < 16; xx++) {
uint32_t c= texture_sample(r->wall_texture, (float)xx / 16.0f, (float)yy / 16.0f, true);
if(8 + xx >= 0 && 8 + xx < render_3d_sdl_get_width(r->display) && 8 + yy >= 0 && 8 + yy < render_3d_sdl_get_height(r->display)) render_3d_sdl_get_pixels(r->display)[(8 + yy) * render_3d_sdl_get_width(r->display) + (8 + xx)]= c;
}
}
}
const uint32_t* fp= texture_get_pixels(r->floor_texture);
if(fp) {
for(int yy= 0; yy < 16; yy++) {
for(int xx= 0; xx < 16; xx++) {
uint32_t c= texture_sample(r->floor_texture, (float)xx / 16.0f, (float)yy / 16.0f, true);
if(8 + xx >= 0 && 8 + xx < render_3d_sdl_get_width(r->display) && 28 + yy >= 0 && 28 + yy < render_3d_sdl_get_height(r->display)) render_3d_sdl_get_pixels(r->display)[(28 + yy) * render_3d_sdl_get_width(r->display) + (8 + xx)]= c;
}
}
}
fprintf(stderr, "render_3d: debug texture overlay drawn\n");
}
static void render_3d_setup_floor_decals(Render3DContext* r, const GameState* gs, Decal** decals_out, TileBucket** buckets_out, int* decal_count_out) {
int map_w= gs->width;
int map_h= gs->height;
int bucket_count= map_w * map_h;
if(bucket_count > r->bucket_pool_cap) {
r->bucket_pool= realloc(r->bucket_pool, (size_t)bucket_count * sizeof(*r->bucket_pool));
r->bucket_pool_cap= bucket_count;
}
*buckets_out= r->bucket_pool;
if(*buckets_out) memset(*buckets_out, 0, (size_t)bucket_count * sizeof(**buckets_out));
int max_decals= gs->food_count;
for(int pi= 0; pi < gs->num_players; pi++) max_decals+= gs->players[pi].length;
if(max_decals > r->decal_pool_cap) {
r->decal_pool= realloc(r->decal_pool, (size_t)max_decals * sizeof(*r->decal_pool));
r->decal_pool_cap= max_decals;
}
*decals_out= r->decal_pool;
int decal_count= 0;
if(*buckets_out && *decals_out) {
Decal* decals= *decals_out;
TileBucket* buckets= *buckets_out;
for(int i= 0; i < gs->food_count; i++) {
decals[decal_count].x= (float)gs->food[i].x + 0.5f;
decals[decal_count].y= (float)gs->food[i].y + 0.5f;
decals[decal_count].radius= 0.15f;
decals[decal_count].factor= 0.4f;
decals[decal_count].radius_sq= 0.0225f;
decals[decal_count].factor_256= 102;
decal_count++;
}
for(int pi= 0; pi < gs->num_players; pi++) {
const PlayerState* player= &gs->players[pi];
if(!player->active) continue;
for(int bi= 0; bi < player->length; bi++) {
decals[decal_count].x= (float)player->body[bi].x + 0.5f;
decals[decal_count].y= (float)player->body[bi].y + 0.5f;
float r_val= (bi == 0) ? 0.25f : 0.2f;
decals[decal_count].radius= r_val;
decals[decal_count].factor= 0.4f;
decals[decal_count].radius_sq= r_val * r_val;
decals[decal_count].factor_256= 102;
decal_count++;
}
}
for(int di= 0; di < decal_count; di++) {
int min_tx= (int)floorf(decals[di].x - decals[di].radius);
int max_tx= (int)floorf(decals[di].x + decals[di].radius);
int min_ty= (int)floorf(decals[di].y - decals[di].radius);
int max_ty= (int)floorf(decals[di].y + decals[di].radius);
if(min_tx < 0) min_tx= 0;
if(min_ty < 0) min_ty= 0;
if(max_tx >= map_w) max_tx= map_w - 1;
if(max_ty >= map_h) max_ty= map_h - 1;
for(int ty= min_ty; ty <= max_ty; ty++) {
for(int tx= min_tx; tx <= max_tx; tx++) {
TileBucket* b= &buckets[ty * map_w + tx];
if(b->count < (int)(sizeof(b->ids) / sizeof(b->ids[0]))) b->ids[b->count++]= (short)di;
}
}
}
}
*decal_count_out= decal_count;
}
static void render_3d_draw_floor_ceiling_pass(Render3DContext* r, int screen_w, int screen_h, int horizon, float interp_cam_x, float interp_cam_y, float cos_cam, float sin_cam, Decal* decals, TileBucket* buckets, int decal_count) {
uint32_t floor_color= render_3d_sdl_color(139, 69, 19, 255);
uint32_t ceiling_color= render_3d_sdl_color(65, 105, 225, 255);
uint32_t* pix= render_3d_sdl_get_pixels(r->display);
float wall_scale= projection_get_wall_scale(r->projector);
const uint32_t* floor_pix= texture_get_pixels(r->floor_texture);
int floor_w= texture_get_img_w(r->floor_texture);
int floor_h_tex= texture_get_img_h(r->floor_texture);
float floor_tex_scale= r->config.floor_texture_scale;
int map_w= r->game_state->width;
int map_h= r->game_state->height;
bool fast_floor_tex= r->cached_fast_floor_tex;
for(int y= 0; y < screen_h; y++) {
if(y < horizon) {
uint32_t* row_pix= &pix[y * screen_w];
for(int x= 0; x < screen_w; x++) row_pix[x]= ceiling_color;
continue;
}
float p= (float)(y - horizon);
if(p < 1.0f) p= 1.0f;
float pos_z= 0.5f * (float)screen_h * wall_scale;
float row_distance_center= pos_z / p;
uint32_t* row_pix= &pix[y * screen_w];
/* DDA approach: compute world-space endpoints of this scanline */
/* Leftmost ray (x=0) */
float cos_a_left= cos_cam * r->cos_offsets[0] - sin_cam * r->sin_offsets[0];
float sin_a_left= sin_cam * r->cos_offsets[0] + cos_cam * r->sin_offsets[0];
float cos_angle_diff_left= r->cos_offsets[0];
float perp_left= (cos_angle_diff_left > 0.01f) ? row_distance_center / cos_angle_diff_left : row_distance_center;
/* Rightmost ray (x=screen_w-1) */
int last_x= screen_w - 1;
float cos_a_right= cos_cam * r->cos_offsets[last_x] - sin_cam * r->sin_offsets[last_x];
float sin_a_right= sin_cam * r->cos_offsets[last_x] + cos_cam * r->sin_offsets[last_x];
float cos_angle_diff_right= r->cos_offsets[last_x];
float perp_right= (cos_angle_diff_right > 0.01f) ? row_distance_center / cos_angle_diff_right : row_distance_center;
/* World-space endpoints */
float wx_left= interp_cam_x + cos_a_left * perp_left;
float wy_left= interp_cam_y + sin_a_left * perp_left;
float wx_right= interp_cam_x + cos_a_right * perp_right;
float wy_right= interp_cam_y + sin_a_right * perp_right;
/* Incremental step across scanline */
float dx= (wx_right - wx_left) / (float)last_x;
float dy= (wy_right - wy_left) / (float)last_x;
float floor_x= wx_left;
float floor_y= wy_left;
for(int x= 0; x < screen_w; x++) {
uint32_t base_col;
bool in_shadow= false;
uint32_t shadow_factor_256= 256;
if(buckets) {
int tx= (int)floorf(floor_x), ty= (int)floorf(floor_y);
if(tx >= 0 && tx < map_w && ty >= 0 && ty < map_h) {
TileBucket* b= &buckets[ty * map_w + tx];
for(int bi= 0; bi < b->count; bi++) {
int di= b->ids[bi];
float dx_shadow= floor_x - decals[di].x, dy_shadow= floor_y - decals[di].y;
if(dx_shadow * dx_shadow + dy_shadow * dy_shadow < decals[di].radius_sq) {
in_shadow= true;
shadow_factor_256= (uint32_t)decals[di].factor_256;
break;
}
}
}
}
if(floor_pix) {
if(fast_floor_tex) {
int tx= (int)(floor_x * floor_tex_scale * (float)floor_w) % floor_w;
int ty= (int)(floor_y * floor_tex_scale * (float)floor_h_tex) % floor_h_tex;
if(tx < 0) tx+= floor_w;
if(ty < 0) ty+= floor_h_tex;
base_col= floor_pix[ty * floor_w + tx];
} else {
base_col= texture_sample(r->floor_texture, floor_x * floor_tex_scale, floor_y * floor_tex_scale, true);
}
} else {
base_col= floor_color;
}
if(in_shadow) {
uint32_t red= (base_col >> 16) & 0xFF;
uint32_t green= (base_col >> 8) & 0xFF;
uint32_t blue= base_col & 0xFF;
red= (red * shadow_factor_256) >> 8;
green= (green * shadow_factor_256) >> 8;
blue= (blue * shadow_factor_256) >> 8;
row_pix[x]= (0xFFu << 24) | (red << 16) | (green << 8) | blue;
} else {
row_pix[x]= base_col;
}
/* Increment floor position for next column */
floor_x+= dx;
floor_y+= dy;
}
}
}
static void render_3d_draw_walls_pass(Render3DContext* r, int screen_w, int screen_h, int horizon, float interp_cam_x, float interp_cam_y, float interp_cam_angle, float cos_cam, float sin_cam) {
uint32_t* pix= render_3d_sdl_get_pixels(r->display);
const float* s_angle_offsets= camera_get_cached_angle_offsets(r->camera);
const uint32_t* wall_pix= texture_get_pixels(r->wall_texture);
int wall_w= texture_get_img_w(r->wall_texture);
int wall_h_tex= texture_get_img_h(r->wall_texture);
bool fast_wall_tex= r->cached_fast_wall_tex;
for(int x= 0; x < screen_w; x++) {
float ray_angle= interp_cam_angle + s_angle_offsets[x];
float ray_cos= cos_cam * r->cos_offsets[x] - sin_cam * r->sin_offsets[x];
float ray_sin= sin_cam * r->cos_offsets[x] + cos_cam * r->sin_offsets[x];
RayHit hit;
if(raycast_cast_ray_fast(r->raycaster, interp_cam_x, interp_cam_y, ray_cos, ray_sin, &hit)) {
WallProjection proj;
projection_project_wall_perp(r->projector, hit.distance, ray_angle, interp_cam_angle, &proj);
float pd= hit.distance * r->cos_offsets[x];
if(pd <= 0.001f) pd= 0.001f;
if(r->column_depths) r->column_depths[x]= pd;
float tex_coord= raycast_get_texture_coord(&hit, hit.is_vertical) * r->config.wall_texture_scale;
int full_wall_h= proj.wall_height > 0 ? proj.wall_height : 1;
float tex_v_coord_step= (1.0f / (float)full_wall_h) * r->config.wall_texture_scale;
int unclamped_start= horizon - (full_wall_h / 2);
float tex_v_start= (float)(proj.draw_start - unclamped_start) * tex_v_coord_step;
if(wall_pix && fast_wall_tex) {
int tx= (int)(tex_coord * (float)wall_w) % wall_w;
if(tx < 0) tx+= wall_w;
/* Q16 fixed-point for vertical coordinate (16 fractional bits) */
const int FRAC_BITS= 16;
const int FRAC_ONE= 1 << FRAC_BITS;
int ty_fixed= (int)(tex_v_start * (float)wall_h_tex * (float)FRAC_ONE + 0.5f);
int ty_step_fixed= (int)(tex_v_coord_step * (float)wall_h_tex * (float)FRAC_ONE + 0.5f);
for(int yy= proj.draw_start; yy <= proj.draw_end; yy++) {
int ty= (ty_fixed >> FRAC_BITS) % wall_h_tex;
if(ty < 0) ty+= wall_h_tex;
if(pix && yy >= 0 && yy < screen_h) pix[yy * screen_w + x]= wall_pix[ty * wall_w + tx];
ty_fixed+= ty_step_fixed;
}
} else {
float tex_v_c= tex_v_start;
for(int yy= proj.draw_start; yy <= proj.draw_end; yy++) {
uint32_t col= texture_sample(r->wall_texture, tex_coord, tex_v_c, !fast_wall_tex);
if(pix && yy >= 0 && yy < screen_h) pix[yy * screen_w + x]= col;
tex_v_c+= tex_v_coord_step;
}
}
} else if(r->column_depths) {
r->column_depths[x]= INFINITY;
}
}
}
void render_3d_draw(const GameState* gs, const char* name, const void* sc, int scc, float dt) {
(void)name;
(void)sc;
(void)scc;
if(!g_render_3d.initialized || !gs) return;
g_render_3d.game_state= gs;
float c_dt= dt > 0.5f ? 0.5f : dt;
if(!g_render_3d.env_cached) {
g_render_3d.cached_debug_timing= env_bool("SNAKE_DEBUG_3D_TIMING", 0);
g_render_3d.cached_fast_wall_tex= env_bool("SNAKE_3D_FAST_WALLS", 1);
g_render_3d.cached_fast_floor_tex= env_bool("SNAKE_3D_FAST_FLOOR", 1);
g_render_3d.cached_debug_textures= env_bool("SNAKE_DEBUG_TEXTURES", 0);
g_render_3d.env_cached= true;
}
double t0= g_render_3d.cached_debug_timing ? render_3d_now() : 0.0;
camera_update_interpolation(g_render_3d.camera, c_dt);
render_3d_update_fps(dt);
float f_interp= camera_get_interpolation_fraction(g_render_3d.camera);
const int sw= render_3d_sdl_get_width(g_render_3d.display), sh= render_3d_sdl_get_height(g_render_3d.display);
int horizon= sh / 2;
camera_prepare_angle_offsets(g_render_3d.camera, sw);
const float* offsets= camera_get_cached_angle_offsets(g_render_3d.camera);
if(sw != g_render_3d.offsets_cap && offsets) {
g_render_3d.cos_offsets= realloc(g_render_3d.cos_offsets, (size_t)sw * sizeof(float));
g_render_3d.sin_offsets= realloc(g_render_3d.sin_offsets, (size_t)sw * sizeof(float));
for(int i= 0; i < sw; i++) {
g_render_3d.cos_offsets[i]= cosf(offsets[i]);
g_render_3d.sin_offsets[i]= sinf(offsets[i]);
}
g_render_3d.offsets_cap= sw;
}
render_3d_draw_debug_overlays(&g_render_3d);
float icx, icy, ica;
camera_get_interpolated_position(g_render_3d.camera, &icx, &icy);
ica= camera_get_interpolated_angle(g_render_3d.camera);
float cos_c= cosf(ica), sin_c= sinf(ica);
Decal* d_p= NULL;
TileBucket* b_p= NULL;
int d_c= 0;
render_3d_setup_floor_decals(&g_render_3d, gs, &d_p, &b_p, &d_c);
render_3d_draw_floor_ceiling_pass(&g_render_3d, sw, sh, horizon, icx, icy, cos_c, sin_c, d_p, b_p, d_c);
render_3d_draw_walls_pass(&g_render_3d, sw, sh, horizon, icx, icy, ica, cos_c, sin_c);
if(g_render_3d.sprite_renderer) {
sprite_clear(g_render_3d.sprite_renderer);
for(int i= 0; i < gs->food_count; i++) sprite_add_color_shaded(g_render_3d.sprite_renderer, (float)gs->food[i].x + 0.5f, (float)gs->food[i].y + 0.5f, 0.25f, -0.5f, true, -1, 0, render_3d_sdl_color(255, 0, 0, 255));
for(int p= 0; p < gs->num_players; p++) {
const PlayerState* pl= &gs->players[p];
if(!pl->active || pl->length == 0 || p == g_render_3d.config.active_player) continue;
float hx= pl->prev_head.x + (((float)pl->body[0].x + 0.5f) - pl->prev_head.x) * f_interp;
float hy= pl->prev_head.y + (((float)pl->body[0].y + 0.5f) - pl->prev_head.y) * f_interp;
sprite_add_color_shaded(g_render_3d.sprite_renderer, hx, hy, 1.0f, 0.0f, true, -1, 0, pl->color ? pl->color : render_3d_sdl_color(0, 128, 0, 255));
}
for(int p= 0; p < gs->num_players; p++) {
const PlayerState* pl= &gs->players[p];
if(!pl->active || pl->length <= 1) continue;
uint32_t bc= pl->color ? render_3d_shade_color(pl->color, 60) : render_3d_sdl_color(0, 128, 0, 255);
for(int bi= 1; bi < pl->length; bi++) {
float sx= pl->prev_segment[bi].x + (((float)pl->body[bi].x + 0.5f) - pl->prev_segment[bi].x) * f_interp;
float sy= pl->prev_segment[bi].y + (((float)pl->body[bi].y + 0.5f) - pl->prev_segment[bi].y) * f_interp;
sprite_add_color(g_render_3d.sprite_renderer, sx, sy, g_render_3d.config.tail_height_scale, 0.0f, true, -1, 0, bc);
}
}
sprite_project_all(g_render_3d.sprite_renderer);
sprite_sort_by_depth(g_render_3d.sprite_renderer);
sprite_draw(g_render_3d.sprite_renderer, g_render_3d.display, g_render_3d.column_depths);
}
render_3d_draw_minimap(&g_render_3d, f_interp);
render_3d_draw_fps_counter();
(void)render_3d_sdl_present(g_render_3d.display);
if(g_render_3d.cached_debug_timing) { render_3d_log("3D_TIMING: total=%.2fms screen=%dx%d map=%dx%d\n", (render_3d_now() - t0) * 1000.0, sw, sh, gs->width, gs->height); }
}
void render_3d_set_active_player(int player_index) __attribute__((used));
void render_3d_set_active_player(int player_index) {
if(g_render_3d.initialized) g_render_3d.config.active_player= player_index;
}
void render_3d_set_fov(float fov_degrees) __attribute__((used));
void render_3d_set_fov(float fov_degrees) {
if(g_render_3d.initialized) g_render_3d.config.fov_degrees= fov_degrees;
}
struct SDL3DContext* render_3d_get_display(void) { return g_render_3d.display; }
void render_3d_on_tick(const GameState* game_state) {
if(!g_render_3d.initialized || !game_state) return;
if(g_render_3d.config.active_player < game_state->num_players) {
const PlayerState* player= &game_state->players[g_render_3d.config.active_player];
if(player->length > 0) camera_set_from_player(g_render_3d.camera, player->body[0].x, player->body[0].y, player->current_dir);
}
}
void render_3d_set_tick_rate_ms(int ms) {
if(!g_render_3d.initialized) return;
if(ms <= 0) ms= 1;
camera_set_update_interval(g_render_3d.camera, (float)ms / 1000.0f);
if(camera_get_interp_time(g_render_3d.camera) > camera_get_update_interval(g_render_3d.camera)) camera_set_interpolation_time(g_render_3d.camera, camera_get_update_interval(g_render_3d.camera));
}
void render_3d_shutdown(void) {
if(!g_render_3d.initialized) return;
render_3d_sdl_destroy(g_render_3d.display);
g_render_3d.display= NULL;
if(g_render_3d.column_depths) {
free(g_render_3d.column_depths);
g_render_3d.column_depths= NULL;
}
if(g_render_3d.cos_offsets) {
free(g_render_3d.cos_offsets);
g_render_3d.cos_offsets= NULL;
}
if(g_render_3d.sin_offsets) {
free(g_render_3d.sin_offsets);
g_render_3d.sin_offsets= NULL;
}
g_render_3d.offsets_cap= 0;
/* Free pre-allocated pools */
if(g_render_3d.decal_pool) {
free(g_render_3d.decal_pool);
g_render_3d.decal_pool= NULL;
}
g_render_3d.decal_pool_cap= 0;
if(g_render_3d.bucket_pool) {
free(g_render_3d.bucket_pool);
g_render_3d.bucket_pool= NULL;
}
g_render_3d.bucket_pool_cap= 0;
g_render_3d.env_cached= false;
sprite_destroy(g_render_3d.sprite_renderer);
g_render_3d.sprite_renderer= NULL;
if(g_render_3d.wall_texture) {
texture_destroy(g_render_3d.wall_texture);
g_render_3d.wall_texture= NULL;
}
if(g_render_3d.floor_texture) {
texture_destroy(g_render_3d.floor_texture);
g_render_3d.floor_texture= NULL;
}
if(g_render_3d.texture) {
texture_destroy(g_render_3d.texture);
g_render_3d.texture= NULL;
}
if(g_render_3d.projector) {
projection_destroy(g_render_3d.projector);
g_render_3d.projector= NULL;
}
if(g_render_3d.raycaster) {
raycaster_destroy(g_render_3d.raycaster);
g_render_3d.raycaster= NULL;
}
if(g_render_3d.camera) {
camera_destroy(g_render_3d.camera);
g_render_3d.camera= NULL;
}
g_render_3d.initialized= false;
g_render_3d.game_state= NULL;
}
