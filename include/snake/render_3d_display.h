#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
char character;
uint16_t color;
} Framebuffer3DCell;
typedef struct {
int width;
int height;
Framebuffer3DCell* buffer;
} Display3D;
void display_3d_init(Display3D* disp, int width, int height);
void display_3d_clear(Display3D* disp, char ch, uint16_t color);
void display_3d_draw_column(Display3D* disp, int col, int start_row, int end_row, char ch, uint16_t color);
void display_3d_draw_cell(Display3D* disp, int x, int y, char ch, uint16_t color);
void display_3d_draw_row(Display3D* disp, int row, int start_col, int end_col, char ch, uint16_t color);
const Framebuffer3DCell* display_3d_get_cell(const Display3D* disp, int x, int y);
void display_3d_present(Display3D* disp);
void display_3d_shutdown(Display3D* disp);