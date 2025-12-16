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
    HighScore* scores[3] = { highscore_create("alice", 10), highscore_create("bob", 5), highscore_create("carol", 7) };
    assert(persist_write_scores(fname, scores, 3));
    for(int i=0;i<3;i++) highscore_destroy(scores[i]);
    HighScore** out = NULL;
    int cnt = persist_read_scores(fname, &out);
    assert(cnt == 3);
    assert(strcmp(highscore_get_name(out[0]), "alice") == 0);
    persist_free_scores(out, cnt);

    assert(persist_append_score(fname, "dave", 20));
    out = NULL;
    cnt = persist_read_scores(fname, &out);
    assert(cnt >= 1);
    persist_free_scores(out, cnt);

    
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 30, 15);
    game_config_set_tick_rate_ms(cfg, 50);
    game_config_set_screen_size(cfg, 80, 25);
    game_config_set_seed(cfg, 12345);
    game_config_set_fov_degrees(cfg, 70.5f);
    game_config_set_show_sprite_debug(cfg, 1);
    game_config_set_num_players(cfg, 2);
    game_config_set_max_players(cfg, 4);
    game_config_set_max_length(cfg, 128);
    game_config_set_max_food(cfg, 4);
    game_config_set_player_name(cfg, "tester");
    game_config_set_wall_height_scale(cfg, 1.25f);
    game_config_set_wall_texture(cfg, "assets/wall_custom.png");
    game_config_set_floor_texture(cfg, "assets/floor_custom.png");
    game_config_set_key_up(cfg, 'i');
    game_config_set_key_down(cfg, 'k');
    game_config_set_key_left(cfg, 'j');
    game_config_set_key_right(cfg, 'l');
    game_config_set_key_quit(cfg, 'x');
    game_config_set_key_restart(cfg, 'n');
    game_config_set_key_pause(cfg, 'm');

    char* cfgfile = make_temp_file();
    assert(persist_write_config(cfgfile, cfg));
    
    {
        FILE* f = fopen(cfgfile, "r");
        if(f) {
            char buf[256];
            while(fgets(buf, sizeof(buf), f)) fputs(buf, stdout);
            fclose(f);
        }
    }
    GameConfig* loaded = NULL;
    assert(persist_load_config(cfgfile, &loaded));
    int bw, bh; game_config_get_board_size(loaded, &bw, &bh);
    assert(bw == 30 && bh == 15);
    assert(game_config_get_seed(loaded) == 12345);
    assert(fabsf(game_config_get_fov_degrees(loaded) - 70.5f) < 0.01f);
    assert(game_config_get_show_sprite_debug(loaded) == 1);
    assert(game_config_get_num_players(loaded) == 2);
    assert(strcmp(game_config_get_player_name(loaded), "tester") == 0);
    assert(fabsf(game_config_get_wall_height_scale(loaded) - 1.25f) < 0.001f);
    assert(strcmp(game_config_get_wall_texture(loaded), "assets/wall_custom.png") == 0);
    assert(strcmp(game_config_get_floor_texture(loaded), "assets/floor_custom.png") == 0);
    assert(game_config_get_key_up(loaded) == 'i' && game_config_get_key_down(loaded) == 'k');
    game_config_destroy(cfg);
    game_config_destroy(loaded);

    unlink(fname); free(fname);
    unlink(cfgfile); free(cfgfile);
    return 0;
}
