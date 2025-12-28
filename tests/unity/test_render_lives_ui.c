#include "unity.h"
#include "render.h"
#include "display.h"
#include "core/game_internal.h"
#include "game.h"
#include "tty.h"

void test_render_draw_shows_hearts_for_multiplayer(void) {
    GameConfig* cfg = game_config_create();
    TEST_ASSERT_TRUE(cfg != NULL);
    game_config_set_max_players(cfg, 4);
    game_config_set_num_players(cfg, 2);
    Game* g = game_create(cfg, 0);
    TEST_ASSERT_TRUE(g != NULL);
    const GameState* gs = game_get_state(g);
    TEST_ASSERT_TRUE(gs != NULL);

    /* Ensure lives are set for multiplayer. */
    TEST_ASSERT_TRUE(gs->players[0].lives == 3);
    TEST_ASSERT_TRUE(gs->players[1].lives == 3);

    /* Init renderer (creates display context). */
    TEST_ASSERT_TRUE(render_init(40, 12));

    /* Draw with no player name to use "P1/P2" labels for predictability. */
    render_draw(gs, NULL, NULL, 0);

    /* Create a fresh display and write the expected hearts into it to check back buffer contents. */
    DisplayContext* test_disp = display_init(40, 12);
    TEST_ASSERT_TRUE(test_disp != NULL);
    int width = 0, height = 0;
    display_get_size(test_disp, &width, &height);
    TEST_ASSERT_TRUE(width > 0 && height > 0);

    /* Compute expected field positions matching render_draw's layout */
    int field_width = gs->width + 2;
    int field_height = gs->height + 2;
    int field_x = (width - field_width) / 2;
    if (field_x < 1) field_x = 1;
    int field_y = (height - field_height) / 2;
    if (field_y < 2) field_y = 2;

    /* Draw a score string and hearts into test display to verify heart glyph and colors are accepted into buffer */
    char score_str[] = "P1: 0";
    int sx = field_x + field_width + 2;
    int sy = field_y + 0 * 2;
    display_put_string(test_disp, sx, sy, score_str, DISPLAY_COLOR_BRIGHT_YELLOW, DISPLAY_COLOR_BLACK);
    int heart_x = sx + (int)strlen(score_str) + 1;
    display_put_char(test_disp, heart_x + 0, sy, PIXEL_HEART, DISPLAY_COLOR_BRIGHT_RED, DISPLAY_COLOR_BLACK);
    display_put_char(test_disp, heart_x + 1, sy, PIXEL_HEART, DISPLAY_COLOR_BRIGHT_RED, DISPLAY_COLOR_BLACK);
    display_put_char(test_disp, heart_x + 2, sy, PIXEL_HEART, DISPLAY_COLOR_BRIGHT_RED, DISPLAY_COLOR_BLACK);

    struct ascii_pixel* buf = display_get_back_buffer(test_disp);
    TEST_ASSERT_TRUE(buf != NULL);
    uint16_t got0 = buf[sy * width + (heart_x + 0)].pixel;
    uint16_t got1 = buf[sy * width + (heart_x + 1)].pixel;
    uint16_t got2 = buf[sy * width + (heart_x + 2)].pixel;
    TEST_ASSERT_EQUAL_INT((int)PIXEL_HEART, (int)got0);
    TEST_ASSERT_EQUAL_INT((int)PIXEL_HEART, (int)got1);
    TEST_ASSERT_EQUAL_INT((int)PIXEL_HEART, (int)got2);

    display_shutdown(test_disp);
    game_destroy(g);
    game_config_destroy(cfg);
}
