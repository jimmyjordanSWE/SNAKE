#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "snake/persist.h"
#include <math.h>

static char* make_temp_file(void) {
    char* template = strdup("/tmp/snake_test_scores_XXXXXX");
    int fd = mkstemp(template);
    if (fd >= 0) close(fd);
    return template;
}

int main(void) {
    char* fname = make_temp_file();
    HighScore scores[3] = {{"alice", 10}, {"bob", 5}, {"carol", 7}};
    assert(persist_write_scores(fname, scores, 3));
    HighScore out[5];
    int cnt = persist_read_scores(fname, out, 5);
    assert(cnt == 3);
    assert(strcmp(out[0].name, "alice") == 0);
    
    assert(persist_append_score(fname, "dave", 20));
    cnt = persist_read_scores(fname, out, 5);
    assert(cnt >= 1);

    
    GameConfig cfg = {
        .board_width = 30,
        .board_height = 15,
        .tick_rate_ms = 50,
        .render_glyphs = 1,
        .screen_width = 80,
        .screen_height = 25,
        .enable_external_3d_view = 1,
        .seed = 12345,
        .fov_degrees = 70.5f,
        
        .show_sprite_debug = 1,
        .active_player = 0,
        .num_players = 2,
        .max_players = 4,
        .max_length = 128,
        .max_food = 4,
        .player_name = "tester",
        .wall_height_scale = 1.25f,
        .wall_texture = "assets/wall_custom.png",
        .floor_texture = "assets/floor_custom.png",
        .key_up = 'i',
        .key_down = 'k',
        .key_left = 'j',
        .key_right = 'l',
        
        .key_quit = 'x',
        .key_restart = 'n',
        .key_pause = 'm',
    };
    char* cfgfile = make_temp_file();
    assert(persist_write_config(cfgfile, &cfg));
    GameConfig loaded;
    assert(persist_load_config(cfgfile, &loaded));
    assert(loaded.board_width == 30 && loaded.board_height == 15);
    assert(loaded.seed == 12345);
    assert(fabsf(loaded.fov_degrees - 70.5f) < 0.01f);
    assert(loaded.show_sprite_debug == 1);
    assert(loaded.num_players == 2);
    assert(strcmp(loaded.player_name, "tester") == 0);
    assert(fabsf(loaded.wall_height_scale - 1.25f) < 0.001f);
    assert(strcmp(loaded.wall_texture, "assets/wall_custom.png") == 0);
    assert(strcmp(loaded.floor_texture, "assets/floor_custom.png") == 0);
    assert(loaded.key_up == 'i' && loaded.key_down == 'k');

    unlink(fname); free(fname);
    unlink(cfgfile); free(cfgfile);
    return 0;
}
