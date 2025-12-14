#pragma once
#include <stdbool.h>
#define PERSIST_MAX_SCORES 10
#define PERSIST_NAME_MAX 32
#define PERSIST_CONFIG_DEFAULT_WIDTH 40
#define PERSIST_CONFIG_DEFAULT_HEIGHT 20
#define PERSIST_CONFIG_DEFAULT_TICK_MS 100
#define PERSIST_CONFIG_DEFAULT_SCREEN_WIDTH 60
#define PERSIST_CONFIG_DEFAULT_SCREEN_HEIGHT 24
typedef struct {
char name[PERSIST_NAME_MAX];
int score;
} HighScore;
typedef struct {
int board_width, board_height;
int tick_rate_ms;
int render_glyphs;
int screen_width, screen_height;
int render_mode;
} GameConfig;
int persist_read_scores(const char* filename, HighScore* scores, int max_count);
bool persist_write_scores(const char* filename, const HighScore* scores, int count);
bool persist_append_score(const char* filename, const char* name, int score);
bool persist_load_config(const char* filename, GameConfig* config);
bool persist_write_config(const char* filename, const GameConfig* config);
