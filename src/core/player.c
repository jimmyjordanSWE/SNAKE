#include "player.h"
#include <stdlib.h>
#include <string.h>
#include "persist.h"

struct PlayerCfg {
    char name[PERSIST_PLAYER_NAME_MAX];
    uint32_t color;
    char key_up, key_down, key_left, key_right;
    char key_quit, key_restart, key_pause;
};

PlayerCfg* player_cfg_create(void) {
    PlayerCfg* p = calloc(1, sizeof(*p));
    if (!p) return NULL;
    p->color = 0xFF008000; // default green
    player_cfg_set_default_bindings_for_index(p, 0);
    return p;
}

void player_cfg_destroy(PlayerCfg* cfg) {
    if (!cfg) return;
    free(cfg);
}

void player_cfg_set_name(PlayerCfg* cfg, const char* name) {
    if (!cfg) return;
    if (!name) { cfg->name[0] = '\0'; return; }
    strncpy(cfg->name, name, sizeof(cfg->name) - 1);
    cfg->name[sizeof(cfg->name) - 1] = '\0';
}

const char* player_cfg_get_name(const PlayerCfg* cfg) {
    if (!cfg) return NULL;
    return cfg->name[0] ? cfg->name : "";
}

void player_cfg_set_color(PlayerCfg* cfg, uint32_t color) {
    if (!cfg) return;
    cfg->color = color;
}

uint32_t player_cfg_get_color(const PlayerCfg* cfg) {
    if (!cfg) return 0;
    return cfg->color;
}

void player_cfg_set_key_up(PlayerCfg* cfg, char c) { if (cfg) cfg->key_up = c; }
char player_cfg_get_key_up(const PlayerCfg* cfg) { return cfg ? cfg->key_up : 0; }
void player_cfg_set_key_down(PlayerCfg* cfg, char c) { if (cfg) cfg->key_down = c; }
char player_cfg_get_key_down(const PlayerCfg* cfg) { return cfg ? cfg->key_down : 0; }
void player_cfg_set_key_left(PlayerCfg* cfg, char c) { if (cfg) cfg->key_left = c; }
char player_cfg_get_key_left(const PlayerCfg* cfg) { return cfg ? cfg->key_left : 0; }
void player_cfg_set_key_right(PlayerCfg* cfg, char c) { if (cfg) cfg->key_right = c; }
char player_cfg_get_key_right(const PlayerCfg* cfg) { return cfg ? cfg->key_right : 0; }

void player_cfg_set_default_bindings_for_index(PlayerCfg* cfg, int idx) {
    if (!cfg) return;
    // Common ergonomic defaults per index
    switch (idx) {
    case 0:
        cfg->key_up = 'w'; cfg->key_down = 's'; cfg->key_left = 'a'; cfg->key_right = 'd';
        break;
    case 1:
        cfg->key_up = 'i'; cfg->key_down = 'k'; cfg->key_left = 'j'; cfg->key_right = 'l';
        break;
    case 2:
        cfg->key_up = '8'; cfg->key_down = '5'; cfg->key_left = '4'; cfg->key_right = '6'; // numpad-like
        break;
    case 3:
        cfg->key_up = 't'; cfg->key_down = 'g'; cfg->key_left = 'f'; cfg->key_right = 'h';
        break;
    default:
        cfg->key_up = 'w'; cfg->key_down = 's'; cfg->key_left = 'a'; cfg->key_right = 'd';
        break;
    }
    cfg->key_quit = 'q'; cfg->key_restart = 'r'; cfg->key_pause = 'p';
}

