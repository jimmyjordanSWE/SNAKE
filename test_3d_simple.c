#include "snake/game.h"
#include "snake/render_3d.h"
#include <stdio.h>

int main(void) {
    GameState game = {0};
    game_init(&game, 32, 32, 42);
    
    Render3DConfig config = {
        .active_player = 0,
        .fov_degrees = 90.0f,
        .show_minimap = false,
        .show_stats = false,
        .screen_width = 800,
        .screen_height = 600
    };
    
    fprintf(stderr, "Initializing 3D rendering...\n");
    bool result = render_3d_init(&game, &config);
    fprintf(stderr, "render_3d_init returned: %d\n", result);
    
    if (result) {
        fprintf(stderr, "Drawing frame...\n");
        render_3d_draw(&game, "Test", NULL, 0);
        fprintf(stderr, "Draw complete\n");
        render_3d_shutdown();
    }
    
    return result ? 0 : 1;
}
