#include "unity.h"
#include "render_3d.h"
#include "render_3d_sdl.h"
#include "core/game_internal.h"
#include <stdlib.h>


void test_render_minimap_uses_player_color(void) {
    SDL3DContext* ctx = render_3d_sdl_create(200, 200);
    TEST_ASSERT_TRUE(ctx != NULL);
    GameState gs = {0};
    gs.width = 3;
    gs.height = 3;
    gs.num_players = 1;
    gs.max_players = 1;
    gs.players = calloc((size_t)gs.max_players, sizeof *gs.players);
    TEST_ASSERT_TRUE(gs.players != NULL);

    gs.players[0].active = true;
    gs.players[0].length = 1;
    gs.players[0].body = calloc((size_t)1, sizeof(SnakePoint));
    TEST_ASSERT_TRUE(gs.players[0].body != NULL);
    gs.players[0].body[0] = (SnakePoint){1, 1};
    gs.players[0].prev_head_x = 1.5f;
    gs.players[0].prev_head_y = 1.5f;

    uint32_t pcol = render_3d_sdl_color(10, 20, 30, 255);
    gs.players[0].color = pcol;

    render_3d_draw_minimap_into(ctx, &gs);

    uint32_t* pix = render_3d_sdl_get_pixels(ctx);
    TEST_ASSERT_TRUE(pix != NULL);

    int w = render_3d_sdl_get_width(ctx);
    int h = render_3d_sdl_get_height(ctx);
    int found = 0;
    for (int i = 0; i < w * h; i++) {
        if (pix[i] == pcol) {
            found++;
            break;
        }
    }

    /* Expect at least one pixel using the player's head color */
    TEST_ASSERT_TRUE(found > 0);

    free(gs.players[0].body);
    free(gs.players);
    render_3d_sdl_destroy(ctx);
}
