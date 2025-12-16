#include "snake/render_3d_sdl.h"
#include "snake/game_internal.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

/* Optional LSAN guards to silence false-positive leaks from SDL / GLX
 * (only enabled when sanitizer/lsan_interface.h is available). */
#if defined(__has_include)
#  if __has_include(<sanitizer/lsan_interface.h>)
#    include <sanitizer/lsan_interface.h>
#    define LSAN_DISABLE() __lsan_disable()
#    define LSAN_ENABLE() __lsan_enable()
#  else
#    define LSAN_DISABLE() ((void)0)
#    define LSAN_ENABLE() ((void)0)
#  endif
#else
#  define LSAN_DISABLE() ((void)0)
#  define LSAN_ENABLE() ((void)0)
#endif
typedef struct {
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
int width;
int height;
} SDLState;
static SDLState g_sdl_state= {0};
struct SDL3DContext {
int width;
int height;
uint32_t* pixels;
};
SDL3DContext* render_3d_sdl_create(int width, int height) {
SDL3DContext* ctx= (SDL3DContext*)calloc(1, sizeof(*ctx));
if(!ctx) return NULL;
if(!render_3d_sdl_init(width, height, ctx)) {
free(ctx);
return NULL;
}
return ctx;
}
void render_3d_sdl_destroy(SDL3DContext* ctx) {
if(!ctx) return;
render_3d_sdl_shutdown(ctx);
free(ctx);
}
int render_3d_sdl_get_width(const SDL3DContext* ctx) { return ctx ? ctx->width : 0; }
int render_3d_sdl_get_height(const SDL3DContext* ctx) { return ctx ? ctx->height : 0; }
uint32_t* render_3d_sdl_get_pixels(SDL3DContext* ctx) { return ctx ? ctx->pixels : NULL; }
bool render_3d_sdl_init(int width, int height, SDL3DContext* ctx_out) {
if(!ctx_out || width <= 0 || height <= 0) return false;
/* Disable LeakSanitizer for the SDL/GL initialization sequence which
 * allocates internal resources in upstream libraries (libGLX / SDL). */
LSAN_DISABLE();
if(SDL_Init(SDL_INIT_VIDEO) < 0) { LSAN_ENABLE(); return false; }
SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
g_sdl_state.window= SDL_CreateWindow("SNAKE 3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
if(!g_sdl_state.window) {
SDL_Quit();
LSAN_ENABLE();
return false;
}
SDL_ShowWindow(g_sdl_state.window);
SDL_RaiseWindow(g_sdl_state.window);
SDL_PumpEvents();
g_sdl_state.renderer= SDL_CreateRenderer(g_sdl_state.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
if(!g_sdl_state.renderer) {
SDL_DestroyWindow(g_sdl_state.window);
SDL_Quit();
LSAN_ENABLE();
return false;
}
SDL_SetRenderDrawColor(g_sdl_state.renderer, 0, 0, 0, 255);
SDL_RenderClear(g_sdl_state.renderer);
SDL_RenderPresent(g_sdl_state.renderer);
g_sdl_state.texture= SDL_CreateTexture(g_sdl_state.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
if(!g_sdl_state.texture) {
SDL_DestroyRenderer(g_sdl_state.renderer);
SDL_DestroyWindow(g_sdl_state.window);
SDL_Quit();
LSAN_ENABLE();
return false;
}
g_sdl_state.width= width;
g_sdl_state.height= height;
SDL_RaiseWindow(g_sdl_state.window);
SDL_PumpEvents();
uint32_t* pixels= (uint32_t*)malloc((size_t)width * (size_t)height * sizeof(uint32_t));
if(!pixels) {
SDL_DestroyTexture(g_sdl_state.texture);
SDL_DestroyRenderer(g_sdl_state.renderer);
SDL_DestroyWindow(g_sdl_state.window);
SDL_Quit();
LSAN_ENABLE();
return false;
}
ctx_out->width= width;
ctx_out->height= height;
ctx_out->pixels= pixels;
/* Re-enable LSAN now that initialization is complete. */
LSAN_ENABLE();
return true;
}
void render_3d_sdl_shutdown(SDL3DContext* ctx) {
if(!ctx) return;
if(ctx->pixels) {
free(ctx->pixels);
ctx->pixels= NULL;
}
if(g_sdl_state.texture) {
SDL_DestroyTexture(g_sdl_state.texture);
g_sdl_state.texture= NULL;
}
if(g_sdl_state.renderer) {
SDL_DestroyRenderer(g_sdl_state.renderer);
g_sdl_state.renderer= NULL;
}
if(g_sdl_state.window) {
SDL_DestroyWindow(g_sdl_state.window);
g_sdl_state.window= NULL;
}
SDL_Quit();
ctx->width= 0;
ctx->height= 0;
}
void render_3d_sdl_set_pixel(SDL3DContext* ctx, int x, int y, uint32_t col) {
if(!ctx || !ctx->pixels || x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) return;
ctx->pixels[y * ctx->width + x]= col;
}
void render_3d_sdl_blend_pixel(SDL3DContext* ctx, int x, int y, uint32_t src_col) {
if(!ctx || !ctx->pixels || x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) return;
uint32_t dst= ctx->pixels[y * ctx->width + x];
uint8_t sa= (uint8_t)((src_col >> 24) & 0xFFu);
if(sa == 255) {
ctx->pixels[y * ctx->width + x]= src_col;
return;
}
if(sa == 0) return;
uint8_t sr= (uint8_t)((src_col >> 16) & 0xFFu);
uint8_t sg= (uint8_t)((src_col >> 8) & 0xFFu);
uint8_t sb= (uint8_t)(src_col & 0xFFu);
uint8_t dr= (uint8_t)((dst >> 16) & 0xFFu);
uint8_t dg= (uint8_t)((dst >> 8) & 0xFFu);
uint8_t db= (uint8_t)(dst & 0xFFu);
float a= (float)sa / 255.0f;
uint8_t out_r= (uint8_t)(sr * a + dr * (1.0f - a));
uint8_t out_g= (uint8_t)(sg * a + dg * (1.0f - a));
uint8_t out_b= (uint8_t)(sb * a + db * (1.0f - a));
ctx->pixels[y * ctx->width + x]= (0xFFu << 24) | ((uint32_t)out_r << 16) | ((uint32_t)out_g << 8) | (uint32_t)out_b;
}
void render_3d_sdl_draw_column(SDL3DContext* ctx, int x, int y_start, int y_end, uint32_t col) {
if(!ctx || !ctx->pixels || x < 0 || x >= ctx->width) return;
if(y_start < 0) y_start= 0;
if(y_end >= ctx->height) y_end= ctx->height - 1;
for(int y= y_start; y <= y_end; y++) ctx->pixels[y * ctx->width + x]= col;
}
void render_3d_sdl_draw_filled_circle(SDL3DContext* ctx, int center_x, int center_y, int radius, uint32_t col) {
if(!ctx || !ctx->pixels || radius <= 0) return;
int x1= center_x - radius;
int x2= center_x + radius;
int y1= center_y - radius;
int y2= center_y + radius;
if(x1 < 0) x1= 0;
if(x2 >= ctx->width) x2= ctx->width - 1;
if(y1 < 0) y1= 0;
if(y2 >= ctx->height) y2= ctx->height - 1;
for(int y= y1; y <= y2; y++) {
for(int x= x1; x <= x2; x++) {
int dx= x - center_x;
int dy= y - center_y;
if(dx * dx + dy * dy <= radius * radius) render_3d_sdl_blend_pixel(ctx, x, y, col);
}
}
}

void render_3d_sdl_draw_filled_rect(SDL3DContext* ctx, int x, int y, int w_rect, int h_rect, uint32_t col) {
	if(!ctx || !ctx->pixels || w_rect <= 0 || h_rect <= 0) return;
	int x0 = x;
	int y0 = y;
	int x1 = x + w_rect - 1;
	int y1 = y + h_rect - 1;
	if(x0 < 0) x0 = 0;
	if(y0 < 0) y0 = 0;
	if(x1 >= ctx->width) x1 = ctx->width - 1;
	if(y1 >= ctx->height) y1 = ctx->height - 1;
	for(int yy = y0; yy <= y1; ++yy) {
		for(int xx = x0; xx <= x1; ++xx) {
			render_3d_sdl_blend_pixel(ctx, xx, yy, col);
		}
	}
}
void render_3d_sdl_clear(SDL3DContext* ctx, uint32_t col) {
if(!ctx || !ctx->pixels) return;
int total= ctx->width * ctx->height;
for(int i= 0; i < total; i++) ctx->pixels[i]= col;
}
bool render_3d_sdl_present(SDL3DContext* ctx) {
if(!ctx || !g_sdl_state.renderer || !g_sdl_state.texture) return false;
SDL_PumpEvents();
SDL_Event event;
while(SDL_PollEvent(&event)) {
switch(event.type) {
case SDL_QUIT: return false;
case SDL_KEYDOWN:
if(event.key.keysym.sym == SDLK_ESCAPE) return false;
break;
default: break;
}
}
SDL_UpdateTexture(g_sdl_state.texture, NULL, ctx->pixels, ctx->width * (int)sizeof(uint32_t));
SDL_RenderClear(g_sdl_state.renderer);
SDL_RenderCopy(g_sdl_state.renderer, g_sdl_state.texture, NULL, NULL);
SDL_RenderPresent(g_sdl_state.renderer);
return true;
}
