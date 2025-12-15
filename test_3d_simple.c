#include "snake/game.h"
#include "snake/render_3d.h"
#include <stdio.h>

int main(void) {
    GameState game = {0};
    GameConfig cfg = { .board_width = 32, .board_height = 32, .tick_rate_ms = 100, .render_glyphs = 0, .screen_width = 800, .screen_height = 600, .enable_external_3d_view = 1, .seed = 42, .fov_degrees = 90.0f, .show_sprite_debug = 0, .active_player = 0, .num_players = 1, .max_players = 2, .max_length = 64, .max_food = 4 };
    game_init(&game, 32, 32, &cfg);
    
    Render3DConfig config = {
        .active_player = 0,
        .fov_degrees = 90.0f,
        /* show_minimap/show_stats removed (unused) */
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
    
    game_free(&game);
    return result ? 0 : 1;
}
