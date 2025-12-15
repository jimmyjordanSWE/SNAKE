#include "snake/game.h"
#include "snake/collision.h"
#include "snake/utils.h"
#include <stddef.h>
#include <stdlib.h>
/* Module constants to avoid magic literals and improve testability. */
#define SPAWN_MAX_ATTEMPTS 1000
#define FOOD_RESPAWN_MIN   1
#define FOOD_RESPAWN_MAX   3
#define FOOD_MIN_ATTEMPTS  32
static SnakePoint random_point(GameState* game)
{
    SnakePoint p;
    p.x = snake_rng_range(&game->rng_state, 0, game->width - 1);
    p.y = snake_rng_range(&game->rng_state, 0, game->height - 1);
    return p;
}
static bool point_in_player(const PlayerState* player, SnakePoint p)
{
    if (player == NULL || !player->active || player->length <= 0)
        return false;
    for (int i = 0; i < player->length; i++)
    {
        SnakePoint s = player->body[i];
        if (s.x == p.x && s.y == p.y)
            return true;
    }
    return false;
}
static bool point_in_any_snake(const GameState* game, SnakePoint p)
{
    if (game == NULL)
        return false;
    for (int i = 0; i < game->max_players; i++)
        if (point_in_player(&game->players[i], p))
            return true;
    return false;
}
static bool point_is_food(const GameState* game, SnakePoint p)
{
    if (game == NULL)
        return false;
    for (int i = 0; i < game->food_count; i++)
        if (game->food[i].x == p.x && game->food[i].y == p.y)
            return true;
    return false;
}
static void food_respawn(GameState* game)
{
    if (game == NULL)
        return;
    int num_to_spawn =
        snake_rng_range(&game->rng_state, FOOD_RESPAWN_MIN, FOOD_RESPAWN_MAX);
    game->food_count = 0;
    int max_attempts = game->width * game->height * 2;
    if (max_attempts < FOOD_MIN_ATTEMPTS)
        max_attempts = FOOD_MIN_ATTEMPTS;
    for (int i = 0; i < num_to_spawn && game->food_count < game->max_food; i++)
    {
        for (int attempt = 0; attempt < max_attempts; attempt++)
        {
            SnakePoint p = random_point(game);
            if (!point_in_any_snake(game, p) && !point_is_food(game, p))
            {
                game->food[game->food_count] = p;
                game->food_count++;
                break;
            }
        }
    }
}
static void player_move(PlayerState* player, SnakePoint next_head, bool grow)
{
    if (player == NULL || !player->active || player->length <= 0)
        return;
    bool actual_grow = grow && player->length < player->max_length;
    int  last        = actual_grow ? player->length : (player->length - 1);
    for (int i = last; i > 0; i--)
        player->body[i] = player->body[i - 1];
    player->body[0] = next_head;
    if (actual_grow)
        player->length++;
}
static SnakeDir opposite_dir(SnakeDir dir)
{
    switch (dir)
    {
        case SNAKE_DIR_UP:
            return SNAKE_DIR_DOWN;
        case SNAKE_DIR_DOWN:
            return SNAKE_DIR_UP;
        case SNAKE_DIR_LEFT:
            return SNAKE_DIR_RIGHT;
        case SNAKE_DIR_RIGHT:
            return SNAKE_DIR_LEFT;
        default:
            return SNAKE_DIR_UP;
    }
}
static bool spawn_player(GameState* game, int player_index)
{
    if (game == NULL || player_index < 0 || player_index >= game->max_players)
        return false;
    if (game->width < 2 || game->height < 1)
        return false;
    PlayerState* player = &game->players[player_index];
    player->active      = true;
    player->needs_reset = false;
    player->length      = 2;
    for (int attempt = 0; attempt < SPAWN_MAX_ATTEMPTS; attempt++)
    {
        SnakePoint head = (SnakePoint){
            .x = snake_rng_range(&game->rng_state, 2, game->width - 3),
            .y = snake_rng_range(&game->rng_state, 2, game->height - 3),
        };
        int      dist_left  = head.x;
        int      dist_right = game->width - 1 - head.x;
        int      dist_up    = head.y;
        int      dist_down  = game->height - 1 - head.y;
        int      max_dist   = dist_left;
        SnakeDir best_dir   = SNAKE_DIR_LEFT;
        if (dist_right > max_dist)
        {
            max_dist = dist_right;
            best_dir = SNAKE_DIR_RIGHT;
        }
        if (dist_up > max_dist)
        {
            max_dist = dist_up;
            best_dir = SNAKE_DIR_UP;
        }
        if (dist_down > max_dist)
        {
            max_dist = dist_down;
            best_dir = SNAKE_DIR_DOWN;
        }
        player->current_dir = best_dir;
        player->queued_dir  = best_dir;
        SnakePoint tail     = collision_next_head(head, opposite_dir(best_dir));
        if (collision_is_wall(tail, game->width, game->height))
            continue;
        bool overlaps = false;
        for (int i = 0; i < game->max_players; i++)
        {
            if (i == player_index)
                continue;
            if (point_in_player(&game->players[i], head)
                || point_in_player(&game->players[i], tail))
            {
                overlaps = true;
                break;
            }
        }
        if (!overlaps)
        {
            player->body[0] = head;
            player->body[1] = tail;
            /* initialize previous head for interpolation */
            player->prev_head_x = (float)head.x + 0.5f;
            player->prev_head_y = (float)head.y + 0.5f;
            return true;
        }
    }
    player->active = false;
    player->length = 0;
    return false;
}
void game_init(GameState* game, int width, int height, const GameConfig* cfg)
{
    if (!game || !cfg)
        return;
    game->width  = (width < 2) ? 2 : width;
    game->height = (height < 2) ? 2 : height;
    snake_rng_seed(&game->rng_state, cfg->seed);
    game->status      = GAME_STATUS_RUNNING;
    game->food_count  = 0;
    game->num_players = cfg->num_players;
    if (game->num_players < 1)
        game->num_players = 1;
    game->max_players = cfg->max_players;
    game->max_length  = cfg->max_length;
    game->max_food    = cfg->max_food;
    /* allocate dynamic arrays */
    game->players =
        (PlayerState*)malloc(sizeof(PlayerState) * (size_t)game->max_players);
    if (!game->players)
        return;
    game->food =
        (SnakePoint*)malloc(sizeof(SnakePoint) * (size_t)game->max_food);
    if (!game->food)
    {
        free(game->players);
        game->players = NULL;
        return;
    }
    for (int i = 0; i < game->max_players; i++)
    {
        game->players[i].score          = 0;
        game->players[i].died_this_tick = false;
        game->players[i].score_at_death = 0;
        game->players[i].active         = false;
        game->players[i].needs_reset    = false;
        game->players[i].length         = 0;
        game->players[i].max_length     = game->max_length;
        game->players[i].body =
            (SnakePoint*)malloc(sizeof(SnakePoint) * (size_t)game->max_length);
        if (!game->players[i].body)
        {
            for (int j = 0; j < i; j++)
                free(game->players[j].body);
            free(game->food);
            free(game->players);
            game->players = NULL;
            game->food    = NULL;
            return;
        }
    }
    for (int i = 0; i < game->num_players; i++)
        (void)spawn_player(game, i);
    food_respawn(game);
}

void game_free(GameState* game)
{
    if (!game)
        return;
    if (game->players)
    {
        for (int i = 0; i < game->max_players; i++)
            free(game->players[i].body);
        free(game->players);
        game->players = NULL;
    }
    if (game->food)
    {
        free(game->food);
        game->food = NULL;
    }
}
void game_reset(GameState* game)
{
    if (!game)
        return;
    for (int i = 0; i < game->max_players; i++)
    {
        game->players[i].score          = 0;
        game->players[i].died_this_tick = false;
        game->players[i].score_at_death = 0;
        game->players[i].active         = false;
        game->players[i].needs_reset    = false;
        game->players[i].length         = 0;
        game->players[i].max_length     = game->max_length;
    }
    for (int i = 0; i < game->num_players; i++)
        (void)spawn_player(game, i);
    game->status = GAME_STATUS_RUNNING;
    food_respawn(game);
}
void game_tick(GameState* game)
{
    if (game == NULL)
        return;
    if (game->status != GAME_STATUS_RUNNING)
        return;
    int num_players = game->num_players;
    if (num_players < 0)
        num_players = 0;
    if (num_players > SNAKE_MAX_PLAYERS)
        num_players = SNAKE_MAX_PLAYERS;
    for (int i = 0; i < num_players; i++)
    {
        game->players[i].needs_reset    = false;
        game->players[i].died_this_tick = false;
        game->players[i].score_at_death = 0;
    }
    for (int i = 0; i < num_players; i++)
    {
        PlayerState* player = &game->players[i];
        if (!player->active || player->length <= 0)
            continue;
        if (player->queued_dir != opposite_dir(player->current_dir))
            player->current_dir = player->queued_dir;
    }
    collision_detect_and_resolve(game);
    for (int i = 0; i < num_players; i++)
    {
        PlayerState* player = &game->players[i];
        if (!player->active)
            continue;
        if (!player->needs_reset)
            continue;
        player->died_this_tick = true;
        player->score_at_death = player->score;
        player->score          = 0;
    }
    for (int i = 0; i < num_players; i++)
    {
        PlayerState* player = &game->players[i];
        if (player->needs_reset)
            (void)spawn_player(game, i);
    }
    /* If no players could be (re)spawned the game is over. */
    {
        int active_count = 0;
        for (int i = 0; i < num_players; i++)
            if (game->players[i].active)
                active_count++;
        if (active_count == 0)
            game->status = GAME_STATUS_GAME_OVER;
    }
    bool food_consumed = false;
    for (int i = 0; i < num_players; i++)
    {
        PlayerState* player = &game->players[i];
        if (!player->active || player->length <= 0)
            continue;
        if (player->needs_reset)
            continue;
        SnakePoint current_head = player->body[0];
        /* record previous head position (centered) for interpolation */
        player->prev_head_x = (float)current_head.x + 0.5f;
        player->prev_head_y = (float)current_head.y + 0.5f;
        SnakePoint next_head =
            collision_next_head(current_head, player->current_dir);
        bool eat = false;
        for (int f = 0; f < game->food_count; f++)
        {
            if (next_head.x == game->food[f].x
                && next_head.y == game->food[f].y)
            {
                eat = true;
                for (int j = f; j < game->food_count - 1; j++)
                    game->food[j] = game->food[j + 1];
                game->food_count--;
                food_consumed = true;
                break;
            }
        }
        player_move(player, next_head, eat);
        if (eat)
            player->score++;
    }
    if (food_consumed && game->food_count == 0)
        food_respawn(game);
}
int game_get_num_players(const GameState* game)
{
    if (game == NULL)
        return 0;
    int count = game->num_players;
    if (count < 0)
        return 0;
    if (count > game->max_players)
        return game->max_players;
    return count;
}
bool game_player_is_active(const GameState* game, int player_index)
{
    if (game == NULL || player_index < 0 || player_index >= game->max_players)
        return false;
    return game->players[player_index].active;
}
int game_player_current_score(const GameState* game, int player_index)
{
    if (game == NULL || player_index < 0 || player_index >= game->max_players)
        return 0;
    return game->players[player_index].score;
}
bool game_player_died_this_tick(const GameState* game, int player_index)
{
    if (game == NULL || player_index < 0 || player_index >= game->max_players)
        return false;
    return game->players[player_index].died_this_tick;
}
int game_player_score_at_death(const GameState* game, int player_index)
{
    if (game == NULL || player_index < 0 || player_index >= game->max_players)
        return 0;
    return game->players[player_index].score_at_death;
}

/* Test helper: set `game` food positions deterministically for unit tests. */
void game_set_food(GameState* game, const SnakePoint* food, int count)
{
    if (!game || !food || count <= 0)
        return;
    if (count > game->max_food)
        count = game->max_food;
    for (int i = 0; i < count; i++)
        game->food[i] = food[i];
    game->food_count = count;
}
