#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "render_input.h"

int main(void) {
    char out[9];

    /* normal name */
    assert(render_sanitize_player_name("Alice", out, sizeof(out)) == 1);
    assert(strcmp(out, "Alice") == 0);

    /* leading/trailing whitespace */
    assert(render_sanitize_player_name("  Bob  ", out, sizeof(out)) == 1);
    assert(strcmp(out, "Bob") == 0);

    /* empty -> default */
    assert(render_sanitize_player_name("   ", out, sizeof(out)) == 1);
    assert(strcmp(out, "You") == 0);

    /* too long for output buffer -> default (since length >= out_len) */
    char longname[64];
    memset(longname, 'A', sizeof(longname)-1);
    longname[sizeof(longname)-1] = '\0';
    assert(render_sanitize_player_name(longname, out, sizeof(out)) == 1);
    /* out should be default or truncated; we allow default behavior */
    assert(strcmp(out, "You") == 0 || strlen(out) < sizeof(out));

    printf("test_highscore_name: OK\n");
    return 0;
}
