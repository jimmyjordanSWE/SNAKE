#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "persist.h"

static int run_case(const char* content, int expect_count) {
    const char* fn = "test_scores.tmp";
    FILE* f = fopen(fn, "w");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    HighScore** arr = NULL;
    int count = persist_read_scores(fn, &arr);
    remove(fn);
    if (arr) persist_free_scores(arr, count);
    return count;
}

int main(void) {
    /* well-formed */
    int c = run_case("Alice 100\nBob 50\n", 2);
    assert(c == 2);

    /* too long name */
    char longname[512];
    memset(longname, 'A', sizeof(longname)-2);
    longname[sizeof(longname)-2] = ' ';
    longname[sizeof(longname)-1] = '\0';
    int c2 = run_case(longname, 0);
    assert(c2 == 0);

    /* non-numeric score */
    int c3 = run_case("Eve abc\n", 0);
    assert(c3 == 0);

    /* empty lines and comments ignored */
    int c4 = run_case("# comment\n\nCarol 30\n", 1);
    assert(c4 == 1);

    /* truncated line (no score) */
    int c5 = run_case("Dave \n", 0);
    assert(c5 == 0);

    /* missing name (score only) */
    int c6 = run_case(" 100\n", 0);
    assert(c6 == 0);

    /* missing newline at EOF */
    int c7 = run_case("Ellen 40", 1);
    assert(c7 == 1);

    /* very large score (non-numeric/overflow) */
    int c8 = run_case("Frank 9999999999999999999999999999\n", 0);
    assert(c8 == 0);

    /* empty file */
    int c9 = run_case("", 0);
    assert(c9 == 0);

    printf("test_persist: OK\n");
    return 0;
}