#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "stb_image.h"

/* Using only the vendored file-based API (no memory API in this vendored copy) */

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    /* Early exit for empty inputs */
    if (size == 0)
        return 0;

    /* Cap the amount we write to disk to keep fuzz iterations fast (1MB) */
    size_t write_size = size > 1048576 ? 1048576 : size;

    /* Exercise the file-based API for coverage */
    char tmp[] = "/tmp/snake_stb_XXXXXX";
    int fd = mkstemp(tmp);
    if (fd != -1) {
        FILE* f = fdopen(fd, "wb");
        if (f) {
            if (write_size > 0)
                fwrite(data, 1, write_size, f);
            (void)fflush(f);
            fclose(f);
        } else {
            close(fd);
        }

        int x = 0, y = 0, n = 0;
        /* Exercise the file-based API for coverage (inputs are capped to 1MB above) */
        unsigned char* img = stbi_load(tmp, &x, &y, &n, 0);
        if (img)
            stbi_image_free(img);
        /* Call again to exercise allocations/frees */
        img = stbi_load(tmp, &x, &y, &n, 0);
        if (img)
            stbi_image_free(img);

        (void)remove(tmp);
    }

    return 0;
}
