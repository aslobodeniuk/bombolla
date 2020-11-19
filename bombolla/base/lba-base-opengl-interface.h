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

typedef char lba_GLchar;
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
#ifdef LBA_OPENGL_IFACE_IMPLEMENTATION
#define LBA_TEST_TYPE_SIZE(x)                     \
  G_STATIC_ASSERT(sizeof (x) == sizeof (lba_##x))

LBA_TEST_TYPE_SIZE (GLchar);
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

struct _BaseOpenGLInterface
{
  GTypeInterface parent_iface;


  lba_GLbitfield LBA_GL_COLOR_BUFFER_BIT;
  lba_GLbitfield LBA_GL_STENCIL_BUFFER_BIT;
  lba_GLbitfield LBA_GL_DEPTH_BUFFER_BIT;

  lba_GLenum LBA_GL_TRIANGLES;
  lba_GLenum LBA_GL_POLYGON;
  lba_GLenum LBA_GL_QUADS;
  void (*glBegin) (lba_GLenum mode);
  void (*glEnd) (void);


  lba_GLenum LBA_GL_PROJECTION;
  lba_GLenum LBA_GL_MODELVIEW;
  void (*glMatrixMode) (lba_GLenum mode);

  void (*glEnable) (lba_GLenum cap);
  void (*glDisable) (lba_GLenum cap);

  lba_GLenum LBA_GL_DEPTH_TEST;

  void (*glClearColor) (lba_GLfloat red,
      lba_GLfloat green, lba_GLfloat blue, lba_GLfloat alpha);


  void (*glClear) (lba_GLbitfield mask);

  void (*glLoadIdentity) (void);

  void (*glRotatef) (lba_GLfloat angle,
      lba_GLfloat x, lba_GLfloat y, lba_GLfloat z);

  void (*glColor3f) (lba_GLfloat red, lba_GLfloat green, lba_GLfloat blue);
  void (*glVertex3f) (lba_GLfloat x, lba_GLfloat y, lba_GLfloat z);

  void (*glFlush) (void);

  lba_GLenum LBA_GL_LIGHT0;
  lba_GLenum LBA_GL_DIFFUSE;
  lba_GLenum LBA_GL_POSITION;
  void (*glLightfv) (lba_GLenum light,
      lba_GLenum pnam, const lba_GLfloat * params);


  void (*glGenTextures) (lba_GLsizei n, lba_GLuint * textures);

  lba_GLenum LBA_GL_TEXTURE_2D;
  void (*glBindTexture) (lba_GLenum target, lba_GLuint texture);


  lba_GLenum LBA_GL_TEXTURE_MIN_FILTER;
  lba_GLenum LBA_GL_TEXTURE_MAG_FILTER;

  lba_GLenum LBA_GL_NEAREST;
  void (*glTexParameteri) (lba_GLenum target,
      lba_GLenum pname, lba_GLint param);

  lba_GLenum LBA_GL_RGBA;
  lba_GLenum LBA_GL_LUMINANCE;

  lba_GLenum LBA_GL_UNSIGNED_BYTE;
  void (*glTexImage2D) (lba_GLenum target,
      lba_GLint level,
      lba_GLint internalformat,
      lba_GLsizei width,
      lba_GLsizei height,
      lba_GLint border, lba_GLenum format, lba_GLenum type, const void *data);

  void (*glOrtho) (lba_GLdouble left,
      lba_GLdouble right,
      lba_GLdouble bottom,
      lba_GLdouble top, lba_GLdouble nearVal, lba_GLdouble farVal);

  void (*glTexCoord2i) (lba_GLint s, lba_GLint t);

  void (*glVertex2i) (lba_GLint x, lba_GLint y);
  
  lba_GLuint (*glCreateShader) (lba_GLenum shaderType);
  
  void (*glShaderSource) (lba_GLuint shader,
      lba_GLsizei count,
      const lba_GLchar * const *string,
      const lba_GLint *length);
  
  void (*glCompileShader) (lba_GLuint shader);

  void (*glGetShaderiv) (lba_GLuint shader,
      lba_GLenum pname,
      lba_GLint *params);

  lba_GLenum LBA_GL_COMPILE_STATUS;

  void (*glGetShaderInfoLog) (lba_GLuint shader,
      lba_GLsizei maxLength,
      lba_GLsizei *length,
      lba_GLchar *infoLog);

  lba_GLenum LBA_GL_VERTEX_SHADER;
  lba_GLenum LBA_GL_FRAGMENT_SHADER;

  lba_GLuint (*glCreateProgram) (void);

  void (*glAttachShader) (lba_GLuint program,
      lba_GLuint shader);

  
  void (*glLinkProgram) (lba_GLuint program);

  void (*glGetProgramiv) (lba_GLuint program,
      lba_GLenum pname,
      lba_GLint *params);

  lba_GLenum LBA_GL_LINK_STATUS;

  void (*glGetProgramInfoLog) (lba_GLuint program,
      lba_GLsizei maxLength,
      lba_GLsizei *length,
      lba_GLchar *infoLog);

  void (*glDeleteShader) (lba_GLuint shader);

  void (*glGenerateMipmap) (lba_GLenum target);
  
  void (*glUseProgram) (lba_GLuint program);
};

#ifdef LBA_OPENGL_IFACE_IMPLEMENTATION
static void
lba_opengl_interface_init (struct _BaseOpenGLInterface * iface)
{
#define iface_set(x) iface->x = x
#define iface_set_lba(x) iface->LBA_##x = x

  iface_set_lba (GL_COLOR_BUFFER_BIT);
  iface_set_lba (GL_STENCIL_BUFFER_BIT);
  iface_set_lba (GL_DEPTH_BUFFER_BIT);
  iface_set_lba (GL_TRIANGLES);
  iface_set_lba (GL_POLYGON);
  iface_set_lba (GL_MODELVIEW);
  iface_set_lba (GL_LIGHT0);
  iface_set_lba (GL_DIFFUSE);
  iface_set_lba (GL_POSITION);

  iface_set (glEnable);
  iface_set (glDisable);
  iface_set_lba (GL_DEPTH_TEST);
  iface_set (glClearColor);
  iface_set (glClear);
  iface_set (glLoadIdentity);
  iface_set (glRotatef);
  iface_set (glBegin);
  iface_set (glEnd);

  iface_set (glColor3f);
  iface_set (glVertex3f);
  iface_set (glFlush);
  iface_set (glRotatef);
  iface_set (glMatrixMode);
  iface_set (glLightfv);

  iface_set (glGenTextures);
  iface_set (glBindTexture);
  iface_set (glTexParameteri);
  iface_set (glTexImage2D);
  iface_set (glOrtho);
  iface_set (glTexCoord2i);
  iface_set (glVertex2i);

  iface_set_lba (GL_TEXTURE_2D);
  iface_set_lba (GL_NEAREST);
  iface_set_lba (GL_TEXTURE_MIN_FILTER);
  iface_set_lba (GL_TEXTURE_MAG_FILTER);

  iface_set_lba (GL_RGBA);
  iface_set_lba (GL_LUMINANCE);
  iface_set_lba (GL_UNSIGNED_BYTE);
  iface_set_lba (GL_QUADS);

  iface_set (glCreateShader);
  iface_set (glShaderSource);
  iface_set (glCompileShader);
  iface_set (glGetShaderiv);
  iface_set_lba (GL_COMPILE_STATUS);
  iface_set (glGetShaderInfoLog);

  iface_set_lba (GL_VERTEX_SHADER);
  iface_set_lba (GL_FRAGMENT_SHADER);

  iface_set (glCreateProgram);
  iface_set (glAttachShader);
  iface_set (glLinkProgram);
  iface_set (glGetProgramiv);
  iface_set_lba (GL_LINK_STATUS);
  iface_set (glGetProgramInfoLog);
  iface_set (glDeleteShader);
  iface_set (glGenerateMipmap);
  iface_set (glUseProgram);
  
  
#undef iface_set
#undef iface_set_lba
}
#endif

#define G_TYPE_BASE_OPENGL base_opengl_get_type ()
G_DECLARE_INTERFACE (BaseOpenGL, base_opengl, G, BASE_OPENGL, GObject)
#endif
