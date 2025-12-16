#include <assert.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "snake/render_3d_projection.h"

int main(void) {
    
    Projection3D* p = projection_create(800, 600, (float)(M_PI/3.0), 1.0f);
    if(!p) return 2;
    projection_destroy(p);
    return 0;
}
