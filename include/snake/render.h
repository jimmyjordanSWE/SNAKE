#pragma once

#include <stdbool.h>

#include "snake/game.h"

typedef enum {

    RENDER_GLYPHS_UTF8 = 0,
    RENDER_GLYPHS_ASCII = 1,
} RenderGlyphs;

/* Select glyph set for snake drawing. Default is UTF-8 (box drawing). */
void render_set_glyphs(RenderGlyphs glyphs);

bool render_init(int min_width, int min_height);
void render_shutdown(void);
void render_draw(const GameState* game);

/* Draws an upbeat startup welcome screen and waits-for-input prompt (caller handles input). */
void render_draw_welcome_screen(void);

/* Marks a score as having been achieved in the current run (used to highlight highscores). */
void render_note_session_score(const char* name, int score);

/* Draws a "YOU DIED" overlay on top of the regular frame. */
void render_draw_death_overlay(const GameState* game, int anim_frame, bool show_prompt);
