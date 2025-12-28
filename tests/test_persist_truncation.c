/* Legacy test moved to Unity: tests/unity/test_persist_truncation.c */
#include <stdio.h>
int main(void) { fprintf(stderr, "Deprecated: use tests/unity/test_persist_truncation.c\n"); return 77; }

    /* Create a truncated line that does not end with a newline and exceeds buffer size */
    size_t bufsize = 256; /* matches PERSIST_SCORE_BUFFER */
    char *big = malloc(bufsize + 64);
    if (!big) { fclose(f); return -1; }
    /* Fill so that fgets will read a partial line without a newline */
    for (size_t i = 0; i < bufsize + 10; ++i) big[i] = 'A';
    /* Do not add a newline at the end of this big line */
    fwrite(big, 1, bufsize + 10, f);
    /* Now add a well-formed second line */
    fprintf(f, "\nBob 10\n");
    free(big);
    fclose(f);

    HighScore** arr = NULL;
    int count = persist_read_scores(fn, &arr);
    /* The truncated entry should be ignored, only the valid entry should be read */
    assert(count == 1);
    if (arr) persist_free_scores(arr, count);

    remove(fn);
    printf("test_persist_truncation: OK\n");
    return 0;
}
