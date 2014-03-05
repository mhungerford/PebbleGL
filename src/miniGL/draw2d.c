#include <stdint.h>
#include <string.h>

#include "draw2d.h"

#pragma GCC push_options
#pragma GCC optimize ("O3")

#define MIN(a, b) \
  ((a) < (b)) ? (a) : (b)
#define MAX(a, b) \
  ((a) > (b)) ? (a) : (b)

#define ABS(x) ((x)<0) ? -(x) : (x)

#define CHECK_CLIP( x1, y1 ) \
  (x1 >= 0 && x1 < FRAMEBUFFER_WIDTH && y1 >= 0 && y1 < FRAMEBUFFER_HEIGHT)

//4-bit grayscale, only need support in framebuffer and draw_pixel
uint8_t framebuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT / 2];
static uint8_t current_color = 0x00; //Default to black
static uint8_t clear_color = 0x00; //Default to black

static void swap_points(int32_t* v0, int32_t* v1) {
  *v0 ^= *v1; 
  *v1 ^= *v0; 
  *v0 ^= *v1;
}


static void draw_pixel(int32_t x0, int32_t y0) {
  if (CHECK_CLIP(x0, y0)) {
    int i = (y0*FRAMEBUFFER_WIDTH + x0) / 2;
    if (x0 % 2 == 1) {
      framebuffer[i] = (framebuffer[i] & 0xF0) | (current_color & 0x0F);
    } else {
      framebuffer[i] = (framebuffer[i] & 0x0F) | ((current_color & 0x0F) << 4);
    }
  }
}


void d2d_DrawLine(int x0, int y0, int x1, int y1) { 
  // compute difference between start and end
  int dx = ABS(x1 - x0);
  int dy = ABS(y1 - y0);
  int step_x = (x0 > x1) ? -1 : 1;  // direction of x steps
  int step_y = (y0 > y1) ? -1 : 1;  // direction of y steps

  int delta;  // accumulated error from slope
  int x = x0; // placement points (initially start of line)
  int y = y0; // placement points (initially start of line)

  // find the fastest direction of traversal
  if( dx > dy ) {
    draw_pixel(x, y);
    // rounding error ok, we are moving in pixels(ints)
    delta = dx / 2;
    while( x != x1 ) {
      delta -= dy;
      if( delta < 0 ) {
        y += step_y;
        delta += dx;
      }
      x += step_x;
      draw_pixel(x, y);
    }
  } else {
    // rounding error ok, we are moving in pixels(ints)
    draw_pixel(x, y);
    delta = dy / 2;
    while( y != y1 ) {
      delta -= dx;
      if( delta < 0 ) {
        x += step_x;
        delta += dy;
      }
      y += step_y;
      draw_pixel(x, y);
    }
  }
}

//y0 and y1 guaranteed to be the same, so no need for stepping
//no need for clipping if we test and bound
// TODO: need some more work, start or end pixel not aligned
void d2d_DrawScanLine(int x0, int y0, int x1, int y1) { 
  if (y0 >= FRAMEBUFFER_HEIGHT || y0 < 0) return;

  if (x0 > x1) swap_points(&x0, &x1);

  x0 = MAX(x0, 0);
  x1 = MIN(x1, FRAMEBUFFER_WIDTH-1);
  memset(&framebuffer[y0 * FRAMEBUFFER_WIDTH / 2 + x0 / 2], 
    (current_color << 4) | current_color,
    (x1 - x0) / 2);
}

/*
 * Sets the color closest to the (r,g,b) value given in the parameters.  Will
 * use the global greyscale mode to determine how to best map and set the 
 * color.
 */
void d2d_SetColor(int r, int g, int b) {
  //current_color = (r+r+r+b+g+g+g+g)>>3; //Fast Luminosity 8-bit
  current_color = (r+r+r+b+g+g+g+g)>>6; //Fast Luminosity 4-bit
}

/*
 * Using the current greyscale mode, draws a pixel on the screen using the 
 * current color.
 */
void d2d_DrawPixel(int x0, int y0) {
  //if (CHECK_CLIP(x0, y0)) {
    draw_pixel( x0, y0 );
  //}
}

void d2d_ClearColor(int r, int g, int b){
  //current_color = (r+r+r+b+g+g+g+g)>>3; //Fast Luminosity 8-bit
  clear_color = (r+r+r+b+g+g+g+g)>>6; //Fast Luminosity 4-bit
}

void d2d_ClearWindow(int sx, int sy, int w, int h) {
  //fast char aligned memset
  uint32_t clearvalue = clear_color << 4 | clear_color;
  memset( framebuffer, clearvalue, sizeof(framebuffer));

  //if( (sx%2) && (w%2) ){
  //  for(int row = sy; row < sy+h; row++){
  //    memset( &framebuffer[ (sy*FRAMEBUFFER_WIDTH + sx) / 2 ], 0xFF, w/2 );
  //  }
  //}else{ //slow hack, need to manually keep other pixel when not aligned
    //Todo
  //}
}
#pragma GCC pop_options
