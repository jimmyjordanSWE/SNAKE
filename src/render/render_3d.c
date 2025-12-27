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
#define MINIMAP_TARGET_OCCUPANCY_PERCENT 50
#define MINIMAP_MIN_TARGET_SIZE 160
#define MINIMAP_MIN_CELL_PIXELS 3
#define MINIMAP_PADDING 8
typedef enum {
    RENDER_MODE_2D = 0,
    RENDER_MODE_3D,
    RENDER_MODE_COUNT
} RenderMode;
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
} Render3DContext;
static Render3DContext g_render_3d = {0};
static void render_3d_draw_char(SDL3DContext* disp, int x, int y, char c, uint32_t col, int scale);
static void render_3d_log(const char* fmt, ...) {
    char buf[512];
    time_t t = time(NULL);
    struct tm tm;
    if (localtime_r(&t, &tm) == NULL)
        tm = (struct tm){0};
    int n = snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d] ", 1900 + tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
                     tm.tm_hour, tm.tm_min, tm.tm_sec);
    va_list ap;
    va_start(ap, fmt);
    size_t rem;
    if (n < 0 || n >= (int)sizeof(buf)) {
        rem = 0;
        n = (int)sizeof(buf) - 1;
    } else {
        rem = sizeof(buf) - (size_t)n;
    }
    vsnprintf(buf + (n > 0 ? n : 0), rem, fmt, ap);
    va_end(ap);
    FILE* f = fopen("build/render_debug.log", "a");
    if (!f)
        f = fopen("render_debug.log", "a");
    if (!f)
        return;
    fprintf(f, "%s", buf);
    fclose(f);
}
static void render_3d_draw_minimap(Render3DContext* r, float interp_t) {
    if (!r || !r->game_state) {
        render_3d_log("minimap: context or game_state NULL\n");
        return;
    }
    const GameState* gs = r->game_state;
    int map_w = gs->width;
    int map_h = gs->height;
    if (map_w <= 0 || map_h <= 0) {
        render_3d_log("minimap: map size invalid %dx%d\n", map_w, map_h);
        return;
    }
    int cell_px = render_3d_compute_minimap_cell_px(render_3d_sdl_get_width(r->display),
                                                    render_3d_sdl_get_height(r->display), map_w, map_h);
    int map_px_w = cell_px * map_w;
    int map_px_h = cell_px * map_h;
    int padding = MINIMAP_PADDING;
    int x0 = render_3d_sdl_get_width(r->display) - padding - map_px_w;
    int y0 = render_3d_sdl_get_height(r->display) - padding - map_px_h;
    if (x0 < padding)
        x0 = padding;
    if (y0 < padding)
        y0 = padding;
    /* debug: early getenv removed to avoid unused-variable warning; use env_bool() for checks */
    if (env_bool("SNAKE_DEBUG_MINIMAP", 0)) {
        render_3d_log("minimap: called gs=%p map=%dx%d display=%dx%d x0=%d "
                      "y0=%d cell_px=%d\n",
                      (void*)gs, map_w, map_h, render_3d_sdl_get_width(r->display),
                      render_3d_sdl_get_height(r->display), x0, y0, cell_px);
    }
    uint32_t bg = render_3d_sdl_color(0, 0, 0, 200);
    render_3d_sdl_draw_filled_rect(r->display, x0, y0, map_px_w, map_px_h, bg);
    uint32_t border = render_3d_sdl_color(255, 255, 255, 255);
    for (int xx = 0; xx < map_px_w; xx++) {
        render_3d_sdl_set_pixel(r->display, x0 + xx, y0, border);
        render_3d_sdl_set_pixel(r->display, x0 + xx, y0 + map_px_h - 1, border);
    }
    for (int yy = 0; yy < map_px_h; yy++) {
        render_3d_sdl_set_pixel(r->display, x0, y0 + yy, border);
        render_3d_sdl_set_pixel(r->display, x0 + map_px_w - 1, y0 + yy, border);
    }
    if (env_bool("SNAKE_DEBUG_MINIMAP", 0)) {
        uint32_t dbgcol = render_3d_sdl_color(255, 0, 255, 255);
        for (int xx = 0; xx < map_px_w; xx++) {
            render_3d_sdl_blend_pixel(r->display, x0 + xx, y0, dbgcol);
            render_3d_sdl_blend_pixel(r->display, x0 + xx, y0 + map_px_h - 1, dbgcol);
        }
        for (int yy = 0; yy < map_px_h; yy++) {
            render_3d_sdl_blend_pixel(r->display, x0, y0 + yy, dbgcol);
            render_3d_sdl_blend_pixel(r->display, x0 + map_px_w - 1, y0 + yy, dbgcol);
        }
        render_3d_log("minimap: debug border drawn at %d,%d size %dx%d (display %dx%d)\n", x0, y0, map_px_w, map_px_h,
                      render_3d_sdl_get_width(r->display), render_3d_sdl_get_height(r->display));
    }
    uint32_t food_col = render_3d_sdl_color(255, 64, 64, 255);
    for (int i = 0; i < gs->food_count; i++) {
        int fx = x0 + gs->food[i].x * cell_px + cell_px / 2;
        int fy = y0 + gs->food[i].y * cell_px + cell_px / 2;
        int radius = cell_px > 2 ? (cell_px / 3) : 1;
        render_3d_sdl_draw_filled_circle(r->display, fx, fy, radius, food_col);
    }
    uint32_t player_cols[3] = {render_3d_sdl_color(0, 128, 0, 255), render_3d_sdl_color(0, 200, 255, 255),
                               render_3d_sdl_color(255, 255, 0, 255)};
    uint32_t tail_col = render_3d_sdl_color(0, 128, 0, 255);
    for (int p = 0; p < gs->num_players; p++) {
        const PlayerState* pl = &gs->players[p];
        if (!pl->active || pl->length <= 0)
            continue;
        for (int bi = 1; bi < pl->length; bi++) {
            float seg_x_f, seg_y_f;
            if (pl->prev_segment_x && pl->prev_segment_y) {
                seg_x_f = pl->prev_segment_x[bi] + (((float)pl->body[bi].x + 0.5f) - pl->prev_segment_x[bi]) * interp_t;
                seg_y_f = pl->prev_segment_y[bi] + (((float)pl->body[bi].y + 0.5f) - pl->prev_segment_y[bi]) * interp_t;
            } else {
                seg_x_f = (float)pl->body[bi].x + 0.5f;
                seg_y_f = (float)pl->body[bi].y + 0.5f;
            }
            int tx = x0 + (int)(seg_x_f * (float)cell_px + 0.5f);
            int ty = y0 + (int)(seg_y_f * (float)cell_px + 0.5f);
            int bw = cell_px > 2 ? (cell_px * 3 / 4) : 1;
            int radius = bw / 2;
            if (radius <= 0)
                radius = 1;
            render_3d_sdl_draw_filled_circle(r->display, tx, ty, radius, tail_col);
            (void)bi;
        }
        float head_x = pl->prev_head_x + (((float)pl->body[0].x + 0.5f) - pl->prev_head_x) * interp_t;
        float head_y = pl->prev_head_y + (((float)pl->body[0].y + 0.5f) - pl->prev_head_y) * interp_t;
        int hx = x0 + (int)(head_x * (float)cell_px + 0.5f);
        int hy = y0 + (int)(head_y * (float)cell_px + 0.5f);
        int hr = cell_px > 2 ? (cell_px / 2) : 1;
        uint32_t pcol = player_cols[p % (int)(sizeof(player_cols) / sizeof(player_cols[0]))];
        render_3d_sdl_draw_filled_circle(r->display, hx, hy, hr, pcol);
        int dir_off_x = 0, dir_off_y = 0;
        int off = (cell_px / 2) + 1;
        switch (pl->current_dir) {
        case SNAKE_DIR_UP:
            dir_off_y = -off;
            break;
        case SNAKE_DIR_DOWN:
            dir_off_y = off;
            break;
        case SNAKE_DIR_LEFT:
            dir_off_x = -off;
            break;
        case SNAKE_DIR_RIGHT:
            dir_off_x = off;
            break;
        default:
            break;
        }
        render_3d_sdl_draw_filled_circle(r->display, hx + dir_off_x, hy + dir_off_y, hr > 1 ? hr / 2 : 1, pcol);
    }
}
void render_3d_draw_minimap_into(struct SDL3DContext* ctx, const GameState* gs) {
    Render3DContext tmp = {0};
    tmp.display = ctx;
    tmp.game_state = gs;
    render_3d_draw_minimap(&tmp, 0.0f);
}
static void render_3d_update_fps(float delta_seconds) {
    if (delta_seconds <= 0.0f)
        return;
    g_render_3d.frame_times[g_render_3d.frame_time_idx] = delta_seconds;
    g_render_3d.frame_time_idx = (g_render_3d.frame_time_idx + 1) % 60;
    float avg_time = 0.0f;
    for (int i = 0; i < 60; i++) {
        avg_time += g_render_3d.frame_times[i];
    }
    avg_time /= 60.0f;
    g_render_3d.current_fps = (avg_time > 0.0f) ? (1.0f / avg_time) : 0.0f;
}
static void render_3d_draw_fps_counter(void) {
    if (!g_render_3d.display)
        return;
    char fps_buf[32];
    snprintf(fps_buf, sizeof(fps_buf), "FPS: %.1f", g_render_3d.current_fps);
    uint32_t fps_color = render_3d_sdl_color(0, 255, 0, 255);
    render_3d_draw_char(g_render_3d.display, 4, 4, 'F', fps_color, 1);
    render_3d_draw_char(g_render_3d.display, 10, 4, 'P', fps_color, 1);
    render_3d_draw_char(g_render_3d.display, 16, 4, 'S', fps_color, 1);
    render_3d_draw_char(g_render_3d.display, 22, 4, ':', fps_color, 1);
    int x = 28;
    int int_part = (int)g_render_3d.current_fps;
    int frac_part = (int)((g_render_3d.current_fps - (float)int_part) * 10.0f);
    if (int_part >= 100) {
        render_3d_draw_char(g_render_3d.display, x, 4, (char)('0' + (int_part / 100)), fps_color, 1);
        x += 6;
        int_part %= 100;
    }
    if (int_part >= 10 || (int)g_render_3d.current_fps >= 10) {
        render_3d_draw_char(g_render_3d.display, x, 4, (char)('0' + (int_part / 10)), fps_color, 1);
        x += 6;
    }
    render_3d_draw_char(g_render_3d.display, x, 4, (char)('0' + (int_part % 10)), fps_color, 1);
    x += 6;
    render_3d_draw_char(g_render_3d.display, x, 4, '.', fps_color, 1);
    x += 6;
    render_3d_draw_char(g_render_3d.display, x, 4, (char)('0' + frac_part), fps_color, 1);
}
static const uint8_t font5x7_A_Z[][7] = {
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x0A, 0x11}, {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    {0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04}, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},
};
static const uint8_t font5x7_digits[][7] = {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
};
static const uint8_t font5x7_exclaim[7] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04};
static void render_3d_draw_char(SDL3DContext* disp, int x, int y, char c, uint32_t col, int scale) {
    if (!disp)
        return;
    if (c == ' ')
        return;
    const uint8_t* glyph = NULL;
    if (c >= 'A' && c <= 'Z') {
        glyph = font5x7_A_Z[c - 'A'];
    } else if (c >= '0' && c <= '9') {
        glyph = font5x7_digits[c - '0'];
    } else if (c == '!') {
        glyph = font5x7_exclaim;
    } else {
        return;
    }
    for (int row = 0; row < 7; row++) {
        uint8_t bits = glyph[row];
        for (int colbit = 0; colbit < 5; colbit++) {
            if (bits & (1 << (4 - colbit))) {
                int px = x + colbit * scale;
                int py = y + row * scale;
                for (int yy = 0; yy < scale; yy++)
                    for (int xx = 0; xx < scale; xx++)
                        render_3d_sdl_blend_pixel(disp, px + xx, py + yy, col);
            }
        }
    }
}
static void render_3d_draw_text_centered(SDL3DContext* disp, int y, const char* text, uint32_t col, int scale) {
    if (!disp || !text)
        return;
    int char_w = 5 * scale;
    int spacing = scale * 2;
    int text_px_w = (int)strlen(text) * (char_w + spacing) - spacing;
    int x = (render_3d_sdl_get_width(disp) - text_px_w) / 2;
    for (const char* p = text; *p; ++p) {
        char c = (char)toupper((unsigned char)*p);
        render_3d_draw_char(disp, x, y, c, col, scale);
        x += char_w + spacing;
    }
}
void render_3d_draw_death_overlay(const GameState* game, int anim_frame, bool show_prompt) {
    (void)anim_frame;
    (void)game;
    if (!g_render_3d.initialized || !g_render_3d.display)
        return;
    SDL3DContext* d = g_render_3d.display;
    int w = render_3d_sdl_get_width(d) / 2;
    int h = render_3d_sdl_get_height(d) / 4;
    int x = (render_3d_sdl_get_width(d) - w) / 2;
    int y = (render_3d_sdl_get_height(d) - h) / 2;
    uint32_t bg = render_3d_sdl_color(0, 0, 0, 200);
    render_3d_sdl_draw_filled_rect(d, x, y, w, h, bg);
    int ly = y + 8;
    render_3d_draw_text_centered(d, ly, "YOU DIED", render_3d_sdl_color(255, 255, 255, 255), 4);
    ly += 4 * 7 + 4 * 7;
    if (show_prompt) {
        render_3d_draw_text_centered(d, ly, "PRESS ANY KEY TO RESTART", render_3d_sdl_color(160, 255, 160, 255), 2);
        ly += 2 * 7 + 2 * 7;
        render_3d_draw_text_centered(d, ly, "OR Q TO QUIT", render_3d_sdl_color(160, 255, 160, 255), 2);
    }
    (void)render_3d_sdl_present(d);
}
void render_3d_draw_congrats_overlay(int score, const char* name_entered) {
    if (!g_render_3d.initialized || !g_render_3d.display)
        return;
    SDL3DContext* d = g_render_3d.display;
    int w = render_3d_sdl_get_width(d) * 2 / 3;
    int h = render_3d_sdl_get_height(d) / 3;
    int x = (render_3d_sdl_get_width(d) - w) / 2;
    int y = (render_3d_sdl_get_height(d) - h) / 2;
    uint32_t bg = render_3d_sdl_color(0, 0, 0, 200);
    render_3d_sdl_draw_filled_rect(d, x, y, w, h, bg);
    int ly = y + 6;
    render_3d_draw_text_centered(d, ly, "CONGRATULATIONS!", render_3d_sdl_color(255, 255, 0, 255), 3);
    ly += 3 * 7 + 3 * 7;
    char buf[64];
    snprintf(buf, sizeof(buf), "SCORE: %d", score);
    render_3d_draw_text_centered(d, ly, buf, render_3d_sdl_color(200, 200, 255, 255), 2);
    ly += 2 * 7 + 2 * 7;
    if (name_entered && name_entered[0]) {
        char nb[32];
        snprintf(nb, sizeof(nb), "NAME: %.*s", 8, name_entered);
        render_3d_draw_text_centered(d, ly, nb, render_3d_sdl_color(160, 255, 160, 255), 2);
    } else {
        render_3d_draw_text_centered(d, ly, "ENTER A SHORT NAME", render_3d_sdl_color(160, 255, 160, 255), 2);
    }
    (void)render_3d_sdl_present(d);
}
int render_3d_compute_minimap_cell_px(int d_w, int d_h, int m_w, int m_h) {
    if (d_w <= 0 || d_h <= 0 || m_w <= 0 || m_h <= 0)
        return 0;
    int max_dim = m_w > m_h ? m_w : m_h;
    int smaller = d_w < d_h ? d_w : d_h;
    int target = (smaller * MINIMAP_TARGET_OCCUPANCY_PERCENT) / 100;
    if (target < MINIMAP_MIN_TARGET_SIZE)
        target = MINIMAP_MIN_TARGET_SIZE;
    int cell_px = target / max_dim;
    if (cell_px < MINIMAP_MIN_CELL_PIXELS)
        cell_px = MINIMAP_MIN_CELL_PIXELS;
    return cell_px;
}
bool render_3d_init(const GameState* game_state, const Render3DConfig* config) {
    if (g_render_3d.initialized)
        return true;
    if (!game_state)
        return false;
    g_render_3d.game_state = game_state;
    if (config) {
        g_render_3d.config = *config;
    } else {
        g_render_3d.config.active_player = 0;
        g_render_3d.config.fov_degrees = 90.0f;
        g_render_3d.config.show_sprite_debug = false;
        g_render_3d.config.screen_width = 800;
        g_render_3d.config.screen_height = 600;
        g_render_3d.config.wall_height_scale = (float)PERSIST_CONFIG_DEFAULT_WALL_SCALE;
        g_render_3d.config.tail_height_scale = (float)PERSIST_CONFIG_DEFAULT_TAIL_SCALE;
        g_render_3d.config.wall_texture_path[0] = '\0';
        g_render_3d.config.floor_texture_path[0] = '\0';
    }
    g_render_3d.display = render_3d_sdl_create(g_render_3d.config.screen_width, g_render_3d.config.screen_height);
    if (!g_render_3d.display)
        return false;
    g_render_3d.camera = camera_create(g_render_3d.config.fov_degrees, g_render_3d.config.screen_width, 0.5f);
    if (!g_render_3d.camera)
        return false;
    g_render_3d.raycaster = raycaster_create(game_state->width, game_state->height, NULL);
    if (!g_render_3d.raycaster)
        return false;
    g_render_3d.projector = projection_create(g_render_3d.config.screen_width, g_render_3d.config.screen_height,
                                              g_render_3d.config.fov_degrees * 3.14159265359f / 180.0f,
                                              g_render_3d.config.wall_height_scale);
    if (!g_render_3d.projector)
        return false;
    g_render_3d.texture = texture_create();
    g_render_3d.wall_texture = texture_create();
    g_render_3d.floor_texture = texture_create();
    if (!g_render_3d.texture || !g_render_3d.wall_texture || !g_render_3d.floor_texture)
        return false;
    if (g_render_3d.config.wall_texture_path[0]) {
        if (!texture_load_from_file(g_render_3d.wall_texture, g_render_3d.config.wall_texture_path)) {
            fprintf(stderr,
                    "render_3d_init: failed to load %s (using procedural "
                    "fallback)\n",
                    g_render_3d.config.wall_texture_path);
        }
    } else {
        if (!texture_load_from_file(g_render_3d.wall_texture, PERSIST_CONFIG_DEFAULT_WALL_TEXTURE)) {
            fprintf(stderr,
                    "render_3d_init: failed to load %s (using procedural "
                    "fallback)\n",
                    PERSIST_CONFIG_DEFAULT_WALL_TEXTURE);
        }
    }
    if (g_render_3d.config.floor_texture_path[0]) {
        if (!texture_load_from_file(g_render_3d.floor_texture, g_render_3d.config.floor_texture_path)) {
            fprintf(stderr, "render_3d_init: failed to load %s (using flat floor color)\n",
                    g_render_3d.config.floor_texture_path);
        }
    } else {
        if (!texture_load_from_file(g_render_3d.floor_texture, PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE)) {
            fprintf(stderr, "render_3d_init: failed to load %s (using flat floor color)\n",
                    PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE);
        }
    }
    g_render_3d.column_depths = calloc((size_t)render_3d_sdl_get_width(g_render_3d.display), sizeof(float));
    if (!g_render_3d.column_depths)
        return false;
    g_render_3d.sprite_renderer = sprite_create(100, g_render_3d.camera, g_render_3d.projector);
    if (!g_render_3d.sprite_renderer)
        return false;
    g_render_3d.initialized = true;
    return true;
}
void render_3d_draw(const GameState* game_state,
                    const char* player_name,
                    const void* scores,
                    int score_count,
                    float delta_seconds) {
    (void)player_name;
    (void)scores;
    (void)score_count;
    if (!g_render_3d.initialized || !game_state)
        return;
    g_render_3d.game_state = game_state;
#define MAX_DELTA_SECONDS 0.5f
    float clamped_delta = delta_seconds;
    if (clamped_delta > MAX_DELTA_SECONDS)
        clamped_delta = MAX_DELTA_SECONDS;
    camera_update_interpolation(g_render_3d.camera, clamped_delta);
    render_3d_update_fps(delta_seconds);
    float frame_interp_t = camera_get_interpolation_fraction(g_render_3d.camera);
    uint32_t sky_color = 0xFF87CEEB;
    render_3d_sdl_clear(g_render_3d.display, sky_color);
    uint32_t floor_color = 0xFF8B4513;
    uint32_t ceiling_color = 0xFF4169E1;
    const int screen_w = render_3d_sdl_get_width(g_render_3d.display);
    const int screen_h = render_3d_sdl_get_height(g_render_3d.display);
    uint32_t* pix = render_3d_sdl_get_pixels(g_render_3d.display);
    int horizon = screen_h / 2;
    const float inv_half_h = (screen_h > 0) ? (2.0f / (float)screen_h) : 0.0f;
    const bool has_floor_tex = texture_has_image(g_render_3d.floor_texture);
    const int map_w = g_render_3d.game_state->width;
    const int map_h = g_render_3d.game_state->height;
    static float* s_floor_row_dist = NULL;
    static int s_floor_row_dist_cap = 0;
    static bool s_floor_row_dist_valid = false;
    if (screen_h != s_floor_row_dist_cap || !s_floor_row_dist_valid) {
        float* new_buf = (float*)realloc(s_floor_row_dist, (size_t)screen_h * sizeof(float));
        if (new_buf) {
            s_floor_row_dist = new_buf;
            s_floor_row_dist_cap = screen_h;
            s_floor_row_dist_valid = true;
        } else {
            s_floor_row_dist_valid = false;
        }
    }
    if (s_floor_row_dist && s_floor_row_dist_valid) {
        float wall_scale = projection_get_wall_scale(g_render_3d.projector);
        for (int yy = 0; yy < screen_h; yy++) {
            if (yy <= horizon) {
                s_floor_row_dist[yy] = 0.0f;
                continue;
            }
            float p = (float)(yy - horizon) * inv_half_h;
            if (p < 1e-6f) {
                s_floor_row_dist[yy] = 0.0f;
                continue;
            }
            float rowDist = (wall_scale / p) - 0.5f;
            if (!isfinite(rowDist) || rowDist <= 0.0f) {
                s_floor_row_dist[yy] = 0.0f;
                continue;
            }
            s_floor_row_dist[yy] = rowDist;
        }
    }
    if (env_bool("SNAKE_DEBUG_TEXTURES", 0)) {
        const uint32_t* wp = texture_get_pixels(g_render_3d.wall_texture);
        if (wp) {
            for (int yy = 0; yy < 16; yy++) {
                for (int xx = 0; xx < 16; xx++) {
                    uint32_t c = texture_sample(g_render_3d.wall_texture, (float)xx / 16.0f, (float)yy / 16.0f, true);
                    if (8 + xx >= 0 && 8 + xx < render_3d_sdl_get_width(g_render_3d.display) && 8 + yy >= 0 &&
                        8 + yy < render_3d_sdl_get_height(g_render_3d.display))
                        render_3d_sdl_get_pixels(
                            g_render_3d.display)[(8 + yy) * render_3d_sdl_get_width(g_render_3d.display) + (8 + xx)] =
                            c;
                }
            }
        }
        const uint32_t* fp = texture_get_pixels(g_render_3d.floor_texture);
        if (fp) {
            for (int yy = 0; yy < 16; yy++) {
                for (int xx = 0; xx < 16; xx++) {
                    uint32_t c = texture_sample(g_render_3d.floor_texture, (float)xx / 16.0f, (float)yy / 16.0f, true);
                    if (8 + xx >= 0 && 8 + xx < render_3d_sdl_get_width(g_render_3d.display) && 28 + yy >= 0 &&
                        28 + yy < render_3d_sdl_get_height(g_render_3d.display))
                        render_3d_sdl_get_pixels(
                            g_render_3d.display)[(28 + yy) * render_3d_sdl_get_width(g_render_3d.display) + (8 + xx)] =
                            c;
                }
            }
        }
        fprintf(stderr, "render_3d: debug texture overlay drawn\n");
    }
    float interp_cam_x, interp_cam_y;
    camera_get_interpolated_position(g_render_3d.camera, &interp_cam_x, &interp_cam_y);
    float interp_cam_angle = camera_get_interpolated_angle(g_render_3d.camera);
    for (int x = 0; x < screen_w; x++) {
        float ray_angle;
        camera_get_ray_angle(g_render_3d.camera, x, &ray_angle);
        RayHit hit;
        float cos_a = cosf(ray_angle);
        float sin_a = sinf(ray_angle);
        float eps_fwd = 0.0002f;
        float eps_perp = 0.0002f;
        float origin_x = interp_cam_x + cos_a * eps_fwd - sin_a * eps_perp;
        float origin_y = interp_cam_y + sin_a * eps_fwd + cos_a * eps_perp;
        if (raycast_cast_ray(g_render_3d.raycaster, origin_x, origin_y, ray_angle, &hit)) {
            WallProjection proj;
            if (hit.distance < 0.5f) {
                if (g_render_3d.config.show_sprite_debug) {
                    fprintf(stderr,
                            "[ray-debug] col=%d dist=%.4f hit=(%.3f,%.3f) "
                            "vert=%d\n",
                            x, hit.distance, hit.hit_x, hit.hit_y, hit.is_vertical);
                }
            }
            projection_project_wall_perp(g_render_3d.projector, hit.distance, ray_angle, interp_cam_angle, &proj);
            render_3d_sdl_draw_column(g_render_3d.display, x, 0, proj.draw_start - 1, ceiling_color);
            Texel texel;
            float pd = hit.distance * cosf(ray_angle - interp_cam_angle);
            if (pd <= 0.1f)
                pd = 0.1f;
            if (g_render_3d.column_depths)
                g_render_3d.column_depths[x] = pd;
            float tex_coord = raycast_get_texture_coord(&hit, hit.is_vertical) * g_render_3d.config.wall_texture_scale;
            int wall_h = proj.draw_end - proj.draw_start + 1;
            if (wall_h <= 0)
                wall_h = 1;
            for (int yy = proj.draw_start; yy <= proj.draw_end; yy++) {
                float v = (float)(yy - proj.draw_start) / (float)wall_h;
                float tex_v = v * g_render_3d.config.wall_texture_scale;
                uint32_t col = 0;
                if (texture_has_image(g_render_3d.wall_texture)) {
                    col = texture_sample(g_render_3d.wall_texture, tex_coord, tex_v, true);
                } else {
                    texture_get_texel(g_render_3d.texture, pd, hit.is_vertical, tex_coord, &texel);
                    col = texel.color;
                }
                if (pix && yy >= 0 && yy < screen_h)
                    pix[yy * screen_w + x] = col;
            }
            int fh0 = proj.draw_end + 1;
            int fh1 = screen_h - 1;
            if (fh0 <= fh1) {
                if (has_floor_tex) {
                    for (int yy = fh0; yy <= fh1; yy++) {
                        float rowDist = s_floor_row_dist ? s_floor_row_dist[yy] : 0.0f;
                        if (rowDist <= 0.0f)
                            continue;
                        float angle_diff = ray_angle - interp_cam_angle;
                        float cos_angle_diff = cosf(angle_diff);
                        if (fabsf(cos_angle_diff) < 1e-6f)
                            continue;
                        float dist_along_ray = rowDist / cos_angle_diff;
                        float world_x = interp_cam_x + cos_a * dist_along_ray;
                        float world_y = interp_cam_y + sin_a * dist_along_ray;
                        if (world_x < 0.0f || world_x >= (float)map_w || world_y < 0.0f || world_y >= (float)map_h) {
                            if (pix)
                                pix[yy * screen_w + x] = floor_color;
                            continue;
                        }
                        float u = world_x * g_render_3d.config.floor_texture_scale;
                        float v = world_y * g_render_3d.config.floor_texture_scale;
                        uint32_t col = texture_sample(g_render_3d.floor_texture, u, v, false);
                        if (pix)
                            pix[yy * screen_w + x] = col;
                    }
                } else {
                    render_3d_sdl_draw_column(g_render_3d.display, x, fh0, fh1, floor_color);
                }
            }
        } else {
            if (g_render_3d.column_depths)
                g_render_3d.column_depths[x] = INFINITY;
            render_3d_sdl_draw_column(g_render_3d.display, x, 0, horizon - 1, ceiling_color);
            int fh0b = horizon;
            int fh1b = screen_h - 1;
            if (fh0b <= fh1b) {
                if (has_floor_tex) {
                    for (int yy = fh0b; yy <= fh1b; yy++) {
                        float rowDist = s_floor_row_dist ? s_floor_row_dist[yy] : 0.0f;
                        if (rowDist <= 0.0f)
                            continue;
                        float angle_diff = ray_angle - interp_cam_angle;
                        float cos_angle_diff = cosf(angle_diff);
                        if (fabsf(cos_angle_diff) < 1e-6f)
                            continue;
                        float dist_along_ray = rowDist / cos_angle_diff;
                        float world_x = interp_cam_x + cos_a * dist_along_ray;
                        float world_y = interp_cam_y + sin_a * dist_along_ray;
                        if (world_x < 0.0f || world_x >= (float)map_w || world_y < 0.0f || world_y >= (float)map_h) {
                            if (pix)
                                pix[yy * screen_w + x] = floor_color;
                            continue;
                        }
                        float u = world_x * g_render_3d.config.floor_texture_scale;
                        float v = world_y * g_render_3d.config.floor_texture_scale;
                        uint32_t col = texture_sample(g_render_3d.floor_texture, u, v, false);
                        if (pix)
                            pix[yy * screen_w + x] = col;
                    }
                } else {
                    render_3d_sdl_draw_column(g_render_3d.display, x, fh0b, fh1b, floor_color);
                }
            }
        }
    }
    sprite_clear(g_render_3d.sprite_renderer);
    for (int i = 0; i < game_state->food_count; i++) {
        uint32_t apple_col = render_3d_sdl_color(255, 0, 0, 255);
        sprite_add_color(g_render_3d.sprite_renderer, (float)game_state->food[i].x + 0.5f,
                         (float)game_state->food[i].y + 0.5f, 0.25f, 0.0f, true, -1, 0, apple_col);
    }
    for (int p = 0; p < game_state->num_players; p++) {
        if (p == g_render_3d.config.active_player)
            continue;
        const PlayerState* player = &game_state->players[p];
        if (!player->active || player->length == 0)
            continue;
        float head_x = player->prev_head_x + (((float)player->body[0].x + 0.5f) - player->prev_head_x) * frame_interp_t;
        float head_y = player->prev_head_y + (((float)player->body[0].y + 0.5f) - player->prev_head_y) * frame_interp_t;
        uint32_t player_cols[3] = {render_3d_sdl_color(0, 128, 0, 255), render_3d_sdl_color(0, 200, 255, 255),
                                   render_3d_sdl_color(255, 255, 0, 255)};
        uint32_t pcol = player_cols[p % (int)(sizeof(player_cols) / sizeof(player_cols[0]))];
        sprite_add_color(g_render_3d.sprite_renderer, head_x, head_y, 1.0f, 0.0f, true, -1, 0, pcol);
    }
    for (int p = 0; p < game_state->num_players; p++) {
        const PlayerState* player = &game_state->players[p];
        if (!player->active || player->length <= 1)
            continue;
        for (int bi = 1; bi < player->length; bi++) {
            float seg_x = (float)player->body[bi].x + 0.5f;
            float seg_y = (float)player->body[bi].y + 0.5f;
            if (player->prev_segment_x && player->prev_segment_y) {
                seg_x = player->prev_segment_x[bi] +
                        (((float)player->body[bi].x + 0.5f) - player->prev_segment_x[bi]) * frame_interp_t;
                seg_y = player->prev_segment_y[bi] +
                        (((float)player->body[bi].y + 0.5f) - player->prev_segment_y[bi]) * frame_interp_t;
            }
            float tail_h = g_render_3d.config.tail_height_scale;
            uint32_t body_col = render_3d_sdl_color(0, 128, 0, 255);
            sprite_add_color(g_render_3d.sprite_renderer, seg_x, seg_y, tail_h, 0.0f, true, -1, 0, body_col);
        }
    }
    {
        if (env_bool("SNAKE_DEBUG_TAIL", 0)) {
            fprintf(stderr, "[tail-dbg] sprite_count=%d\n", sprite_get_count(g_render_3d.sprite_renderer));
        }
    }
    sprite_project_all(g_render_3d.sprite_renderer);
    sprite_sort_by_depth(g_render_3d.sprite_renderer);
    sprite_draw(g_render_3d.sprite_renderer, g_render_3d.display, g_render_3d.column_depths);
    render_3d_draw_minimap(&g_render_3d, frame_interp_t);
    render_3d_draw_fps_counter();
    if (!render_3d_sdl_present(g_render_3d.display)) {
    }
}
void render_3d_set_active_player(int player_index) __attribute__((used));
void render_3d_set_active_player(int player_index) {
    if (g_render_3d.initialized)
        g_render_3d.config.active_player = player_index;
}
void render_3d_set_fov(float fov_degrees) __attribute__((used));
void render_3d_set_fov(float fov_degrees) {
    if (g_render_3d.initialized)
        g_render_3d.config.fov_degrees = fov_degrees;
}
struct SDL3DContext* render_3d_get_display(void) {
    return g_render_3d.display;
}
void render_3d_on_tick(const GameState* game_state) {
    if (!g_render_3d.initialized || !game_state)
        return;
    if (g_render_3d.config.active_player < game_state->num_players) {
        const PlayerState* player = &game_state->players[g_render_3d.config.active_player];
        if (player->length > 0)
            camera_set_from_player(g_render_3d.camera, player->body[0].x, player->body[0].y, player->current_dir);
    }
}
void render_3d_set_tick_rate_ms(int ms) {
    if (!g_render_3d.initialized)
        return;
    if (ms <= 0)
        ms = 1;
    camera_set_update_interval(g_render_3d.camera, (float)ms / 1000.0f);
    if (camera_get_interp_time(g_render_3d.camera) > camera_get_update_interval(g_render_3d.camera))
        camera_set_interpolation_time(g_render_3d.camera, camera_get_update_interval(g_render_3d.camera));
}
void render_3d_shutdown(void) {
    if (!g_render_3d.initialized)
        return;
    render_3d_sdl_destroy(g_render_3d.display);
    g_render_3d.display = NULL;
    if (g_render_3d.column_depths) {
        free(g_render_3d.column_depths);
        g_render_3d.column_depths = NULL;
    }
    sprite_destroy(g_render_3d.sprite_renderer);
    g_render_3d.sprite_renderer = NULL;
    if (g_render_3d.wall_texture) {
        texture_destroy(g_render_3d.wall_texture);
        g_render_3d.wall_texture = NULL;
    }
    if (g_render_3d.floor_texture) {
        texture_destroy(g_render_3d.floor_texture);
        g_render_3d.floor_texture = NULL;
    }
    if (g_render_3d.texture) {
        texture_destroy(g_render_3d.texture);
        g_render_3d.texture = NULL;
    }
    if (g_render_3d.projector) {
        projection_destroy(g_render_3d.projector);
        g_render_3d.projector = NULL;
    }
    if (g_render_3d.raycaster) {
        raycaster_destroy(g_render_3d.raycaster);
        g_render_3d.raycaster = NULL;
    }
    if (g_render_3d.camera) {
        camera_destroy(g_render_3d.camera);
        g_render_3d.camera = NULL;
    }
    g_render_3d.initialized = false;
    g_render_3d.game_state = NULL;
}
