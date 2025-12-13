#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Fixed board dimensions - board will always be this size and centered on terminal */
#define FIXED_BOARD_WIDTH 40
#define FIXED_BOARD_HEIGHT 20

typedef struct {
    int x;
    int y;
} SnakePoint;

typedef enum {
    SNAKE_DIR_UP = 0,
    SNAKE_DIR_DOWN,
    SNAKE_DIR_LEFT,
    SNAKE_DIR_RIGHT,
} SnakeDir;

typedef enum {
    GAME_STATUS_RUNNING = 0,
    GAME_STATUS_PAUSED,
    GAME_STATUS_GAME_OVER,
} GameStatus;
