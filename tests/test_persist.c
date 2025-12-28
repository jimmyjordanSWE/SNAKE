/* Legacy test moved to Unity: tests/unity/test_persist.c */
#include <stdio.h>
int main(void) { fprintf(stderr, "Deprecated: use tests/unity/test_persist.c\n"); return 77; }

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

    /* multiple spaces between name and score */
    int c7b = run_case("Alice   100\n", 1);
    assert(c7b == 1);

    /* leading spaces before name */
    int c7c = run_case("   Bob 20\n", 1);
    assert(c7c == 1);

    /* trailing spaces after score */
    int c7d = run_case("Carol 30   \n", 1);
    assert(c7d == 1);

    /* tab between name and score */
    int c7e = run_case("Dora\t40\n", 1);
    assert(c7e == 1);

    /* exact maximum name length (8 chars allowed) */
    int c7f = run_case("ABCDEFGH 77\n", 1);
    assert(c7f == 1);

    /* name too long (9 chars) should be rejected */
    int c7g = run_case("ABCDEFGHI 88\n", 0);
    assert(c7g == 0);

    /* very large score (non-numeric/overflow) */
    int c8 = run_case("Frank 9999999999999999999999999999\n", 0);
    assert(c8 == 0);

    /* empty file */
    int c9 = run_case("", 0);
    assert(c9 == 0);

    printf("test_persist: OK\n");
    return 0;
}