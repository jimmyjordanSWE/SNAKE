#include <assert.h>
#include "snake/render_3d_texture.h"

int main(void) {
    Texture3D* tex = texture_create();
    if(!tex) return 2;
    
    texture_load_from_file(tex, "assets/wall.png");
    texture_load_from_file(tex, "assets/floor.png");
    texture_destroy(tex);
    return 0;
}
