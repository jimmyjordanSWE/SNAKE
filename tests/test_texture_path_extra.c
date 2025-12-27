#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "render_3d_texture.h"
#include "persist.h"

int main(void) {
    Texture3D* tex = texture_create();
    assert(tex != NULL);

    /* Path with dot components in the middle */
    bool ok = texture_load_from_file(tex, "images/../image.png");
    assert(ok == false);

    /* Path with current-dir prefix */
    ok = texture_load_from_file(tex, "./image.png");
    assert(ok == false);

    /* Windows style backslash should be rejected as unsafe on POSIX */
    ok = texture_load_from_file(tex, "some\\path\\image.png");
    assert(ok == false);

    /* Very long path at the edge of limits should be rejected safely */
    size_t max_len = PERSIST_TEXTURE_PATH_MAX - 1;
    char* edge = malloc(max_len + 1);
    if (!edge) return 1;
    for (size_t i = 0; i < max_len; ++i) edge[i] = 'b';
    edge[max_len] = '\0';
    ok = texture_load_from_file(tex, edge);
    assert(ok == false);
    free(edge);

    /* Paths with multiple consecutive separators */
    ok = texture_load_from_file(tex, "//tmp//image.png");
    assert(ok == false);

    texture_destroy(tex);
    printf("test_texture_path_extra: OK\n");
    return 0;
}
