#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "stb_image.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    char tmp[] = "/tmp/snake_stb_XXXXXX";
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

    /* Use the file-based API exposed in the bundled header */
    int x = 0, y = 0, n = 0;
    unsigned char* img = stbi_load(tmp, &x, &y, &n, 0);
    if (img)
        stbi_image_free(img);

    (void)remove(tmp);
    return 0;
}
