#pragma once
#include "snake/persist.h"
#include "snake/types.h"
#include <stdbool.h>
#include <stdint.h>

#define SNAKE_MAX_PLAYERS 2
#define SNAKE_MAX_LENGTH  1024
#define SNAKE_MAX_FOOD    3

/* Opaque Game object. Use the constructor/destructor and the event-based
 * `game_step()` to interact with the simulation. The module remains
 * deterministic when given the same seed and inputs and is designed to be
 * driven by an authoritative host in multiplayer scenarios. */
typedef struct Game Game;

/* Events produced by a single tick. Kept intentionally small and simple to
 * make serialization for multiplayer easy. */
#define GAME_EVENTS_MAX_PLAYERS 8
typedef struct
{
    /* Players that died this tick and their score at death. */
    int  died_players[GAME_EVENTS_MAX_PLAYERS];
    int  died_scores[GAME_EVENTS_MAX_PLAYERS];
    int  died_count;

    /* True if this tick caused a transition to GAME_STATUS_GAME_OVER. */
    bool game_over;

    /* True if food was respawned this tick. */
    bool food_respawned;
} GameEvents;

#include "snake/input.h"

/* NOTE: The following structure definitions are kept available for internal
 * modules and tests that currently work with raw game state (collision,
 * rendering, tests). Prefer using the opaque `Game` API above for new code.
 */
typedef struct
{
    SnakeDir    current_dir, queued_dir;
    int         score;
    bool        died_this_tick;
    int         score_at_death;
    SnakePoint* body;
    int         length;
    int         max_length;
    bool        active, needs_reset;
    /* previous head position for interpolation (world coords, centered)
     * updated on each tick before applying movement */
    float prev_head_x;
    float prev_head_y;
} PlayerState;
typedef struct
{
    int          width, height;
    uint32_t     rng_state;
    GameStatus   status;
    int          num_players;
    int          max_players;
    SnakePoint*  food;
    int          food_count;
    int          max_food;
    /* Test/diagnostic flag: set to true when food was respawned during last
     * tick; cleared on observation. */
    bool         last_food_respawned;
    PlayerState* players;
    int          max_length;
} GameState;

/* Create and destroy */
Game* game_create(const GameConfig* cfg, uint32_t seed_override);
void  game_destroy(Game* g);

/* Input and progression */
/* Enqueue an input for `player_index` to be applied on the next `game_step`. */
int  game_enqueue_input(Game* g, int player_index, const InputState* in);
void game_step(Game* g, GameEvents* out_events);

/* Accessors for rendering and queries */
const GameState* game_get_state(const Game* g);
int              game_get_num_players(const Game* g);
bool             game_player_is_active(const Game* g, int player_index);
int              game_player_current_score(const Game* g, int player_index);
bool             game_player_died_this_tick(const Game* g, int player_index);
int              game_player_score_at_death(const Game* g, int player_index);

/* Keep a small test-only helper API to support existing unit tests. These
 * are intended for tests only and may be removed or restricted later. */
void game_reset(Game* g);

void game_test_set_dimensions(Game* g, int width, int height);
void game_test_set_num_players(Game* g, int n);
void game_test_set_player_needs_reset(Game* g, int player_index, bool needs);
void game_test_set_food(Game* g, const SnakePoint* food, int count);
/* Test helper: get a mutable pointer to the internal state for unit tests. */
GameState* game_test_get_state(Game* g);


