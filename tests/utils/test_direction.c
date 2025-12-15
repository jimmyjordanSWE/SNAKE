#include "snake/direction.h"
#include <assert.h>

int main(void)
{
    /* verify turning logic */
    assert(snake_dir_turn_left(SNAKE_DIR_UP) == SNAKE_DIR_LEFT);
    assert(snake_dir_turn_left(SNAKE_DIR_DOWN) == SNAKE_DIR_RIGHT);
    assert(snake_dir_turn_left(SNAKE_DIR_LEFT) == SNAKE_DIR_DOWN);
    assert(snake_dir_turn_left(SNAKE_DIR_RIGHT) == SNAKE_DIR_UP);

    assert(snake_dir_turn_right(SNAKE_DIR_UP) == SNAKE_DIR_RIGHT);
    assert(snake_dir_turn_right(SNAKE_DIR_DOWN) == SNAKE_DIR_LEFT);
    assert(snake_dir_turn_right(SNAKE_DIR_LEFT) == SNAKE_DIR_UP);
    assert(snake_dir_turn_right(SNAKE_DIR_RIGHT) == SNAKE_DIR_DOWN);

    return 0;
}
