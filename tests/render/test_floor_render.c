#include "snake/render_3d.h"
#include "snake/game_internal.h" /* bring in full GameState definition for tests */
#include "snake/render_3d_sdl.h"
#include "unity.h"

static void test_render_3d_floor_textured(void) {
    GameState gs = {0};
    gs.width = 16; gs.height = 16; gs.num_players = 1;

    Render3DConfig cfg = {0};
    cfg.screen_width = 160;
    cfg.screen_height = 120;
    snprintf(cfg.floor_texture_path, sizeof(cfg.floor_texture_path), "%s", "assets/floor.png");

    TEST_ASSERT_TRUE_MSG(render_3d_init(&gs, &cfg), "render_3d_init failed");

    /* Render one frame */
    render_3d_draw(&gs, NULL, NULL, 0, 0.016f);

    struct SDL3DContext* d = render_3d_get_display();
    TEST_ASSERT_NOT_NULL(d);
    uint32_t* pix = render_3d_sdl_get_pixels(d);
    TEST_ASSERT_NOT_NULL(pix);

    int w = render_3d_sdl_get_width(d);
    int h = render_3d_sdl_get_height(d);

    uint32_t flat_floor = 0xFF8B4513;
    bool found_textured = false;
    /* Scan lower half of screen for any pixel different from flat floor color */
    for(int yy = h/2; yy < h && !found_textured; yy++) {
        for(int xx = 0; xx < w; xx++) {
            uint32_t c = pix[yy * w + xx];
            if(c != flat_floor) { found_textured = true; break; }
        }
    }

    TEST_ASSERT_TRUE_MSG(found_textured, "No textured floor pixels found; floor may not be textured or assets/floor.png missing");

    render_3d_shutdown();
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_render_3d_floor_textured);
    return UnityEnd();
}
