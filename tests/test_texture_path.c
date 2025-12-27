#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "render_3d_texture.h"

int main(void) {
    Texture3D* tex = texture_create();
    assert(tex != NULL);

    /* Non-existent simple filename */
    bool ok = texture_load_from_file(tex, "this_file_does_not_exist.png");
    assert(ok == false);

    /* Very long filename should not overflow buffers */
    size_t long_len = 2048;
    char* longname = malloc(long_len + 1);
    if (!longname) return 1;
    for (size_t i = 0; i < long_len; ++i) longname[i] = 'A';
    longname[long_len] = '\0';
    ok = texture_load_from_file(tex, longname);
    assert(ok == false);
    free(longname);

    /* Path with many parent components */
    char many_parents[4096];
    size_t mp_used = 0, mp_avail = sizeof(many_parents);
    many_parents[0] = '\0';
    for (int i = 0; i < 2000; i += 3) {
        if (mp_used + 3 + 1 >= mp_avail) break;
        memcpy(many_parents + mp_used, "../", 3);
        mp_used += 3;
        many_parents[mp_used] = '\0';
    }
    /* Safely append the filename */
    if (mp_used + strlen("image.png") + 1 < mp_avail) {
        strncat(many_parents, "image.png", mp_avail - mp_used - 1);
    }
    ok = texture_load_from_file(tex, many_parents);
    assert(ok == false);

    /* Reject unsafe paths */
    ok = texture_load_from_file(tex, "/etc/passwd");
    assert(ok == false);
    ok = texture_load_from_file(tex, "../image.png");
    assert(ok == false);

    texture_destroy(tex);
    printf("test_texture_path: OK\n");
    return 0;
}
