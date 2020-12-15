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
  gboolean enabled_update;
} GLLight;


typedef struct _GLLightClass
{
  Basegl3dClass parent;
} GLLightClass;


static void
gl_light_draw (Basegl3d * base, BaseOpenGLInterface * i)
{
  GLLight *self = (GLLight *)base;
  BaseDrawable *drawable = (BaseDrawable *) base;

  if (self->enabled_update) {
    if (drawable->enabled) {
      lba_GLfloat lightColor0[] = { 0.5, 0.5, 0.5, 1.0 };
      lba_GLfloat lightPos0[] = { 0.5, 0.5, 0.5, 1.0 };

      LBA_LOG ("Enabling light..");
      i->glEnable (i->LBA_GL_LIGHT0);
      /* Reset transformations */
      i->glMatrixMode (i->LBA_GL_MODELVIEW);
      i->glLoadIdentity ();

      /* Add a positioned light */
      i->glLightfv (i->LBA_GL_LIGHT0, i->LBA_GL_DIFFUSE, lightColor0);
      i->glLightfv (i->LBA_GL_LIGHT0, i->LBA_GL_POSITION, lightPos0);
    } else {
      LBA_LOG ("Disabling light..");
      i->glDisable (i->LBA_GL_LIGHT0);
    }
    self->enabled_update = FALSE;
  }
}

void
gl_light_enabled (GObject * gobject, GParamSpec * pspec, gpointer user_data)
{
  BaseDrawable *drawable = (BaseDrawable *) gobject;
  GLLight *self = (GLLight *)gobject;

  if (drawable->scene) {
    LBA_LOG ("update..");
    self->enabled_update = TRUE;
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
  Basegl3dClass *base_gl3d_class = (Basegl3dClass *) klass;

  base_gl3d_class->draw = gl_light_draw;
}


G_DEFINE_TYPE (GLLight, gl_light, G_TYPE_BASEGL3D)

/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (gl_light);
