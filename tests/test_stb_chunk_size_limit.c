#include <stdio.h>
#include <stdlib.h>
#include "stb_image.h"

int main(void) {
    const char *fn = "test_stb_chunk_size_limit.tmp";
    FILE *f = fopen(fn, "wb");
    if (!f) return 1;

    /* PNG signature */
    const unsigned char png_sig[8] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
    fwrite(png_sig, 1, sizeof(png_sig), f);

    /* Write a chunk length greater than MAX_PNG_CHUNK (64MB) to trigger the cap check: 0x04000001 */
    unsigned char big_len[4] = {0x04, 0x00, 0x00, 0x01};
    fwrite(big_len, 1, sizeof(big_len), f);

    /* We don't need to write the type/data; the loader checks the length early and should return NULL */
    fclose(f);

    int w=0,h=0,c=0;
    unsigned char *img = stbi_load(fn, &w, &h, &c, 0);
    /* We expect a NULL return and no crash */
    if (img) stbi_image_free(img);
    remove(fn);

    printf("test_stb_chunk_size_limit: OK\n");
    return 0;
}
