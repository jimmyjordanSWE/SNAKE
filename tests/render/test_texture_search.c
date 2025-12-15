#include "snake/render_3d_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

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
        {255,0,0}, /* red */
        {0,255,0}, /* green */
        {0,0,255}, /* blue */
        {255,255,255} /* white */
    };
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
    /* Create a temp dir with structure: <tmp>/assets/wall.bmp and chdir into <tmp>/run and try to load "assets/wall.bmp" */
    char tmpl[] = "/tmp/snake_test.XXXXXX";
    char* dir = mkdtemp(tmpl);
    if(!dir) { perror("mkdtemp"); return 1; }
    char assets_dir[1024]; snprintf(assets_dir, sizeof(assets_dir), "%s/assets", dir);
    if(mkdir(assets_dir, 0755) != 0) { perror("mkdir assets"); return 1; }
    char run_dir[1024]; snprintf(run_dir, sizeof(run_dir), "%s/run", dir);
    if(mkdir(run_dir, 0755) != 0) { perror("mkdir run"); return 1; }
    char bmp_path[1024]; bmp_path[0] = '\0'; strncpy(bmp_path, assets_dir, sizeof(bmp_path)-1); bmp_path[sizeof(bmp_path)-1] = '\0';
    strncat(bmp_path, "/wall.bmp", sizeof(bmp_path) - strlen(bmp_path) - 1);
    if(!write_test_bmp(bmp_path)) { fprintf(stderr, "failed write bmp\n"); return 1; }

    /* chdir into run dir (child) and attempt to load "assets/wall.bmp" */
    if(chdir(run_dir) != 0) { perror("chdir"); return 1; }

    Texture3D tex = {0};
    texture_init(&tex);
    if(!texture_load_from_file(&tex, "assets/wall.bmp")) { fprintf(stderr, "texture_load_from_file failed to find fallback\n"); return 1; }
    if(tex.img_w != 2 || tex.img_h != 2) { fprintf(stderr, "unexpected dims %d x %d\n", tex.img_w, tex.img_h); return 1; }
    if(tex.pixels == NULL) { fprintf(stderr, "no pixels\n"); return 1; }
    texture_free_image(&tex);

    /* cleanup */
    unlink(bmp_path);
    rmdir(assets_dir);
    rmdir(run_dir);
    rmdir(dir);
    return 0;
}
