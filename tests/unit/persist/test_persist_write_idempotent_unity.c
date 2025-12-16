#include "snake/persist.h"
#include "unity.h"
#include <stdlib.h>
#include <unistd.h>

static void test_write_config_idempotent(void);

static char* make_temp_file(void) {
    char* template = strdup("/tmp/snake_test_cfgid_XXXXXX");
    int fd = mkstemp(template);
    if (fd >= 0) close(fd);
    return template;
}

void test_write_config_idempotent(void) {
    GameConfig* cfg = game_config_create();
    game_config_set_board_size(cfg, 25, 11);
    game_config_set_player_name(cfg, "idtest");
    char* fname = make_temp_file();
    TEST_ASSERT_TRUE_MSG(persist_write_config(fname, cfg), "first write should succeed");
    
    TEST_ASSERT_TRUE_MSG(persist_write_config(fname, cfg), "second write (idempotent) should succeed");
    unlink(fname); free(fname);
    game_config_destroy(cfg);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_write_config_idempotent);
    return UnityEnd();
}
