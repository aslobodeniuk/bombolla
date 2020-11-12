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


static void
gl_light_draw (Basegl3d * base, BaseOpenGLInterface * i)
{
  Base3d *s3d = (Base3d *) base;
  double x, y, z;

  x = s3d->x;
  y = s3d->y;
  z = s3d->z;

  LBA_LOG ("draw (%f, %f, %f)", x, y, z);

  /* Reset transformations */
  i->glMatrixMode(i->LBA_GL_MODELVIEW);
  i->glLoadIdentity();

  // Add a positioned light
  lba_GLfloat lightColor0[] = {0.5, 0.5, 0.5, 1.0};
  lba_GLfloat lightPos0[] = {x, y, z, 1.0};

  i->glLightfv(i->LBA_GL_LIGHT0, i->LBA_GL_DIFFUSE, lightColor0);
  i->glLightfv(i->LBA_GL_LIGHT0, i->LBA_GL_POSITION, lightPos0);
}


static void
gl_light_init (GLLight * self)
{
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
