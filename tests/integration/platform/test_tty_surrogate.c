#include <assert.h>
#include "snake/tty.h"

int main(void) {
    
    tty_init_surrogate();
    tty_shutdown_surrogate();
    return 0;
}
