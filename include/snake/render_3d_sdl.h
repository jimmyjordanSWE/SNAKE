#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int width;
    int height;
    uint32_t* pixels; /* ARGB8888 pixel data */
} SDL3DContext;

/**
 * Initialize SDL3D rendering context.
 *
 * @param width       Requested window width in pixels
 * @param height      Requested window height in pixels
 * @param ctx_out     Output context (must not be NULL)
 * @return            true on success, false on failure
 */
bool render_3d_sdl_init(int width, int height, SDL3DContext* ctx_out);

/**
 * Shutdown SDL3D rendering context and clean up resources.
 *
 * @param ctx Context to shutdown (must not be NULL)
 */
void render_3d_sdl_shutdown(SDL3DContext* ctx);

/**
 * Draw a vertical line of pixels in a single column.
 *
 * @param ctx          Context (must not be NULL)
 * @param x            X coordinate of the column
 * @param y_start      Starting Y coordinate (inclusive)
 * @param y_end        Ending Y coordinate (inclusive)
 * @param col          ARGB8888 color value
 */
void render_3d_sdl_draw_column(SDL3DContext* ctx, int x, int y_start, int y_end, uint32_t col);

/**
 * Clear the frame buffer to a specified color.
 *
 * @param ctx Context (must not be NULL)
 * @param col ARGB8888 color value to fill with
 */
void render_3d_sdl_clear(SDL3DContext* ctx, uint32_t col);

/**
 * Set a single pixel.
 *
 * @param ctx Context (must not be NULL)
 * @param x   X coordinate
 * @param y   Y coordinate
 * @param col ARGB8888 color value
 */
static inline void render_3d_sdl_set_pixel(SDL3DContext* ctx, int x, int y, uint32_t col) {
    if (!ctx || !ctx->pixels || x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) return;
    ctx->pixels[y * ctx->width + x] = col;
}

/**
 * Draw a filled circle.
 *
 * @param ctx          Context (must not be NULL)
 * @param center_x     X coordinate of circle center
 * @param center_y     Y coordinate of circle center
 * @param radius       Circle radius in pixels
 * @param col          ARGB8888 color value
 */
void render_3d_sdl_draw_filled_circle(SDL3DContext* ctx, int center_x, int center_y, int radius, uint32_t col);

/**
 * Present the current frame buffer to the screen and handle events.
 * Returns false if quit event received.
 *
 * @param ctx Context (must not be NULL)
 * @return    true to continue, false if quit requested
 */
bool render_3d_sdl_present(SDL3DContext* ctx);

/**
 * Helper: Create an ARGB8888 color from RGBA components.
 */
static inline uint32_t render_3d_sdl_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}
