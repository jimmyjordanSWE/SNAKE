#include <assert.h>
#include "snake/render_3d_texture.h"

int main(void) {
    Texture3D* t = texture_create();
    if(!t) return 2;
    texture_destroy(t);
    return 0;
}
