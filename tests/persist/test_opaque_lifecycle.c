#include <assert.h>
#include <string.h>
#include "snake/persist.h"

int main(void) {
    
    HighScore* hs = highscore_create("x", 42);
    assert(hs != NULL);
    assert(strcmp(highscore_get_name(hs), "x") == 0);
    assert(highscore_get_score(hs) == 42);
    highscore_destroy(hs);

    
    highscore_destroy(NULL);

    
    GameConfig* cfg = game_config_create();
    assert(cfg != NULL);
    
    game_config_set_seed(cfg, 0xDEADBEEF);
    assert(game_config_get_seed(cfg) == 0xDEADBEEF);
    game_config_set_num_players(cfg, 1);
    assert(game_config_get_num_players(cfg) == 1);

    game_config_destroy(cfg);
    
    game_config_destroy(NULL);

    return 0;
}
