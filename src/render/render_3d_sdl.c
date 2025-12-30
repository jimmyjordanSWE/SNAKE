#include "render_3d_sdl.h"
#include "game_internal.h"
#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef void (*lsan_fn_t)(void);
static void call_lsan_disable(void) {
    static lsan_fn_t fn = (lsan_fn_t)(-1);
    if (fn == (lsan_fn_t)(-1)) {
        fn = (lsan_fn_t)(uintptr_t)dlsym(RTLD_DEFAULT, "__lsan_disable");
    }
    if (fn)
        fn();
}
static void call_lsan_enable(void) {
    static lsan_fn_t fn = (lsan_fn_t)(-1);
    if (fn == (lsan_fn_t)(-1)) {
        fn = (lsan_fn_t)(uintptr_t)dlsym(RTLD_DEFAULT, "__lsan_enable");
    }
    if (fn)
        fn();
}
#define LSAN_DISABLE() call_lsan_disable()
#define LSAN_ENABLE() call_lsan_enable()
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int width;
    int height;
} SDLState;
struct SDL3DContext {
    int width;
    int height;
    uint32_t* pixels;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    Uint32 texture_format;
    bool initialized;
};
SDL3DContext* render_3d_sdl_create(int width, int height, int vsync) {
    SDL3DContext* ctx = calloc(1, sizeof *ctx);
    if (!ctx)
        return NULL;
    if (!render_3d_sdl_init(width, height, vsync, ctx)) {
        free(ctx);
        return NULL;
    }
    return ctx;
}
void render_3d_sdl_destroy(SDL3DContext* ctx) {
    if (!ctx)
        return;
    render_3d_sdl_shutdown(ctx);
    free(ctx);
}
int render_3d_sdl_get_width(const SDL3DContext* ctx) {
    return ctx ? ctx->width : 0;
}
int render_3d_sdl_get_height(const SDL3DContext* ctx) {
    return ctx ? ctx->height : 0;
}
uint32_t* render_3d_sdl_get_pixels(SDL3DContext* ctx) {
    return ctx ? ctx->pixels : NULL;
}
bool render_3d_sdl_init(int width, int height, int vsync, SDL3DContext* ctx_out) {
    if (!ctx_out || width <= 0 || height <= 0)
        return false;
    if (ctx_out->initialized)
        return true;
    if (width > (int)(SIZE_MAX / ((size_t)height * sizeof(uint32_t))))
        return false;
    LSAN_DISABLE();
    int err = 0;
    int sdl_inited = 0;
    ctx_out->pixels = NULL;
    ctx_out->window = NULL;
    ctx_out->renderer = NULL;
    ctx_out->texture = NULL;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        /* SDL unavailable: fall back to a plain pixel buffer if possible */
        ctx_out->pixels = malloc((size_t)width * (size_t)height * sizeof *ctx_out->pixels);
        if (!ctx_out->pixels) {
            err = 1;
            goto out;
        }
        ctx_out->width = width;
        ctx_out->height = height;
        ctx_out->initialized = true;
        goto out;
    }
    sdl_inited = 1;
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    ctx_out->window = SDL_CreateWindow("SNAKE 3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                                       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALWAYS_ON_TOP);
    if (!ctx_out->window) {
        err = 2;
        goto out;
    }
    SDL_ShowWindow(ctx_out->window);
    SDL_SetWindowAlwaysOnTop(ctx_out->window, SDL_TRUE);
    SDL_RaiseWindow(ctx_out->window);
    SDL_PumpEvents();
    ctx_out->renderer = SDL_CreateRenderer(ctx_out->window, -1, SDL_RENDERER_ACCELERATED);
    if (!ctx_out->renderer) {
        err = 3;
        goto out;
    }
    SDL_SetRenderDrawColor(ctx_out->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx_out->renderer);
    SDL_RenderPresent(ctx_out->renderer);
    {
        SDL_RendererInfo info;
        SDL_GetRendererInfo(ctx_out->renderer, &info);
        Uint32 fmt = SDL_PIXELFORMAT_ARGB8888;
        if (info.num_texture_formats > 0)
            fmt = info.texture_formats[0];
        ctx_out->texture = SDL_CreateTexture(ctx_out->renderer, fmt, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (!ctx_out->texture) {
            err = 4;
            goto out;
        }
        ctx_out->texture_format = fmt;
    }
    ctx_out->pixels = malloc((size_t)width * (size_t)height * sizeof *ctx_out->pixels);
    if (!ctx_out->pixels) {
        err = 5;
        goto out;
    }
    ctx_out->width = width;
    ctx_out->height = height;
    ctx_out->initialized = true;
out:
    if (err) {
        if (ctx_out->pixels) {
            free(ctx_out->pixels);
            ctx_out->pixels = NULL;
        }
        if (ctx_out->texture) {
            SDL_DestroyTexture(ctx_out->texture);
            ctx_out->texture = NULL;
        }
        if (ctx_out->renderer) {
            SDL_DestroyRenderer(ctx_out->renderer);
            ctx_out->renderer = NULL;
        }
        if (ctx_out->window) {
            SDL_DestroyWindow(ctx_out->window);
            ctx_out->window = NULL;
        }
        if (sdl_inited)
            SDL_Quit();
        ctx_out->width = 0;
        ctx_out->height = 0;
        ctx_out->initialized = false;
    }
    LSAN_ENABLE();
    return ctx_out->initialized;
}
void render_3d_sdl_shutdown(SDL3DContext* ctx) {
    if (!ctx || !ctx->initialized)
        return;
    if (ctx->pixels) {
        free(ctx->pixels);
        ctx->pixels = NULL;
    }
    if (ctx->texture) {
        SDL_DestroyTexture(ctx->texture);
        ctx->texture = NULL;
        ctx->texture_format = 0;
    }
    if (ctx->renderer) {
        SDL_DestroyRenderer(ctx->renderer);
        ctx->renderer = NULL;
    }
    if (ctx->window) {
        SDL_DestroyWindow(ctx->window);
        ctx->window = NULL;
    }
    SDL_Quit();
    ctx->width = 0;
    ctx->height = 0;
    ctx->initialized = false;
}
void render_3d_sdl_set_pixel(SDL3DContext* ctx, int x, int y, uint32_t col) {
    if (!ctx || !ctx->pixels || x < 0 || x >= ctx->width || y < 0 || y >= ctx->height)
        return;
    ctx->pixels[y * ctx->width + x] = col;
}
static inline uint32_t blend_px(uint32_t dst, uint32_t src) {
    uint8_t sa = (uint8_t)((src >> 24) & 0xFFu);
    if (sa == 255)
        return src;
    if (sa == 0)
        return dst;
    uint8_t sr = (uint8_t)((src >> 16) & 0xFFu), sg = (uint8_t)((src >> 8) & 0xFFu), sb = (uint8_t)(src & 0xFFu);
    uint8_t dr = (uint8_t)((dst >> 16) & 0xFFu), dg = (uint8_t)((dst >> 8) & 0xFFu), db = (uint8_t)(dst & 0xFFu);
    int inv = 255 - sa;
    /* Fast approximation: (x + 128) >> 8 ≈ x / 255 */
    uint8_t rr = (uint8_t)(((sr * sa + dr * inv) + 128) >> 8);
    uint8_t rg = (uint8_t)(((sg * sa + dg * inv) + 128) >> 8);
    uint8_t rb = (uint8_t)(((sb * sa + db * inv) + 128) >> 8);
    return (0xFFu << 24) | ((uint32_t)rr << 16) | ((uint32_t)rg << 8) | (uint32_t)rb;
}
void render_3d_sdl_blend_pixel(SDL3DContext* ctx, int x, int y, uint32_t src_col) {
    if (!ctx || !ctx->pixels || x < 0 || x >= ctx->width || y < 0 || y >= ctx->height)
        return;
    uint32_t* p = &ctx->pixels[y * ctx->width + x];
    *p = blend_px(*p, src_col);
}
void render_3d_sdl_draw_column(SDL3DContext* ctx, int x, int y_start, int y_end, uint32_t col) {
    if (!ctx || !ctx->pixels || x < 0 || x >= ctx->width)
        return;
    if (y_start < 0)
        y_start = 0;
    if (y_end >= ctx->height)
        y_end = ctx->height - 1;
    if (y_start > y_end)
        return;
    uint32_t* dst = &ctx->pixels[y_start * ctx->width + x];
    int count = y_end - y_start + 1;
    for (int i = 0; i < count; i++) {
        *dst = col;
        dst += ctx->width;
    }
}
void render_3d_sdl_draw_filled_circle(SDL3DContext* ctx, int center_x, int center_y, int radius, uint32_t col) {
    if (!ctx || !ctx->pixels || radius <= 0)
        return;
    int x1 = center_x - radius;
    int x2 = center_x + radius;
    int y1 = center_y - radius;
    int y2 = center_y + radius;
    if (x1 < 0)
        x1 = 0;
    if (x2 >= ctx->width)
        x2 = ctx->width - 1;
    if (y1 < 0)
        y1 = 0;
    if (y2 >= ctx->height)
        y2 = ctx->height - 1;
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            int dx = x - center_x;
            int dy = y - center_y;
            if (dx * dx + dy * dy <= radius * radius)
                render_3d_sdl_blend_pixel(ctx, x, y, col);
        }
    }
}
void render_3d_sdl_draw_filled_rect(SDL3DContext* ctx, int x, int y, int w_rect, int h_rect, uint32_t col) {
    if (!ctx || !ctx->pixels || w_rect <= 0 || h_rect <= 0)
        return;
    int x0 = x;
    int y0 = y;
    int x1 = x + w_rect - 1;
    int y1 = y + h_rect - 1;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 >= ctx->width)
        x1 = ctx->width - 1;
    if (y1 >= ctx->height)
        y1 = ctx->height - 1;
    uint8_t alpha = (uint8_t)((col >> 24) & 0xFFu);
    if (alpha == 255) {
        for (int yy = y0; yy <= y1; ++yy) {
            uint32_t* row = &ctx->pixels[yy * ctx->width + x0];
            int count = x1 - x0 + 1;
            for (int i = 0; i < count; i++)
                row[i] = col;
        }
    } else if (alpha > 0) {
        for (int yy = y0; yy <= y1; ++yy) {
            for (int xx = x0; xx <= x1; ++xx) {
                render_3d_sdl_blend_pixel(ctx, xx, yy, col);
            }
        }
    }
}
void render_3d_sdl_clear(SDL3DContext* ctx, uint32_t col) {
    if (!ctx || !ctx->pixels)
        return;
    int total = ctx->width * ctx->height;
    for (int i = 0; i < total; i++)
        ctx->pixels[i] = col;
}
bool render_3d_sdl_present(SDL3DContext* ctx) {
    if (!ctx || !ctx->renderer || !ctx->texture)
        return false;
    SDL_PumpEvents();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            return false;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                return false;
            break;
        default:
            break;
        }
    }
    /* Try lock+write path first; fall back to SDL_UpdateTexture if needed. */
    void* tex_pixels = NULL;
    int tex_pitch = 0;
    if (SDL_LockTexture(ctx->texture, NULL, &tex_pixels, &tex_pitch) == 0) {
        if (ctx->texture_format == SDL_PIXELFORMAT_ARGB8888) {
            size_t row_bytes = (size_t)ctx->width * sizeof(uint32_t);
            /* Fast path: single memcpy when pitch matches */
            if (tex_pitch == (int)row_bytes) {
                memcpy(tex_pixels, ctx->pixels, row_bytes * (size_t)ctx->height);
            } else {
                uint8_t* dst = (uint8_t*)tex_pixels;
                uint8_t* src = (uint8_t*)ctx->pixels;
                for (int y = 0; y < ctx->height; ++y) {
                    memcpy(dst, src + (size_t)y * row_bytes, row_bytes);
                    dst += tex_pitch;
                }
            }
        } else {
            SDL_ConvertPixels(ctx->width, ctx->height, SDL_PIXELFORMAT_ARGB8888, ctx->pixels,
                              ctx->width * (int)sizeof(uint32_t), ctx->texture_format, tex_pixels, tex_pitch);
        }
        SDL_UnlockTexture(ctx->texture);
    } else {
        if (ctx->texture_format == SDL_PIXELFORMAT_ARGB8888) {
            SDL_UpdateTexture(ctx->texture, NULL, ctx->pixels, ctx->width * (int)sizeof(uint32_t));
        } else {
            size_t bufsize = (size_t)ctx->width * (size_t)ctx->height * 4;
            uint8_t* tmp = malloc(bufsize);
            if (tmp) {
                SDL_ConvertPixels(ctx->width, ctx->height, SDL_PIXELFORMAT_ARGB8888, ctx->pixels,
                                  ctx->width * (int)sizeof(uint32_t), ctx->texture_format, tmp, ctx->width * 4);
                SDL_UpdateTexture(ctx->texture, NULL, tmp, ctx->width * 4);
                free(tmp);
            } else {
                SDL_UpdateTexture(ctx->texture, NULL, ctx->pixels, ctx->width * (int)sizeof(uint32_t));
            }
        }
    }
    /* texture covers full target; no need to clear the renderer first */
    SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
    SDL_RenderPresent(ctx->renderer);
    return true;
}
