#include "snake/render.h"
#include "snake/tty.h"

#include <stdio.h>
#include <string.h>

static tty_context* g_tty = NULL;

bool render_init(int min_width, int min_height) {
    /* Ensure minimum size is reasonable */
    if (min_width < 20) { min_width = 20; }
    if (min_height < 10) { min_height = 10; }

    g_tty = tty_open(NULL, min_width, min_height);
    return g_tty != NULL && tty_size_valid(g_tty);
}

void render_shutdown(void) {
    if (g_tty) {
        tty_close(g_tty);
        g_tty = NULL;
    }
}

static void draw_box(int x, int y, int width, int height, uint16_t fg_color) {
    if (!g_tty) { return; }

    int tty_width = 0, tty_height = 0;
    tty_get_size(g_tty, &tty_width, &tty_height);

    /* Top-left corner */
    if (x >= 0 && y >= 0 && x < tty_width && y < tty_height) { tty_put_pixel(g_tty, x, y, PIXEL_MAKE(PIXEL_BOX_TL, fg_color, COLOR_BLACK)); }

    /* Top-right corner */
    if (x + width - 1 >= 0 && y >= 0 && x + width - 1 < tty_width && y < tty_height) {
        tty_put_pixel(g_tty, x + width - 1, y, PIXEL_MAKE(PIXEL_BOX_TR, fg_color, COLOR_BLACK));
    }

    /* Bottom-left corner */
    if (x >= 0 && y + height - 1 >= 0 && x < tty_width && y + height - 1 < tty_height) {
        tty_put_pixel(g_tty, x, y + height - 1, PIXEL_MAKE(PIXEL_BOX_BL, fg_color, COLOR_BLACK));
    }

    /* Bottom-right corner */
    if (x + width - 1 >= 0 && y + height - 1 >= 0 && x + width - 1 < tty_width && y + height - 1 < tty_height) {
        tty_put_pixel(g_tty, x + width - 1, y + height - 1, PIXEL_MAKE(PIXEL_BOX_BR, fg_color, COLOR_BLACK));
    }

    /* Top and bottom edges */
    for (int i = 1; i < width - 1; i++) {
        if (x + i >= 0 && x + i < tty_width) {
            if (y >= 0 && y < tty_height) { tty_put_pixel(g_tty, x + i, y, PIXEL_MAKE(PIXEL_BOX_H, fg_color, COLOR_BLACK)); }
            if (y + height - 1 >= 0 && y + height - 1 < tty_height) {
                tty_put_pixel(g_tty, x + i, y + height - 1, PIXEL_MAKE(PIXEL_BOX_H, fg_color, COLOR_BLACK));
            }
        }
    }

    /* Left and right edges */
    for (int i = 1; i < height - 1; i++) {
        if (y + i >= 0 && y + i < tty_height) {
            if (x >= 0 && x < tty_width) { tty_put_pixel(g_tty, x, y + i, PIXEL_MAKE(PIXEL_BOX_V, fg_color, COLOR_BLACK)); }
            if (x + width - 1 >= 0 && x + width - 1 < tty_width) { tty_put_pixel(g_tty, x + width - 1, y + i, PIXEL_MAKE(PIXEL_BOX_V, fg_color, COLOR_BLACK)); }
        }
    }
}

static void draw_string(int x, int y, const char* str, uint16_t fg_color) {
    if (!g_tty || !str) { return; }

    int tty_width = 0, tty_height = 0;
    tty_get_size(g_tty, &tty_width, &tty_height);

    for (int i = 0; str[i] && x + i < tty_width && y >= 0 && y < tty_height; i++) { tty_put_pixel(g_tty, x + i, y, PIXEL_MAKE(str[i], fg_color, COLOR_BLACK)); }
}

void render_draw(const GameState* game) {
    if (!g_tty || !game) { return; }

    tty_clear_back(g_tty);

    int tty_width = 0, tty_height = 0;
    tty_get_size(g_tty, &tty_width, &tty_height);

    /* Draw playfield box */
    int field_x = 1;
    int field_y = 2;
    int field_width = game->width + 2;
    int field_height = game->height + 2;

    draw_box(field_x, field_y, field_width, field_height, COLOR_CYAN);

    /* Draw food */
    if (game->food_present) { tty_put_pixel(g_tty, field_x + 1 + game->food.x, field_y + 1 + game->food.y, PIXEL_MAKE('*', COLOR_GREEN, COLOR_BLACK)); }

    /* Draw snakes */
    for (int p = 0; p < game->num_players; p++) {
        const PlayerState* player = &game->players[p];
        if (!player->active) { continue; }

        uint16_t snake_color = (p == 0) ? COLOR_BRIGHT_YELLOW : COLOR_BRIGHT_RED;

        for (int i = 0; i < player->length; i++) {
            char ch = (i == 0) ? '@' : 'o';
            tty_put_pixel(g_tty, field_x + 1 + player->body[i].x, field_y + 1 + player->body[i].y, PIXEL_MAKE(ch, snake_color, COLOR_BLACK));
        }
    }

    /* Draw HUD */
    char status_str[64];
    const char* status_name = "UNKNOWN";
    switch (game->status) {
    case GAME_STATUS_RUNNING:
        status_name = "RUNNING";
        break;
    case GAME_STATUS_PAUSED:
        status_name = "PAUSED";
        break;
    case GAME_STATUS_GAME_OVER:
        status_name = "GAME OVER";
        break;
    }

    snprintf(status_str, sizeof(status_str), "Status: %s", status_name);
    draw_string(1, 0, status_str, COLOR_WHITE);

    /* Draw player scores */
    for (int p = 0; p < game->num_players; p++) {
        if (game->players[p].active) {
            char score_str[32];
            snprintf(score_str, sizeof(score_str), "P%d: %d", p + 1, game->players[p].score);

            uint16_t color = (p == 0) ? COLOR_BRIGHT_YELLOW : COLOR_BRIGHT_RED;
            draw_string(field_x + field_width + 2, field_y + p * 2, score_str, color);
        }
    }

    /* Draw help text at bottom */
    if (tty_height > field_y + field_height + 1) { draw_string(1, field_y + field_height + 1, "Q: Quit", COLOR_WHITE); }

    tty_flip(g_tty);
}
