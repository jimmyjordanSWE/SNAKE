#include "snake/game.h"
#include "snake/collision.h"
#include "snake/utils.h"

#include <stddef.h>

static SnakePoint random_point(GameState* game) {
    SnakePoint p;
    p.x = snake_rng_range(&game->rng_state, 0, game->width - 1);
    p.y = snake_rng_range(&game->rng_state, 0, game->height - 1);
    return p;
}

static bool point_in_player(const PlayerState* player, SnakePoint p) {
    if (player == NULL || !player->active || player->length <= 0) { return false; }

    for (int i = 0; i < player->length; i++) {
        SnakePoint s = player->body[i];
        if (s.x == p.x && s.y == p.y) { return true; }
    }

    return false;
}

static bool point_in_any_snake(const GameState* game, SnakePoint p) {
    if (game == NULL) { return false; }

    for (int i = 0; i < SNAKE_MAX_PLAYERS; i++) {
        if (point_in_player(&game->players[i], p)) { return true; }
    }

    return false;
}

static void food_respawn(GameState* game) {
    if (game == NULL) { return; }

    int max_attempts = game->width * game->height * 2;
    if (max_attempts < 32) { max_attempts = 32; }

    for (int attempt = 0; attempt < max_attempts; attempt++) {
        SnakePoint p = random_point(game);
        if (!point_in_any_snake(game, p)) {
            game->food = p;
            game->food_present = true;
            return;
        }
    }

    game->food_present = false;
}

static void player_move(PlayerState* player, SnakePoint next_head, bool grow) {
    if (player == NULL || !player->active || player->length <= 0) { return; }

    bool actual_grow = grow && player->length < SNAKE_MAX_LENGTH;
    int last = actual_grow ? player->length : (player->length - 1);

    for (int i = last; i > 0; i--) { player->body[i] = player->body[i - 1]; }

    player->body[0] = next_head;
    if (actual_grow) { player->length++; }
}

static bool spawn_player(GameState* game, int player_index) {
    if (game == NULL || player_index < 0 || player_index >= SNAKE_MAX_PLAYERS) { return false; }

    if (game->width < 2 || game->height < 1) { return false; }

    PlayerState* player = &game->players[player_index];
    player->active = true;
    player->needs_reset = false;
    player->length = 2;
    SnakeDir random_dir = (SnakeDir)snake_rng_range(&game->rng_state, 0, 3);
    player->current_dir = random_dir;
    player->queued_dir = random_dir;

    for (int attempt = 0; attempt < 1000; attempt++) {
        SnakePoint head = (SnakePoint){
            .x = snake_rng_range(&game->rng_state, 1, game->width - 1),
            .y = snake_rng_range(&game->rng_state, 0, game->height - 1),
        };
        SnakePoint tail = collision_next_head(head, random_dir);

        bool overlaps = false;
        for (int i = 0; i < SNAKE_MAX_PLAYERS; i++) {
            if (i == player_index) { continue; }
            if (point_in_player(&game->players[i], head) || point_in_player(&game->players[i], tail)) {
                overlaps = true;
                break;
            }
        }

        if (!overlaps) {
            player->body[0] = head;
            player->body[1] = tail;
            return true;
        }
    }

    player->active = false;
    player->length = 0;
    return false;
}

void game_init(GameState* game, int width, int height, uint32_t seed) {
    if (!game) { return; }

    game->width = (width < 2) ? 2 : width;
    game->height = (height < 2) ? 2 : height;
    snake_rng_seed(&game->rng_state, seed);

    game->status = GAME_STATUS_RUNNING;
    game->food_present = false;

    game->num_players = 1;

    for (int i = 0; i < SNAKE_MAX_PLAYERS; i++) {
        game->players[i].score = 0;
        game->players[i].died_this_tick = false;
        game->players[i].score_at_death = 0;
        game->players[i].active = false;
        game->players[i].needs_reset = false;
        game->players[i].length = 0;
    }

    for (int i = 0; i < game->num_players; i++) { (void)spawn_player(game, i); }

    food_respawn(game);
}

void game_reset(GameState* game) {
    if (!game) { return; }

    for (int i = 0; i < SNAKE_MAX_PLAYERS; i++) {
        game->players[i].score = 0;
        game->players[i].died_this_tick = false;
        game->players[i].score_at_death = 0;
        game->players[i].active = false;
        game->players[i].needs_reset = false;
        game->players[i].length = 0;
    }

    for (int i = 0; i < game->num_players; i++) { (void)spawn_player(game, i); }

    game->status = GAME_STATUS_RUNNING;
    food_respawn(game);
}

void game_tick(GameState* game) {
    if (game == NULL) { return; }

    if (game->status != GAME_STATUS_RUNNING) { return; }

    int num_players = game->num_players;
    if (num_players < 0) { num_players = 0; }
    if (num_players > SNAKE_MAX_PLAYERS) { num_players = SNAKE_MAX_PLAYERS; }

    for (int i = 0; i < num_players; i++) {
        game->players[i].needs_reset = false;
        game->players[i].died_this_tick = false;
        game->players[i].score_at_death = 0;
    }

    for (int i = 0; i < num_players; i++) {
        PlayerState* player = &game->players[i];
        if (!player->active || player->length <= 0) { continue; }

        player->current_dir = player->queued_dir;
    }

    collision_detect_and_resolve(game);

    /* Apply death effects (capture score, reset score) before respawn */
    for (int i = 0; i < num_players; i++) {
        PlayerState* player = &game->players[i];
        if (!player->active) { continue; }
        if (!player->needs_reset) { continue; }

        player->died_this_tick = true;
        player->score_at_death = player->score;
        player->score = 0;
    }

    for (int i = 0; i < num_players; i++) {
        PlayerState* player = &game->players[i];
        if (player->needs_reset) { (void)spawn_player(game, i); }
    }

    bool food_consumed = false;

    for (int i = 0; i < num_players; i++) {
        PlayerState* player = &game->players[i];
        if (!player->active || player->length <= 0) { continue; }

        if (player->needs_reset) { continue; }

        SnakePoint current_head = player->body[0];
        SnakePoint next_head = collision_next_head(current_head, player->current_dir);

        bool eat = false;
        if (game->food_present && next_head.x == game->food.x && next_head.y == game->food.y) { eat = true; }

        player_move(player, next_head, eat);
        if (eat) {
            player->score++;
            food_consumed = true;
        }
    }

    if (food_consumed) {
        game->food_present = false;
        food_respawn(game);
    }
}
