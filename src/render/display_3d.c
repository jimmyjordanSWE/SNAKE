#include "snake/render_3d_display.h"
#include <stdlib.h>
#include <string.h>

struct Display3D {
    int width;
    int height;
    Framebuffer3DCell* buffer;
};

Display3D* display_3d_create(int width, int height) {
    Display3D* d = (Display3D*)calloc(1, sizeof(*d));
    if(!d) return NULL;
    display_3d_init(d, width, height);
    return d;
}
void display_3d_destroy(Display3D* disp) {
    if(!disp) return;
    display_3d_shutdown(disp);
    free(disp);
}
int display_3d_get_width(const Display3D* disp) { return disp ? disp->width : 0; }
int display_3d_get_height(const Display3D* disp) { return disp ? disp->height : 0; }
Framebuffer3DCell* display_3d_get_buffer(Display3D* disp) { return disp ? disp->buffer : NULL; }

void display_3d_init(Display3D* disp, int width, int height) {
if(!disp) return;
disp->width= width;
disp->height= height;
disp->buffer= (Framebuffer3DCell*)malloc(sizeof(Framebuffer3DCell) * (size_t)width * (size_t)height);
if(disp->buffer) display_3d_clear(disp, ' ', 0);
}
void display_3d_clear(Display3D* disp, char ch, uint16_t color) {
if(!disp || !disp->buffer) return;
Framebuffer3DCell cell= {.character= ch, .color= color};
for(int i= 0; i < disp->width * disp->height; i++) disp->buffer[i]= cell;
}
void display_3d_draw_column(Display3D* disp, int col, int start_row, int end_row, char ch, uint16_t color) {
if(!disp || col < 0 || col >= disp->width) return;
if(start_row < 0) start_row= 0;
if(end_row >= disp->height) end_row= disp->height - 1;
for(int row= start_row; row <= end_row; row++) display_3d_draw_cell(disp, col, row, ch, color);
}
void display_3d_draw_cell(Display3D* disp, int x, int y, char ch, uint16_t color) {
if(!disp || !disp->buffer || x < 0 || x >= disp->width || y < 0 || y >= disp->height) return;
int idx= y * disp->width + x;
disp->buffer[idx].character= ch;
disp->buffer[idx].color= color;
}
void display_3d_draw_row(Display3D* disp, int row, int start_col, int end_col, char ch, uint16_t color) {
if(!disp || row < 0 || row >= disp->height) return;
if(start_col < 0) start_col= 0;
if(end_col >= disp->width) end_col= disp->width - 1;
for(int col= start_col; col <= end_col; col++) display_3d_draw_cell(disp, col, row, ch, color);
}
const Framebuffer3DCell* display_3d_get_cell(const Display3D* disp, int x, int y) {
if(!disp || !disp->buffer || x < 0 || x >= disp->width || y < 0 || y >= disp->height) return NULL;
int idx= y * disp->width + x;
return &disp->buffer[idx];
}
void display_3d_present(Display3D* disp) {
if(!disp) return;
}
void display_3d_shutdown(Display3D* disp) {
if(!disp) return;
if(disp->buffer) {
free(disp->buffer);
disp->buffer= NULL;
}
} 
