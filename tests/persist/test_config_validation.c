#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "snake/persist.h"

static char* make_temp_file(void) {
    char* template = strdup("/tmp/snake_test_cfg_XXXXXX");
    int fd = mkstemp(template);
    if (fd >= 0) close(fd);
    return template;
}

int main(void) {
    /* Test clamping: values below min should be clamped */
    char* f1 = make_temp_file();
    FILE* f = fopen(f1, "w");
    fprintf(f, "board_width=5\n");
    fprintf(f, "board_height=5\n");
    fprintf(f, "tick_rate_ms=1\n");
    fprintf(f, "screen_width=1\n");
    fprintf(f, "screen_height=1\n");
    fclose(f);
    GameConfig* cfg = NULL;
    assert(persist_load_config(f1, &cfg) == true);
    int bw, bh, sw, sh;
    game_config_get_board_size(cfg, &bw, &bh);
    game_config_get_screen_size(cfg, &sw, &sh);
    /* minimums in code: board_width >=20, board_height >=10, tick_rate_ms >=10, screen_width>=20 screen_height>=10 */
    assert(bw >= 20 && bh >= 10);
    assert(game_config_get_tick_rate_ms(cfg) >= 10);
    assert(sw >= 20 && sh >= 10);
    game_config_destroy(cfg);

    /* Test clamping above max */
    char* f2 = make_temp_file();
    f = fopen(f2, "w");
    fprintf(f, "board_width=1000000\n");
    fprintf(f, "board_height=999999\n");
    fclose(f);
    cfg = NULL;
    assert(persist_load_config(f2, &cfg) == true);
    game_config_get_board_size(cfg, &bw, &bh);
    /* enforced upper bounds: board_width <=100 board_height <=50 */
    assert(bw <= 100 && bh <= 50);
    game_config_destroy(cfg);

    /* Test boolean parsing variants and glyphs parsing */
    char* f3 = make_temp_file();
    f = fopen(f3, "w");
    fprintf(f, "enable_external_3d_view=YeS\n");
    fprintf(f, "show_sprite_debug=0\n");
    fprintf(f, "render_glyphs=ascii\n");
    fclose(f);
    cfg = NULL;
    assert(persist_load_config(f3, &cfg) == true);
    assert(game_config_get_enable_external_3d_view(cfg) == 1);
    assert(game_config_get_show_sprite_debug(cfg) == 0);
    assert(game_config_get_render_glyphs(cfg) == 1); /* ascii -> 1 */
    game_config_destroy(cfg);

    /* Test unknown keys are ignored (no crash and valid keys still applied) */
    char* f4 = make_temp_file();
    f = fopen(f4, "w");
    fprintf(f, "unused_key=foobar\n");
    fprintf(f, "player_name=tester123\n");
    fclose(f);
    cfg = NULL;
    assert(persist_load_config(f4, &cfg) == true);
    assert(strcmp(game_config_get_player_name(cfg), "tester123") == 0);
    game_config_destroy(cfg);

    /* Test missing file behavior: returns defaults and false */
    GameConfig* defcfg = NULL;
    assert(persist_load_config(NULL, &defcfg) == false);
    /* default values should be sane */
    game_config_get_board_size(defcfg, &bw, &bh);
    assert(bw >= 20 && bh >= 10);
    game_config_destroy(defcfg);

    unlink(f1); free(f1);
    unlink(f2); free(f2);
    unlink(f3); free(f3);
    unlink(f4); free(f4);
    return 0;
}
