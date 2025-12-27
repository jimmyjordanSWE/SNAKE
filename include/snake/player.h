#pragma once
#include <stdint.h>
#include <stdbool.h>

// Player configuration and lightweight API for per-player settings.
// Ownership: callers allocate/destroy PlayerCfg via provided functions.
typedef struct PlayerCfg PlayerCfg;

// Create/destroy cfg
PlayerCfg* player_cfg_create(void);
void player_cfg_destroy(PlayerCfg* cfg);

// Name (shown in legend), max length PERSIST_PLAYER_NAME_MAX
void player_cfg_set_name(PlayerCfg* cfg, const char* name);
const char* player_cfg_get_name(const PlayerCfg* cfg);

// Color as 0xAARRGGBB
void player_cfg_set_color(PlayerCfg* cfg, uint32_t color);
uint32_t player_cfg_get_color(const PlayerCfg* cfg);

// Key bindings: use char key for simplicity; 0 means unset.
void player_cfg_set_key_up(PlayerCfg* cfg, char c);
char player_cfg_get_key_up(const PlayerCfg* cfg);
void player_cfg_set_key_down(PlayerCfg* cfg, char c);
char player_cfg_get_key_down(const PlayerCfg* cfg);
void player_cfg_set_key_left(PlayerCfg* cfg, char c);
char player_cfg_get_key_left(const PlayerCfg* cfg);
char player_cfg_get_key_right(const PlayerCfg* cfg);
void player_cfg_set_key_right(PlayerCfg* cfg, char c);

// Convenience: default presets
void player_cfg_set_default_bindings_for_index(PlayerCfg* cfg, int idx);

