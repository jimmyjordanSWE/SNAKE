#include "direction.h"
SnakeDir snake_dir_turn_left(SnakeDir d) {
switch(d) {
case SNAKE_DIR_UP: return SNAKE_DIR_LEFT;
case SNAKE_DIR_DOWN: return SNAKE_DIR_RIGHT;
case SNAKE_DIR_LEFT: return SNAKE_DIR_DOWN;
case SNAKE_DIR_RIGHT: return SNAKE_DIR_UP;
default: return d;
}
}
SnakeDir snake_dir_turn_right(SnakeDir d) {
switch(d) {
case SNAKE_DIR_UP: return SNAKE_DIR_RIGHT;
case SNAKE_DIR_DOWN: return SNAKE_DIR_LEFT;
case SNAKE_DIR_LEFT: return SNAKE_DIR_UP;
case SNAKE_DIR_RIGHT: return SNAKE_DIR_DOWN;
default: return d;
}
}
