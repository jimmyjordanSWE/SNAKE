#include "console.h"
#include "persist.h"
#include "snakegame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    GameConfig* config = NULL;
    if (persist_load_config("snake_cfg.txt", &config)) {
        console_info("Loaded configuration from snake_cfg.txt\n");
    } else {
        console_info("No config file found; using defaults\n");
        config = game_config_create();
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
