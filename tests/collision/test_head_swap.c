#include "snake/game.h"
#include "snake/collision.h"
#include <stdio.h>

int main(void) {
    GameState g = {0};
    GameConfig cfg = { .board_width = 10, .board_height = 10, .tick_rate_ms = 100, .render_glyphs = 0, .screen_width = 80, .screen_height = 25, .render_mode = 1, .seed = 42, .fov_degrees = 90.0f, .show_minimap = 0, .show_stats = 0, .show_sprite_debug = 0, .active_player = 0, .num_players = 2, .max_players = 4, .max_length = 16, .max_food = 4 };
    game_init(&g, 10, 10, &cfg);
    g.status = GAME_STATUS_RUNNING;

    PlayerState *a = &g.players[0];
    PlayerState *b = &g.players[1];

    a->active = true; a->length = 2; a->body[0] = (SnakePoint){5,5}; a->body[1] = (SnakePoint){4,5}; a->current_dir = SNAKE_DIR_RIGHT;
    b->active = true; b->length = 2; b->body[0] = (SnakePoint){6,5}; b->body[1] = (SnakePoint){7,5}; b->current_dir = SNAKE_DIR_LEFT;

    collision_detect_and_resolve(&g);

    if(!a->needs_reset || !b->needs_reset) {
        fprintf(stderr, "FAIL: both players should be reset on head-swap\n");
        game_free(&g);
        return 1;
    }
    game_free(&g);
    return 0;
}
