#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include "render_3d_texture.h"
#include "persist.h"

TEST(test_texture_path_extra) {
    Texture3D* tex = texture_create();
    TEST_ASSERT_TRUE(tex != NULL);

    bool ok = texture_load_from_file(tex, "images/../image.png");
    TEST_ASSERT_FALSE(ok);

    ok = texture_load_from_file(tex, "./image.png");
    TEST_ASSERT_FALSE(ok);

    ok = texture_load_from_file(tex, "some\\path\\image.png");
    TEST_ASSERT_FALSE(ok);

    size_t max_len = PERSIST_TEXTURE_PATH_MAX - 1;
    char* edge = malloc(max_len + 1);
    TEST_ASSERT_TRUE(edge != NULL);
    for (size_t i = 0; i < max_len; ++i) edge[i] = 'b';
    edge[max_len] = '\0';
    ok = texture_load_from_file(tex, edge);
    TEST_ASSERT_FALSE(ok);
    free(edge);

    ok = texture_load_from_file(tex, "//tmp//image.png");
    TEST_ASSERT_FALSE(ok);

    texture_destroy(tex);
}


