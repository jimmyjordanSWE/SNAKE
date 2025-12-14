#include <assert.h>
#include "snake/utils.h"

int main(void) {
    assert(snake_in_bounds(0,0,10,10));
    assert(snake_in_bounds(9,9,10,10));
    assert(!snake_in_bounds(-1,0,10,10));
    assert(!snake_in_bounds(10,5,10,10));
    return 0;
}
