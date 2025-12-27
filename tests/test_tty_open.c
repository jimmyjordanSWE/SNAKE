#include <stdio.h>
#include <string.h>
#include "snake/tty.h"

int main(void) {
    /* Create a path longer than tty_path buffer (256) to force reject */
    char longpath[512];
    memset(longpath, 'A', sizeof(longpath) - 1);
    longpath[sizeof(longpath) - 1] = '\0';
    tty_context* t = tty_open(longpath, 10, 5);
    if (t != NULL) {
        tty_close(t);
        printf("FAIL: expected tty_open to return NULL for oversized path\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
