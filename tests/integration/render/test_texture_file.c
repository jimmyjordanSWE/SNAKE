#include <assert.h>
#include <stdio.h>
#include "snake/render_3d_texture.h"

int main(void) {
    
    const char* fname = "tmp_test_texture.bmp";
    FILE* f = fopen(fname, "wb");
    if(!f) return 2;
    
    unsigned char hdr[54] = {0};
    fwrite(hdr, 1, sizeof(hdr), f);
    fclose(f);
    Texture3D* tex = texture_create();
    if(!tex) return 2;
    texture_init(tex);
    
    texture_load_from_file(tex, fname);
    texture_destroy(tex);
    remove(fname);
    return 0;
}
