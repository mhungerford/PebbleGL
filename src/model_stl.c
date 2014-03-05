#include <stdlib.h>

#include "model_stl.h"
#include "miniGL/miniGL.h"
#define GL_COLOR_BUFFER_BIT GL_COLOR


//Evil hack for softfloat workarounds
//__extendsfdf2 pulls in all of ieee754-df.S (about 3164 bytes)
//vs. 140 for just extendsfdf2
extern double llvm_extendsfdf2(float a);

void gl_init() {
  glViewport(0, 0, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();

  //Invert y here, as pebble origin is top-left, opengl is bottom-left
  glOrtho(int2sll(-72), int2sll(72), 
    int2sll(-72), int2sll(72), int2sll(-144), int2sll(30));

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();

#if 1
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_CULL_FACE);
  glPolygonMode(GL_FRONT, GL_FILL);
  //gluPerspective(40.0, 1.4, -100.0, 0.0);


  GLfloat amb[4] = {dbl2sll(0.3), dbl2sll(0.3), dbl2sll(0.3), int2sll(0)};
  GLfloat dif[4] = {int2sll(1),int2sll(1),int2sll(1),int2sll(0)};
  GLfloat lightpos[] = {int2sll(30), int2sll(64), int2sll(-34), int2sll(1)};
  glLightfv(GL_LIGHT0,GL_POSITION,lightpos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
  glLightfv(GL_LIGHT0, GL_AMBIENT, amb);

#else
//   gluPerspective(40, 1.0, 20.0, 100.0);
  glDisable(GL_LIGHTING);
  glEnable(GL_CULL_FACE);
  glPolygonMode(GL_FRONT, GL_LINE);
#endif
}

void gl_drawframe(uint8_t* model, bool wireframe, uint8_t rotation, bool reset) {
  glPolygonMode(GL_FRONT, (wireframe) ? GL_LINE : GL_FILL);

  int triangle_count = *(int*)&model[80];
  glClearColor(int2sll(0),int2sll(0),int2sll(0),int2sll(0));
  glClear(GL_COLOR_BUFFER_BIT);

  if (reset) {
    glLoadIdentity();
  }

  if (rotation == 0) {
    glRotatef(int2sll(-10), int2sll(1), int2sll(1), int2sll(0));
  } else {
    glRotatef(int2sll(-10), int2sll(1), int2sll(1), int2sll(1));
  }
  for (int i = 0; i < triangle_count; i++){
    struct stl_data stl = *(struct stl_data*)&model[80 + 4 + i*sizeof(stl)];
//     float red   = (   stl.color & 0x001F )        / 31.0;
//     float green = ( ( stl.color & 0x03E0 ) >> 5 ) / 31.0;
//     float blue  = ( ( stl.color & 0x7C00 ) >> 10) / 31.0;

//     printf("stl[%i]: red:%f green:%f blue:%f \n",i,red,green,blue);
//     printf("normal( %f, %f, %f )\n",
//       stl.normal[0], stl.normal[1], stl.normal[2]);
//     printf(
//       "vertex1( %f ,%f ,%f )\n"
//       "vertex2( %f ,%f ,%f )\n"
//       "vertex3( %f ,%f ,%f )\n",
//       stl.vertex1[0],stl.vertex1[1],stl.vertex1[2],
//       stl.vertex2[0],stl.vertex2[1],stl.vertex2[2],
//       stl.vertex3[0],stl.vertex3[1],stl.vertex3[2]);

    //glColor3f(red,green,blue);
    glColor3f(int2sll(1), int2sll(1), int2sll(1));
    glBegin(GL_POLYGON);
    glNormal3f(
      dbl2sll(llvm_extendsfdf2(stl.normal[0])),
      dbl2sll(llvm_extendsfdf2(stl.normal[1])),
      dbl2sll(llvm_extendsfdf2(stl.normal[2])));
    glVertex3f(
      dbl2sll(llvm_extendsfdf2(stl.vertex1[0])), 
      dbl2sll(llvm_extendsfdf2(stl.vertex1[1])), 
      dbl2sll(llvm_extendsfdf2(stl.vertex1[2])));
    glVertex3f(
      dbl2sll(llvm_extendsfdf2(stl.vertex2[0])), 
      dbl2sll(llvm_extendsfdf2(stl.vertex2[1])), 
      dbl2sll(llvm_extendsfdf2(stl.vertex2[2])));
    glVertex3f(
      dbl2sll(llvm_extendsfdf2(stl.vertex3[0])), 
      dbl2sll(llvm_extendsfdf2(stl.vertex3[1])), 
      dbl2sll(llvm_extendsfdf2(stl.vertex3[2])));
    glEnd();
  }
  //glFlush();
}
