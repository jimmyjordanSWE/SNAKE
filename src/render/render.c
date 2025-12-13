#include "snake/render.h"
#include "snake/persist.h"
#include "snake/tty.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static tty_context* g_tty = NULL;
static int last_score_count = -1;
static HighScore last_scores[PERSIST_MAX_SCORES];

#define SESSION_MAX_SCORES 32
static HighScore g_session_scores[SESSION_MAX_SCORES];
static int g_session_score_count = 0;

static bool is_session_score(const HighScore* s) {
    if (!s) { return false; }

    for (int i = 0; i < g_session_score_count; i++) {
        if (g_session_scores[i].score == s->score && strcmp(g_session_scores[i].name, s->name) == 0) { return true; }
    }

    return false;
}

static uint16_t gradient_color_for_rank(int rank, int total) {
    /* A simple bright-to-dark gradient using ANSI 16-color palette. */
    static const uint16_t palette[] = {
        COLOR_BRIGHT_CYAN,
        COLOR_CYAN,
        COLOR_BLUE,
        COLOR_BRIGHT_BLACK,
    };

    int palette_len = (int)(sizeof(palette) / sizeof(palette[0]));
    if (rank < 0) { rank = 0; }
    if (total <= 1) { return palette[0]; }
    if (total < 0) { total = 0; }

    int idx = (rank * (palette_len - 1)) / (total - 1);
    if (idx < 0) { idx = 0; }
    if (idx >= palette_len) { idx = palette_len - 1; }
    return palette[idx];
}

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
    g_session_score_count = 0;
    if (g_tty == NULL) { return false; }
    if (!tty_size_valid(g_tty)) {
        tty_close(g_tty);
        g_tty = NULL;
        return false;
    }
    return true;
}

void render_shutdown(void) {
    if (g_tty) {
        tty_close(g_tty);
        g_tty = NULL;
    }

    g_session_score_count = 0;
}

void render_note_session_score(const char* name, int score) {
    if (!name || score <= 0) { return; }

    for (int i = 0; i < g_session_score_count; i++) {
        if (g_session_scores[i].score == score && strcmp(g_session_scores[i].name, name) == 0) { return; }
    }

    if (g_session_score_count >= SESSION_MAX_SCORES) { return; }
    snprintf(g_session_scores[g_session_score_count].name, sizeof(g_session_scores[g_session_score_count].name), "%s", name);
    g_session_scores[g_session_score_count].score = score;
    g_session_score_count++;
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

static void draw_centered_string(int y, const char* str, uint16_t fg_color) {
    if (!g_tty || !str) { return; }

    int tty_width = 0, tty_height = 0;
    tty_get_size(g_tty, &tty_width, &tty_height);
    if (y < 0 || y >= tty_height) { return; }

    int len = (int)strlen(str);
    if (len < 0) { len = 0; }

    int x = 0;
    if (len < tty_width) { x = (tty_width - len) / 2; }
    draw_string(x, y, str, fg_color);
}

static void draw_top_bar(void) {
    if (!g_tty) { return; }

    int tty_width = 0, tty_height = 0;
    tty_get_size(g_tty, &tty_width, &tty_height);
    if (tty_width <= 0 || tty_height <= 0) { return; }

    /* A simple top bar: a horizontal line with the title centered. */
    for (int x = 0; x < tty_width; x++) { tty_put_pixel(g_tty, x, 0, PIXEL_MAKE(PIXEL_BOX_H, COLOR_CYAN, COLOR_BLACK)); }
    draw_centered_string(0, " SNAKE ", COLOR_BRIGHT_WHITE);
}

static void invalidate_front_buffer(tty_context* ctx) {
    if (!ctx || !ctx->front) { return; }
    memset(ctx->front, 0, (size_t)ctx->width * (size_t)ctx->height * sizeof(*ctx->front));
    ctx->dirty = true;
}

void render_draw(const GameState* game) {
    if (!g_tty || !game) { return; }

    tty_clear_back(g_tty);

    int tty_width = 0, tty_height = 0;
    tty_get_size(g_tty, &tty_width, &tty_height);

    /* Draw playfield box centered on screen */
    int field_width = game->width + 2;
    int field_height = game->height + 2;

    /* Center horizontally and vertically, leaving room for HUD on the right and top bar */
    int field_x = (tty_width - field_width) / 2;
    if (field_x < 1) { field_x = 1; }

    int field_y = (tty_height - field_height) / 2;
    if (field_y < 2) { field_y = 2; }

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

    /* Top bar */
    draw_top_bar();

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
        invalidate_front_buffer(g_tty);
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
            uint16_t fg = gradient_color_for_rank(i, max_display);
            if (is_session_score(&display_scores[i]) || strstr(display_scores[i].name, "(live)") != NULL) { fg = COLOR_BRIGHT_GREEN; }
            draw_string(field_x + field_width + 2, hiscore_y + i + 1, hiscore_str, fg);
        }
    } else {
        draw_string(field_x + field_width + 2, hiscore_y + 1, "(none yet)", COLOR_CYAN);
    }

    /* Draw help text at bottom */
    if (tty_height > field_y + field_height + 1) { draw_string(1, field_y + field_height + 1, "Q: Quit", COLOR_WHITE); }

    tty_flip(g_tty);
}

void render_draw_welcome_screen(void) {
    if (!g_tty) { return; }

    tty_clear_back(g_tty);

    int tty_width = 0, tty_height = 0;
    tty_get_size(g_tty, &tty_width, &tty_height);

    draw_top_bar();

    int mid_y = tty_height / 2;
    if (mid_y < 3) { mid_y = 3; }

    draw_centered_string(mid_y - 2, "Welcome to SNAKE!", COLOR_BRIGHT_GREEN);
    draw_centered_string(mid_y - 1, "Grab food, grow longer, beat your high score.", COLOR_WHITE);
    draw_centered_string(mid_y + 1, "Press any key to start", COLOR_BRIGHT_YELLOW);
    draw_centered_string(mid_y + 2, "(Press Q to quit)", COLOR_CYAN);

    tty_flip(g_tty);
}

static void fill_rect(int x, int y, int width, int height, uint16_t fg_color, uint16_t bg_color, uint16_t ch) {
    if (!g_tty) { return; }
    if (width <= 0 || height <= 0) { return; }

    int tty_width = 0, tty_height = 0;
    tty_get_size(g_tty, &tty_width, &tty_height);

    for (int yy = 0; yy < height; yy++) {
        int py = y + yy;
        if (py < 0 || py >= tty_height) { continue; }

        for (int xx = 0; xx < width; xx++) {
            int px = x + xx;
            if (px < 0 || px >= tty_width) { continue; }
            tty_put_pixel(g_tty, px, py, PIXEL_MAKE(ch, fg_color, bg_color));
        }
    }
}

void render_draw_death_overlay(const GameState* game, int anim_frame, bool show_prompt) {
    (void)anim_frame;
    if (!g_tty) { return; }

    int tty_width = 0, tty_height = 0;
    tty_get_size(g_tty, &tty_width, &tty_height);

    /* Centered overlay box */
    int box_w = 34;
    int box_h = 8;
    int box_x = (tty_width - box_w) / 2;
    int box_y = (tty_height - box_h) / 2;

    /* Ensure box is always visible with at least 1 column margin */
    if (box_x < 1) { box_x = 1; }
    if (box_y < 1) { box_y = 1; }

    /* Clear the playfield area to hide the next spawn location */
    if (game) {
        int field_width = game->width + 2;
        int field_height = game->height + 2;
        int field_x = (tty_width - field_width) / 2;
        int field_y = (tty_height - field_height) / 2;
        if (field_x < 1) { field_x = 1; }
        if (field_y < 2) { field_y = 2; }

        fill_rect(field_x, field_y, field_width, field_height, COLOR_BLACK, COLOR_BLACK, ' ');
        tty_flip(g_tty);
    }

    uint16_t border = COLOR_BRIGHT_RED;
    uint16_t title = COLOR_BRIGHT_WHITE;

    /* Draw the dialog box on top of the existing UI */
    draw_box(box_x, box_y, box_w, box_h, border); // DEBUG: the box IS being drawn correctly
    tty_flip(g_tty);
    fill_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, COLOR_WHITE, COLOR_BLACK, ' '); // still correct box here
    tty_flip(g_tty);

    draw_string(box_x + 12, box_y + 2, "YOU DIED", title);
    tty_flip(g_tty);

    if (show_prompt) {
        draw_string(box_x + 3, box_y + 4, "Press any key to restart", COLOR_BRIGHT_GREEN);
        tty_flip(g_tty);
        draw_string(box_x + 10, box_y + 5, "or Q to quit", COLOR_BRIGHT_GREEN);
        tty_flip(g_tty);
    }

    tty_flip(g_tty);
}
