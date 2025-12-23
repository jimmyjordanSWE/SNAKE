#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifndef SNAKE_BOARD_DIMENSIONS_MOVED
#define SNAKE_BOARD_DIMENSIONS_MOVED 1
#endif
typedef struct {
int x, y;
} SnakePoint;
typedef enum {
    SNAKE_DIR_UP= 0,
    SNAKE_DIR_DOWN,
    SNAKE_DIR_LEFT,
    SNAKE_DIR_RIGHT,
} SnakeDir;
typedef enum {
    GAME_STATUS_RUNNING= 0,
    GAME_STATUS_PAUSED,
    GAME_STATUS_GAME_OVER,
} GameStatus;
