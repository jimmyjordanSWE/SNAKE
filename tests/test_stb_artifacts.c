#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include "stb_image.h"

int main(void) {
    const char* dir = "build/fuzz/artifacts";
    DIR *d = opendir(dir);
    if (!d) {
        printf("test_stb_artifacts: artifacts directory not present, skipping\n");
        return 0;
    }
    struct dirent *ent;
    int tested = 0;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "leak-", 5) != 0 && strncmp(ent->d_name, "crash-", 6) != 0)
            continue;
        tested++;
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        FILE* f = fopen(path, "rb");
        if (!f) {
            printf("missing artifact: %s, skipping\n", path);
            continue;
        }
        fclose(f);
        int w=0,h=0,c=0;
        unsigned char *img = stbi_load(path, &w, &h, &c, 0);
        if (img) stbi_image_free(img);
        printf("tested: %s -> w=%d h=%d c=%d img=%p\n", ent->d_name, w, h, c, (void*)img);
    }
    closedir(d);
    if (tested == 0) {
        printf("test_stb_artifacts: no relevant artifacts found, skipping\n");
    } else {
        printf("test_stb_artifacts: tested %d artifacts\n", tested);
    }
    return 0;
}
