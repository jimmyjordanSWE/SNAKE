#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include "render_3d_texture.h"

TEST(test_texture_path) {
    Texture3D* tex = texture_create();
    TEST_ASSERT_TRUE(tex != NULL);

    bool ok = texture_load_from_file(tex, "this_file_does_not_exist.png");
    TEST_ASSERT_FALSE(ok);

    size_t long_len = 2048;
    char* longname = malloc(long_len + 1);
    TEST_ASSERT_TRUE(longname != NULL);
    for (size_t i = 0; i < long_len; ++i) longname[i] = 'A';
    longname[long_len] = '\0';
    ok = texture_load_from_file(tex, longname);
    TEST_ASSERT_FALSE(ok);
    free(longname);

    char many_parents[4096];
    size_t mp_used = 0, mp_avail = sizeof(many_parents);
    many_parents[0] = '\0';
    for (int i = 0; i < 2000; i += 3) {
        if (mp_used + 3 + 1 >= mp_avail) break;
        memcpy(many_parents + mp_used, "../", 3);
        mp_used += 3;
        many_parents[mp_used] = '\0';
    }
    if (mp_used + strlen("image.png") + 1 < mp_avail) {
        strncat(many_parents, "image.png", mp_avail - mp_used - 1);
    }
    ok = texture_load_from_file(tex, many_parents);
    TEST_ASSERT_FALSE(ok);

    ok = texture_load_from_file(tex, "/etc/passwd");
    TEST_ASSERT_FALSE(ok);
    ok = texture_load_from_file(tex, "../image.png");
    TEST_ASSERT_FALSE(ok);

    texture_destroy(tex);
}


