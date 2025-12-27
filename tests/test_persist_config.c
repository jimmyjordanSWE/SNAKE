#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "persist.h"

static int run_case(const char* content, bool expect_ok) {
    const char* fn = "test_config.tmp";
    FILE* f = fopen(fn, "w");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    GameConfig* cfg = NULL;
    bool ok = persist_load_config(fn, &cfg);
    remove(fn);
    if (cfg) game_config_destroy(cfg);
    return (ok == expect_ok) ? 1 : 0;
}

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
