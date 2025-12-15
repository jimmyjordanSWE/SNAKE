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
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 30, 15);
    game_config_set_tick_rate_ms(cfg, 50);
    game_config_set_render_glyphs(cfg, 1);
    game_config_set_screen_size(cfg, 80, 25);
    game_config_set_enable_external_3d_view(cfg, 1);
    game_config_set_seed(cfg, 12345);
    game_config_set_fov_degrees(cfg, 70.5f);
    game_config_set_show_sprite_debug(cfg, 1);
    game_config_set_active_player(cfg, 0);
    game_config_set_num_players(cfg, 2);
    game_config_set_player_name(cfg, "tester");
    game_config_set_wall_height_scale(cfg, 1.25f);
    game_config_set_wall_texture(cfg, "assets/wall_custom.png");
    game_config_set_floor_texture(cfg, "assets/floor_custom.png");
    game_config_set_key_up(cfg, 'i'); game_config_set_key_down(cfg, 'k'); game_config_set_key_left(cfg, 'j'); game_config_set_key_right(cfg, 'l');
    game_config_set_key_quit(cfg, 'x'); game_config_set_key_restart(cfg, 'n'); game_config_set_key_pause(cfg, 'm');

    char* cfgfile = make_temp_file();
    assert(persist_write_config(cfgfile, cfg));

    struct stat st_before;
    assert(stat(cfgfile, &st_before) == 0);

    
    assert(persist_write_config(cfgfile, cfg));

    struct stat st_after;
    assert(stat(cfgfile, &st_after) == 0);

    
    assert(st_before.st_mtime == st_after.st_mtime);

    unlink(cfgfile); free(cfgfile);
    game_config_destroy(cfg);
    return 0;
}
