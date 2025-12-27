#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "stb_image.h"

int main(void) {
    const char *fn = "build/fuzz/artifacts/leak-c46c42b93580ff0584604ce5ad8028e84a9abb63";
    FILE *f = fopen(fn, "rb");
    if (!f) {
        printf("test_stb_leak_plte: artifact missing, skipping\n");
        return 0;
    }
    fclose(f);

    int w=0,h=0,c=0;
    unsigned char *img = stbi_load(fn, &w, &h, &c, 0);
    if (img) stbi_image_free(img);

    printf("test_stb_leak_plte: OK (no crash)\n");
    return 0;
}
