#pragma once
#include "persist.h"
#include <stdbool.h>
#include <stdint.h>
#ifndef SNAKE_BOARD_DIMENSIONS_MOVED
#define SNAKE_BODY_MAX_LEN 256
#endif

/* A single point/segment in the game grid (integer coordinates) */
typedef struct {
int x, y;
} SnakePoint;

/* Float-based point for smooth rendering interpolation.
   Used only for visual representation, not game logic. */
typedef struct {
float x, y;
} SnakePointF;


/* Canonical visual representation of any snake (local, remote, or AI).
   This struct is used for rendering and network serialization.
   For game logic (direction, lives, etc.), see PlayerState. */
typedef struct {
SnakePoint body[SNAKE_BODY_MAX_LEN];
int length;
uint32_t color;
char name[PERSIST_PLAYER_NAME_MAX];
int score;
bool active;
} SnakeBody;


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

