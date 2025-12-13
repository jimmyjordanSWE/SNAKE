#pragma once

#include <stdbool.h>

#include "snake/game.h"

bool render_init(int min_width, int min_height);
void render_shutdown(void);
void render_draw(const GameState* game);

/* Marks a score as having been achieved in the current run (used to highlight highscores). */
void render_note_session_score(const char* name, int score);

/* Draws a "YOU DIED" overlay on top of the regular frame. */
void render_draw_death_overlay(const GameState* game, int anim_frame, bool show_prompt);
