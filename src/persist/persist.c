#include "snake/persist.h"

bool persist_load_highscores(const char* path) {
    (void)path;
    return false;
}

bool persist_save_highscores(const char* path, const GameState* game) {
    (void)path;
    (void)game;
    return false;
}
