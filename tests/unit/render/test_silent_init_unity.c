#include "snake/render_3d.h"
#include "snake/game_internal.h"
#include "unity.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void test_render_3d_init_is_silent(void) {
    GameState gs = {0};
    gs.width = 40; gs.height = 20; gs.num_players = 1;

    Render3DConfig cfg = {0};
    cfg.screen_width = 200;
    cfg.screen_height = 150;

    /* capture stderr to a temporary file */
    char tmpl[] = "/tmp/snake_test_stderr.XXXXXX";
    int fd = mkstemp(tmpl);
    TEST_ASSERT_TRUE_MSG(fd >= 0, "mkstemp failed");

    int saved = dup(STDERR_FILENO);
    TEST_ASSERT_TRUE_MSG(saved >= 0, "dup failed");
    TEST_ASSERT_TRUE_MSG(dup2(fd, STDERR_FILENO) >= 0, "dup2 failed");
    close(fd);

    /* init + shutdown while stderr redirected */
    TEST_ASSERT_TRUE_MSG(render_3d_init(&gs, &cfg), "render_3d_init failed");
    render_3d_shutdown();

    fflush(stderr);

    /* restore stderr */
    TEST_ASSERT_TRUE_MSG(dup2(saved, STDERR_FILENO) >= 0, "dup2 restore failed");
    close(saved);

    /* read captured output */
    FILE* f = fopen(tmpl, "r");
    TEST_ASSERT_TRUE_MSG(f != NULL, "failed to open tmp stderr file");
    char buf[1024*4] = {0};
    size_t r = fread(buf, 1, sizeof(buf) - 1, f);
    (void)r;
    fclose(f);
    unlink(tmpl);

    TEST_ASSERT_TRUE_MSG(strstr(buf, "render_3d_init: requested size") == NULL, "unexpected verbose render_3d_init message on stderr");
    TEST_ASSERT_TRUE_MSG(strstr(buf, "texture_load_from_file: loaded") == NULL, "unexpected texture load message on stderr");
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_render_3d_init_is_silent);
    return UnityEnd();
}
