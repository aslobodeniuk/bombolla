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

#include "bombolla/base/lba-basewindow.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include <glib/gstdio.h>

#include <GL/glew.h>
#include "GL/freeglut.h"
#include "GL/gl.h"
#define LBA_OPENGL_IFACE_IMPLEMENTATION 1
#include "bombolla/base/lba-base-opengl-interface.h"

typedef struct _GlutWindow
{
  BaseWindow parent;

  GThread *mainloop_thr;
} GlutWindow;


typedef struct _GlutWindowClass
{
  BaseWindowClass parent;
} GlutWindowClass;


static GlutWindow *global_self;

void
glut_window_on_mouse_cb (int button, int state, int x, int y)
{
  LBA_LOG ("button = %d, state = %d, x = %d, y = %d\n", button, state, x, y);
}


void
glut_window_on_reshape_cb (int width, int height)
{
  LBA_LOG ("width = %d, height = %d\n", width, height);
}

void
glut_window_on_keyboard_cb (unsigned char key, int x, int y)
{
  LBA_LOG ("key = %x, x = %d, y = %d\n", key, x, y);
}

void
glut_window_on_special_key_cb (int key, int x, int y)
{
  LBA_LOG ("special key = %d, x = %d, y = %d\n", key, x, y);
}


static void
glut_window_request_redraw (BaseWindow * base)
{
  glutPostRedisplay ();
}

static void
glut_window_on_display_cb (void)
{
  LBA_LOG ("display cb");

  glClearColor (0.4, 0.4, 0.4, 1.0);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* Now let friend objects draw something */
  base_window_notify_display ((BaseWindow *) global_self);

  glFlush ();
  glutSwapBuffers ();
}

static gpointer
glut_window_mainloop (gpointer data)
{
  glutMainLoop ();
  return NULL;
}

static void
glut_window_close (BaseWindow * base)
{
  GlutWindow *self = (GlutWindow *) base;

  glutLeaveMainLoop ();
  g_thread_join (self->mainloop_thr);
}

static void
glut_window_on_close (void)
{
  LBA_LOG ("Window closed by user\n");
}

/* Fixme: defaults are not set */
static void
glut_window_open (BaseWindow * base)
{
  GLfloat ambientColor[] = { 0.2, 0.2, 0.2, 1.0 };
  GlutWindow *self = (GlutWindow *) base;

  glutInitWindowSize (base->width, base->height);
  glutInitWindowPosition (base->x_pos, base->y_pos);

  glutCreateWindow (base->title);

  /* TODO: param */
  glEnable (GL_DEPTH_TEST);
  glEnable (GL_COLOR_MATERIAL);
  glEnable (GL_LIGHTING);
  glEnable (GL_NORMALIZE);
  // Add an ambient light
  glLightModelfv (GL_LIGHT_MODEL_AMBIENT, ambientColor);


  glutDisplayFunc (glut_window_on_display_cb);
  glutSpecialFunc (glut_window_on_special_key_cb);
  glutKeyboardFunc (glut_window_on_keyboard_cb);
  glutReshapeFunc (glut_window_on_reshape_cb);
  glutMouseFunc (glut_window_on_mouse_cb);
  glutCloseFunc (glut_window_on_close);

  self->mainloop_thr =
      g_thread_new ("glutMainLoop", glut_window_mainloop, NULL);
}


static void
glut_window_init (GlutWindow * self)
{
  global_self = self;
}


/* =================== CLASS */
static void
glut_window_class_init (GlutWindowClass * klass)
{
  BaseWindowClass *base_class = BASE_WINDOW_CLASS (klass);

  base_class->open = glut_window_open;
  base_class->close = glut_window_close;
  base_class->request_redraw = glut_window_request_redraw;
}

static void
glut_window_opengl_interface_init (BaseOpenGLInterface * iface)
{
  static gsize initialization_value = 0;

  if (g_once_init_enter (&initialization_value)) {
    /* Let glut load opengl.
     * Glut allows us to do it only once per proccess execution */
    int argc = 1;
    char *argv[] = { "hack", NULL };
    gsize setup_value = 1;
    lba_GLenum err;

    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInit (&argc, argv);

    glutInitWindowSize (100,100);
    glutInitWindowPosition (0, 0);
    
    glutCreateWindow ("");
    
    
    err = glewInit();
    if (GLEW_OK != err)
    {
      /* Problem: glewInit failed, something is seriously wrong. */
      fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      g_abort ();
    }
    
    g_once_init_leave (&initialization_value, setup_value);
  }

  lba_opengl_interface_init (iface);
}


G_DEFINE_TYPE_WITH_CODE (GlutWindow, glut_window, G_TYPE_BASE_WINDOW,
    G_IMPLEMENT_INTERFACE (G_TYPE_BASE_OPENGL,
        glut_window_opengl_interface_init))

/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (glut_window);
