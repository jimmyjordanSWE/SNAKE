#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 100, 100, 0);
    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, 0);
    SDL_Texture *tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 2, 2);
    
    uint32_t pixels[4] = {
        0xFFFF0000,  /* Red */
        0xFF00FF00,  /* Green */
        0xFF0000FF,  /* Blue */
        0xFFFFFFFF   /* White */
    };
    
    printf("Testing pixel format SDL_PIXELFORMAT_ARGB8888\n");
    printf("Pixel value 0xFFFF0000 should be red\n");
    printf("Pixel value 0xFF00FF00 should be green\n");
    printf("Pixel value 0xFF0000FF should be blue\n");
    printf("Pixel value 0xFFFFFFFF should be white\n");
    
    SDL_UpdateTexture(tex, NULL, pixels, 2 * sizeof(uint32_t));
    SDL_RenderCopy(rend, tex, NULL, NULL);
    SDL_RenderPresent(rend);
    SDL_Delay(1000);
    
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
