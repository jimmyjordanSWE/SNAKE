#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct SDL3DContext SDL3DContext;
SDL3DContext* render_3d_sdl_create(int width, int height);
void render_3d_sdl_destroy(SDL3DContext* ctx);
int render_3d_sdl_get_width(const SDL3DContext* ctx);
int render_3d_sdl_get_height(const SDL3DContext* ctx);
uint32_t* render_3d_sdl_get_pixels(SDL3DContext* ctx);
bool render_3d_sdl_init(int width, int height, SDL3DContext* ctx_out);
void render_3d_sdl_shutdown(SDL3DContext* ctx);
void render_3d_sdl_draw_column(SDL3DContext* ctx, int x, int y_start, int y_end, uint32_t col);
void render_3d_sdl_clear(SDL3DContext* ctx, uint32_t col);
void render_3d_sdl_set_pixel(SDL3DContext* ctx, int x, int y, uint32_t col);
void render_3d_sdl_blend_pixel(SDL3DContext* ctx, int x, int y, uint32_t src_col);
void render_3d_sdl_draw_filled_circle(SDL3DContext* ctx, int center_x, int center_y, int radius, uint32_t col);
void render_3d_sdl_draw_filled_rect(SDL3DContext* ctx, int x, int y, int w, int h, uint32_t col);
bool render_3d_sdl_present(SDL3DContext* ctx);
static inline uint32_t render_3d_sdl_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}
