#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "persist.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    char tmp[] = "/tmp/snake_fuzz_XXXXXX";
    int fd = mkstemp(tmp);
    if (fd == -1)
        return 0;
    FILE* f = fdopen(fd, "wb");
    if (!f) {
        close(fd);
        remove(tmp);
        return 0;
    }
    size_t wrote = 0;
    if (size > 0)
        wrote = fwrite(data, 1, size, f);
    (void)fflush(f);
    fclose(f);
    HighScore** arr = NULL;
    if (wrote == size) {
        int cnt = persist_read_scores(tmp, &arr);
        if (cnt > 0)
            persist_free_scores(arr, cnt);
    }
    (void)remove(tmp);
    return 0; 
}