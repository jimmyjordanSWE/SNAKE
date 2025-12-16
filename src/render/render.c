#include "snake/render.h"
#include "snake/display.h"
#include "snake/game_internal.h"
#include "snake/input.h"
#include "snake/persist.h"
#include "snake/platform.h"
#include "snake/render_3d.h"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static DisplayContext* g_display = NULL;
static RenderGlyphs g_glyphs = RENDER_GLYPHS_UTF8;
static int last_score_count = -1;
typedef struct {
    char name[PERSIST_NAME_MAX];
    int score;
} DisplayScore;
static DisplayScore last_scores[PERSIST_MAX_SCORES];
#define SESSION_MAX_SCORES 32
static DisplayScore g_session_scores[SESSION_MAX_SCORES];
static int g_session_score_count = 0;
void render_set_glyphs(RenderGlyphs glyphs) {
    if (glyphs != RENDER_GLYPHS_ASCII)
        glyphs = RENDER_GLYPHS_UTF8;
    g_glyphs = glyphs;
}
static bool is_session_score_from_display(const DisplayScore* s) {
    if (!s)
        return false;
    for (int i = 0; i < g_session_score_count; i++)
        if (g_session_scores[i].score == s->score && strcmp(g_session_scores[i].name, s->name) == 0)
            return true;
    return false;
}
static bool is_session_score(const HighScore* s) {
    if (!s)
        return false;
    const char* name = highscore_get_name(s);
    int score = highscore_get_score(s);
    for (int i = 0; i < g_session_score_count; i++)
        if (g_session_scores[i].score == score && strcmp(g_session_scores[i].name, name) == 0)
            return true;
    return false;
}
static uint16_t gradient_color_for_rank(int rank, int total) {
    (void)total;
    static const uint16_t palette[] = {
        DISPLAY_COLOR_BRIGHT_YELLOW,  DISPLAY_COLOR_YELLOW,      DISPLAY_COLOR_BRIGHT_RED,
        DISPLAY_COLOR_BRIGHT_MAGENTA, DISPLAY_COLOR_BRIGHT_CYAN,
    };
    int palette_len = (int)(sizeof(palette) / sizeof(palette[0]));
    if (rank < 0)
        rank = 0;
    if (rank >= palette_len)
        rank = palette_len - 1;
    return palette[rank];
}
static int compare_display_scores_desc(const void* a, const void* b) {
    const DisplayScore* sa = (const DisplayScore*)a;
    const DisplayScore* sb = (const DisplayScore*)b;
    if (sa->score > sb->score)
        return -1;
    if (sa->score < sb->score)
        return 1;
    return strcmp(sa->name, sb->name);
}
bool render_init(int min_width, int min_height) {
    if (min_width < 20)
        min_width = 20;
    if (min_height < 10)
        min_height = 10;
    g_display = display_init(min_width, min_height);
    g_session_score_count = 0;
    if (g_display == NULL)
        return false;
    if (!display_size_valid(g_display)) {
        display_shutdown(g_display);
        g_display = NULL;
        return false;
    }
    return true;
}
void render_shutdown(void) {
    if (g_display) {
        display_shutdown(g_display);
        g_display = NULL;
    }
    g_session_score_count = 0;
}
void render_note_session_score(const char* name, int score) {
    if (!name || score <= 0)
        return;
    for (int i = 0; i < g_session_score_count; i++)
        if (g_session_scores[i].score == score && strcmp(g_session_scores[i].name, name) == 0)
            return;
    if (g_session_score_count >= SESSION_MAX_SCORES)
        return;
    snprintf(g_session_scores[g_session_score_count].name, sizeof(g_session_scores[g_session_score_count].name), "%s",
             name);
    g_session_scores[g_session_score_count].score = score;
    g_session_score_count++;
}
static void draw_box(int x, int y, int width, int height, uint16_t fg_color) {
    if (!g_display)
        return;
    int display_width = 0, display_height = 0;
    display_get_size(g_display, &display_width, &display_height);
    if (x >= 0 && y >= 0 && x < display_width && y < display_height)
        display_put_char(g_display, x, y, DISPLAY_CHAR_BOX_TL, fg_color, DISPLAY_COLOR_BLACK);
    if (x + width - 1 >= 0 && y >= 0 && x + width - 1 < display_width && y < display_height)
        display_put_char(g_display, x + width - 1, y, DISPLAY_CHAR_BOX_TR, fg_color, DISPLAY_COLOR_BLACK);
    if (x >= 0 && y + height - 1 >= 0 && x < display_width && y + height - 1 < display_height)
        display_put_char(g_display, x, y + height - 1, DISPLAY_CHAR_BOX_BL, fg_color, DISPLAY_COLOR_BLACK);
    if (x + width - 1 >= 0 && y + height - 1 >= 0 && x + width - 1 < display_width && y + height - 1 < display_height)
        display_put_char(g_display, x + width - 1, y + height - 1, DISPLAY_CHAR_BOX_BR, fg_color, DISPLAY_COLOR_BLACK);
    for (int i = 1; i < width - 1; i++) {
        if (x + i >= 0 && x + i < display_width) {
            if (y >= 0 && y < display_height)
                display_put_char(g_display, x + i, y, DISPLAY_CHAR_BOX_H, fg_color, DISPLAY_COLOR_BLACK);
            if (y + height - 1 >= 0 && y + height - 1 < display_height)
                display_put_char(g_display, x + i, y + height - 1, DISPLAY_CHAR_BOX_H, fg_color, DISPLAY_COLOR_BLACK);
        }
    }
    for (int i = 1; i < height - 1; i++) {
        if (y + i >= 0 && y + i < display_height) {
            if (x >= 0 && x < display_width)
                display_put_char(g_display, x, y + i, DISPLAY_CHAR_BOX_V, fg_color, DISPLAY_COLOR_BLACK);
            if (x + width - 1 >= 0 && x + width - 1 < display_width)
                display_put_char(g_display, x + width - 1, y + i, DISPLAY_CHAR_BOX_V, fg_color, DISPLAY_COLOR_BLACK);
        }
    }
}
static void draw_string(int x, int y, const char* str, uint16_t fg_color) {
    if (!g_display || !str)
        return;
    display_put_string(g_display, x, y, str, fg_color, DISPLAY_COLOR_BLACK);
}
static void draw_centered_string(int y, const char* str, uint16_t fg_color) {
    if (!g_display || !str)
        return;
    int display_width = 0, display_height = 0;
    display_get_size(g_display, &display_width, &display_height);
    if (y < 0 || y >= display_height)
        return;
    int len = (int)strlen(str);
    int x = 0;
    if (len < display_width)
        x = (display_width - len) / 2;
    draw_string(x, y, str, fg_color);
}
static void draw_top_bar(void) {
    if (!g_display)
        return;
    int display_width = 0, display_height = 0;
    display_get_size(g_display, &display_width, &display_height);
    if (display_width <= 0 || display_height <= 0)
        return;
    display_put_hline(g_display, 0, 0, display_width, DISPLAY_CHAR_BOX_H, DISPLAY_COLOR_CYAN, DISPLAY_COLOR_BLACK);
    draw_centered_string(0, " SNAKE ", DISPLAY_COLOR_BRIGHT_WHITE);
}
static void invalidate_front_buffer(DisplayContext* ctx) {
    if (!ctx)
        return;
    display_force_redraw(ctx);
}
static void fill_background(void) {
    if (!g_display)
        return;
    int display_width = 0, display_height = 0;
    display_get_size(g_display, &display_width, &display_height);
    for (int y = 0; y < display_height; y++)
        for (int x = 0; x < display_width; x++)
            display_put_char(g_display, x, y, ' ', DISPLAY_COLOR_BLACK, DISPLAY_COLOR_BLACK);
}
typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
} SegmentLinks;
static SegmentLinks links_to_neighbor(SnakePoint from, SnakePoint neighbor) {
    SegmentLinks links = {0};
    int dx = neighbor.x - from.x;
    int dy = neighbor.y - from.y;
    if (dx == 1 && dy == 0)
        links.right = true;
    else if (dx == -1 && dy == 0)
        links.left = true;
    else if (dx == 0 && dy == 1)
        links.down = true;
    else if (dx == 0 && dy == -1)
        links.up = true;
    return links;
}
static uint16_t glyph_for_segment_utf8(const SnakePoint* body, int length, int idx) {
    if (!body || length <= 0 || idx < 0 || idx >= length)
        return (uint16_t)'o';
    if (idx == 0)
        return DISPLAY_CHAR_CIRCLE;
    SnakePoint cur = body[idx];
    SegmentLinks a = {0};
    SegmentLinks b = {0};
    if (idx > 0)
        a = links_to_neighbor(cur, body[idx - 1]);
    if (idx + 1 < length)
        b = links_to_neighbor(cur, body[idx + 1]);
    if (idx + 1 == length) {
        if (a.left)
            b.right = true;
        else if (a.right)
            b.left = true;
        else if (a.up)
            b.down = true;
        else if (a.down)
            b.up = true;
    }
    /* Use a simple circular glyph for body segments on minimap for a cleaner
     * look. */
    SegmentLinks links = {
        .up = a.up || b.up,
        .down = a.down || b.down,
        .left = a.left || b.left,
        .right = a.right || b.right,
    };
    /* Render body segments as circles instead of box-drawing characters */
    return DISPLAY_CHAR_CIRCLE;
}
static uint16_t glyph_for_segment_ascii(const SnakePoint* body, int length, int idx) {
    if (!body || length <= 0 || idx < 0 || idx >= length)
        return (uint16_t)'o';
    if (idx == 0)
        return (uint16_t)'@';
    SnakePoint cur = body[idx];
    SegmentLinks a = {0};
    SegmentLinks b = {0};
    if (idx > 0)
        a = links_to_neighbor(cur, body[idx - 1]);
    if (idx + 1 < length)
        b = links_to_neighbor(cur, body[idx + 1]);
    if (idx + 1 == length) {
        if (a.left)
            b.right = true;
        else if (a.right)
            b.left = true;
        else if (a.up)
            b.down = true;
        else if (a.down)
            b.up = true;
    }
    SegmentLinks links = {
        .up = a.up || b.up,
        .down = a.down || b.down,
        .left = a.left || b.left,
        .right = a.right || b.right,
    };
    /* Render body segments as a simple 'o' to approximate circles in ASCII mode
     */
    return (uint16_t)'o';
}
static uint16_t glyph_for_segment(const SnakePoint* body, int length, int idx) {
    if (g_glyphs == RENDER_GLYPHS_ASCII)
        return glyph_for_segment_ascii(body, length, idx);
    return glyph_for_segment_utf8(body, length, idx);
}
void render_draw(const GameState* game, const char* player_name, HighScore** scores, int score_count) {
    if (!g_display || !game)
        return;
    display_clear(g_display);
    fill_background();
    int display_width = 0, display_height = 0;
    display_get_size(g_display, &display_width, &display_height);
    int field_width = game->width + 2;
    int field_height = game->height + 2;
    int field_x = (display_width - field_width) / 2;
    if (field_x < 1)
        field_x = 1;
    int field_y = (display_height - field_height) / 2;
    if (field_y < 2)
        field_y = 2;
    draw_box(field_x, field_y, field_width, field_height, DISPLAY_COLOR_CYAN);
    for (int i = 0; i < game->food_count; i++)
        display_put_char(g_display, field_x + 1 + game->food[i].x, field_y + 1 + game->food[i].y, '*',
                         DISPLAY_COLOR_GREEN, DISPLAY_COLOR_BLACK);
    for (int p = 0; p < game->num_players; p++) {
        const PlayerState* player = &game->players[p];
        if (!player->active)
            continue;
        uint16_t snake_color = (p == 0) ? DISPLAY_COLOR_BRIGHT_YELLOW : DISPLAY_COLOR_BRIGHT_RED;
        for (int i = 0; i < player->length; i++) {
            uint16_t ch = glyph_for_segment(player->body, player->length, i);
            display_put_char(g_display, field_x + 1 + player->body[i].x, field_y + 1 + player->body[i].y, ch,
                             snake_color, DISPLAY_COLOR_BLACK);
        }
    }
    draw_top_bar();
    for (int p = 0; p < game->num_players; p++) {
        if (game->players[p].active) {
            char score_str[32];
            if (p == 0 && player_name != NULL)
                snprintf(score_str, sizeof(score_str), "%s: %d", player_name, game->players[p].score);
            else
                snprintf(score_str, sizeof(score_str), "P%d: %d", p + 1, game->players[p].score);
            uint16_t color = (p == 0) ? DISPLAY_COLOR_BRIGHT_YELLOW : DISPLAY_COLOR_BRIGHT_RED;
            draw_string(field_x + field_width + 2, field_y + p * 2, score_str, color);
        }
    }
    if (!scores)
        score_count = 0;
    if (score_count < 0)
        score_count = 0;
    if (score_count > PERSIST_MAX_SCORES)
        score_count = PERSIST_MAX_SCORES;
    DisplayScore display_scores[PERSIST_MAX_SCORES + SNAKE_MAX_PLAYERS];
    int display_count = 0;
    for (int i = 0; i < score_count && display_count < (int)(sizeof(display_scores) / sizeof(display_scores[0])); i++) {
        if (!scores || !scores[i])
            continue;
        const char* hs_name = highscore_get_name(scores[i]);
        if (!hs_name)
            hs_name = "";
        snprintf(display_scores[display_count].name, sizeof(display_scores[display_count].name), "%.*s",
                 (int)sizeof(display_scores[display_count].name) - 1, hs_name);
        display_scores[display_count].score = highscore_get_score(scores[i]);
        display_count++;
    }
    for (int p = 0; p < game->num_players && display_count < (int)(sizeof(display_scores) / sizeof(display_scores[0]));
         p++) {
        if (!game->players[p].active)
            continue;
        if (game->players[p].score <= 0)
            continue;
        char tmp_name_buf[64];
        if (p == 0 && player_name != NULL)
            snprintf(tmp_name_buf, sizeof(tmp_name_buf), "%s (live)", player_name);
        else
            snprintf(tmp_name_buf, sizeof(tmp_name_buf), "P%d (live)", p + 1);
        /* Truncate into the fixed display buffer size, safe for any
         * PERSIST_NAME_MAX */
        snprintf(display_scores[display_count].name, sizeof(display_scores[display_count].name), "%.*s",
                 (int)sizeof(display_scores[display_count].name) - 1, tmp_name_buf);
        display_scores[display_count].score = game->players[p].score;
        display_count++;
    }
    if (display_count > 1)
        qsort(display_scores, (size_t)display_count, sizeof(DisplayScore), compare_display_scores_desc);
    int max_display = (display_count > 5) ? 5 : display_count;
    bool scores_changed = (score_count != last_score_count);
    if (!scores_changed && score_count > 0 && scores) {
        for (int i = 0; i < score_count; i++) {
            if (last_scores[i].score != highscore_get_score(scores[i]) ||
                strcmp(last_scores[i].name, highscore_get_name(scores[i])) != 0) {
                scores_changed = true;
                break;
            }
        }
    }
    if (scores_changed && scores) {
        last_score_count = score_count;
        for (int i = 0; i < score_count && i < PERSIST_MAX_SCORES; i++) {
            const char* hs_name = highscore_get_name(scores[i]);
            if (!hs_name)
                hs_name = "";
            snprintf(last_scores[i].name, sizeof(last_scores[i].name), "%.*s", (int)sizeof(last_scores[i].name) - 1,
                     hs_name);
            last_scores[i].score = highscore_get_score(scores[i]);
        }
        invalidate_front_buffer(g_display);
    }
    int hiscore_y = field_y + 5;
    draw_string(field_x + field_width + 2, hiscore_y, "High Scores:", DISPLAY_COLOR_WHITE);
    if (max_display > 0) {
        for (int i = 0; i < max_display && hiscore_y + i + 1 < display_height; i++) {
            char hiscore_str[80];
            int written = snprintf(hiscore_str, sizeof(hiscore_str), "%d. ", i + 1);
            if (written > 0 && written < (int)sizeof(hiscore_str))
                snprintf(hiscore_str + written, sizeof(hiscore_str) - (size_t)written, "%.15s: %d",
                         display_scores[i].name, display_scores[i].score);
            uint16_t fg = gradient_color_for_rank(i, max_display);
            if (is_session_score_from_display(&display_scores[i]) || strstr(display_scores[i].name, "(live)") != NULL)
                fg = DISPLAY_COLOR_BRIGHT_GREEN;
            draw_string(field_x + field_width + 2, hiscore_y + i + 1, hiscore_str, fg);
        }
    } else {
        draw_string(field_x + field_width + 2, hiscore_y + 1, "(none yet)", DISPLAY_COLOR_CYAN);
    }
    if (display_height > field_y + field_height + 1)
        draw_string(1, field_y + field_height + 1, "Q: Quit", DISPLAY_COLOR_WHITE);
    display_present(g_display);
}
void render_draw_startup_screen(char* player_name_out, int max_len) {
    /* Non-blocking startup: set a short default player name and show a brief
     * splash. */
    if (!g_display || !player_name_out || max_len <= 0)
        return;
    int display_width = 0, display_height = 0;
    display_get_size(g_display, &display_width, &display_height);
    display_clear(g_display);
    int y = 1;
    draw_centered_string(y++, "========================================", DISPLAY_COLOR_BRIGHT_YELLOW);
    draw_centered_string(y++, "", DISPLAY_COLOR_WHITE);
    draw_centered_string(y++, "   S N A K E", DISPLAY_COLOR_BRIGHT_GREEN);
    draw_centered_string(y++, "      THE LEGEND", DISPLAY_COLOR_BRIGHT_GREEN);
    draw_centered_string(y++, "", DISPLAY_COLOR_WHITE);
    draw_centered_string(y++, "========================================", DISPLAY_COLOR_BRIGHT_YELLOW);
    y++;
    draw_centered_string(y++, "A Game of Hunger & Growth", DISPLAY_COLOR_CYAN);
    draw_centered_string(y++, "Navigate. Consume. Dominate.", DISPLAY_COLOR_CYAN);
    display_present(g_display);
    /* Keep splash on screen briefly, but do not block for name input. */
    platform_sleep_ms(600);
    /* Default player name per new feature */
    if (max_len > 0)
        snprintf(player_name_out, (size_t)max_len, "You");
}

void render_prompt_for_highscore_name(char* player_name_out, int max_len, int score) {
    if (!g_display || !player_name_out || max_len <= 0)
        return;
    /* Cap max_len to the required 8+NUL for highscore names */
    if (max_len > 9)
        max_len = 9;
    char name_input[9] = {0};
    int input_pos = 0;
    bool name_confirmed = false;
    while (!name_confirmed) {
        display_clear(g_display);
        int y = 1;
        draw_centered_string(y++, "CONGRATULATIONS!", DISPLAY_COLOR_BRIGHT_GREEN);
        draw_centered_string(y++, "You made the high score list!", DISPLAY_COLOR_BRIGHT_YELLOW);
        char score_str[32];
        snprintf(score_str, sizeof(score_str), "Score: %d", score);
        draw_centered_string(y++, score_str, DISPLAY_COLOR_CYAN);
        y++;
        draw_centered_string(y++, "ENTER A SHORT NAME (max 8 chars)", DISPLAY_COLOR_BRIGHT_YELLOW);
        y++;
        int box_width = 20;
        int display_width = 0, display_height = 0;
        display_get_size(g_display, &display_width, &display_height);
        int box_x = (display_width - box_width) / 2;
        if (box_x < 1)
            box_x = 1;
        draw_string(box_x, y, "+", DISPLAY_COLOR_BRIGHT_YELLOW);
        for (int i = 0; i < box_width - 2; i++)
            display_put_char(g_display, box_x + 1 + i, y, '-', DISPLAY_COLOR_BRIGHT_YELLOW, DISPLAY_COLOR_BLACK);
        draw_string(box_x + box_width - 1, y, "+", DISPLAY_COLOR_BRIGHT_YELLOW);
        y++;
        draw_string(box_x, y, "|", DISPLAY_COLOR_BRIGHT_YELLOW);
        draw_string(box_x + 2, y, name_input, DISPLAY_COLOR_BRIGHT_GREEN);
        if (input_pos < 8)
            draw_string(box_x + 2 + input_pos, y, "_", DISPLAY_COLOR_BRIGHT_YELLOW);
        draw_string(box_x + box_width - 1, y, "|", DISPLAY_COLOR_BRIGHT_YELLOW);
        y++;
        draw_string(box_x, y, "+", DISPLAY_COLOR_BRIGHT_YELLOW);
        for (int i = 0; i < box_width - 2; i++)
            display_put_char(g_display, box_x + 1 + i, y, '-', DISPLAY_COLOR_BRIGHT_YELLOW, DISPLAY_COLOR_BLACK);
        draw_string(box_x + box_width - 1, y, "+", DISPLAY_COLOR_BRIGHT_YELLOW);
        y += 2;
        draw_centered_string(y++, "Press ENTER to confirm, leave blank to use default", DISPLAY_COLOR_WHITE);
        /* mirror to 3D if available */
        if (name_input[0])
            render_3d_draw_congrats_overlay(score, name_input);
        else
            render_3d_draw_congrats_overlay(score, NULL);
        display_present(g_display);
        platform_sleep_ms(20);
        unsigned char buf[16];
        ssize_t nread = read(STDIN_FILENO, buf, sizeof(buf));
        if (nread > 0) {
            for (ssize_t i = 0; i < nread; i++) {
                unsigned char c = buf[i];
                if (c == '\n' || c == '\r') {
                    name_confirmed = true;
                    break;
                } else if (c == 8 || c == 127) {
                    if (input_pos > 0) {
                        input_pos--;
                        name_input[input_pos] = '\0';
                    }
                } else if (c >= 32 && c < 127) {
                    if (input_pos < 8) {
                        name_input[input_pos] = (char)c;
                        input_pos++;
                        name_input[input_pos] = '\0';
                    }
                }
            }
        }
    }
    char* start = name_input;
    while (*start && isspace((unsigned char)*start))
        start++;
    char* end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1]))
        end--;
    *end = '\0';
    if (strlen(start) > 0 && strlen(start) < (size_t)max_len)
        snprintf(player_name_out, (size_t)max_len, "%s", start);
    else
        snprintf(player_name_out, (size_t)max_len, "You");
}
static void fill_rect(int x, int y, int width, int height, uint16_t fg_color, uint16_t bg_color, uint16_t ch) {
    if (!g_display)
        return;
    if (width <= 0 || height <= 0)
        return;
    int display_width = 0, display_height = 0;
    display_get_size(g_display, &display_width, &display_height);
    for (int yy = 0; yy < height; yy++) {
        int py = y + yy;
        if (py < 0 || py >= display_height)
            continue;
        for (int xx = 0; xx < width; xx++) {
            int px = x + xx;
            if (px < 0 || px >= display_width)
                continue;
            display_put_char(g_display, px, py, ch, fg_color, bg_color);
        }
    }
}
void render_draw_death_overlay(const GameState* game, int anim_frame, bool show_prompt) {
    (void)anim_frame;
    if (!g_display)
        return;
    int display_width = 0, display_height = 0;
    display_get_size(g_display, &display_width, &display_height);
    int box_w = 34;
    int box_h = 8;
    int box_x = (display_width - box_w) / 2;
    int box_y = (display_height - box_h) / 2;
    if (box_x < 1)
        box_x = 1;
    if (box_y < 1)
        box_y = 1;
    if (game) {
        int field_width = game->width + 2;
        int field_height = game->height + 2;
        int field_x = (display_width - field_width) / 2;
        int field_y = (display_height - field_height) / 2;
        if (field_x < 1)
            field_x = 1;
        if (field_y < 2)
            field_y = 2;
        fill_rect(field_x, field_y, field_width, field_height, DISPLAY_COLOR_BLACK, DISPLAY_COLOR_BLACK, ' ');
    }
    uint16_t border = DISPLAY_COLOR_BRIGHT_RED;
    uint16_t title = DISPLAY_COLOR_BRIGHT_WHITE;
    draw_box(box_x, box_y, box_w, box_h, border);
    fill_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, DISPLAY_COLOR_WHITE, DISPLAY_COLOR_BLACK, ' ');
    draw_string(box_x + 12, box_y + 2, "YOU DIED", title);
    if (show_prompt) {
        draw_string(box_x + 3, box_y + 4, "Press any key to restart", DISPLAY_COLOR_BRIGHT_GREEN);
        draw_string(box_x + 10, box_y + 5, "or Q to quit", DISPLAY_COLOR_BRIGHT_GREEN);
    }
    display_present(g_display);
}
