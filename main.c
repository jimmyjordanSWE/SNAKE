#include "console.h"
#include "persist.h"
#include "snakegame.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CONFIG_FILENAME "snake_cfg.txt"
int main(int argc, char** argv) {
    const char* config_file = CONFIG_FILENAME;
    if (argc > 1 && argv[1] && argv[1][0] != '\0') {
        config_file = argv[1];
    }
    GameConfig* config = NULL;
    bool loaded = persist_load_config(config_file, &config);
    if (loaded) {
        console_info("Loaded configuration from %s\n", config_file);
    } else {
        console_info("No config file '%s' found or failed to parse; using defaults\n", config_file);
    }
    /* Initialize network logging early so the file is created by default */
    extern void net_log_init(void);
    net_log_init();

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
