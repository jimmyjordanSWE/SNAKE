#include "snake/render_3d_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
    
    
    unsigned char colors[4][3] = {
        {255,0,0}, 
        {0,255,0}, 
        {0,0,255}, 
        {255,255,255} 
    };
    
    for(unsigned int y=0; y<height; y++){
        for(unsigned int x=0; x<width; x++){
            unsigned int idx = (y*rowSize) + x*3;
            unsigned char* c;
            if(y==0 && x==0) c = colors[2]; 
            else if(y==0 && x==1) c = colors[3]; 
            else if(y==1 && x==0) c = colors[0]; 
            else c = colors[1]; 
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
    Texture3D* tex = texture_create();
    if(!tex) { fprintf(stderr, "texture_create failed\n"); remove(fname); return 1; }
    if(!texture_load_from_file(tex, fname)) { fprintf(stderr, "texture_load_from_file failed\n"); texture_destroy(tex); remove(fname); return 1; }
    if(texture_get_img_w(tex) != 2 || texture_get_img_h(tex) != 2) { fprintf(stderr, "unexpected dims %d x %d\n", texture_get_img_w(tex), texture_get_img_h(tex)); texture_destroy(tex); remove(fname); return 1; }
    
    uint32_t c_red = texture_sample(tex, 0.25f, 0.25f, false); 
    uint32_t c_blue = texture_sample(tex, 0.25f, 0.75f, false);
    uint32_t c_green = texture_sample(tex, 0.75f, 0.75f, false);
    uint32_t c_white = texture_sample(tex, 0.75f, 0.25f, false);
    (void)c_red; (void)c_blue; (void)c_green; (void)c_white;
    
    if(texture_get_pixels(tex) == NULL) { fprintf(stderr, "no pixels\n"); texture_destroy(tex); remove(fname); return 1; }
    if(c_red == 0 || c_blue == 0 || c_green == 0 || c_white == 0) { fprintf(stderr, "sample returned zero color\n"); texture_destroy(tex); remove(fname); return 1; }
    texture_free_image(tex);
    texture_destroy(tex);
    remove(fname);
    return 0;
}
