/* la Bombolla GObject shell.
 * Copyright (C) 2020 Aleksandr Slobodeniuk
 *
 *   This file is part of bombolla.
 *
 *   Bombolla is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Bombolla is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with bombolla.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BASE_OPENGL_INTERFACE_H__
#define __BASE_OPENGL_INTERFACE_H__

#include <glib-object.h>

typedef unsigned int lba_GLenum;
typedef unsigned char lba_GLboolean;
typedef unsigned int lba_GLbitfield;
typedef void lba_GLvoid;
typedef signed char lba_GLbyte; /* 1-byte signed */
typedef short lba_GLshort;      /* 2-byte signed */
typedef int lba_GLint;          /* 4-byte signed */
typedef unsigned char lba_GLubyte;      /* 1-byte unsigned */
typedef unsigned short lba_GLushort;    /* 2-byte unsigned */
typedef unsigned int lba_GLuint;        /* 4-byte unsigned */
typedef int lba_GLsizei;        /* 4-byte signed */
typedef float lba_GLfloat;      /* single precision float */
typedef float lba_GLclampf;     /* single precision float in [0,1] */
typedef double lba_GLdouble;    /* double precision float */
typedef double lba_GLclampd;    /* double precision float in [0,1] */

/* When implementation is including this header, it can test if types
 * are matching the sizes */
#ifdef LBA_OPENGL_TEST
#define LBA_TEST_TYPE_SIZE(x)                     \
  G_STATIC_ASSERT(sizeof (x) == sizeof (lba_##x))

LBA_TEST_TYPE_SIZE (GLenum);
LBA_TEST_TYPE_SIZE (GLboolean);
LBA_TEST_TYPE_SIZE (GLbitfield);
LBA_TEST_TYPE_SIZE (GLvoid);
LBA_TEST_TYPE_SIZE (GLbyte);
LBA_TEST_TYPE_SIZE (GLshort);
LBA_TEST_TYPE_SIZE (GLint);
LBA_TEST_TYPE_SIZE (GLubyte);
LBA_TEST_TYPE_SIZE (GLushort);
LBA_TEST_TYPE_SIZE (GLuint);
LBA_TEST_TYPE_SIZE (GLsizei);
LBA_TEST_TYPE_SIZE (GLfloat);
LBA_TEST_TYPE_SIZE (GLclampf);
LBA_TEST_TYPE_SIZE (GLdouble);
LBA_TEST_TYPE_SIZE (GLclampd);
#endif

#define G_TYPE_BASE_OPENGL base_opengl_get_type ()
G_DECLARE_INTERFACE (BaseOpenGL, base_opengl, G, BASE_OPENGL, GObject)

     struct _BaseOpenGLInterface
     {
       GTypeInterface parent_iface;

       /* Constants */
       /* -------------------------------------- */
       /* Indicates the buffers currently enabled for color writing. */
       lba_GLbitfield LBA_GL_COLOR_BUFFER_BIT;
       /* Indicates the stencil buffer. */
       lba_GLbitfield LBA_GL_STENCIL_BUFFER_BIT;
       /* Indicates the depth buffer. */
       lba_GLbitfield LBA_GL_DEPTH_BUFFER_BIT;
       /* -------------------------------------- */

       lba_GLenum LBA_GL_TRIANGLES;
       lba_GLenum LBA_GL_POLYGON;


       /* glMatrixMode — specify which matrix is the current matrix */
       void (*glMatrixMode) (lba_GLenum mode);
       lba_GLenum LBA_GL_MODELVIEW;

       /* -------------------------------------- */
       /* glEnable — enable or disable server-side GL capabilities */
       void (*glEnable) (lba_GLenum cap);
       void (*glDisable) (lba_GLenum cap);

  /**
   * @GL_DEPTH_TEST:
   *
   * If enabled, do depth comparisons and update the depth buffer.
   * Note that even if the depth buffer exists and the depth mask is non-zero,
   * the depth buffer is not updated if the depth test is disabled.
   * See glDepthFunc and glDepthRange. */
       lba_GLenum LBA_GL_DEPTH_TEST;

       void (*glClearColor) (lba_GLfloat red,
           lba_GLfloat green, lba_GLfloat blue, lba_GLfloat alpha);


       void (*glClear) (lba_GLbitfield mask);

       void (*glLoadIdentity) (void);

       void (*glRotatef) (lba_GLfloat angle,
           lba_GLfloat x, lba_GLfloat y, lba_GLfloat z);


       /* glBegin — delimit the vertices of a primitive or a group of like primitives */
       void (*glBegin) (lba_GLenum mode);
       void (*glEnd) (void);

       /* glColor — set the current color */
       void (*glColor3f) (lba_GLfloat red, lba_GLfloat green, lba_GLfloat blue);

  /** glVertex — specify a vertex
   * glVertex commands are used within glBegin/glEnd pairs to specify point, line,
   * and polygon vertices. The current color, normal, texture coordinates, and fog
   * coordinate are associated with the vertex when glVertex is called.
   *
   * When only x and y are specified, z defaults to 0 and w defaults to 1.
   * When x, y, and z are specified, w defaults to 1.
   *
   * NOTES:
   * Invoking glVertex outside of a glBegin/glEnd pair results in undefined behavior. 
   * */
       void (*glVertex3f) (lba_GLfloat x, lba_GLfloat y, lba_GLfloat z);

       /* glFlush — force execution of GL commands in finite time  */
       void (*glFlush) (void);

       lba_GLenum LBA_GL_LIGHT0;
       lba_GLenum LBA_GL_DIFFUSE;
       lba_GLenum LBA_GL_POSITION;

       void (*glLightfv) (lba_GLenum light,
           lba_GLenum pname, const lba_GLfloat * params);

     };


#endif
