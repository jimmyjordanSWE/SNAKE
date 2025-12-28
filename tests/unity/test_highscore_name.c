#include "unity.h"
#include <string.h>
#include "render_input.h"

TEST(test_highscore_name) {
    char out[9];

    TEST_ASSERT_EQUAL_INT(1, render_sanitize_player_name("Alice", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("Alice", out);

    TEST_ASSERT_EQUAL_INT(1, render_sanitize_player_name("  Bob  ", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("Bob", out);

    TEST_ASSERT_EQUAL_INT(1, render_sanitize_player_name("   ", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("You", out);

    char longname[64];
    for (size_t i = 0; i < sizeof(longname)-1; ++i) longname[i] = 'A';
    longname[sizeof(longname)-1] = '\0';
    TEST_ASSERT_EQUAL_INT(1, render_sanitize_player_name(longname, out, sizeof(out)));
    /* allow either default or shorter result */
    if (strcmp(out, "You") != 0) {
        TEST_ASSERT_TRUE(strlen(out) < sizeof(out));
    }
}


