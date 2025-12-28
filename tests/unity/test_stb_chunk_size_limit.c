#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include "stb_image.h"

TEST(test_stb_chunk_size_limit) {
    const char *fn = "test_stb_chunk_size_limit.tmp";
    FILE *f = fopen(fn, "wb");
    TEST_ASSERT_TRUE(f != NULL);

    const unsigned char png_sig[8] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
    fwrite(png_sig, 1, sizeof(png_sig), f);

    unsigned char big_len[4] = {0x04, 0x00, 0x00, 0x01};
    fwrite(big_len, 1, sizeof(big_len), f);

    fclose(f);

    int w=0,h=0,c=0;
    unsigned char *img = stbi_load(fn, &w, &h, &c, 0);
    if (img) stbi_image_free(img);
    remove(fn);
}
