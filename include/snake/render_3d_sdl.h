#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    int       width;
    int       height;
    uint32_t* pixels;
} SDL3DContext;
bool render_3d_sdl_init(int width, int height, SDL3DContext* ctx_out);
void render_3d_sdl_shutdown(SDL3DContext* ctx);
void render_3d_sdl_draw_column(
    SDL3DContext* ctx, int x, int y_start, int y_end, uint32_t col);
void render_3d_sdl_clear(SDL3DContext* ctx, uint32_t col);
static inline void
render_3d_sdl_set_pixel(SDL3DContext* ctx, int x, int y, uint32_t col)
{
    if (!ctx || !ctx->pixels || x < 0 || x >= ctx->width || y < 0
        || y >= ctx->height)
        return;
    ctx->pixels[y * ctx->width + x] = col;
}
void render_3d_sdl_draw_filled_circle(
    SDL3DContext* ctx, int center_x, int center_y, int radius, uint32_t col);
bool render_3d_sdl_present(SDL3DContext* ctx);
static inline uint32_t
render_3d_sdl_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8)
           | (uint32_t)b;
}
