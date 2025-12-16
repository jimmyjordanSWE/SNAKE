#include "snake/render_3d.h"
#include "snake/render_3d_sdl.h"
#include "snake/game_internal.h"
#include "unity.h"
#include <stdlib.h>

static void test_minimap_head_drawn_after_body(void) {
    SDL3DContext* ctx = render_3d_sdl_create(200, 200);
    TEST_ASSERT_TRUE_MSG(ctx != NULL, "SDL3DContext should be created");

    GameState gs = {0};
    gs.width = 5; gs.height = 5; gs.num_players = 1; gs.food_count = 0;
    gs.players = (PlayerState*)calloc(1, sizeof(PlayerState));
    TEST_ASSERT_TRUE_MSG(gs.players != NULL, "players alloc");

    PlayerState* p = &gs.players[0];
    p->active = true;
    p->length = 3;
    p->body = (SnakePoint*)calloc((size_t)p->length, sizeof(SnakePoint));
    TEST_ASSERT_TRUE_MSG(p->body != NULL, "body alloc");

    /* Head and next body segment occupy the same cell to reproduce overlap */
    p->body[0].x = 2; p->body[0].y = 2;
    p->body[1].x = 2; p->body[1].y = 2; /* overlapping */
    p->body[2].x = 1; p->body[2].y = 2;
    p->prev_head_x = (float)p->body[0].x + 0.5f;
    p->prev_head_y = (float)p->body[0].y + 0.5f;
    p->current_dir = SNAKE_DIR_RIGHT;

    /* Render minimap into test context */
    render_3d_draw_minimap_into(ctx, &gs);

    int cell_px = render_3d_compute_minimap_cell_px(render_3d_sdl_get_width(ctx), render_3d_sdl_get_height(ctx), gs.width, gs.height);
    int map_px_w = cell_px * gs.width;
    int map_px_h = cell_px * gs.height;
    int padding = 8;
    int x0 = render_3d_sdl_get_width(ctx) - padding - map_px_w;
    int y0 = render_3d_sdl_get_height(ctx) - padding - map_px_h;
    if(x0 < padding) x0 = padding;
    if(y0 < padding) y0 = padding;

    int hx = x0 + (int)(((float)p->body[0].x + 0.5f) * (float)cell_px + 0.5f);
    int hy = y0 + (int)(((float)p->body[0].y + 0.5f) * (float)cell_px + 0.5f);

    uint32_t* pix = render_3d_sdl_get_pixels(ctx);
    int w = render_3d_sdl_get_width(ctx);
    uint32_t pxcol = pix[hy * w + hx];

    uint32_t head_col = render_3d_sdl_color(0, 255, 0, 255);
    uint32_t tail_col = render_3d_sdl_color(128, 128, 128, 255);

    /* Ensure the head pixel shows the player's color, not the tail color */
    TEST_ASSERT_EQUAL_INT(head_col, pxcol);
    TEST_ASSERT_TRUE_MSG(pxcol != tail_col, "pixel should not be tail color");

    /* Cleanup */
    free(p->body);
    free(gs.players);
    render_3d_sdl_destroy(ctx);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_minimap_head_drawn_after_body);
    return UnityEnd();
}
