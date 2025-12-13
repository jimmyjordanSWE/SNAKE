#include "snake/utils.h"

bool snake_in_bounds(int x, int y, int width, int height) {
    return x >= 0 && y >= 0 && x < width && y < height;
}
