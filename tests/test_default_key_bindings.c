/* Legacy test moved to Unity: tests/unity/test_default_key_bindings.c */
#include <stdio.h>
int main(void) { fprintf(stderr, "Deprecated: use tests/unity/test_default_key_bindings.c\n"); return 77; }

    /* Per-player defaults (only check up to 4 players or SNAKE_MAX_PLAYERS) */
#ifdef PERSIST_CONFIG_DEFAULT_KEY_UP_2
    if (SNAKE_MAX_PLAYERS >= 2)
        assert(game_config_get_player_key_up(cfg, 1) == PERSIST_CONFIG_DEFAULT_KEY_UP_2);
#endif
#ifdef PERSIST_CONFIG_DEFAULT_KEY_UP_3
    if (SNAKE_MAX_PLAYERS >= 3)
        assert(game_config_get_player_key_up(cfg, 2) == PERSIST_CONFIG_DEFAULT_KEY_UP_3);
#endif
#ifdef PERSIST_CONFIG_DEFAULT_KEY_UP_4
    if (SNAKE_MAX_PLAYERS >= 4)
        assert(game_config_get_player_key_up(cfg, 3) == PERSIST_CONFIG_DEFAULT_KEY_UP_4);
#endif

    game_config_destroy(cfg);
    return 0;
}