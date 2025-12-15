#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "snake/tty.h"
#include "snake/render.h"

int main(void)
{
    /* Create temporary file to capture tty output */
    char tmpl[] = "/tmp/ttytestXXXXXX";
    int fd = mkstemp(tmpl);
    if (fd == -1)
        return 1;
    close(fd);

    /* Force deterministic terminal size */
    tty_set_test_size(10, 1);
    tty_context* ctx = tty_open(tmpl, 0, 0);
    assert(ctx != NULL);

    /* Place a surrogate pair (U+1F600 GRINNING FACE = D83D DE00) */
    struct ascii_pixel* b = tty_get_buffer(ctx);
    b[0].pixel = 0xD83Du; /* high surrogate */
    b[1].pixel = 0xDE00u; /* low surrogate */

    tty_flip(ctx);
    tty_close(ctx);

    /* Read output and look for UTF-8 sequence F0 9F 98 80 */
    FILE* f = fopen(tmpl, "rb");
    assert(f != NULL);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc((size_t)sz + 1);
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);

    /* UTF-8 bytes for U+1F600 */
    const unsigned char smile[] = {0xF0, 0x9F, 0x98, 0x80};
    bool found = false;
    for (long i = 0; i + 4 <= sz; i++) {
        if ((unsigned char)buf[i] == smile[0] && (unsigned char)buf[i+1] == smile[1]
            && (unsigned char)buf[i+2] == smile[2] && (unsigned char)buf[i+3] == smile[3])
        {
            found = true;
            break;
        }
    }
    free(buf);
    /* cleanup temp file */
    unlink(tmpl);
    assert(found);
    return 0;
}
