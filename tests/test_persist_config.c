/* Legacy test moved to Unity: tests/unity/test_persist_config.c */
#include <stdio.h>
int main(void) { fprintf(stderr, "Deprecated: use tests/unity/test_persist_config.c\n"); return 77; }

int main(void) {
    /* well-formed */
    const char* good =
        "player_name=Alice\n"
        "board_width=30\n"
        "board_height=20\n"
        "wall_texture=/tmp/wall.png\n"
        "floor_texture=/tmp/floor.png\n";
    assert(run_case(good, true) == 1);

    /* very long player name -> should not overflow buffer and function should succeed */
    char longname[1024];
    for (int i = 0; i < (int)sizeof(longname)-2; ++i) longname[i] = 'X';
    longname[sizeof(longname)-2] = '\n';
    longname[sizeof(longname)-1] = '\0';
    int r = run_case(longname, true);
    assert(r == 1);

    /* malformed numeric -> should not crash, config returned but value left default */
    const char* badnum = "board_width=notanumber\n";
    assert(run_case(badnum, true) == 1);

    /* out of range -> should be clamped or ignored */
    const char* outofrange = "board_width=100000\n";
    assert(run_case(outofrange, true) == 1);

    /* missing file -> returns default config (ok == false but cfg created) */
    GameConfig* cfg = NULL;
    bool ok = persist_load_config("/path/that/does/not/exist.conf", &cfg);
    assert(cfg != NULL);
    game_config_destroy(cfg);
    (void)ok; /* ok expected to be false when filename NULL or missing */

    printf("test_persist_config: OK\n");
    return 0;
}
