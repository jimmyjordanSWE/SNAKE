#include "console.h"
#include "persist.h"
#include "snakegame.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CONFIG_FILENAME "snake_cfg.txt"
int main(void) {
    GameConfig* config = NULL;
    bool loaded = persist_load_config(CONFIG_FILENAME, &config);
    if (loaded) {
        console_info("Loaded configuration from %s\n", CONFIG_FILENAME);
    } else {
        console_info("No config file '%s' found or failed to parse; using defaults\n", CONFIG_FILENAME);
    }
    int init_err = 0;
    SnakeGame* game = snake_game_new(config, &init_err);
    if (config)
        game_config_destroy(config);
    if (!game)
        return init_err;
    int rc = snake_game_run(game);
    snake_game_free(game);
    return rc;
}
