#include "snake/render_3d_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Write a simple 2x2 24-bit BMP file with known colors to `filename`. */
static int write_test_bmp(const char* filename) {
    unsigned int width = 2, height = 2;
    unsigned short bpp = 24;
    unsigned int rowSize = ((bpp * width + 31) / 32) * 4;
    unsigned int dataSize = rowSize * height;
    unsigned int fileSize = 54 + dataSize;
    unsigned char header[54] = {0};
    header[0] = 'B'; header[1] = 'M';
    memcpy(header + 2, &fileSize, 4);
    unsigned int dataOffset = 54;
    memcpy(header + 10, &dataOffset, 4);
    unsigned int dibSize = 40;
    memcpy(header + 14, &dibSize, 4);
    memcpy(header + 18, &width, 4);
    memcpy(header + 22, &height, 4);
    unsigned short planes = 1;
    memcpy(header + 26, &planes, 2);
    memcpy(header + 28, &bpp, 2);
    memcpy(header + 34, &dataSize, 4);
    unsigned char* pixelData = (unsigned char*)calloc(1, dataSize);
    if(!pixelData) return 0;
    /* BMP stores pixels bottom-up, BGR order, row padding to rowSize */
    /* Let's set pixels: (x,y): (0,0)=red, (1,0)=green, (0,1)=blue, (1,1)=white */
    unsigned char colors[4][3] = {
        {255,0,0}, /* red */
        {0,255,0}, /* green */
        {0,0,255}, /* blue */
        {255,255,255} /* white */
    };
    /* row 0 is bottom row */
    for(unsigned int y=0; y<height; y++){
        for(unsigned int x=0; x<width; x++){
            unsigned int idx = (y*rowSize) + x*3;
            unsigned char* c;
            if(y==0 && x==0) c = colors[2]; /* bottom-left = blue */
            else if(y==0 && x==1) c = colors[3]; /* bottom-right = white */
            else if(y==1 && x==0) c = colors[0]; /* top-left = red */
            else c = colors[1]; /* top-right = green */
            pixelData[idx+0] = c[2];
            pixelData[idx+1] = c[1];
            pixelData[idx+2] = c[0];
        }
    }
    FILE* f = fopen(filename, "wb");
    if(!f) { free(pixelData); return 0; }
    fwrite(header, 1, 54, f);
    fwrite(pixelData, 1, dataSize, f);
    fclose(f);
    free(pixelData);
    return 1;
}

int main(void) {
    const char* fname = "tmp_test_texture.bmp";
    if(!write_test_bmp(fname)) { fprintf(stderr, "failed write bmp\n"); return 1; }
    Texture3D tex = {0};
    texture_init(&tex);
    if(!texture_load_from_file(&tex, fname)) { fprintf(stderr, "texture_load_from_file failed\n"); return 1; }
    if(tex.img_w != 2 || tex.img_h != 2) { fprintf(stderr, "unexpected dims %d x %d\n", tex.img_w, tex.img_h); return 1; }
    /* sample centers: u=(x+0.5)/2, v=(y+0.5)/2 where (0,0) is left-top -> sample top-left red at (0.25,0.75) because our sample uses v flipped */
    uint32_t c_red = texture_sample(&tex, 0.25f, 0.25f, false); /* bottom-left example */
    uint32_t c_blue = texture_sample(&tex, 0.25f, 0.75f, false);
    uint32_t c_green = texture_sample(&tex, 0.75f, 0.75f, false);
    uint32_t c_white = texture_sample(&tex, 0.75f, 0.25f, false);
    (void)c_red; (void)c_blue; (void)c_green; (void)c_white;
    /* basic sanity: colors non-zero and image data present */
    if(tex.pixels == NULL) { fprintf(stderr, "no pixels\n"); return 1; }
    if(c_red == 0 || c_blue == 0 || c_green == 0 || c_white == 0) { fprintf(stderr, "sample returned zero color\n"); return 1; }
    texture_free_image(&tex);
    remove(fname);
    return 0;
}
