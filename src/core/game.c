#include "snake/game.h"
#include "snake/utils.h"

static SnakePoint random_point(GameState* game) {
    SnakePoint p;
    p.x = snake_rng_range(&game->rng_state, 0, game->width - 1);
    p.y = snake_rng_range(&game->rng_state, 0, game->height - 1);
    return p;
}

void game_init(GameState* game, int width, int height, uint32_t seed) {
    if (!game) { return; }

    game->width = width;
    game->height = height;
    snake_rng_seed(&game->rng_state, seed);

    game->status = GAME_STATUS_RUNNING;
    game->food_present = false;

    for (int i = 0; i < SNAKE_MAX_PLAYERS; i++) {
        game->players[i].score = 0;
        game->players[i].current_dir = SNAKE_DIR_RIGHT;
        game->players[i].queued_dir = SNAKE_DIR_RIGHT;
    }

    game->food = random_point(game);
    game->food_present = true;
}

void game_reset(GameState* game) {
    if (!game) { return; }

    for (int i = 0; i < SNAKE_MAX_PLAYERS; i++) {
        game->players[i].score = 0;
        game->players[i].current_dir = SNAKE_DIR_RIGHT;
        game->players[i].queued_dir = SNAKE_DIR_RIGHT;
    }

    game->status = GAME_STATUS_RUNNING;
    game->food = random_point(game);
    game->food_present = true;
}

void game_tick(GameState* game) {
    (void)game;
    // Step 0 implementation: core logic intentionally not implemented yet.
}
