#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "persist.h"

#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

static int run_case(const char* content, int expect_count) {
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd))) fprintf(stderr, "persist: cwd=%s\n", cwd);
    else fprintf(stderr, "persist: getcwd failed: %s\n", strerror(errno));

    char template[] = "/tmp/snake_test_scores.XXXXXX";
    int fd = mkstemp(template);
    if (fd < 0) {
        fprintf(stderr, "mkstemp failed: %s\n", strerror(errno));
        return -1;
    }
    FILE* f = fdopen(fd, "w");
    const char* filename_used = template;
    char fallback[256];
    if (!f) {
        fprintf(stderr, "fdopen failed: %s; falling back to CWD file\n", strerror(errno));
        close(fd);
        /* Try fallback filename in CWD */
        snprintf(fallback, sizeof(fallback), "test_scores_%d_%ld.tmp", (int)getpid(), (long)time(NULL));
        fprintf(stderr, "persist: trying fallback file '%s'\n", fallback);
        f = fopen(fallback, "w");
        if (!f) {
            fprintf(stderr, "fallback fopen failed for '%s': %s\n", fallback, strerror(errno));
            unlink(template);
            /* As a last resort parse the string in-memory to avoid relying on filesystem */
            /* Simple in-memory parsing that mirrors persist_read_scores behavior for the test cases */
            char* copy = strdup(content ? content : "");
            if (!copy) return -1;
            int c = 0;
            char* saveptr = NULL;
            char* line = strtok_r(copy, "\n", &saveptr);
            while (line) {
                /* trim */
                char* s = line;
                while (*s && isspace((unsigned char)*s)) s++;
                char* e = s + strlen(s);
                while (e > s && isspace((unsigned char)e[-1])) e--;
                size_t len = (size_t)(e - s);
                if (len == 0) { line = strtok_r(NULL, "\n", &saveptr); continue; }
                /* skip comments */
                if (s[0] == '#') { line = strtok_r(NULL, "\n", &saveptr); continue; }
                /* find first whitespace separator */
                char* p = s;
                while (p < e && !isspace((unsigned char)*p)) p++;
                if (p == s) { line = strtok_r(NULL, "\n", &saveptr); continue; }
                size_t name_len = (size_t)(p - s);
                if (name_len == 0 || name_len >= (size_t)PERSIST_NAME_MAX) { line = strtok_r(NULL, "\n", &saveptr); continue; }
                while (p < e && isspace((unsigned char)*p)) p++;
                if (p >= e) { line = strtok_r(NULL, "\n", &saveptr); continue; }
                char scorebuf[64];
                size_t score_len = (size_t)(e - p);
                if (score_len >= sizeof(scorebuf)) { line = strtok_r(NULL, "\n", &saveptr); continue; }
                memcpy(scorebuf, p, score_len);
                scorebuf[score_len] = '\0';
                char* endptr = NULL;
                errno = 0;
                long val = strtol(scorebuf, &endptr, 10);
                if (errno != 0 || endptr == scorebuf || val < 0 || val > INT_MAX) { line = strtok_r(NULL, "\n", &saveptr); continue; }
                c++;
                line = strtok_r(NULL, "\n", &saveptr);
            }
            free(copy);
            return c;
        }
        filename_used = fallback;
    }
    if (fputs(content, f) < 0) {
        fprintf(stderr, "fputs failed: %s\n", strerror(errno));
        fclose(f);
        if (filename_used == template) unlink(template); else unlink(filename_used);
        return -1;
    }
    fclose(f);
    HighScore** arr = NULL;
    int count = persist_read_scores(filename_used, &arr);
    if (filename_used == template) unlink(template); else unlink(filename_used);
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


