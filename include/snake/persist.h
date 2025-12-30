#pragma once
#include <stdbool.h>
#include <stdint.h>
#define PERSIST_MAX_SCORES 10
#define PERSIST_NAME_MAX 9
#define PERSIST_CONFIG_DEFAULT_WIDTH 18
#define PERSIST_CONFIG_DEFAULT_HEIGHT 18
#define PERSIST_CONFIG_DEFAULT_TICK_MS 250
#define PERSIST_CONFIG_DEFAULT_SCREEN_WIDTH 60
#define PERSIST_CONFIG_DEFAULT_SCREEN_HEIGHT 24
#define PERSIST_CONFIG_DEFAULT_MAX_PLAYERS 2
#define PERSIST_CONFIG_DEFAULT_MAX_LENGTH 1024
#define PERSIST_CONFIG_DEFAULT_MAX_FOOD 3
#define PERSIST_CONFIG_DEFAULT_SEED 42
#define PERSIST_CONFIG_MIN_BOARD_WIDTH 10
#define PERSIST_CONFIG_MAX_BOARD_WIDTH 100
#define PERSIST_CONFIG_MIN_BOARD_HEIGHT 10
#define PERSIST_CONFIG_MAX_BOARD_HEIGHT 100
#define PERSIST_CONFIG_MIN_TICK_MS 10
#define PERSIST_CONFIG_MAX_TICK_MS 1000
#define PERSIST_CONFIG_MIN_SCREEN_WIDTH 20
#define PERSIST_CONFIG_MAX_SCREEN_WIDTH 4096
#define PERSIST_CONFIG_MIN_SCREEN_HEIGHT 10
#define PERSIST_CONFIG_MAX_SCREEN_HEIGHT 2160
#define PERSIST_CONFIG_DEFAULT_FOV_DEGREES 90.0f
#define PERSIST_CONFIG_DEFAULT_SHOW_SPRITE_DEBUG 0
#define PERSIST_CONFIG_DEFAULT_ACTIVE_PLAYER 0
#define PERSIST_CONFIG_DEFAULT_NUM_PLAYERS 2
#define PERSIST_CONFIG_DEFAULT_ENABLE_EXTERNAL_3D_VIEW 1
#define PERSIST_CONFIG_DEFAULT_WALL_SCALE 1.50f
#define PERSIST_CONFIG_DEFAULT_TAIL_SCALE 0.50f
#define PERSIST_CONFIG_DEFAULT_WALL_TEXTURE_SCALE 1.0f
#define PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE_SCALE 1.0f
#define PERSIST_TEXTURE_PATH_MAX 128
#define PERSIST_CONFIG_DEFAULT_WALL_TEXTURE "assets/wall.png"
#define PERSIST_CONFIG_DEFAULT_FLOOR_TEXTURE "assets/floor.png"
/* Player 1 defaults to arrow keys (no single-char left/right). */
#define PERSIST_CONFIG_DEFAULT_KEY_LEFT '\0'
#define PERSIST_CONFIG_DEFAULT_KEY_RIGHT '\0'
#define PERSIST_CONFIG_DEFAULT_KEY_QUIT '\x1b'
#define PERSIST_CONFIG_DEFAULT_KEY_RESTART 'r'
#define PERSIST_CONFIG_DEFAULT_KEY_PAUSE 'p'
/* Per-player defaults (player index 1-based: p2 is player 2). */
#define PERSIST_CONFIG_DEFAULT_KEY_LEFT_2 'w'
#define PERSIST_CONFIG_DEFAULT_KEY_RIGHT_2 'q'
#define PERSIST_CONFIG_DEFAULT_KEY_LEFT_3 't'
#define PERSIST_CONFIG_DEFAULT_KEY_RIGHT_3 'y'
#define PERSIST_CONFIG_DEFAULT_KEY_LEFT_4 'o'
#define PERSIST_CONFIG_DEFAULT_KEY_RIGHT_4 'p'
#define PERSIST_PLAYER_NAME_MAX 32
typedef struct HighScore HighScore;
// Returns a newly allocated HighScore; caller must call highscore_destroy() to free it.
HighScore* highscore_create(const char* name, int score);
void highscore_destroy(HighScore* hs);
const char* highscore_get_name(const HighScore* hs);
int highscore_get_score(const HighScore* hs);
void highscore_set_name(HighScore* hs, const char* name);
void highscore_set_score(HighScore* hs, int score);
// Reads scores from `filename`. On success, returns number of scores and allocates an array
// of `HighScore*`s returned via `*out_scores`; caller must call `persist_free_scores(*out_scores, <returned count>)`.
int persist_read_scores(const char* filename, HighScore*** out_scores);
bool persist_write_scores(const char* filename, HighScore** scores, int count);
bool persist_append_score(const char* filename, const char* name, int score);
void persist_free_scores(HighScore** scores, int count);
typedef struct GameConfig GameConfig;
// Returns a newly allocated GameConfig; caller must call game_config_destroy() to free it.
GameConfig* game_config_create(void);
void game_config_destroy(GameConfig* cfg);
void game_config_set_board_size(GameConfig* cfg, int w, int h);
void game_config_get_board_size(const GameConfig* cfg, int* w_out, int* h_out);
void game_config_set_tick_rate_ms(GameConfig* cfg, int ms);
int game_config_get_tick_rate_ms(const GameConfig* cfg);
void game_config_set_screen_size(GameConfig* cfg, int w, int h);
void game_config_get_screen_size(const GameConfig* cfg, int* w_out, int* h_out);
void game_config_set_seed(GameConfig* cfg, uint32_t seed);
uint32_t game_config_get_seed(const GameConfig* cfg);
void game_config_set_fov_degrees(GameConfig* cfg, float fov);
float game_config_get_fov_degrees(const GameConfig* cfg);
void game_config_set_player_name(GameConfig* cfg, const char* name);
const char* game_config_get_player_name(const GameConfig* cfg);
void game_config_set_render_glyphs(GameConfig* cfg, int v);
int game_config_get_render_glyphs(const GameConfig* cfg);
void game_config_set_show_sprite_debug(GameConfig* cfg, int v);
int game_config_get_show_sprite_debug(const GameConfig* cfg);
void game_config_set_num_players(GameConfig* cfg, int n);
int game_config_get_num_players(const GameConfig* cfg);
void game_config_set_max_players(GameConfig* cfg, int n);
int game_config_get_max_players(const GameConfig* cfg);
void game_config_set_max_length(GameConfig* cfg, int v);
int game_config_get_max_length(const GameConfig* cfg);
void game_config_set_max_food(GameConfig* cfg, int v);
int game_config_get_max_food(const GameConfig* cfg);
void game_config_set_wall_height_scale(GameConfig* cfg, float v);
float game_config_get_wall_height_scale(const GameConfig* cfg);
void game_config_set_tail_height_scale(GameConfig* cfg, float v);
float game_config_get_tail_height_scale(const GameConfig* cfg);
void game_config_set_wall_texture(GameConfig* cfg, const char* path);
const char* game_config_get_wall_texture(const GameConfig* cfg);
void game_config_set_floor_texture(GameConfig* cfg, const char* path);
const char* game_config_get_floor_texture(const GameConfig* cfg);
void game_config_set_wall_texture_scale(GameConfig* cfg, float v);
float game_config_get_wall_texture_scale(const GameConfig* cfg);
void game_config_set_floor_texture_scale(GameConfig* cfg, float v);
float game_config_get_floor_texture_scale(const GameConfig* cfg);
void game_config_set_key_quit(GameConfig* cfg, char c);
char game_config_get_key_quit(const GameConfig* cfg);
void game_config_set_key_restart(GameConfig* cfg, char c);
char game_config_get_key_restart(const GameConfig* cfg);
void game_config_set_key_pause(GameConfig* cfg, char c);
char game_config_get_key_pause(const GameConfig* cfg);

/* Per-player name & color */
void game_config_set_player_name_for(GameConfig* cfg, int player_idx, const char* name);
const char* game_config_get_player_name_for(const GameConfig* cfg, int player_idx);
void game_config_set_player_color(GameConfig* cfg, int player_idx, uint32_t color);
uint32_t game_config_get_player_color(const GameConfig* cfg, int player_idx);

/* Per-player key bindings (player index 0..SNAKE_MAX_PLAYERS-1) */
void game_config_set_player_key_left(GameConfig* cfg, int player_idx, char c);
char game_config_get_player_key_left(const GameConfig* cfg, int player_idx);
void game_config_set_player_key_right(GameConfig* cfg, int player_idx, char c);
char game_config_get_player_key_right(const GameConfig* cfg, int player_idx);

void game_config_set_enable_external_3d_view(GameConfig* cfg, int v);
int game_config_get_enable_external_3d_view(const GameConfig* cfg);
void game_config_set_active_player(GameConfig* cfg, int v);
int game_config_get_active_player(const GameConfig* cfg);

/* Multiplayer configuration */
#define PERSIST_MP_IDENTIFIER_MAX 37
#define PERSIST_MP_HOST_MAX 128
#define PERSIST_MP_SESSION_MAX 16

void game_config_set_mp_enabled(GameConfig* cfg, int v);
int game_config_get_mp_enabled(const GameConfig* cfg);
void game_config_set_mp_server(GameConfig* cfg, const char* host, int port);
const char* game_config_get_mp_server_host(const GameConfig* cfg);
int game_config_get_mp_server_port(const GameConfig* cfg);
void game_config_set_mp_identifier(GameConfig* cfg, const char* id);
const char* game_config_get_mp_identifier(const GameConfig* cfg);

/* Optional: force-join a specific session (e.g. set in config) */
void game_config_set_mp_session(GameConfig* cfg, const char* session);
const char* game_config_get_mp_session(const GameConfig* cfg);

/* Headless mode: run without TTY/SDL graphics, print state to stdout */
void game_config_set_headless(GameConfig* cfg, int v);
int game_config_get_headless(const GameConfig* cfg);

bool persist_load_config(const char* filename, GameConfig** out_config);
bool persist_write_config(const char* filename, const GameConfig* config);
bool persist_config_has_unknown_keys(const char* filename);
