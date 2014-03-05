#ifndef DRAW2D_H
#define DRAW2D_H

void d2d_DrawLine(int x0, int y0, int x1, int y1);
void d2d_DrawScanLine(int x0, int y0, int x1, int y1);
void d2d_SetColor(int r, int g, int b);
void d2d_ClearColor(int r, int g, int b);
void d2d_DrawPixel(int x0, int y0);
void d2d_ClearWindow(int sx, int sy, int w, int h);

#endif

