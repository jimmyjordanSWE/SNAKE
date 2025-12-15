#include "snake/console.h"
#include "snake/persist.h"
#include "snake/snakegame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
GameConfig config;
/* Prefer explicit project config file, fallback to legacy dotfile. */
if(persist_load_config("snake_cfg.txt", &config)) {
console_info("Loaded configuration from snake_cfg.txt\n");
} else if(persist_load_config(".snake_config", &config)) {
console_info("Loaded configuration from .snake_config\n");
} else {
console_info("No config file found; using defaults\n");
}
/* Delegate main loop and runtime lifecycle to snakegame module */
return snakegame_run(&config);
}
