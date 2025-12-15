#pragma once
#include <stdbool.h>
#include <stdint.h>
#define PERSIST_MAX_SCORES 10
#define PERSIST_NAME_MAX 32
#define PERSIST_CONFIG_DEFAULT_WIDTH 40
#define PERSIST_CONFIG_DEFAULT_HEIGHT 20
#define PERSIST_CONFIG_DEFAULT_TICK_MS 100
#define PERSIST_CONFIG_DEFAULT_SCREEN_WIDTH 60
#define PERSIST_CONFIG_DEFAULT_SCREEN_HEIGHT 24
#define PERSIST_CONFIG_DEFAULT_MAX_PLAYERS 2
#define PERSIST_CONFIG_DEFAULT_MAX_LENGTH 1024
#define PERSIST_CONFIG_DEFAULT_MAX_FOOD 3
#define PERSIST_CONFIG_DEFAULT_SEED 42
#define PERSIST_CONFIG_DEFAULT_FOV_DEGREES 90.0f
#define PERSIST_CONFIG_DEFAULT_SHOW_SPRITE_DEBUG 0
#define PERSIST_CONFIG_DEFAULT_ACTIVE_PLAYER 0
#define PERSIST_CONFIG_DEFAULT_NUM_PLAYERS 1
#define PERSIST_CONFIG_DEFAULT_ENABLE_EXTERNAL_3D_VIEW 1
#define PERSIST_CONFIG_DEFAULT_WALL_SCALE 1.50f
#define PERSIST_CONFIG_DEFAULT_TAIL_SCALE 0.50f
#define PERSIST_TEXTURE_PATH_MAX 128
#define PERSIST_CONFIG_DEFAULT_WALL_TEXTURE "assets/wall.png"
#define PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE "assets/floor.png"
#define PERSIST_CONFIG_DEFAULT_KEY_UP 'w'
#define PERSIST_CONFIG_DEFAULT_KEY_DOWN 's'
#define PERSIST_CONFIG_DEFAULT_KEY_LEFT 'a'
#define PERSIST_CONFIG_DEFAULT_KEY_RIGHT 'd'
/* strafing removed; left/right keys are now "turn left/right" */
#define PERSIST_CONFIG_DEFAULT_KEY_QUIT 'q'
#define PERSIST_CONFIG_DEFAULT_KEY_RESTART 'r'
#define PERSIST_CONFIG_DEFAULT_KEY_PAUSE 'p'
#define PERSIST_PLAYER_NAME_MAX 32
typedef struct {
char name[PERSIST_NAME_MAX];
int score;
} HighScore;
typedef struct {
int board_width, board_height;
int tick_rate_ms;
int render_glyphs;
int screen_width, screen_height;
/* replaced: render_mode -> enable_external_3d_view
            When true the optional external SDL 3D view will be started. */
int enable_external_3d_view;
uint32_t seed;
float fov_degrees;
/* show_minimap and show_stats were removed (unused) */
int show_sprite_debug;
int active_player;
int num_players;
char player_name[PERSIST_PLAYER_NAME_MAX];
int max_players;
int max_length;
int max_food;
/* 3D rendering tunables */
float wall_height_scale;
float tail_height_scale;
char wall_texture[PERSIST_TEXTURE_PATH_MAX];
char floor_texture[PERSIST_TEXTURE_PATH_MAX];
/* key bindings (single ASCII char) */
char key_up;
char key_down;
char key_left;
char key_right;
/* strafing removed */
char key_quit;
char key_restart;
char key_pause;
} GameConfig;
int persist_read_scores(const char* filename, HighScore* scores, int max_count);
bool persist_write_scores(const char* filename, const HighScore* scores, int count);
bool persist_append_score(const char* filename, const char* name, int score);
bool persist_load_config(const char* filename, GameConfig* config);
bool persist_write_config(const char* filename, const GameConfig* config);
