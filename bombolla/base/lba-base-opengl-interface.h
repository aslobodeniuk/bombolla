#ifndef __BASE_OPENGL_INTERFACE_H__
#define __BASE_OPENGL_INTERFACE_H__

#include <glib-object.h>

typedef unsigned int	lba_GLenum;
typedef unsigned char	lba_GLboolean;
typedef unsigned int	lba_GLbitfield;
typedef void		lba_GLvoid;
typedef signed char	lba_GLbyte;		/* 1-byte signed */
typedef short		lba_GLshort;	/* 2-byte signed */
typedef int		lba_GLint;		/* 4-byte signed */
typedef unsigned char	lba_GLubyte;	/* 1-byte unsigned */
typedef unsigned short	lba_GLushort;	/* 2-byte unsigned */
typedef unsigned int	lba_GLuint;		/* 4-byte unsigned */
typedef int		lba_GLsizei;	/* 4-byte signed */
typedef float		lba_GLfloat;	/* single precision float */
typedef float		lba_GLclampf;	/* single precision float in [0,1] */
typedef double		lba_GLdouble;	/* double precision float */
typedef double		lba_GLclampd;	/* double precision float in [0,1] */

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
      lba_GLfloat green,
      lba_GLfloat blue,
      lba_GLfloat alpha);

  
  void (*glClear) (lba_GLbitfield mask);
  
  void (*glLoadIdentity) (void);

  void (*glRotatef) (lba_GLfloat angle,
      lba_GLfloat x,
      lba_GLfloat y,
      lba_GLfloat z);


  /* glBegin — delimit the vertices of a primitive or a group of like primitives */
  void (*glBegin) (lba_GLenum mode);
  void (*glEnd)   (void);

  /* glColor — set the current color */
  void (*glColor3f) (lba_GLfloat red,
      lba_GLfloat green,
      lba_GLfloat blue);

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
  void (*glVertex3f) (lba_GLfloat x,
      lba_GLfloat y,
      lba_GLfloat z);

  /* glFlush — force execution of GL commands in finite time  */
  void (*glFlush) (void);
};


#endif
