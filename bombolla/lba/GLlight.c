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

#include "bombolla/base/lba-basegl3d.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

typedef struct _GLLight
{
  Basegl3d parent;
} GLLight;


typedef struct _GLLightClass
{
  Basegl3dClass parent;
} GLLightClass;


void
gl_light_enabled (GObject * gobject, GParamSpec * pspec, gpointer user_data)
{
  BaseOpenGLInterface *i;
  Basegl3d *gl3d = (Basegl3d *) gobject;
  BaseDrawable *drawable = (BaseDrawable *) gobject;

  /* OpenGL interface is available only if drawing scene is set */
  i = basegl3d_get_iface (gl3d);

  if (i) {
    if (drawable->enabled) {
      lba_GLfloat lightColor0[] = { 0.5, 0.5, 0.5, 1.0 };
      lba_GLfloat lightPos0[] = { 0.5, 0.5, 0.5, 1.0 };

      i->glEnable (i->LBA_GL_LIGHT0);
      /* Reset transformations */
      i->glMatrixMode (i->LBA_GL_MODELVIEW);
      i->glLoadIdentity ();

      /* Add a positioned light */
      i->glLightfv (i->LBA_GL_LIGHT0, i->LBA_GL_DIFFUSE, lightColor0);
      i->glLightfv (i->LBA_GL_LIGHT0, i->LBA_GL_POSITION, lightPos0);
    } else {
      i->glDisable (i->LBA_GL_LIGHT0);
    }

    /* We have changed the light, so request redraw on window */
    g_signal_emit_by_name (drawable->scene, "request-redraw", NULL);
  }
}

static void
gl_light_init (GLLight * self)
{
  g_signal_connect (self, "notify::enabled", G_CALLBACK (gl_light_enabled),
      self);
  g_signal_connect (self, "notify::drawing-scene",
      G_CALLBACK (gl_light_enabled), self);
}


static void
gl_light_class_init (GLLightClass * klass)
{
  /* Light is drawable, but it doesn't need to be redrawn on every
   * frame: with opengl it's set once, so we only enable or disable it */
}


G_DEFINE_TYPE (GLLight, gl_light, G_TYPE_BASEGL3D)

/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (gl_light);
