/**
 * miniGL.c
 *
 * miniGL for the Palm Computing platform
 * Michael Sherman <msherman@dsbox.com>
 * (c) 2000 Digital Sandbox, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma GCC diagnostic ignored "-Wchar-subscripts"
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic warning "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "miniGL.h" //pulls in sllmath for GLfloat GLdouble
#include "draw2d.h"
#include <stdint.h>

#define W 1 //used for integer calcs
#define EPSILON dbl2sll(0.02)

// Convenience function
sll slladd_4(sll a, sll b, sll c, sll d){
  return slladd(slladd(a, b), slladd(c, d));
}

// The global structure for this library
uint32_t refcount;

// float modv_matrix[MAX_MAT_STACK_MODV][16];
// char  modv_level;
GLfloat proj_matrix[MAX_MAT_STACK_PROJ][16];
char  proj_level;

GLfloat cur_matrix[16];
GLfloat per_matrix[16];
GLfloat scr_matrix[16];
GLfloat cur_color[4];
GLfloat cur_normal[4];
GLfloat vertices_color[MAX_VERTICES][4];
GLfloat vertices_normal[MAX_VERTICES][4];
GLfloat vertices[MAX_VERTICES][4];
GLfloat scr_vertices[MAX_VERTICES][3];
int num_vertices;
int screen_width;
int screen_height;
int culling;
int screen_startx;
int screen_starty;
int two_created;
int lighting;
int wireframe;
int greyscale_mode;
GLenum cur_mode;
GLenum matrix_mode;
Light lights[1];//8];
#define NUM_LIGHTS 1
// End global structure

/**
 * The routine to open the shared library.  If some other program
 * already has it open, it increments the use counter.  Else it
 * allocates and initializes all global variables.
 */
void miniGL_init(void) {
  /** Initialize globals here */
  refcount = 1;
//   modv_level = 0;
  proj_level = 0;
  for (int i=0;i<16;i++) {
    cur_matrix[i] = int2sll(0);
    per_matrix[i] = int2sll(0);
    scr_matrix[i] = int2sll(0);
  }
  num_vertices = 0;
  two_created = 0;
  lighting = 0;
  for (int i=0;i<NUM_LIGHTS;i++) {
    lights[i].enabled = 0;
  }
  screen_startx = 0;
  screen_starty = 0;
  screen_width = FRAMEBUFFER_WIDTH;
  screen_height = FRAMEBUFFER_HEIGHT;
}

/**
 * Mutiply a 4x4 matrix by a 4x1 vector to result in a 4x1 vector.
 *
 * m - input 4x4 matrix
 * p - input 4x1 vector
 * ret - output 4x1 matrix
 */
void MatrixMultVector(const GLfloat *m, const GLfloat *p, GLfloat *ret) {
  GLfloat p0,p1,p2,p3;

  p0 = p[0];
  p1 = p[1];
  p2 = p[2];
  p3 = p[3];

  ret[0] = slladd_4(
    sllmul(m[0], p0), 
    sllmul(m[4], p1), 
    sllmul(m[8], p2), 
    sllmul(m[12], p3));
  ret[1] = slladd_4(
    sllmul(m[1], p0), 
    sllmul(m[5], p1), 
    sllmul(m[9], p2), 
    sllmul(m[13], p3));
  ret[2] = slladd_4(
    sllmul(m[2], p0), 
    sllmul(m[6], p1), 
    sllmul(m[10], p2), 
    sllmul(m[14], p3));
  ret[3] = slladd_4(
    sllmul(m[3], p0), 
    sllmul(m[7], p1), 
    sllmul(m[11], p2), 
    sllmul(m[15], p3));
}

/**
 * v = p1 - p0
 */
void VectorMinusVector(const GLfloat *p1, const GLfloat *p0, GLfloat *v) {
  for (int i = 0; i < 4; i++) {
    v[i] = sllsub(p1[i], p0[i]);
  }
}

/**
 * returns the dot product of vectors v1 and v2.
 */
GLfloat VectorDotVector(const GLfloat *v1, const GLfloat *v2) {
  return slladd_4(sllmul(v1[0], v2[0]), sllmul(v1[1], v2[1]),
    sllmul(v1[2], v2[2]), sllmul(v1[3], v2[3]));
}

void VectorNormalize(GLfloat *v) {
  GLfloat l_sqr = slladd_4(
    sllmul(v[0], v[0]),
    sllmul(v[1], v[1]),
    sllmul(v[2], v[2]),
    sllmul(v[3], v[3]));
  GLfloat l_inv = sllinv(sllsqrt(l_sqr)); // ie. (1 / l_sqr)

  v[0] = sllmul(v[0], l_inv);
  v[1] = sllmul(v[1], l_inv);
  v[2] = sllmul(v[2], l_inv);
  v[3] = sllmul(v[3], l_inv);
}

/**
 * cross = c0 x c1 (cross product).
 */
void VectorCrossVector(const GLfloat *v1, const GLfloat *v2, GLfloat *cross) {
  cross[0] = sllsub(sllmul(v1[1], v2[2]), sllmul(v2[1], v1[2]));
  cross[1] = sllsub(sllmul(v1[2], v2[0]), sllmul(v2[2], v1[0]));
  cross[2] = sllsub(sllmul(v1[0], v2[1]), sllmul(v2[0], v1[1]));
  cross[3] = int2sll(1);
}

/**
 * Initialize a 4x4 matrix to the identity matrix (all zeroes except ones in
 * the diagonal).
 *
 * m - input 4x4 matrix 
 */
void InitializeMatrix(GLfloat *m) {
  int i;

  for (i=0;i<16;i++)
    m[i] = int2sll(0);

  m[0] = int2sll(1);
  m[5] = int2sll(1);
  m[10] = int2sll(1);
  m[15] = int2sll(1);
}

void MatrixToFixed(const GLfloat* d, GLfloat* f) {
  int i;

  for (i=0; i<16; i++)
    f[i] = d[i];
}

void glMultMatrixf(const GLfloat *input) {
  GLfloat result[16];
  const GLfloat *c;
  GLfloat *m;
  int i,j;

  c = input;
  m = cur_matrix;

  /** 4x4 matrix multiplication */
  for (j=0;j<4;j++) {
    result[0+j] = slladd_4(
      sllmul(c[0+j],  m[0]),
      sllmul(c[4+j],  m[1]),
      sllmul(c[8+j],  m[2]),
      sllmul(c[12+j], m[3]));
    result[4+j] = slladd_4(
      sllmul(c[0+j],  m[4]),
      sllmul(c[4+j],  m[5]),
      sllmul(c[8+j],  m[6]),
      sllmul(c[12+j], m[7]));
    result[8+j] = slladd_4(
      sllmul(c[0+j],  m[8]),
      sllmul(c[4+j],  m[9]),
      sllmul(c[8+j],  m[10]), 
      sllmul(c[12+j], m[11]));
    result[12+j] = slladd_4(
      sllmul(c[0+j],  m[12]),
      sllmul(c[4+j],  m[13]),
      sllmul(c[8+j],  m[14]),
      sllmul(c[12+j], m[15]));
  }

  for (i=0;i<16;i++) {
    m[i] = result[i];
  }
}

/**
 * Loads the input matrix m into the current matrix, overwriting any values
 * stored in the current matrix.
 *
 * m - input 4x4 matrix
 */
void glLoadMatrixf(const GLfloat *m) {
  int i;
  GLfloat *c;

  c = cur_matrix;

  for (i=0;i<16;i++) {
    c[i] = m[i];
  }
}

/**
 * The gl call to load in the identity matrix as the current geometry transform
 * matrix.  Same as InitializeMatrix(...) but uses the gl function name.
 */
void glLoadIdentity(void) {
  GLfloat *m;

  m = cur_matrix;

  if (m == NULL)
    return;

  InitializeMatrix(cur_matrix);

}

/**
 * Doesn't do anything right now since there are no colors.
 */
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
  d2d_ClearColor(
    int2sll(sllmul(red, 255)),
    int2sll(sllmul(green, 255)),
    int2sll(sllmul(blue, 255)));
}

/**
 * Inititalize z-buffer.
 */
#ifdef ZBUFFER
void ZBufferInit() {
  int j;

  /** create z-buffer */
  /** Only use scan-line zbuffer to save heap space */
  zbuffer = (GLubyte *) MemChunkNew(MemHeapID(0, 0),
    FRAMEBUFFER_WIDTH, // * FRAMEBUFFER_HEIGHT,
    memNewChunkFlagNonMovable);
  /** remember to clear this */

  /** initialize to deepest values */
  for (j=0;j<160;j++) {
    zbuffer[j] = 0xff; // set to largest (deepest) value
  }
}
#endif

/**
 * Clears the gl viewport as specified through the glViewport(...) call.  
 * Basically draws a "white" rectangle over the whole region to clear it.
 *
 * mask - buffer to clear 
 */
void glClear(GLbitfield mask) {
  Err err;
  GLint i,j;

  if (mask != GL_COLOR)
    return; /** anything else not supported now */

  d2d_ClearWindow(screen_startx, screen_starty,
    screen_width, screen_height);
}

/**
 * Set the current color to be the RGB value (r,g,b) with an alpha value of 1.
 *
 * r - red [0,1]
 * g - green [0,1]
 * b - blue [0,1]
 */
void glColor3f(GLfloat r, GLfloat g, GLfloat b) {

  cur_color[0] = sllmul(r, int2sll(255));
  cur_color[1] = sllmul(g, int2sll(255));
  cur_color[2] = sllmul(b, int2sll(255));
  cur_color[3] = int2sll(255);

  d2d_SetColor(sll2int(cur_color[0]), sll2int(cur_color[1]), sll2int(cur_color[2]));
}

/**
 * Set the current color to be the RGB value (r,g,b) with an alpha value of a.
 *
 * r - red [0,1]
 * g - green [0,1]
 * b - blue [0,1]
 * a - alpha transparency [0,1]
 */
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {

  cur_color[0] = sllmul(r, int2sll(255));
  cur_color[1] = sllmul(g, int2sll(255));
  cur_color[2] = sllmul(b, int2sll(255));
  cur_color[3] = sllmul(a, int2sll(255));

  d2d_SetColor(sll2int(cur_color[0]), sll2int(cur_color[1]), sll2int(cur_color[2]));
}

/**
 * Sets up the perspective projection matrix and screen coord translation 
 * matrix to be an orthographic projection (no perspective).
 *
 * left - world x coordinate of left edge of projection plane
 * right - world x coordinate of right edge of projection plane
 * bottom - world y coordinate of bottom edge of projection plane
 * top - world y coordinate of top edge of projection plane
 * near - world z coordinate of distance from camera to projection plane
 * distant - back clipping plane
 */
void glOrtho(GLdouble left, GLdouble right,
  GLdouble bottom, GLdouble top, GLdouble near,
  GLdouble distant) {

  GLfloat width = sllsub(right, left);
  GLfloat height = sllsub(top, bottom);

  InitializeMatrix(per_matrix);
  per_matrix[10] = int2sll(0);
  per_matrix[11] = sllinv(int2sll(32000));

  InitializeMatrix(scr_matrix);
  scr_matrix[0] =  slldiv(int2sll(screen_width), width);
  scr_matrix[5] = -slldiv(int2sll(screen_height), height);
  scr_matrix[10] = int2sll(0);
  scr_matrix[12] = int2sll(screen_startx + screen_width/2);
  scr_matrix[13] = int2sll(screen_starty + screen_height/2);

}

/**
 * Sets up the perspective projection matrix and screen coord translation 
 * matrix to be a 2D orthographic projection (no perspective).  Distance from
 * camera to projection plane tends to infinity.
 *
 * left - world x coordinate of left edge of projection plane
 * right - world x coordinate of right edge of projection plane
 * bottom - world y coordinate of bottom edge of projection plane
 * top - world y coordinate of top edge of projection plane
 */
void gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom,
  GLdouble top) {
  glOrtho(left, right, bottom, top, int2sll(-1), int2sll(1));
}

/**
 * Sets up the viewport.  Same stuff as WinSetGLArea below.
 */
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  screen_startx = x;
  screen_starty = y;
  screen_width = width;
  screen_height = height;
  // fill in here.
}

/**
 * Sets up the perspective projection matrix and screen coordinate transform
 * matrix to correctly render perspective projections given a field of view
 * fovy and an aspect ratio aspect (x/y).  Note:  GLU function.
 *
 * fovy - field of view in degrees [0,180]
 * aspect - aspect ratio as width/height (standard television is 4/3)
 * near - distance from camera to projection plane (world coords)
 * distant - back clipping plane world z value
 */
void gluPerspective(GLdouble fovy,
  GLdouble aspect, GLdouble near, GLdouble distant) {
  GLfloat *m;
  GLfloat tmp_mat[16];
  GLfloat half_angle;
  GLfloat f, height, width;

  half_angle = fovy > int2sll(0) ? fovy : 
  slldiv(sllmul(int2sll(45), CONST_PI), int2sll(360));
  f = slldiv(sllcos(half_angle), sllsin(half_angle));
  height = sllmul(int2sll(2), slldiv(near, f));
  width  = sllmul(height, (aspect > int2sll(0) ? aspect : int2sll(1)));


  m = cur_matrix;
  if (m == NULL)
    return;

  InitializeMatrix(per_matrix);
  (per_matrix)[10] = int2sll(0);
  (per_matrix)[11] = sllinv(near);

  InitializeMatrix(scr_matrix);
  (scr_matrix)[0] = slldiv(int2sll(screen_width), width);
  (scr_matrix)[5] = -slldiv(int2sll(screen_height), height);
  (scr_matrix)[10] = int2sll(0);
  (scr_matrix)[12] = int2sll(screen_startx + screen_width/2);
  (scr_matrix)[13] = int2sll(screen_starty + screen_height/2);

}

/**
 * Begin a graphics primitive.
 */
void glBegin(GLenum mode) {


  num_vertices = 0;
  cur_mode = mode;

}

/**
 * Special function for BackFacing
 */
void MatrixMultVector2D(const GLfloat *m, const GLfloat *p, GLfloat *ret) {
  GLfloat p0, p1, p2, p3;

  p0 = p[0];
  p1 = p[1];
  p2 = p[2];
  p3 = p[3];

  ret[0] = slladd_4(
    sllmul(m[0],  p0),
    sllmul(m[4],  p1),
    sllmul(m[8],  p2),
    sllmul(m[12], p3));
  ret[1] = slladd_4(
    sllmul(m[1],  p0),
    sllmul(m[5],  p1),
    sllmul(m[9],  p2),
    sllmul(m[13], p3));
  ret[3] = slladd_4(
    sllmul(m[3],  p0),
    sllmul(m[7],  p1),
    sllmul(m[11], p2),
    sllmul(m[15], p3));
}

/**
 * Check if it's facing away from the camera in order to determine whether or
 * not to draw it if culling is turned on.
 */
int BackFacing(int l, int m, int n) {
  GLfloat *q0;
  GLfloat *q1;
  GLfloat *q2;

  q0 = (scr_vertices)[l];
  q1 = (scr_vertices)[m];
  q2 = (scr_vertices)[n];

  /** Then use cross product to find direction it's facing */
  /*VectorMinusVector(q1, q0, c0);
    VectorMinusVector(q2, q0, c1);*/
  /** Reduce size to keep from overflowing (/8) */
  const GLfloat c00 = slldiv(sllsub(q1[0], q0[0]), 8);
  const GLfloat c01 = slldiv(sllsub(q1[1], q0[1]), 8);
  const GLfloat c10 = slldiv(sllsub(q2[0], q0[0]), 8);
  const GLfloat c11 = slldiv(sllsub(q2[1], q0[1]), 8);
  /** Partial cross product replaces VectorCrossVector */


  /** Return (cross > 0) */
  return sllmul(c00, c11) > sllmul(c10, c01);
}               

void DoLightingCalc(GLfloat pos[4], GLfloat normal[4],
  RGBColorType *color1) {
  GLfloat blaa[4] = {int2sll(0),int2sll(0),int2sll(0),int2sll(0)};
  GLfloat cosangle = int2sll(0);


  if (!lighting) {
    color1->r = sll2int(cur_color[0]);
    color1->g = sll2int(cur_color[1]);
    color1->b = sll2int(cur_color[2]);


    return;
  }


  for (int i=0;i<NUM_LIGHTS;i++) { /** Go through all eight lights */
    if (lights[i].enabled == 0)
      continue;

    VectorMinusVector(lights[i].position,
      pos, blaa);

    VectorNormalize(normal);
    VectorNormalize(blaa);
    cosangle = VectorDotVector(normal, blaa);
    if (cosangle < int2sll(0)) {
      color1->r = sll2int(sllmul(cur_color[0], lights[i].ambient[0]));
      color1->g = sll2int(sllmul(cur_color[1], lights[i].ambient[1]));
      color1->b = sll2int(sllmul(cur_color[2], lights[i].ambient[2]));
    } else {
      color1->r = sll2int(
        sllmul(cur_color[0],
          slladd(lights[i].ambient[0], 
            sllmul(lights[i].diffuse[0], cosangle))));
      color1->g = sll2int(
        sllmul(cur_color[1],
          slladd(lights[i].ambient[1], 
            sllmul(lights[i].diffuse[1], cosangle))));
      color1->b = sll2int(
        sllmul(cur_color[2],
          slladd(lights[i].ambient[2],
            sllmul(lights[i].diffuse[2], cosangle))));
    }
  }

}

/**
 * Draw a line using the z-buffer and shading.
 */
void DrawScanLine(GLfloat *start, GLfloat *end, GLfloat *startnormal,
  GLfloat *endnormal, int mode) {
  RGBColorType color1, color2, color;
  int ty, tx, tz;
  int i,j;
  GLfloat temp[4], normal[4];

#ifdef ZBUFFER
  /**
   * Do bidness here to draw point by point; test and set z-buffer
   * along the way.  Put shading calcs in here, too, by interpolating
   * the normal/intensities across the line.  Add in a beginning value
   * and ending value to the two passed points to interpolate.  If
   * doing Phong shading, interpolate normal (expensive!) or for now
   * just do Gouraud shading by interpolating color values given at
   * each end of the lines.
   */

  temp[3] = 1;
  temp[1] = start[2];

  if (mode == GOURAUD) {
    DoLightingCalc(start, startnormal, &color1);
    DoLightingCalc(end, endnormal, &color2);
  }

  for (j=end[0];j<=start[0];j++) {
    if (mode == PHONG) {
      temp[0] = j;
      temp[2] = start[3]; // interpolated z

      /** Change normal for Phong shading */
      for (i=0;i<4;i++) {
        //normal[i] = normal[i];
      }

      /** Do lighting calc */
      DoLightingCalc(temp, normal, &color);
    } else {
      int startval = sll2int(start[0]);
      int endval = sll2int(end[0]);
      /** interpolate between color1 and color2 */
      color.r = ((j-startval)/(endval-startval))*color2.r
      + (1-(j-startval)/(endval-startval))*color1.r;
      color.g = ((j-startval)/(endval-startval))*color2.g
      + (1-(j-startval)/(endval-startval))*color1.g;
      color.b = ((j-startval)/(endval-startval))*color2.b
      + (1-(j-startval)/(endval-startval))*color1.b;
    }

    d2d_SetColor(color.r, color.g, color.b);
    // if (z bidness)
    //  do stuff

    d2d_DrawPixel(j,sll2int(start[1]));
  }
#else
  /** Just draw the 2D scan line */
  d2d_DrawLine(
    sll2int(start[0]), sll2int(start[1]), 
    sll2int(end[0]), sll2int(end[1]));
#endif
}

void SetColor() {
  d2d_SetColor(sll2int(cur_color[0]), sll2int(cur_color[1]), sll2int(cur_color[2]));
}

void TransformToScreen(const GLfloat* in, GLfloat* ret) {
  GLfloat *m;
  GLfloat p0, p1, p2, p3;

  m = scr_matrix;
  p0 = in[0];
  p1 = in[1];
  p2 = in[2];
  p3 = in[3];

  ret[0] = slladd_4(
    sllmul(m[0],  p0),
    sllmul(m[4],  p1),
    sllmul(m[8],  p2),
    sllmul(m[12], p3));
  ret[1] = slladd_4(
    sllmul(m[1],  p0),
    sllmul(m[5],  p1),
    sllmul(m[9],  p2),
    sllmul(m[13], p3));
  ret[2] = slladd_4(
    sllmul(m[2],  p0),
    sllmul(m[6],  p1),
    sllmul(m[10], p2),
    sllmul(m[14], p3));

}

/**
 * End a graphics primitive and draw to the buffer.
 */
void glEnd(void) {
  int i, sx1, sx2, sy1, sy2, j, oldtopy, oldbottomy, oldi, oldj,
      o, n, y, tempx, tempy, q;
  GLfloat x1, y1, x2, y2, cosangle;
  GLfloat  p1[4], in1[4], out1[4], normal[4], start[4],
           end[4];
  RGBColorType color1, color2;
  Line I, II;
  oldi = 0;


  if (num_vertices < 2)
    return;

  //char foo[20], bar[20];
  //StrCopy(foo, "num_vertices = ");
  //StrIToA(bar, num_vertices);
  //StrCat(foo, bar);
  //ErrFatalDisplayIf(true, foo);

  /** Translate vertices to screen coords */
  for(i=0; i < num_vertices; i++) {
    /** Geometric transform */
    //MatrixMultVector(modv_matrix[modv_level], vertices[i], p1);
    MatrixMultVector(cur_matrix, vertices[i], p1);

    /** Perspective transform */
    //MatrixMultVector(proj_matrix[proj_level], p1, in1);
    MatrixMultVector(per_matrix, p1, in1);

    /** Go from 4D back to 3D */
    for(j=0;j<4;j++) {
      in1[j] = slldiv(in1[j], in1[3]);
    }

    /** Translate into screen coords */
    //TransformToScreen(in1, scr_vertices[i]);

    MatrixMultVector(scr_matrix, in1, out1);

    /** Add the point to the final list */
    for(j=0;j<4;j++) {
      scr_vertices[i][j] = out1[j];
    }

  }
  //MatrixMultVector(modv_matrix[modv_level], cur_normal, normal);
  MatrixMultVector(cur_matrix, cur_normal, normal);

  switch(cur_mode) {
#if 0
  case GL_TRIANGLES:
    if (num_vertices < 3)
      break;

    SetColor();

    j = 0;
    while ((j+2) < num_vertices) {
      if (culling && !BackFacing(j,j+1,j+2)) {
        for (o=j+1;o<(j+3);o++) {
          d2d_DrawLine(
            sll2int(scr_vertices[o-1][0]),
            sll2int(scr_vertices[o-1][1]),
            sll2int(scr_vertices[o][0]),
            sll2int(scr_vertices[o][1]));
        }
        d2d_DrawLine(
          sll2int(scr_vertices[o-1][0]),
          sll2int(scr_vertices[o-1][1]),
          sll2int(scr_vertices[j][0]),
          sll2int(scr_vertices[j][1]));
      }
      j+=3;
    }
    break;

  case GL_TRIANGLE_STRIP:
    if (num_vertices < 3)
      break;

    SetColor();

    /** Draw first line between verts 0 and 1 */
    d2d_DrawLine(
      sll2int(scr_vertices[0][0]),
      sll2int(scr_vertices[0][1]),
      sll2int(scr_vertices[1][0]),
      sll2int(scr_vertices[1][1]));

    /** Need to add culling support */

    o = 2;
    /** For each new vertex, draw two new triangle legs */
    while (o < num_vertices) {

      d2d_DrawLine(
        sll2int(scr_vertices[o][0]),
        sll2int(scr_vertices[o][1]),
        sll2int(scr_vertices[o-1][0]),
        sll2int(scr_vertices[o-1][1]));
      d2d_DrawLine(
        sll2int(scr_vertices[o][0]),
        sll2int(scr_vertices[o][1]),
        sll2int(scr_vertices[o-2][0]),
        sll2int(scr_vertices[o-2][1]));

      o++;
    }
    break;

  case GL_TRIANGLE_FAN:
    if (num_vertices < 3)
      break;

    SetColor();

    for(i=1; i < num_vertices; i++) {
      if (culling && !BackFacing(0,i-1,i)) {
        d2d_DrawLine(
          sll2int(scr_vertices[0][0]),
          sll2int(scr_vertices[0][1]),
          sll2int(scr_vertices[0][2]),
          sll2int(scr_vertices[i][1]));
        d2d_DrawLine(
          sll2int(scr_vertices[i-1][0]),
          sll2int(scr_vertices[i-1][1]),
          sll2int(scr_vertices[i][0]),
          sll2int(scr_vertices[i][1]));
      }
    }
    break;

  case GL_QUADS:
    if (num_vertices < 4)
      break;

    SetColor();

    j = 0;
    while ((j+3) < num_vertices) {

      if (culling && !BackFacing(j,j+1,j+2)) {

        // draw bidness with j, j+1, j+2, j+3 vertices
        for (o=j+1;o<(j+4);o++) {
          d2d_DrawLine(
            sll2int(scr_vertices[o-1][0]),
            sll2int(scr_vertices[o-1][1]),
            sll2int(scr_vertices[o][0]),
            sll2int(scr_vertices[o][1]));
        }
        d2d_DrawLine(
          sll2int(scr_vertices[o-1][0]),
          sll2int(scr_vertices[o-1][1]),
          sll2int(scr_vertices[j][0]),
          sll2int(scr_vertices[j][1]));
      }
      j+=4;
    }
    break;

  case GL_QUAD_STRIP:
    SetColor();

    /** Do this */
    break;

  case GL_LINE_LOOP:
    SetColor();

    if (num_vertices < 1)
      break;

    for(i=1; i < num_vertices; i++) {
      d2d_DrawLine(
        sll2int(scr_vertices[i-1][0]),
        sll2int(scr_vertices[i-1][1]),
        sll2int(scr_vertices[i][0]),
        sll2int(scr_vertices[i][1]));
    }
    /** Close the loop */
    d2d_DrawLine(
      sll2int(scr_vertices[i-1][0]),
      sll2int(scr_vertices[i-1][1]),
      sll2int(scr_vertices[0][0]),
      sll2int(scr_vertices[0][1]));
    break;

  case GL_LINES:
    SetColor();

    if (num_vertices < 1)
      break;

    for(i=1; i < num_vertices; i+=2) {
      d2d_DrawLine(
        sll2int(scr_vertices[i-1][0]),
        sll2int(scr_vertices[i-1][1]),
        sll2int(scr_vertices[i][0]),
        sll2int(scr_vertices[i][1]));
    }
    break;

  case GL_POINTS:
    SetColor();

    for(i=0; i < num_vertices; i++) {
      d2d_DrawLine(
        sll2int(scr_vertices[i][0]),
        sll2int(scr_vertices[i][1]),
        sll2int(scr_vertices[i][0]),
        sll2int(scr_vertices[i][1]));
    }
    break;

  case GL_LINE_STRIP:
    SetColor();

    for(i=1; i < num_vertices; i++) {
      d2d_DrawLine(
        sll2int(scr_vertices[i-1][0]),
        sll2int(scr_vertices[i-1][1]),
        sll2int(scr_vertices[i][0]),
        sll2int(scr_vertices[i][1]));
    }
    break;
#endif
    /** Right now only GL_POLYGON is floodfilled and lit */
  case GL_POLYGON:
    if (num_vertices < 3)
      break;

#if 0
    if (culling && BackFacing(0,1,2)) {
      break;
    }
#endif

    /*
     * For now, floodfill is only applied to GL_POLYGON
     * type for testing.
     */

    if (!wireframe) {
      /** lighting calculation to get color */
      if(lighting){
        DoLightingCalc(vertices[0], normal, &color1);
        d2d_SetColor(color1.r, color1.g, color1.b);
      }

      /** find "highest" and "lowest" points in y dir */
      oldtopy = -1000;
      oldbottomy = 1000;
      for (i=0;i<num_vertices;i++) {
        if (sll2int(scr_vertices[i][1]) > oldtopy) {
          oldtopy = sll2int(scr_vertices[i][1]);
          oldi = i;
        }
        if (sll2int(scr_vertices[i][1]) < oldbottomy) {
          oldbottomy = sll2int(scr_vertices[i][1]);
        }
      }

      /** Flood fill polygon */
      n = num_vertices;

      /** Set first set of boundary lines */
      I.start = (oldi) % n;
      I.end = (oldi+1) % n;
      I.x1 = sll2int(scr_vertices[I.start][0]);
      I.y1 = sll2int(scr_vertices[I.start][1]);
      I.z1 = sll2int(scr_vertices[I.start][2]);
      I.m = slldiv(sllsub(scr_vertices[I.end][1], int2sll(I.y1)),
        sllsub(scr_vertices[I.end][0], int2sll(I.x1)));
      I.mz = slldiv(sllsub(scr_vertices[I.end][1], int2sll(I.y1)),
        sllsub(scr_vertices[I.end][2], int2sll(I.z1)));

      II.start = (oldi) % n;
      II.end = (oldi+n-1) % n;
      II.x1 = sll2int(scr_vertices[II.start][0]);
      II.y1 = sll2int(scr_vertices[II.start][1]);
      II.z1 = sll2int(scr_vertices[II.start][2]);
      II.m = slldiv(sllsub(scr_vertices[II.end][1], int2sll(II.y1)),
        sllsub(scr_vertices[II.end][0], int2sll(II.x1)));
      II.mz = slldiv(sllsub(scr_vertices[II.end][1], int2sll(II.y1)),
        sllsub(scr_vertices[II.end][2], int2sll(II.z1)));

      /** Loop over all y values between "top" and "bottom" */
      //used to be oldtopy -1 (got rid of seams, but 
      //introduced flipped lines at random locations
      for (y = oldtopy; y > oldbottomy; y--) {
        /** Draw one scanline */
        if (I.m > -EPSILON && I.m < EPSILON) {
          start[0] = scr_vertices[I.end][0];
          start[2] = scr_vertices[I.end][2];
        } else {
          start[0] = int2sll(I.x1 + sll2int(slldiv(int2sll(y-I.y1), I.m )));
          start[2] = int2sll(I.z1 + sll2int(slldiv(int2sll(y-I.y1), I.mz)));
        }
        if (II.m > -EPSILON && II.m < EPSILON) {
          end[0] = scr_vertices[II.end][0];
          end[2] = scr_vertices[II.end][2];
        } else {
          end[0] = int2sll(II.x1 + sll2int(slldiv(int2sll(y-II.y1), II.m )));
          end[2] = int2sll(II.z1 + sll2int(slldiv(int2sll(y-II.y1), II.mz)));
        }
        start[1] = int2sll(y);
        start[3] = int2sll(0);  // 1);
        end[1] = int2sll(y);
        end[3] = int2sll(0);  // 1);
#ifdef ZBUFFER
        DrawScanLine(start, end, normal, normal,
          GOURAUD);
#else
        // TODO : Should be able to use d2d_DrawScanLine here eventually
        d2d_DrawLine(sll2int(start[0]), sll2int(start[1]), 
          sll2int(end[0]), sll2int(end[1]));
#endif

        /** Check if the boundary lines should change */
        tempy = sll2int(scr_vertices[I.end][1]);
        if (y>=tempy-W && y<=tempy+W) {
          /** change I */
          oldi++;
          I.start = I.end;
          I.end = (I.end+1) % n;
          I.x1 = sll2int(scr_vertices[I.start][0]);
          I.y1 = sll2int(scr_vertices[I.start][1]);
          I.m = slldiv(
            sllsub(scr_vertices[I.end][1], int2sll(I.y1)),
            sllsub(scr_vertices[I.end][0], int2sll(I.x1)));
        }

        tempy = sll2int(scr_vertices[II.end][1]);
        if (y>=tempy-W && y<=tempy+W) {
          /** change II */
          II.start = II.end;
          II.end = (II.end+n-1) % n;
          II.x1 = sll2int(scr_vertices[II.start][0]);
          II.y1 = sll2int(scr_vertices[II.start][1]);
          II.m = slldiv(
            sllsub(scr_vertices[II.end][1], int2sll(II.y1)),
            sllsub(scr_vertices[II.end][0], int2sll(II.x1)));
        }
      }
    } else { //Wireframe
      /** Draw black edge lines */
      //d2d_SetColor(0,0,0);
      for (i=1;i<num_vertices;i++) {
        d2d_DrawLine(
          sll2int(scr_vertices[i-1][0]),
          sll2int(scr_vertices[i-1][1]),
          sll2int(scr_vertices[i][0]),
          sll2int(scr_vertices[i][1]));
      }
      //Close the loop
      d2d_DrawLine(
        sll2int(scr_vertices[i-1][0]),
        sll2int(scr_vertices[i-1][1]),
        sll2int(scr_vertices[0][0]),
        sll2int(scr_vertices[0][1]));
    }
    break;

  default:
    break;
  }
  num_vertices = 0;
}

/**
 * Copies the back buffer contents into the screen bitmap area to display the
 * drawn primitives.  Only copies the area as set in WinSetGLArea(...).
 */
void glutSwapBuffers(void) {
  glFlush();
}

/**
 * For now does the same as glutSwapBuffers() above.
 */
void glFlush(void) {

#ifdef PRINT_FPS
  ULong t;
  ULong old_t;
  int count_time=0;
  int fps;
  int framecount;
  char text[30];
  /** Don't count time spent swapping buffers now. */
  t = TimGetTicks();
  framecount++;
  if (t - old_t > 0 && count_time) {
    //fps = framecount;
    fps = (t - old_t)*1000/SysTicksPerSecond();
    StrIToA(text, fps);
    StrCat(text, " mspf    ");
    framecount=0;
    old_t = t;
  } else if (!count_time) {
    old_t = t;
    count_time = 1;
  }
#endif

}

/**
 * 
 */
void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
  int i;


  if (num_vertices < MAX_VERTICES) {
    vertices[num_vertices][0] = x;
    vertices[num_vertices][1] = y;
    vertices[num_vertices][2] = z;
    vertices[num_vertices][3] = int2sll(1);

    for (i=0;i<4;i++) {
      vertices_color[num_vertices][i]
      = cur_color[i];
      vertices_normal[num_vertices][i]
      = cur_normal[i];
    }

    num_vertices++;
  }

}

/**
 *
 */
void glVertex2f(GLfloat x, GLfloat y) {
  glVertex3f(x, y, int2sll(0));
}

/**
 * 
 */
void glShadeModel(GLenum mode) {
}

/**
 * 
 */
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
  GLfloat t[16];
  GLdouble theta;

  theta = sllmul(slldiv(angle, int2sll(180)), CONST_PI);

  InitializeMatrix(t);

  if (sll2int(z) == 1) {
    t[0] = sllcos(theta);
    t[4] = -sllsin(theta);
    t[1] = sllsin(theta);
    t[5] = sllcos(theta);
    t[10] = int2sll(1);
    t[15] = int2sll(1);
  } else if (sll2int(y) == 1) {
    t[0] = sllcos(theta);
    t[2] = -sllsin(theta);
    t[8] = sllsin(theta);
    t[10] = sllcos(theta);
    t[5] = int2sll(1);
    t[15] = int2sll(1);
  } else if (sll2int(x) == 1) {
    t[5] = sllcos(theta);
    t[6] = sllsin(theta);
    t[9] = -sllsin(theta);
    t[10] = sllcos(theta);
    t[0] = int2sll(1);
    t[15] = int2sll(1);
  }

  glMultMatrixf(t);
}

/**
 * Constructs a scaling matrix from the given input parameters and multiply
 * the current geometry matrix by that scaling matrix.
 *
 * x,y,z - values to scale in the x, y, and z directions
 */
void glScalef(GLfloat x, GLfloat y, GLfloat z) {
  GLfloat *m;
  GLfloat t[16];


  m = cur_matrix;
  if (m == NULL)
    return;

  InitializeMatrix(t);
  t[0] = x;
  t[5] = y;
  t[10] = z;

  glMultMatrixf(t);

}

/** 
 * Translate the origin by x,y,z by constructing a translation 4x4 matrix and
 * multiplying the current geometry matrix by that new matrix.
 *
 * x, y, z - units to translate in 3D world space
 */
void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
  GLfloat *m;
  GLfloat fx, fy, fz;

  fx = x;
  fy = y;
  fz = z;

  m = cur_matrix;
  if (m == NULL)
    return;

  /** Optimized for speed */
  /** 4x4 matrix multiplication */
  m[0] = slladd(sllmul(fx, m[3]), m[0]);
  m[1] = slladd(sllmul(fy, m[3]), m[1]);
  m[2] = slladd(sllmul(fz, m[3]), m[2]);
  m[4] = slladd(sllmul(fx, m[7]), m[4]);
  m[5] = slladd(sllmul(fy, m[7]), m[5]);
  m[6] = slladd(sllmul(fz, m[7]), m[6]);
  m[8] = slladd(sllmul(fx, m[11]), m[8]);
  m[9] = slladd(sllmul(fy, m[11]), m[9]);
  m[10] = slladd(sllmul(fz, m[11]), m[10]);
  m[12] = slladd(sllmul(fx, m[15]), m[12]);
  m[13] = slladd(sllmul(fy, m[15]), m[13]);
  m[14] = slladd(sllmul(fz, m[15]), m[14]);

}

// void glPopMatrix(void) {
//
//
//   switch (matrix_mode) {
//     case GL_MODELVIEW:
//       /** Error to pop an empty stack */
//       if (modv_level <= 0)
//         return;
//       modv_level--;
//       //cur_matrix = modv_matrix[modv_level];
//       break;
//     case GL_PROJECTION:
//       /** Error to pop an empty stack */
//       if (proj_level <= 0)
//         return;
//       proj_level--;
//       //cur_matrix = proj_matrix[proj_level];
//       break;
//   }
//
// }
//
// void CopyMatrixFTF(const GLfloat* src, GLfloat* dst) {
//   char i;
//
//   for (i=0; i < 16; i++)
//     dst[i] = src[i];
// }

// void glPushMatrix(void) {
//
//
//   switch (matrix_mode) {
//     case GL_MODELVIEW:
//       /** Error to pop an empty stack */
//       if (modv_level >= (MAX_MAT_STACK_MODV - 1))
//         return;
//       CopyMatrixFTF(modv_matrix[modv_level],
//         modv_matrix[modv_level + 1]);
//       modv_level++;
//       //cur_matrix = modv_matrix[modv_level];
//       break;
//     case GL_PROJECTION:
//       /** Error to pop an empty stack */
//       if (proj_level >= (MAX_MAT_STACK_PROJ - 1))
//         return;
//       CopyMatrixFTF(proj_matrix[proj_level],
//         proj_matrix[proj_level + 1]);
//       proj_level++;
//       //cur_matrix = proj_matrix[proj_level];
//       break;
//   }
//
// }

void glMatrixMode(GLenum mode) {


  matrix_mode = mode;
  switch (matrix_mode) {
  case GL_MODELVIEW:
    //cur_matrix = modv_matrix[modv_level];
    break;
  case GL_PROJECTION:
    //cur_matrix = proj_matrix[proj_level];
    break;
  default:
    break;
    //cur_matrix = NULL;
  }

}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {


  cur_normal[0] = nx;
  cur_normal[1] = ny;
  cur_normal[2] = nz;
  cur_normal[3] = int2sll(1);

}

void glEnable(GLenum cap) {


  if (cap == GL_LIGHT0){// && cap <= GL_LIGHT7) {
    lights[(cap - GL_LIGHT0)].enabled = 1;
  }

  switch (cap) {
  case GL_CULL_FACE:
    culling = 1;
    break;
  case GL_LIGHTING:
    lighting = 1;
    break;
  default:
    break;
  }

}

void glDisable(GLenum cap) {
    if (cap == GL_LIGHT0) {// && cap <= GL_LIGHT7) {
      lights[(cap - GL_LIGHT0)].enabled = 0;
    }

    switch (cap) {
    case GL_CULL_FACE:
      culling = 0;
      break;
    case GL_LIGHTING:
      lighting = 0;
      break;
    default:
      break;
    }
}

/**
     * Sets an attribute of a light.  The light is identified by the GLenum light,
     * the property is selected by pname, and the values set by params.
     */
void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
      int i;


      if (params == NULL)
        return;

      if (light > GL_LIGHT7 || light < GL_LIGHT0)
        return;

      switch(pname) {
      case GL_AMBIENT:
        for (i=0;i<NUM_LIGHTS;i++) {
          lights[light-GL_LIGHT0].ambient[i] = params[i];
        }
        break;
      case GL_DIFFUSE:
        for (i=0;i<NUM_LIGHTS;i++) {
          lights[light-GL_LIGHT0].diffuse[i] = params[i];
        }
        break;
      case GL_SPECULAR:
        for (i=0;i<NUM_LIGHTS;i++) {
          lights[light-GL_LIGHT0].specular[i]= params[i];
        }
        break;
      case GL_POSITION:
        for (i=0;i<NUM_LIGHTS;i++) {
          lights[light-GL_LIGHT0].position[i]= params[i];
        }
        break;
      default:
        break;
      }

}

/**
     * Sets an attribute of a light.  The light is identified by the GLenum light,
     * the property is selected by pname, and the values set by params.
     */
void glGetLightfv(GLenum light, GLenum pname, GLfloat *params) {
      int i;


      if (params == NULL)
        return;

      if (light > GL_LIGHT7 || light < GL_LIGHT0)
        return;

      switch(pname) {
      case GL_AMBIENT:
        for (i=0;i<NUM_LIGHTS;i++) {
          params[i] = lights[light-GL_LIGHT0].ambient[i];
        }
        break;
      case GL_DIFFUSE:
        for (i=0;i<NUM_LIGHTS;i++) {
          params[i] = lights[light-GL_LIGHT0].diffuse[i];
        }
        break;
      case GL_SPECULAR:
        for (i=0;i<NUM_LIGHTS;i++) {
          params[i] = lights[light-GL_LIGHT0].specular[i];
        }
        break;
      case GL_POSITION:
        for (i=0;i<NUM_LIGHTS;i++) {
          params[i] = lights[light-GL_LIGHT0].position[i];
        }
        break;
      default:
        break;
      }

}

/**
     *
     */
void glPolygonMode(GLenum face, GLenum mode) {


      if (mode == GL_FILL)
        wireframe = 0;
      else if (mode == GL_LINE)
        wireframe = 1;

}

void glMap2f(GLenum target,
      GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
      GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
      const GLfloat *points ) {
      // nothing here yet.
}

void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2,
      GLint vn, GLfloat v1, GLfloat v2 ) {
      // nothing here yet.
}

void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {
      // nothing here yet.
}

void glPushAttrib(GLbitfield mask) {
  // nothing here yet.
}

void glPopAttrib(void) {
  // nothing here yet.
}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format,
    GLenum type, const GLvoid *pixels) {
  // nothing here yet.
}

void glBitmap(GLsizei width, GLsizei height, GLfloat xbo,
    GLfloat ybo, GLfloat xbi, GLfloat ybi,
    const GLubyte *bitmap) {
  // nothing here yet.
}

/**
 * End of miniGL.c 
 *
 * References:  
 *   Foley, et al.  Computer Graphics.  Addison Wesley, 1990.
 *   Woo, et al.  OpenGL Programming Reference.  Addison Wesley, 1997.
 */

