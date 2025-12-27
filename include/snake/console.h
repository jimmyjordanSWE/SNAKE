#pragma once
#include <stdarg.h>
void console_info(const char* fmt, ...);
void console_error(const char* fmt, ...);
void console_warn(const char* fmt, ...);
void console_box_too_small_for_game(int term_w, int term_h, int min_w, int min_h);
void console_box_paused_terminal_small(int term_w, int term_h, int req_w, int req_h);
void console_terminal_resized(int w, int h);
void console_game_ran(int ticks);
