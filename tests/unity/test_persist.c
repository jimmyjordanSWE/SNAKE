#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "persist.h"

#include <unistd.h>

static int run_case(const char* content, int expect_count) {
    char template[] = "/tmp/snake_test_scores.XXXXXX";
    int fd = mkstemp(template);
    if (fd < 0) return -1;
    FILE* f = fdopen(fd, "w");
    if (!f) { close(fd); unlink(template); return -1; }
    if (fputs(content, f) < 0) { fclose(f); unlink(template); return -1; }
    fclose(f);
    HighScore** arr = NULL;
    int count = persist_read_scores(template, &arr);
    unlink(template);
    if (arr) persist_free_scores(arr, count);
    return count;
}

TEST(test_persist) {
    int c = run_case("Alice 100\nBob 50\n", 2);
    TEST_ASSERT_EQUAL_INT(2, c);

    char longname[512];
    memset(longname, 'A', sizeof(longname)-2);
    longname[sizeof(longname)-2] = ' ';
    longname[sizeof(longname)-1] = '\0';
    int c2 = run_case(longname, 0);
    TEST_ASSERT_EQUAL_INT(0, c2);

    int c3 = run_case("Eve abc\n", 0);
    TEST_ASSERT_EQUAL_INT(0, c3);

    int c4 = run_case("# comment\n\nCarol 30\n", 1);
    TEST_ASSERT_EQUAL_INT(1, c4);

    int c5 = run_case("Dave \n", 0);
    TEST_ASSERT_EQUAL_INT(0, c5);

    int c6 = run_case(" 100\n", 0);
    TEST_ASSERT_EQUAL_INT(0, c6);

    int c7 = run_case("Ellen 40", 1);
    TEST_ASSERT_EQUAL_INT(1, c7);

    int c7b = run_case("Alice   100\n", 1);
    TEST_ASSERT_EQUAL_INT(1, c7b);

    int c7c = run_case("   Bob 20\n", 1);
    TEST_ASSERT_EQUAL_INT(1, c7c);

    int c7d = run_case("Carol 30   \n", 1);
    TEST_ASSERT_EQUAL_INT(1, c7d);

    int c7e = run_case("Dora\t40\n", 1);
    TEST_ASSERT_EQUAL_INT(1, c7e);

    int c7f = run_case("ABCDEFGH 77\n", 1);
    TEST_ASSERT_EQUAL_INT(1, c7f);

    int c7g = run_case("ABCDEFGHI 88\n", 0);
    TEST_ASSERT_EQUAL_INT(0, c7g);

    int c8 = run_case("Frank 9999999999999999999999999999\n", 0);
    TEST_ASSERT_EQUAL_INT(0, c8);

    int c9 = run_case("", 0);
    TEST_ASSERT_EQUAL_INT(0, c9);
}


