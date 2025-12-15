#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "snake/persist.h"

static char* make_temp_file(void) {
    char* template = strdup("/tmp/snake_cfg_XXXXXX");
    int fd = mkstemp(template);
    if (fd >= 0) close(fd);
    return template;
}

int main(void) {
    GameConfig cfg = {0};
    cfg.board_width = 30;
    cfg.board_height = 15;
    cfg.tick_rate_ms = 50;
    cfg.render_glyphs = 1;
    cfg.screen_width = 80;
    cfg.screen_height = 25;
    cfg.enable_external_3d_view = 1;
    cfg.seed = 12345;
    cfg.fov_degrees = 70.5f;
    cfg.show_sprite_debug = 1;
    cfg.active_player = 0;
    cfg.num_players = 2;
    snprintf(cfg.player_name, sizeof(cfg.player_name), "tester");
    cfg.wall_height_scale = 1.25f;
    snprintf(cfg.wall_texture, sizeof(cfg.wall_texture), "assets/wall_custom.png");
    snprintf(cfg.floor_texture, sizeof(cfg.floor_texture), "assets/floor_custom.png");
    cfg.key_up = 'i'; cfg.key_down = 'k'; cfg.key_left = 'j'; cfg.key_right = 'l';
    cfg.key_quit = 'x'; cfg.key_restart = 'n'; cfg.key_pause = 'm';

    char* cfgfile = make_temp_file();
    assert(persist_write_config(cfgfile, &cfg));

    struct stat st_before;
    assert(stat(cfgfile, &st_before) == 0);

    /* write again with identical config */
    assert(persist_write_config(cfgfile, &cfg));

    struct stat st_after;
    assert(stat(cfgfile, &st_after) == 0);

    /* mtime should be unchanged when writing identical content */
    assert(st_before.st_mtime == st_after.st_mtime);

    unlink(cfgfile); free(cfgfile);
    return 0;
}
