#include "snake/render_3d_sdl.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
typedef struct {
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
int width;
int height;
} SDLState;
static SDLState g_sdl_state= {0};
bool render_3d_sdl_init(int width, int height, SDL3DContext* ctx_out) {
if(!ctx_out || width <= 0 || height <= 0) return false;
if(SDL_Init(SDL_INIT_VIDEO) < 0) return false;
SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
g_sdl_state.window= SDL_CreateWindow("SNAKE 3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
if(!g_sdl_state.window) {
SDL_Quit();
return false;
}
SDL_ShowWindow(g_sdl_state.window);
SDL_RaiseWindow(g_sdl_state.window);
SDL_PumpEvents();
g_sdl_state.renderer= SDL_CreateRenderer(g_sdl_state.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
if(!g_sdl_state.renderer) {
SDL_DestroyWindow(g_sdl_state.window);
SDL_Quit();
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
return false;
}
ctx_out->width= width;
ctx_out->height= height;
ctx_out->pixels= pixels;
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
if(dx * dx + dy * dy <= radius * radius) ctx->pixels[y * ctx->width + x]= col;
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
