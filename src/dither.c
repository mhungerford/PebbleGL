#include <inttypes.h>
#include <string.h>
#include "dither.h"

#pragma GCC push_options
#pragma GCC optimize ("O3")


static __attribute__((always_inline)) inline uint8_t extract_grey(uint8_t* grey, int i) {
  uint8_t val;
  if (i%2 == 1) {
    val = (grey[i/2] & 0x0F) << 4;
  } else {
    val = (grey[i/2] & 0xF0);
  }
  return val | (val >> 4);
}

// Note: low bits are ignored.
static __attribute__((always_inline)) inline void set_grey(uint8_t* grey, int i, uint8_t val) {
  if (i%2 == 1) {
    grey[i/2] = (grey[i/2] & 0xF0) | (val >> 4);
  } else {
    grey[i/2] = (grey[i/2] & 0x0F) | (val & 0xF0);
  }
}

static __attribute__((always_inline)) inline int clamp(int n, int a, int b) {
  return n < a ? a : n > b ? b : n;
}

static __attribute__((always_inline)) inline void add_grey(uint8_t* grey, int i, int amount) {
  uint8_t val = extract_grey(grey, i);
  set_grey(grey, i, clamp(val + amount, 0, 255));
}

static __attribute__((always_inline)) inline void set_black(uint8_t* bw, int i) {
  bw[i/8] &= ~(1 << i%8);
}

static __attribute__((always_inline)) inline void set_white(uint8_t* bw, int i) {
  bw[i/8] |= 1 << i%8;
}

static __attribute__((always_inline)) inline void set_bw(uint8_t* bw, int i, uint8_t val) {
  if (val > 127) {
    set_white(bw, i);
  } else {
    set_black(bw, i); 
  }
}

static __attribute__((always_inline)) inline void naive_dither(uint8_t* grey, uint8_t* bw, int num_pixels) {
  for (int i = 0; i < num_pixels; i++) {
    set_bw(bw, i, extract_grey(grey, i));
  }
}

static __attribute__((always_inline)) inline int index_bw(int x, int y) {
  return x*160 + y;
}

static __attribute__((always_inline)) inline int index(int x, int y) {
  return x*144 + y;
}

// WARNING: Modifies grey buffer
void floyd_steinberg_dither(uint8_t* grey, uint8_t* bw, int num_pixels) {
  int w = 144;
  int h = 144;
  //memset(bw, 0, w * h / 8);//clear to black
  //lets move the screen down a bit (24 lines)
  //bw += (160 * 24) / 8;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int g = extract_grey(grey, index(x, y));
      int n = g > 127 ? 255 : 0;
      set_bw(bw, index_bw(x, y), n);
      int error = g - n;
      if (x < w - 1) add_grey(grey, index(x+1, y  ), (7 * error) / 16);
      if (x > 1 && y < h - 1) add_grey(grey, index(x-1, y+1), (3 * error) / 16);
      if (y < h - 1) add_grey(grey, index(x  , y+1), (5 * error) / 16);
      if (x < w - 1 && y < h - 1) add_grey(grey, index(x+1, y+1), (1 * error) / 16);
      continue;
    }
  }
}
#pragma GCC pop_options
