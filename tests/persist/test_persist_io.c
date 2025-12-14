#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "snake/persist.h"

static char* make_temp_file(void) {
    char* template = strdup("/tmp/snake_test_scores_XXXXXX");
    int fd = mkstemp(template);
    if (fd >= 0) close(fd);
    return template;
}

int main(void) {
    char* fname = make_temp_file();
    HighScore scores[3] = {{"alice", 10}, {"bob", 5}, {"carol", 7}};
    assert(persist_write_scores(fname, scores, 3));
    HighScore out[5];
    int cnt = persist_read_scores(fname, out, 5);
    assert(cnt == 3);
    assert(strcmp(out[0].name, "alice") == 0);
    /* append higher score should replace low score when full */
    assert(persist_append_score(fname, "dave", 20));
    cnt = persist_read_scores(fname, out, 5);
    assert(cnt >= 1);

    /* config write/read */
    GameConfig cfg = {30, 15, 50, 1, 80, 25, 1};
    char* cfgfile = make_temp_file();
    assert(persist_write_config(cfgfile, &cfg));
    GameConfig loaded;
    assert(persist_load_config(cfgfile, &loaded));
    assert(loaded.board_width == 30 && loaded.board_height == 15);

    unlink(fname); free(fname);
    unlink(cfgfile); free(cfgfile);
    return 0;
}
