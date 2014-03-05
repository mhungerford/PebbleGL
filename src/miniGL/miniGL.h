/**
 * miniGL.h
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

#ifndef __MINIGL_H__
#define __MINIGL_H__

#define miniGLLibName		"miniGLLib"
#define LibType			'libr'
#define miniGLLibCreator	'MnGL'

#include <stdint.h>
#include <stddef.h>

// Using fixed point sll math library for GLfloat,GLdouble
#define USE_FIXED_POINT 1
//#undef __arm__
#ifdef __arm__
  #pragma message "__arm__ enabled"
#endif
#include "math-sll.h"

typedef sll GLfloat;
typedef sll GLdouble;
typedef sll GLclampf;

typedef enum tagminiGLErrNum {
	miniGLErrNone		=  0,
	miniGLErrParam		= -1,
	miniGLErrNoGlobals	= -2
} miniGLErr;

typedef int Err;

extern uint32_t miniGLLibRef;

#define MAX_VERTICES            4 //30
#define PI                      3.1415926
#define GOURAUD                 0x450
#define PHONG                   0x451
#define MAX_MAT_STACK_MODV      1//32
#define MAX_MAT_STACK_PROJ      1

/**
 * GL library defines
 */
#define GL_POINTS               0x0000
#define GL_LINES                0x0001
#define GL_LINE_LOOP            0x0002
#define GL_LINE_STRIP           0x0003
#define GL_TRIANGLES            0x0004
#define GL_TRIANGLE_STRIP       0x0005
#define GL_TRIANGLE_FAN         0x0006
#define GL_QUADS                0x0007
#define GL_QUAD_STRIP           0x0008
#define GL_POLYGON              0x0009

#define GL_MATRIX_MODE          0x0BA0
#define GL_MODELVIEW            0x1700
#define GL_PROJECTION           0x1701
#define GL_TEXTURE              0x1702

#define GL_RGB                  0x1907
#define GL_RGBA                 0x1908
#define GL_COLOR                0x1800
#define GL_DEPTH                0x1801
#define GL_STENCIL              0x1802

#define GL_FILL                 0x1B02
#define GL_LINE                 0x1B01

#define GL_LIGHTING             0x0B50
#define GL_LIGHT0               0x4000
#define GL_LIGHT1               0x4001
#define GL_LIGHT2               0x4002
#define GL_LIGHT3               0x4003
#define GL_LIGHT4               0x4004
#define GL_LIGHT5               0x4005
#define GL_LIGHT6               0x4006
#define GL_LIGHT7               0x4007
#define GL_SHININESS            0x1601
#define GL_SPECULAR             0x1202
#define GL_DIFFUSE              0x1201
#define GL_AMBIENT              0x1200
#define GL_NORMALIZE            0x0BA1
#define GL_FRONT_AND_BACK       0x0408
#define GL_FRONT                0x0404
#define GL_BACK                 0x0405
#define GL_SHADE_MODEL          0x0B54
#define GL_FLAT                 0x1D00
#define GL_SMOOTH               0x1D01
#define GL_POSITION             0x1203
#define GL_AMBIENT_AND_DIFFUSE  0x1602

#define GL_CULL_FACE            0x0B44
#define GL_CULL_FACE_MODE       0x0B45
#define GL_FRONT_FACE           0x0B46

/** Errors */
#define GL_INVALID_VALUE        0x0501
#define GL_INVALID_ENUM         0x0500
#define GL_INVALID_OPERATION    0x0502
#define GL_STACK_OVERFLOW       0x0503
#define GL_STACK_UNDERFLOW      0x0504
#define GL_OUT_OF_MEMORY        0x0505

/** Hints */
#define GL_FOG_HINT             0x0C54
#define GL_LINE_SMOOTH_HINT     0x0C52
#define GL_PERSPECTIVE_CORRECTION_HINT  0x0C50
#define GL_POINT_SMOOTH_HINT    0x0C51
#define GL_POLYGON_SMOOTH_HINT  0x0C53
#define GL_DONT_CARE            0x1100
#define GL_FASTEST              0x1101
#define GL_NICEST               0x1102

#define GL_ENABLE_BIT           0x00002000
#define GL_EVAL_BIT             0x00010000
#define GL_AUTO_NORMAL          0x0D80
#define GL_MAP2_VERTEX_3        0x0DB7
#define GL_MAP2_TEXTURE_COORD_2 0x0DB4

#define GL_FALSE                0
#define GL_TRUE                 1

/**
 * Typedefs for miniGL
 */
//typedef float GLfloat;
//typedef float GLclampf;
typedef int GLint;
typedef int GLsizei;
typedef char GLbyte;
typedef short GLshort;
//typedef double GLdouble;
typedef unsigned char GLubyte;
typedef unsigned long GLbitfield;
typedef unsigned long GLenum;
typedef void GLvoid;

typedef struct RGBColorType {
  uint8_t pad;
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RGBColorType;

/**
 * Data structures
 */
typedef struct LightStruct {
        int enabled;
        GLfloat ambient[4];
        GLfloat diffuse[4];
        GLfloat specular[4];
        GLfloat position[4];
} Light;

typedef struct LineStruct {
        GLfloat m;
        GLfloat mz;
        GLint x1;
        GLint y1;
        GLint z1;
        GLint start;
        GLint end;
} Line;

typedef struct VertexStruct {
        GLfloat x;
        GLfloat y;
        GLfloat z;  // space coordinates
        GLfloat tx;
        GLfloat ty; // texture coords
        GLfloat nx;
        GLfloat ny;
        GLfloat nz; // vertex normal
        GLfloat r;
        GLfloat g;
        GLfloat b; // vertex color
} Vertex;

/**
 * Library controls functions
 */
void miniGL_init(void);

/**
 * Calls that the calling program will actually make 
 */
void glMultMatrixf(const GLfloat *m);
void glLoadMatrixf(const GLfloat *m);
void glLoadIdentity();
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClear(GLbitfield mask);
void glColor3f(GLfloat r, GLfloat g, GLfloat b);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
        GLdouble near, GLdouble distant);
void gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble near,
        GLdouble distant);
void glBegin(GLenum mode);
void glEnd();
void glFlush();
void glVertex2f(GLfloat x, GLfloat y);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glShadeModel(GLenum mode);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glPopMatrix();
void glPushMatrix();
void glMatrixMode(GLenum mode);
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
void glPolygonMode(GLenum face, GLenum mode);
void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride,
        GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
        const GLfloat *points);
void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1,
        GLfloat v2 );
void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );
void glPushAttrib(GLbitfield mask );
void glPopAttrib();
void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
        const GLvoid *pixels);
void glBitmap(GLsizei width, GLsizei height, GLfloat xbo, GLfloat ybo,
        GLfloat xbi, GLfloat ybi, const GLubyte *bitmap);
void glutSwapBuffers();

#endif

/**
 * End of miniGL.h
 */
