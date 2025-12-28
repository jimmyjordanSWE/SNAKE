#include "unity.h"
#include "stb_image.h"

TEST(test_stb_image_fuzz) {
    /* This test ensures fuzzing harness can be invoked (no actual fuzz inputs here) */
    /* No-op smoke test: expecting stbi_load to handle invalid input gracefully */
    int w=0,h=0,c=0;
    /* create a tiny invalid file to pass through stbi_load path */
    const char* fn = "test_stb_fuzz_small.tmp";
    FILE* f = fopen(fn, "wb"); if (f) { fwrite("\0\0\0", 1, 3, f); fclose(f); }
    unsigned char* img = stbi_load(fn, &w, &h, &c, 0);
    if (img) stbi_image_free(img);
    remove(fn);
}


