/* Legacy test moved to Unity: tests/unity/test_persist_long_lines.c */
#include <stdio.h>
int main(void) { fprintf(stderr, "Deprecated: use tests/unity/test_persist_long_lines.c\n"); return 77; }

int main(void) {
    const char* fn = "test_long_lines.tmp";
    FILE* f = fopen(fn, "w");
    if (!f) return -1;

    /* Very long single line (>> buffer size) */
    size_t long_len = 3 * 256; /* match buffer size used by persist.c */
    char* longline = malloc(long_len + 32);
    if (!longline) {
        fclose(f);
        return -1;
    }
    /* start with a valid entry then pad with garbage */
    strcpy(longline, "Alice 100 ");
    for (size_t i = strlen(longline); i < long_len - 2; ++i) longline[i] = 'X';
    longline[long_len - 2] = '\n';
    longline[long_len - 1] = '\0';
    fputs(longline, f);
    free(longline);
    fclose(f);

    int c = run_case(fn);
    /* Ensure no crash and we get a non-negative result */
    assert(c >= 0);

    remove(fn);

    /* Many entries: exceed PERSIST_MAX_SCORES */
    FILE* f2 = fopen(fn, "w");
    assert(f2 != NULL);
    for (int i = 0; i < PERSIST_MAX_SCORES + 5; ++i) {
        fprintf(f2, "N%02d  %d\n", i, i);
    }
    fclose(f2);
    int c2 = run_case(fn);
    /* Should not exceed the configured max */
    assert(c2 <= PERSIST_MAX_SCORES);

    remove(fn);
    printf("test_persist_long_lines: OK\n");
    return 0;
}
