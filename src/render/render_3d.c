#include "snake/render_3d.h"
#include "snake/game_internal.h"
#include "snake/render_3d_camera.h"
#include "snake/render_3d_projection.h"
#include "snake/render_3d_raycast.h"
#include "snake/render_3d_sdl.h"
#include "snake/render_3d_sprite.h"
#include "snake/render_3d_texture.h"
#include "snake/types.h"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
typedef enum { RENDER_MODE_2D= 0, RENDER_MODE_3D, RENDER_MODE_COUNT } RenderMode;
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
} Render3DContext;
static Render3DContext g_render_3d= {0};
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
FILE* f= fopen("build/render_debug.log", "a");
if(!f) f= fopen("render_debug.log", "a");
if(!f) return;
fprintf(f, "%s", buf);
fclose(f);
}
static void render_3d_draw_minimap(Render3DContext* r) {
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
int cell_px = render_3d_compute_minimap_cell_px(render_3d_sdl_get_width(r->display), render_3d_sdl_get_height(r->display), map_w, map_h);
int map_px_w= cell_px * map_w;
int map_px_h= cell_px * map_h;
int padding= 8;
int x0= render_3d_sdl_get_width(r->display) - padding - map_px_w;
int y0= render_3d_sdl_get_height(r->display) - padding - map_px_h;
if(x0 < padding) x0= padding;
if(y0 < padding) y0= padding;
const char* dbg_minimap_early= getenv("SNAKE_DEBUG_MINIMAP");
if(dbg_minimap_early && dbg_minimap_early[0] == '1') {
render_3d_log("minimap: called gs=%p map=%dx%d display=%dx%d x0=%d "
              "y0=%d cell_px=%d\n",
    (void*)gs, map_w, map_h, render_3d_sdl_get_width(r->display), render_3d_sdl_get_height(r->display), x0, y0, cell_px);
}
uint32_t bg= render_3d_sdl_color(0, 0, 0, 200);
for(int yy= 0; yy < map_px_h; yy++) {
for(int xx= 0; xx < map_px_w; xx++) { render_3d_sdl_blend_pixel(r->display, x0 + xx, y0 + yy, bg); }
}
uint32_t border= render_3d_sdl_color(255, 255, 255, 255);
for(int xx= 0; xx < map_px_w; xx++) {
render_3d_sdl_blend_pixel(r->display, x0 + xx, y0, border);
render_3d_sdl_blend_pixel(r->display, x0 + xx, y0 + map_px_h - 1, border);
}
for(int yy= 0; yy < map_px_h; yy++) {
render_3d_sdl_blend_pixel(r->display, x0, y0 + yy, border);
render_3d_sdl_blend_pixel(r->display, x0 + map_px_w - 1, y0 + yy, border);
}
const char* dbg_minimap= getenv("SNAKE_DEBUG_MINIMAP");
if(dbg_minimap && dbg_minimap[0] == '1') {
uint32_t dbgcol= render_3d_sdl_color(255, 0, 255, 255);
for(int xx= 0; xx < map_px_w; xx++) {
render_3d_sdl_blend_pixel(r->display, x0 + xx, y0, dbgcol);
render_3d_sdl_blend_pixel(r->display, x0 + xx, y0 + map_px_h - 1, dbgcol);
}
for(int yy= 0; yy < map_px_h; yy++) {
render_3d_sdl_blend_pixel(r->display, x0, y0 + yy, dbgcol);
render_3d_sdl_blend_pixel(r->display, x0 + map_px_w - 1, y0 + yy, dbgcol);
}
render_3d_log("minimap: debug border drawn at %d,%d size %dx%d (display %dx%d)\n", x0, y0, map_px_w, map_px_h, render_3d_sdl_get_width(r->display), render_3d_sdl_get_height(r->display));
}
uint32_t food_col= render_3d_sdl_color(255, 64, 64, 255);
for(int i= 0; i < gs->food_count; i++) {
int fx= x0 + gs->food[i].x * cell_px + cell_px / 2;
int fy= y0 + gs->food[i].y * cell_px + cell_px / 2;
int radius= cell_px > 2 ? (cell_px / 3) : 1;
render_3d_sdl_draw_filled_circle(r->display, fx, fy, radius, food_col);
}
uint32_t player_cols[3]= {render_3d_sdl_color(0, 255, 0, 255), render_3d_sdl_color(0, 200, 255, 255), render_3d_sdl_color(255, 255, 0, 255)};
uint32_t tail_col= render_3d_sdl_color(128, 128, 128, 255);
for(int p= 0; p < gs->num_players; p++) {
const PlayerState* pl= &gs->players[p];
if(!pl->active || pl->length <= 0) continue;
float t= camera_get_interpolation_fraction(r->camera);
float head_x= pl->prev_head_x + (((float)pl->body[0].x + 0.5f) - pl->prev_head_x) * t;
float head_y= pl->prev_head_y + (((float)pl->body[0].y + 0.5f) - pl->prev_head_y) * t;
int hx= x0 + (int)(head_x * (float)cell_px + 0.5f);
int hy= y0 + (int)(head_y * (float)cell_px + 0.5f);
int hr= cell_px > 2 ? (cell_px / 2) : 1;
uint32_t pcol= player_cols[p % (int)(sizeof(player_cols) / sizeof(player_cols[0]))];
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
for(int bi= 1; bi < pl->length; bi++) {
int tx= x0 + pl->body[bi].x * cell_px + cell_px / 2;
int ty= y0 + pl->body[bi].y * cell_px + cell_px / 2;
int tr= cell_px > 2 ? (cell_px / 3) : 1;
render_3d_sdl_draw_filled_circle(r->display, tx, ty, tr, tail_col);
}
}
}

/*
 * Minimal 5x7 bitmap font renderer (uppercase + digits + space + !)
 * Each glyph is 5 pixels wide and 7 pixels tall. Stored as 7 bytes (one per row)
 * with the least significant 5 bits used (LSB = leftmost pixel).
 */
static const uint8_t font5x7_A_Z[][7] = {
    /* A */ {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    /* B */ {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    /* C */ {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    /* D */ {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C},
    /* E */ {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    /* F */ {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    /* G */ {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
    /* H */ {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    /* I */ {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    /* J */ {0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
    /* K */ {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    /* L */ {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    /* M */ {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
    /* N */ {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    /* O */ {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    /* P */ {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    /* Q */ {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    /* R */ {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    /* S */ {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
    /* T */ {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    /* U */ {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    /* V */ {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    /* W */ {0x11,0x11,0x11,0x15,0x15,0x0A,0x11},
    /* X */ {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    /* Y */ {0x11,0x11,0x11,0x0A,0x04,0x04,0x04},
    /* Z */ {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
};
static const uint8_t font5x7_digits[][7] = {
    /* 0 */ {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    /* 1 */ {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    /* 2 */ {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
    /* 3 */ {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E},
    /* 4 */ {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    /* 5 */ {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    /* 6 */ {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    /* 7 */ {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    /* 8 */ {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    /* 9 */ {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
};
static const uint8_t font5x7_exclaim[7] = {0x04,0x04,0x04,0x04,0x04,0x00,0x04};

static void render_3d_draw_char(SDL3DContext* disp, int x, int y, char c, uint32_t col, int scale) {
    if(!disp) return;
    if(c == ' ') return; /* skip space */
    const uint8_t* glyph = NULL;
    if(c >= 'A' && c <= 'Z') {
        glyph = font5x7_A_Z[c - 'A'];
    } else if(c >= '0' && c <= '9') {
        glyph = font5x7_digits[c - '0'];
    } else if(c == '!') {
        glyph = font5x7_exclaim;
    } else {
        return; /* unsupported */
    }
    for(int row = 0; row < 7; row++) {
        uint8_t bits = glyph[row];
        for(int colbit = 0; colbit < 5; colbit++) {
            if(bits & (1 << (4 - colbit))) {
                int px = x + colbit * scale;
                int py = y + row * scale;
                for(int yy = 0; yy < scale; yy++) for(int xx = 0; xx < scale; xx++) render_3d_sdl_blend_pixel(disp, px + xx, py + yy, col);
            }
        }
    }
}

static void render_3d_draw_text_centered(SDL3DContext* disp, int y, const char* text, uint32_t col, int scale) {
    if(!disp || !text) return;
    int char_w = 5 * scale;
    int spacing = scale * 2;
    int text_px_w = (int)strlen(text) * (char_w + spacing) - spacing;
    int x = (render_3d_sdl_get_width(disp) - text_px_w) / 2;
    for(const char* p = text; *p; ++p) {
        char c = (char)toupper((unsigned char)*p);
        render_3d_draw_char(disp, x, y, c, col, scale);
        x += char_w + spacing;
    }
}

void render_3d_draw_death_overlay(const GameState* game, int anim_frame, bool show_prompt) {
    (void)anim_frame;
    (void)game;
    if(!g_render_3d.initialized || !g_render_3d.display) return;
    SDL3DContext* d = g_render_3d.display;
    /* semi-transparent box */
    int w = render_3d_sdl_get_width(d) / 2;
    int h = render_3d_sdl_get_height(d) / 4;
    int x = (render_3d_sdl_get_width(d) - w) / 2;
    int y = (render_3d_sdl_get_height(d) - h) / 2;
    uint32_t bg = render_3d_sdl_color(0, 0, 0, 200);
    for(int yy = 0; yy < h; yy++) for(int xx = 0; xx < w; xx++) render_3d_sdl_blend_pixel(d, x + xx, y + yy, bg);
    int ly = y + 8;
    render_3d_draw_text_centered(d, ly, "YOU DIED", render_3d_sdl_color(255,255,255,255), 4);
    /* add an empty line (gap equal to previous line height) */
    ly += 4 * 7 + 4 * 7;
    if(show_prompt) {
        render_3d_draw_text_centered(d, ly, "PRESS ANY KEY TO RESTART", render_3d_sdl_color(160,255,160,255), 2);
        /* add an empty line equal to the previous small line height */
        ly += 2 * 7 + 2 * 7;
        render_3d_draw_text_centered(d, ly, "OR Q TO QUIT", render_3d_sdl_color(160,255,160,255), 2);
    }
    /* Present so overlay is visible immediately */
    (void)render_3d_sdl_present(d);
}

void render_3d_draw_congrats_overlay(int score, const char* name_entered) {
    if(!g_render_3d.initialized || !g_render_3d.display) return;
    SDL3DContext* d = g_render_3d.display;
    int w = render_3d_sdl_get_width(d) * 2 / 3;
    int h = render_3d_sdl_get_height(d) / 3;
    int x = (render_3d_sdl_get_width(d) - w) / 2;
    int y = (render_3d_sdl_get_height(d) - h) / 2;
    uint32_t bg = render_3d_sdl_color(0, 0, 0, 200);
    for(int yy = 0; yy < h; yy++) for(int xx = 0; xx < w; xx++) render_3d_sdl_blend_pixel(d, x + xx, y + yy, bg);
    int ly = y + 6;
    render_3d_draw_text_centered(d, ly, "CONGRATULATIONS!", render_3d_sdl_color(255,255,0,255), 3);
    /* empty line gap */
    ly += 3 * 7 + 3 * 7;
    char buf[64]; snprintf(buf, sizeof(buf), "SCORE: %d", score);
    render_3d_draw_text_centered(d, ly, buf, render_3d_sdl_color(200,200,255,255), 2);
    /* empty line gap */
    ly += 2 * 7 + 2 * 7;
    if(name_entered && name_entered[0]) {
        char nb[32]; snprintf(nb, sizeof(nb), "NAME: %.*s", 8, name_entered);
        render_3d_draw_text_centered(d, ly, nb, render_3d_sdl_color(160,255,160,255), 2);
    } else {
        render_3d_draw_text_centered(d, ly, "ENTER A SHORT NAME", render_3d_sdl_color(160,255,160,255), 2);
    }
    /* Present the overlay now */
    (void)render_3d_sdl_present(d);
}

int render_3d_compute_minimap_cell_px(int d_w, int d_h, int m_w, int m_h) {
    if(d_w <= 0 || d_h <= 0 || m_w <= 0 || m_h <= 0) return 0;
    int max_dim = m_w > m_h ? m_w : m_h;
    int smaller = d_w < d_h ? d_w : d_h;
    /* target minimap occupies ~40% of the smaller display dimension */
    int target = (smaller * 40) / 100;
    if(target < 160) target = 160; /* ensure reasonable visibility for large maps */
    int cell_px = target / max_dim;
    if(cell_px < 2) cell_px = 2; /* prefer at least 2px per cell for readability */
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
fprintf(stderr, "render_3d_init: requested size %dx%d\n", g_render_3d.config.screen_width, g_render_3d.config.screen_height);
g_render_3d.display= render_3d_sdl_create(g_render_3d.config.screen_width, g_render_3d.config.screen_height);
if(!g_render_3d.display) return false;
g_render_3d.camera= camera_create(g_render_3d.config.fov_degrees, g_render_3d.config.screen_width, 0.5f);
if(!g_render_3d.camera) return false;
g_render_3d.raycaster= raycaster_create(game_state->width, game_state->height, NULL);
g_render_3d.projector= projection_create(g_render_3d.config.screen_width, g_render_3d.config.screen_height, g_render_3d.config.fov_degrees * 3.14159265359f / 180.0f, g_render_3d.config.wall_height_scale);
g_render_3d.texture= texture_create();
g_render_3d.wall_texture= texture_create();
g_render_3d.floor_texture= texture_create();
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
g_render_3d.sprite_renderer= sprite_create(100, g_render_3d.camera, g_render_3d.projector);
g_render_3d.initialized= true;
return true;
}
void render_3d_draw(const GameState* game_state, const char* player_name, const void* scores, int score_count, float delta_seconds) {
(void)player_name;
(void)scores;
(void)score_count;
if(!g_render_3d.initialized || !game_state) return;
g_render_3d.game_state= game_state;
camera_update_interpolation(g_render_3d.camera, delta_seconds);
uint32_t sky_color= 0xFF87CEEB;
render_3d_sdl_clear(g_render_3d.display, sky_color);
uint32_t floor_color= 0xFF8B4513;
uint32_t ceiling_color= 0xFF4169E1;
int horizon= render_3d_sdl_get_height(g_render_3d.display) / 2;
const char* dbg= getenv("SNAKE_DEBUG_TEXTURES");
if(dbg && dbg[0] == '1') {
const uint32_t* wp= texture_get_pixels(g_render_3d.wall_texture);
if(wp) {
for(int yy= 0; yy < 16; yy++) {
for(int xx= 0; xx < 16; xx++) {
uint32_t c= texture_sample(g_render_3d.wall_texture, (float)xx / 16.0f, (float)yy / 16.0f, true);
if(8 + xx >= 0 && 8 + xx < render_3d_sdl_get_width(g_render_3d.display) && 8 + yy >= 0 && 8 + yy < render_3d_sdl_get_height(g_render_3d.display)) render_3d_sdl_get_pixels(g_render_3d.display)[(8 + yy) * render_3d_sdl_get_width(g_render_3d.display) + (8 + xx)]= c;
}
}
}
const uint32_t* fp= texture_get_pixels(g_render_3d.floor_texture);
if(fp) {
for(int yy= 0; yy < 16; yy++) {
for(int xx= 0; xx < 16; xx++) {
uint32_t c= texture_sample(g_render_3d.floor_texture, (float)xx / 16.0f, (float)yy / 16.0f, true);
if(8 + xx >= 0 && 8 + xx < render_3d_sdl_get_width(g_render_3d.display) && 28 + yy >= 0 && 28 + yy < render_3d_sdl_get_height(g_render_3d.display)) render_3d_sdl_get_pixels(g_render_3d.display)[(28 + yy) * render_3d_sdl_get_width(g_render_3d.display) + (8 + xx)]= c;
}
}
}
fprintf(stderr, "render_3d: debug texture overlay drawn\n");
}
float interp_cam_x, interp_cam_y;
camera_get_interpolated_position(g_render_3d.camera, &interp_cam_x, &interp_cam_y);
float interp_cam_angle= camera_get_interpolated_angle(g_render_3d.camera);
for(int x= 0; x < render_3d_sdl_get_width(g_render_3d.display); x++) {
float ray_angle;
camera_get_ray_angle(g_render_3d.camera, x, &ray_angle);
RayHit hit;
float cos_a= cosf(ray_angle);
float sin_a= sinf(ray_angle);
float eps_fwd= 0.0002f;
float eps_perp= 0.0002f;
float origin_x= interp_cam_x + cos_a * eps_fwd - sin_a * eps_perp;
float origin_y= interp_cam_y + sin_a * eps_fwd + cos_a * eps_perp;
if(raycast_cast_ray(g_render_3d.raycaster, origin_x, origin_y, ray_angle, &hit)) {
WallProjection proj;
if(hit.distance < 0.5f) {
if(g_render_3d.config.show_sprite_debug) {
fprintf(stderr,
    "[ray-debug] col=%d dist=%.4f hit=(%.3f,%.3f) "
    "vert=%d\n",
    x, hit.distance, hit.hit_x, hit.hit_y, hit.is_vertical);
}
}
projection_project_wall_perp(g_render_3d.projector, hit.distance, ray_angle, interp_cam_angle, &proj);
render_3d_sdl_draw_column(g_render_3d.display, x, 0, proj.draw_start - 1, ceiling_color);
Texel texel;
float pd= hit.distance * cosf(ray_angle - interp_cam_angle);
if(pd <= 0.1f) pd= 0.1f;
if(g_render_3d.column_depths) g_render_3d.column_depths[x]= pd;
float tex_coord= raycast_get_texture_coord(&hit, hit.is_vertical) * (float)TEXTURE_SCALE;
int wall_h= proj.draw_end - proj.draw_start + 1;
if(wall_h <= 0) wall_h= 1;
uint32_t* pix= render_3d_sdl_get_pixels(g_render_3d.display);
int w= render_3d_sdl_get_width(g_render_3d.display);
int h= render_3d_sdl_get_height(g_render_3d.display);
for(int yy= proj.draw_start; yy <= proj.draw_end; yy++) {
float v= (float)(yy - proj.draw_start) / (float)wall_h;
float tex_v= v * (float)TEXTURE_SCALE;
uint32_t col= 0;
if(texture_has_image(g_render_3d.wall_texture)) {
col= texture_sample(g_render_3d.wall_texture, tex_coord, tex_v, true);
} else {
texture_get_texel(g_render_3d.texture, pd, hit.is_vertical, tex_coord, &texel);
col= texel.color;
}
if(pix && x >= 0 && x < w && yy >= 0 && yy < h) pix[yy * w + x]= col;
}
render_3d_sdl_draw_column(g_render_3d.display, x, proj.draw_end, render_3d_sdl_get_height(g_render_3d.display) - 1, floor_color);
} else {
if(g_render_3d.column_depths) g_render_3d.column_depths[x]= INFINITY;
render_3d_sdl_draw_column(g_render_3d.display, x, 0, horizon - 1, ceiling_color);
render_3d_sdl_draw_column(g_render_3d.display, x, horizon, render_3d_sdl_get_height(g_render_3d.display) - 1, floor_color);
}
}
sprite_clear(g_render_3d.sprite_renderer);
for(int i= 0; i < game_state->food_count; i++) { sprite_add(g_render_3d.sprite_renderer, (float)game_state->food[i].x + 0.5f, (float)game_state->food[i].y + 0.5f, 0.25f, 0.0f, true, (int)i, 0); }
for(int p= 0; p < game_state->num_players; p++) {
if(p == g_render_3d.config.active_player) continue;
const PlayerState* player= &game_state->players[p];
if(!player->active || player->length == 0) continue;
float t= camera_get_interpolation_fraction(g_render_3d.camera);
float head_x= player->prev_head_x + (((float)player->body[0].x + 0.5f) - player->prev_head_x) * t;
float head_y= player->prev_head_y + (((float)player->body[0].y + 0.5f) - player->prev_head_y) * t;
sprite_add(g_render_3d.sprite_renderer, head_x, head_y, 1.0f, 0.0f, true, (int)p, 0);
}
for(int p= 0; p < game_state->num_players; p++) {
const PlayerState* player= &game_state->players[p];
if(!player->active || player->length <= 1) continue;
for(int bi= 1; bi < player->length; bi++) {
float seg_x= (float)player->body[bi].x + 0.5f;
float seg_y= (float)player->body[bi].y + 0.5f;
float tail_h= g_render_3d.config.tail_height_scale;
uint32_t gray= render_3d_sdl_color(128, 128, 128, 255);
sprite_add_color(g_render_3d.sprite_renderer, seg_x, seg_y, tail_h, 0.0f, true, -1, 0, gray);
}
}
{
const char* dbg_tail= getenv("SNAKE_DEBUG_TAIL");
if(dbg_tail && dbg_tail[0] == '1') { fprintf(stderr, "[tail-dbg] sprite_count=%d\n", sprite_get_count(g_render_3d.sprite_renderer)); }
}
sprite_project_all(g_render_3d.sprite_renderer);
sprite_sort_by_depth(g_render_3d.sprite_renderer);
sprite_draw(g_render_3d.sprite_renderer, g_render_3d.display, g_render_3d.column_depths);
render_3d_draw_minimap(&g_render_3d);
if(!render_3d_sdl_present(g_render_3d.display)) {}
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
