#include <stdint.h>
#include <string.h>

#include "draw2d.h"

//#pragma GCC optimize ("O3")
#pragma GCC optimize ("O2")

#define MIN(a, b) \
  ((a) < (b)) ? (a) : (b)
#define MAX(a, b) \
  ((a) > (b)) ? (a) : (b)

#define ABS(x) ((x)<0) ? -(x) : (x)

#define CHECK_CLIP( x1, y1 ) \
  (x1 >= 0 && x1 < FRAMEBUFFER_WIDTH && y1 >= 0 && y1 < FRAMEBUFFER_HEIGHT)

uint8_t *screen_buffer; // pointer to actual screen buffer of the app (set during update proc)

static uint8_t current_color = 0x00; //Default to black
static uint8_t clear_color = 0x00; //Default to black

#if 0
static void swap_points(int32_t* v0, int32_t* v1) {
  *v0 ^= *v1;
  *v1 ^= *v0;
  *v0 ^= *v1;
}
#endif

// Bayer matrix for ordered dithering
static const uint8_t ditherMatrix[8][8] = {
        { 0*4, 32*4, 8*4, 40*4, 2*4, 34*4, 10*4, 42*4},
        {48*4, 16*4, 56*4, 24*4, 50*4, 18*4, 58*4, 26*4},
        {12*4, 44*4, 4*4, 36*4, 14*4, 46*4, 6*4, 38*4},
        {60*4, 28*4, 52*4, 20*4, 62*4, 30*4, 54*4, 22*4},
        { 3*4, 35*4, 11*4, 43*4, 1*4, 33*4, 9*4, 41*4},
        {51*4, 19*4, 59*4, 27*4, 49*4, 17*4, 57*4, 25*4},
        {15*4, 47*4, 7*4, 39*4, 13*4, 45*4, 5*4, 37*4},
        {63*4, 31*4, 55*4, 23*4, 61*4, 29*4, 53*4, 21*4}
};

uint8_t ditherPattern(int16_t y, uint8_t gray) {// calculate repeating, dithered gray for this loop
    // TODO: check if unrolling this loop is better
    const uint8_t ditherY = (uint8_t)y % 8;
    uint8_t ditheredByte = 0;
    for(uint8_t bit = 0; bit < 8; bit++){
        ditheredByte <<= 1;
        ditheredByte |= gray > ditherMatrix[ditherY][bit] ? 0x01 : 0x00;
    }
    return ditheredByte;
}

const int16_t SCREEN_WIDTH = FRAMEBUFFER_WIDTH;
const int16_t SCREEN_HEIGHT = FRAMEBUFFER_HEIGHT;
const uint8_t BYTES_PER_ROW = 20;

static void draw_pixel(int x, int y) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT|| x < 0 || y < 0 || !screen_buffer) return;
    int byte_offset = y*BYTES_PER_ROW + x/8;
    screen_buffer[byte_offset] &= ~(1<<(x%8));
    screen_buffer[byte_offset] |= (1<<(x%8));
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

void d2d_DrawScanLine(int _x0, int _y0, int _x1, int _y1) {
    int16_t y = (int16_t) _y0;
    int16_t x1 = (int16_t) _x0;
    int16_t x2 = (int16_t) _x1;
    if(x2<x1){
        //swap_points((int32_t *) &x1, (int32_t *) &x2);
        return;
    }
    x1--; //temp hack to help reduce seams by overlapping edges

    uint8_t gray = current_color;
    uint8_t *pixelData = (uint8_t *) screen_buffer;


    // clipping
    if(y<0 || y>= SCREEN_HEIGHT || x1 > x2)return;
    x1 = MAX(0, x1);
    x2 = MIN(SCREEN_WIDTH-1, x2);

    // start offset
    pixelData += y*BYTES_PER_ROW + x1/8;

    // masks for (optionally first and) last byte of line
    const uint8_t masks[9] = {
            (uint8_t) 0b00000000,
            (uint8_t) 0b00000001,
            (uint8_t) 0b00000011,
            (uint8_t) 0b00000111,
            (uint8_t) 0b00001111,
            (uint8_t) 0b00011111,
            (uint8_t) 0b00111111,
            (uint8_t) 0b01111111,
            (uint8_t) 0b11111111, // simplify masking the mask (see below)
    };
    const int16_t firstAlignedX = x1 & (int16_t)0b1111111111111000;
    const int16_t lastAlignedX = x2 & (int16_t)0b1111111111111000;
    const int16_t unalignedPixelsLeft = x1 % 8;
    const int16_t unalignedPixelsRight = x2 % 8;
    uint8_t ditheredByte= ditherPattern(y, gray);

    // actual loop, based on bytes not individual pixels
    int16_t x = x1;
    if(firstAlignedX < lastAlignedX){
        // first aligned byte
        if(unalignedPixelsLeft){
            const uint8_t mask = masks[unalignedPixelsLeft];
            *pixelData &= mask;
            *pixelData |= ditheredByte & ~mask;
            pixelData++;
            x += 8 - unalignedPixelsLeft;
        }

        // all but the last aligned byte
        while(x < lastAlignedX){
            *pixelData++ = ditheredByte;
            x += 8;
        }
    }

    // last aligned byte
    uint8_t mask = ~masks[unalignedPixelsRight + 1];
    // if there's just one byte on this line, mask the mask
    if(firstAlignedX == lastAlignedX ){
        mask ^= masks[unalignedPixelsLeft];
    }
    *pixelData &= mask;
    *pixelData |= ditheredByte & ~mask;
}

/*
 * Sets the color closest to the (r,g,b) value given in the parameters.  Will
 * use the global greyscale mode to determine how to best map and set the 
 * color.
 */
void d2d_SetColor(int r, int g, int b) {
    current_color = (uint8_t) r; // all components should be equal anyway
//  current_color = (r+r+r+b+g+g+g+g)>>3; //Fast Luminosity 8-bit
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
  clear_color = (uint8_t) r; // all components should be equal anyway
//  current_color = (r+r+r+b+g+g+g+g)>>3; //Fast Luminosity 8-bit
}

void d2d_ClearWindow(int sx, int sy, int w, int h) {
    if(!screen_buffer)return;

    for(int16_t y = (int16_t) 0; y<FRAMEBUFFER_HEIGHT; y++){
        uint8_t dither_byte = ditherPattern(y, clear_color);
        memset(&screen_buffer[y*BYTES_PER_ROW], dither_byte, BYTES_PER_ROW);
    }
}
