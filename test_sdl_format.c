#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
    
    printf("SDL_PIXELFORMAT_ARGB8888 = 0x%08x\n", SDL_PIXELFORMAT_ARGB8888);
    
    SDL_Quit();
    return 0;
}
