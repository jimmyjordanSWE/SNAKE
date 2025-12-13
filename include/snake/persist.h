#pragma once

#include <stdbool.h>

#define PERSIST_MAX_SCORES 10
#define PERSIST_NAME_MAX 32
#define PERSIST_CONFIG_DEFAULT_WIDTH 40
#define PERSIST_CONFIG_DEFAULT_HEIGHT 20
#define PERSIST_CONFIG_DEFAULT_TICK_MS 100
#define PERSIST_CONFIG_DEFAULT_MIN_SCREEN_WIDTH 60
#define PERSIST_CONFIG_DEFAULT_MIN_SCREEN_HEIGHT 24

typedef struct {
    char name[PERSIST_NAME_MAX];
    int score;
} HighScore;

typedef struct {
    int board_width;
    int board_height;
    int tick_rate_ms;

    /* Minimum terminal size required to run the current layout. */
    int min_screen_width;
    int min_screen_height;
} GameConfig;

/* Read high scores from file. Returns number of scores read (0 if file doesn't exist or is invalid). */
int persist_read_scores(const char* filename, HighScore* scores, int max_count);

/* Write high scores to file atomically. Returns true on success. */
bool persist_write_scores(const char* filename, const HighScore* scores, int count);

/* Append score to high scores file, keeping top scores sorted. Returns true on success. */
bool persist_append_score(const char* filename, const char* name, int score);

/* Load game config from file, apply defaults if file missing or invalid. Returns true if file exists and was parsed. */
bool persist_load_config(const char* filename, GameConfig* config);

/* Write game config to file. Returns true on success. */
bool persist_write_config(const char* filename, const GameConfig* config);
