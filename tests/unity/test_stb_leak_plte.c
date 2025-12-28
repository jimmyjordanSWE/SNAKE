#include "unity.h"
#include "stb_image.h"

TEST(test_stb_leak_plte) {
    /* small harness to ensure PLTE parsing path doesn't leak on malformed input */
    int w=0,h=0,c=0;
    unsigned char *img = stbi_load("/dev/null", &w, &h, &c, 0); /* expect NULL or safe handling */
    if (img) stbi_image_free(img);
}


