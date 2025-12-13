#include "snake/render.h"
#include "snake/persist.h"
#include "snake/tty.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static tty_context* g_tty = NULL;
static int last_score_count = -1;
static HighScore last_scores[PERSIST_MAX_SCORES];

static int compare_highscores_desc(const void* a, const void* b) {
    const HighScore* sa = (const HighScore*)a;
    const HighScore* sb = (const HighScore*)b;
    if (sa->score > sb->score) { return -1; }
    if (sa->score < sb->score) { return 1; }
    return strcmp(sa->name, sb->name);
}

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

    /* Draw high scores (top 5 only) */
    HighScore highscores[PERSIST_MAX_SCORES];
    int score_count = persist_read_scores(".snake_scores", highscores, PERSIST_MAX_SCORES);

    /* Merge persisted highscores with live player scores (preview while playing) */
    HighScore display_scores[PERSIST_MAX_SCORES + SNAKE_MAX_PLAYERS];
    int display_count = 0;

    for (int i = 0; i < score_count && display_count < (int)(sizeof(display_scores) / sizeof(display_scores[0])); i++) {
        display_scores[display_count++] = highscores[i];
    }

    for (int p = 0; p < game->num_players && display_count < (int)(sizeof(display_scores) / sizeof(display_scores[0])); p++) {
        if (!game->players[p].active) { continue; }
        if (game->players[p].score <= 0) { continue; }

        HighScore live = {0};
        snprintf(live.name, sizeof(live.name), "P%d (live)", p + 1);
        live.score = game->players[p].score;
        display_scores[display_count++] = live;
    }

    if (display_count > 1) { qsort(display_scores, (size_t)display_count, sizeof(HighScore), compare_highscores_desc); }

    int max_display = (display_count > 5) ? 5 : display_count;

    /* Check if scores changed, force redraw if so */
    bool scores_changed = (score_count != last_score_count);
    if (!scores_changed && score_count > 0) {
        for (int i = 0; i < score_count; i++) {
            if (last_scores[i].score != highscores[i].score || strcmp(last_scores[i].name, highscores[i].name) != 0) {
                scores_changed = true;
                break;
            }
        }
    }

    if (scores_changed) {
        /* Cache current scores and force redraw */
        last_score_count = score_count;
        for (int i = 0; i < score_count; i++) { last_scores[i] = highscores[i]; }
        tty_force_redraw(g_tty);
    }

    int hiscore_y = field_y + 5;
    draw_string(field_x + field_width + 2, hiscore_y, "High Scores:", COLOR_WHITE);

    if (max_display > 0) {
        for (int i = 0; i < max_display && hiscore_y + i + 1 < tty_height; i++) {
            char hiscore_str[80];
            /* Safely format with truncation */
            int written = snprintf(hiscore_str, sizeof(hiscore_str), "%d. ", i + 1);
            if (written > 0 && written < (int)sizeof(hiscore_str)) {
                snprintf(hiscore_str + written, sizeof(hiscore_str) - (size_t)written, "%.15s: %d", display_scores[i].name, display_scores[i].score);
            }
            draw_string(field_x + field_width + 2, hiscore_y + i + 1, hiscore_str, COLOR_CYAN);
        }
    } else {
        draw_string(field_x + field_width + 2, hiscore_y + 1, "(none yet)", COLOR_CYAN);
    }

    /* Draw help text at bottom */
    if (tty_height > field_y + field_height + 1) { draw_string(1, field_y + field_height + 1, "Q: Quit", COLOR_WHITE); }

    tty_flip(g_tty);
}
