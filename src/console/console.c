#include "snake/console.h"
#include <stdarg.h>
#include <stdio.h>
void console_info(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
void console_error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
void console_warn(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
void console_box_too_small_for_game(int term_w, int term_h, int min_w, int min_h) {
    console_info("\n");
    console_info("╔════════════════════════════════════════╗\n");
    console_info("║  TERMINAL TOO SMALL FOR SNAKE GAME    ║\n");
    console_info("║                                        ║\n");
    console_info("║  Current size: %d x %d                  ║\n", term_w, term_h);
    console_info("║  Minimum size: %d x %d                 ║\n", min_w, min_h);
    console_info("║                                        ║\n");
    console_info("║  Please resize your terminal window    ║\n");
    console_info("║  or press Ctrl+C to exit               ║\n");
    console_info("╚════════════════════════════════════════╗\n");
    console_info("\n");
}
void console_box_paused_terminal_small(int term_w, int term_h, int req_w, int req_h) {
    console_info("\n");
    console_info("╔════════════════════════════════════════╗\n");
    console_info("║  TERMINAL TOO SMALL - GAME PAUSED      ║\n");
    console_info("║                                        ║\n");
    console_info("║  Current size: %d x %d                 ║\n", term_w, term_h);
    console_info("║  Required: %d x %d                     ║\n", req_w, req_h);
    console_info("║                                        ║\n");
    console_info("║  Resize your terminal to continue      ║\n");
    console_info("║  or press Ctrl+C to exit               ║\n");
    console_info("╚════════════════════════════════════════╝\n");
    console_info("\n");
}
void console_terminal_resized(int w, int h) {
    console_info("Terminal resized to %dx%d. Resuming game...\n", w, h);
}
void console_game_ran(int ticks) {
    console_info("Game ran for %d ticks\n", ticks);
}
