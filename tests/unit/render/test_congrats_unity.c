#include "snake/render_3d.h"
#include "snake/game_internal.h" /* bring in full GameState definition for tests */
#include "unity.h"

static void test_render_3d_congrats_overlay(void) {
    /* Create a minimal fake game state */
    GameState gs = {0};
    gs.width = 40; gs.height = 20; gs.num_players = 1;

    Render3DConfig cfg = {0};
    cfg.screen_width = 200;
    cfg.screen_height = 150;

    /* init should succeed (it falls back on missing textures) */
    TEST_ASSERT_TRUE_MSG(render_3d_init(&gs, &cfg), "render_3d_init failed");

    /* Call congrats overlay with and without a name */
    render_3d_draw_congrats_overlay(123, NULL);
    render_3d_draw_congrats_overlay(456, "PLAYER");

    /* cleanup */
    render_3d_shutdown();
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_render_3d_congrats_overlay);
    return UnityEnd();
}
