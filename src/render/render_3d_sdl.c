#include "render_3d_sdl.h"
#include "game_internal.h"
#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
    bool initialized;
};
SDL3DContext* render_3d_sdl_create(int width, int height) {
    SDL3DContext* ctx = (SDL3DContext*)calloc(1, sizeof(*ctx));
    if (!ctx)
        return NULL;
    if (!render_3d_sdl_init(width, height, ctx)) {
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
bool render_3d_sdl_init(int width, int height, SDL3DContext* ctx_out) {
    if (!ctx_out || width <= 0 || height <= 0)
        return false;
    if (ctx_out->initialized)
        return true;
    if (width > (int)(SIZE_MAX / ((size_t)height * sizeof(uint32_t))))
        return false;
    LSAN_DISABLE();
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        ctx_out->pixels = (uint32_t*)malloc((size_t)width * (size_t)height * sizeof(uint32_t));
        if (ctx_out->pixels) {
            ctx_out->width = width;
            ctx_out->height = height;
            ctx_out->initialized = true;
            LSAN_ENABLE();
            return true;
        }
        LSAN_ENABLE();
        return false;
    }
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    ctx_out->window = SDL_CreateWindow("SNAKE 3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                                       SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALWAYS_ON_TOP);
    if (!ctx_out->window) {
        SDL_Quit();
        LSAN_ENABLE();
        return false;
    }
    SDL_ShowWindow(ctx_out->window);
    SDL_SetWindowAlwaysOnTop(ctx_out->window, SDL_TRUE);
    SDL_RaiseWindow(ctx_out->window);
    SDL_PumpEvents();
    ctx_out->renderer = SDL_CreateRenderer(ctx_out->window, -1, SDL_RENDERER_ACCELERATED);
    if (!ctx_out->renderer) {
        SDL_DestroyWindow(ctx_out->window);
        SDL_Quit();
        LSAN_ENABLE();
        return false;
    }
    SDL_SetRenderDrawColor(ctx_out->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx_out->renderer);
    SDL_RenderPresent(ctx_out->renderer);
    ctx_out->texture =
        SDL_CreateTexture(ctx_out->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!ctx_out->texture) {
        SDL_DestroyRenderer(ctx_out->renderer);
        SDL_DestroyWindow(ctx_out->window);
        SDL_Quit();
        LSAN_ENABLE();
        return false;
    }
    ctx_out->width = width;
    ctx_out->height = height;
    SDL_RaiseWindow(ctx_out->window);
    SDL_PumpEvents();
    ctx_out->pixels = (uint32_t*)malloc((size_t)width * (size_t)height * sizeof(uint32_t));
    if (!ctx_out->pixels) {
        SDL_DestroyTexture(ctx_out->texture);
        SDL_DestroyRenderer(ctx_out->renderer);
        SDL_DestroyWindow(ctx_out->window);
        SDL_Quit();
        LSAN_ENABLE();
        return false;
    }
    ctx_out->width = width;
    ctx_out->height = height;
    ctx_out->initialized = true;
    LSAN_ENABLE();
    return true;
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
void render_3d_sdl_blend_pixel(SDL3DContext* ctx, int x, int y, uint32_t src_col) {
    if (!ctx || !ctx->pixels || x < 0 || x >= ctx->width || y < 0 || y >= ctx->height)
        return;
    uint32_t dst = ctx->pixels[y * ctx->width + x];
    uint8_t sa = (uint8_t)((src_col >> 24) & 0xFFu);
    if (sa == 255) {
        ctx->pixels[y * ctx->width + x] = src_col;
        return;
    }
    if (sa == 0)
        return;
    uint8_t sr = (uint8_t)((src_col >> 16) & 0xFFu);
    uint8_t sg = (uint8_t)((src_col >> 8) & 0xFFu);
    uint8_t sb = (uint8_t)(src_col & 0xFFu);
    uint8_t dr = (uint8_t)((dst >> 16) & 0xFFu);
    uint8_t dg = (uint8_t)((dst >> 8) & 0xFFu);
    uint8_t db = (uint8_t)(dst & 0xFFu);
    float a = (float)sa / 255.0f;
    uint8_t out_r = (uint8_t)(sr * a + dr * (1.0f - a));
    uint8_t out_g = (uint8_t)(sg * a + dg * (1.0f - a));
    uint8_t out_b = (uint8_t)(sb * a + db * (1.0f - a));
    ctx->pixels[y * ctx->width + x] =
        (0xFFu << 24) | ((uint32_t)out_r << 16) | ((uint32_t)out_g << 8) | (uint32_t)out_b;
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
    SDL_UpdateTexture(ctx->texture, NULL, ctx->pixels, ctx->width * (int)sizeof(uint32_t));
    SDL_RenderClear(ctx->renderer);
    SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
    SDL_RenderPresent(ctx->renderer);
    return true;
}
