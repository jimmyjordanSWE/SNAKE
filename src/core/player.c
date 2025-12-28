#include "player.h"
#include "persist.h"
#include <stdlib.h>
#include <string.h>
struct PlayerCfg {
    char name[PERSIST_PLAYER_NAME_MAX];
    uint32_t color;
    char key_up, key_down, key_left, key_right;
    char key_quit, key_restart, key_pause;
};
PlayerCfg* player_cfg_create(void) {
    PlayerCfg* p = calloc(1, sizeof(*p));
    if (!p)
        return NULL;
    p->color = 0xFF008000; // default green
    player_cfg_set_default_bindings_for_index(p, 0);
    return p;
}
void player_cfg_destroy(PlayerCfg* cfg) {
    if (!cfg)
        return;
    free(cfg);
}
void player_cfg_set_name(PlayerCfg* cfg, const char* name) {
    if (!cfg)
        return;
    if (!name) {
        cfg->name[0] = '\0';
        return;
    }
    strncpy(cfg->name, name, sizeof(cfg->name) - 1);
    cfg->name[sizeof(cfg->name) - 1] = '\0';
}
const char* player_cfg_get_name(const PlayerCfg* cfg) {
    if (!cfg)
        return NULL;
    return cfg->name[0] ? cfg->name : "";
}
void player_cfg_set_color(PlayerCfg* cfg, uint32_t color) {
    if (!cfg)
        return;
    cfg->color = color;
}
uint32_t player_cfg_get_color(const PlayerCfg* cfg) {
    if (!cfg)
        return 0;
    return cfg->color;
}
void player_cfg_set_key_up(PlayerCfg* cfg, char c) {
    if (cfg)
        cfg->key_up = c;
}
char player_cfg_get_key_up(const PlayerCfg* cfg) {
    return cfg ? cfg->key_up : 0;
}
void player_cfg_set_key_down(PlayerCfg* cfg, char c) {
    if (cfg)
        cfg->key_down = c;
}
char player_cfg_get_key_down(const PlayerCfg* cfg) {
    return cfg ? cfg->key_down : 0;
}
void player_cfg_set_key_left(PlayerCfg* cfg, char c) {
    if (cfg)
        cfg->key_left = c;
}
char player_cfg_get_key_left(const PlayerCfg* cfg) {
    return cfg ? cfg->key_left : 0;
}
void player_cfg_set_key_right(PlayerCfg* cfg, char c) {
    if (cfg)
        cfg->key_right = c;
}
char player_cfg_get_key_right(const PlayerCfg* cfg) {
    return cfg ? cfg->key_right : 0;
}
void player_cfg_set_default_bindings_for_index(PlayerCfg* cfg, int idx) {
    if (!cfg)
        return;
    // Common ergonomic defaults per index
    switch (idx) {
    case 0:
        /* Player 1: arrow keys act as left/right; no single-char defaults */
        cfg->key_up = '\0';
        cfg->key_down = '\0';
        cfg->key_left = '\0';
        cfg->key_right = '\0';
        break;
    case 1:
        /* Player 2: left='w', right='q' */
        cfg->key_up = '\0';
        cfg->key_down = '\0';
        cfg->key_left = 'w';
        cfg->key_right = 'q';
        break;
    case 2:
        /* Player 3: left='t', right='y' */
        cfg->key_up = '\0';
        cfg->key_down = '\0';
        cfg->key_left = 't';
        cfg->key_right = 'y';
        break;
    case 3:
        /* Player 4: left='o', right='p' */
        cfg->key_up = '\0';
        cfg->key_down = '\0';
        cfg->key_left = 'o';
        cfg->key_right = 'p';
        break;
    default:
        cfg->key_up = 'w';
        cfg->key_down = 's';
        cfg->key_left = 'a';
        cfg->key_right = 'd';
        break;
    }
    cfg->key_quit = 'q';
    cfg->key_restart = 'r';
    cfg->key_pause = 'p';
}
