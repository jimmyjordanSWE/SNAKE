#include "unity.h"
#include "stb_image.h"

TEST(test_stb_artifacts) {
    /* Ensure stb internal checks do not crash on small invalid inputs */
    int w=0,h=0,c=0;
    unsigned char *img = stbi_load("/dev/null", &w, &h, &c, 0); /* expect NULL or safe handling */
    if (img) stbi_image_free(img);
}


