/* include/snake/types.h */
#ifndef SNAKE_TYPES_H
#define SNAKE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

/* Maximum number of players supported */
#define MAX_PLAYERS 4

/* Maximum snake length (for ring buffer allocation) */
#define MAX_SNAKE_LENGTH 1024

/* Point on the game board */
typedef struct {
    int x;
    int y;
} SnakePoint;

/* Direction enum */
typedef enum { DIR_UP = 0, DIR_DOWN, DIR_LEFT, DIR_RIGHT } SnakeDir;

/* Game status */
typedef enum { GAME_RUNNING = 0, GAME_PAUSED, GAME_OVER } GameStatus;

/* Snake representation using ring buffer */
typedef struct {
    SnakePoint body[MAX_SNAKE_LENGTH];
    int length;
    SnakeDir direction;
    bool active;
} Snake;

/* Player state */
typedef struct {
    SnakePoint body[MAX_SNAKE_LENGTH];
    int length;
    SnakeDir direction;
    SnakeDir next_direction; /* Buffered input for next tick */
    int score;
    bool active;
    bool needs_reset; /* Set by collision detection */
    uint32_t color;   /* For rendering */
} PlayerState;

/* Game state (forward declaration for collision.h) */
typedef struct {
    int board_width;
    int board_height;
    int num_players;
    PlayerState players[MAX_PLAYERS];
    SnakePoint food;
    GameStatus status;
    uint32_t rng_state;
    uint64_t tick_count;
} GameState;

#endif /* SNAKE_TYPES_H */