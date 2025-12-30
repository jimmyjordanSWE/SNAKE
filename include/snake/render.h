#pragma once
#include "game.h"
#include "persist.h"
#include <stdbool.h>
typedef enum {
    RENDER_GLYPHS_UTF8= 0,
    RENDER_GLYPHS_ASCII= 1,
} RenderGlyphs;
void render_set_glyphs(RenderGlyphs glyphs);
bool render_init(int min_width, int min_height);
void render_shutdown(void);
void render_draw(const GameState* game, const char* player_name, HighScore** scores, int score_count);
void render_draw_startup_screen(char* player_name_out, int max_len);
void render_prompt_for_highscore_name(char* player_name_out, int max_len, int score);
void render_note_session_score(const char* name, int score);

/* Push a short text message received from multiplayer; displayed briefly on screen */
void render_push_mp_message(const char* msg);

/* Set the current MP session id (displayed in HUD); pass NULL or empty to clear */
void render_set_session_id(const char* session);

/* Draw remote players received from multiplayer (x,y head positions) */
void render_draw_remote_players(const GameState* game, const int* x, const int* y, const void* names, int count);

void render_draw_death_overlay(const GameState* game, int anim_frame, bool show_prompt);
void render_draw_winner_overlay(const GameState* game, int winner, int score);