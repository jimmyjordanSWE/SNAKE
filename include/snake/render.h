#pragma once
#include "snake/game.h"
#include "snake/persist.h"
#include <stdbool.h>
typedef enum {
    RENDER_GLYPHS_UTF8= 0,
    RENDER_GLYPHS_ASCII= 1,
} RenderGlyphs;
void render_set_glyphs(RenderGlyphs glyphs);
bool render_init(int min_width, int min_height);
void render_shutdown(void);
void render_draw(const GameState* game, const char* player_name, const HighScore* scores, int score_count);
void render_draw_startup_screen(char* player_name_out, int max_len);
void render_note_session_score(const char* name, int score);
void render_draw_death_overlay(const GameState* game, int anim_frame, bool show_prompt);