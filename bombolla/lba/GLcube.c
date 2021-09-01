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

typedef struct _GLCube {
  Basegl3d parent;
} GLCube;

typedef struct _GLCubeClass {
  Basegl3dClass parent;
} GLCubeClass;

static void
gl_cube_draw (Basegl3d * base, BaseOpenGLInterface * i) {
  Base3d *s3d = (Base3d *) base;
  double x,
    y,
    z;
  double size = 0.3;

  x = s3d->x;
  y = s3d->y;
  z = s3d->z;

  LBA_LOG ("draw (%f, %f, %f)", x, y, z);

  /* Reset transformations */
  i->glMatrixMode (i->LBA_GL_MODELVIEW);
  i->glLoadIdentity ();

  /* Rotate a bit */
  i->glRotatef (60.0, 1.0, 1.0, 1.0);
  i->glRotatef (60.0, 1.0, 0.0, 1.0);
  i->glRotatef (60.0, 0.0, 1.0, 1.0);

  // BACK
  i->glBegin (i->LBA_GL_POLYGON);
  i->glColor3f (0.5, 0.3, 0.2);
  i->glVertex3f (x + size, y - size, z + size);
  i->glVertex3f (x + size, y + size, z + size);
  i->glVertex3f (x - size, y + size, z + size);
  i->glVertex3f (x - size, y - size, z + size);
  i->glEnd ();

  // FRONT
  i->glBegin (i->LBA_GL_POLYGON);
  i->glColor3f (0.0, 0.5, 0.0);
  i->glVertex3f (x - size, y + size, z - size);
  i->glVertex3f (x - size, y - size, z - size);
  i->glVertex3f (x + size, y - size, z - size);
  i->glVertex3f (x + size, y + size, z - size);
  i->glEnd ();

  // LEFT
  i->glBegin (i->LBA_GL_POLYGON);
  i->glColor3f (0.5, 0.5, 0.5);
  i->glVertex3f (x - size, y - size, z - size);
  i->glVertex3f (x - size, y - size, z + size);
  i->glVertex3f (x - size, y + size, z + size);
  i->glVertex3f (x - size, y + size, z - size);
  i->glEnd ();

  // RIGHT
  i->glBegin (i->LBA_GL_POLYGON);
  i->glColor3f (0.0, 0.0, 0.0);
  i->glVertex3f (x + size, y - size, z - size);
  i->glVertex3f (x + size, y - size, z + size);
  i->glVertex3f (x + size, y + size, z + size);
  i->glVertex3f (x + size, y + size, z - size);
  i->glEnd ();

  // TOP
  i->glBegin (i->LBA_GL_POLYGON);
  i->glColor3f (0.6, 0.0, 0.0);
  i->glVertex3f (x + size, y + size, z + size);
  i->glVertex3f (x - size, y + size, z + size);
  i->glVertex3f (x - size, y + size, z - size);
  i->glVertex3f (x + size, y + size, z - size);
  i->glEnd ();

  // BOTTOM
  i->glBegin (i->LBA_GL_POLYGON);
  i->glColor3f (0.3, 0.0, 0.3);
  i->glVertex3f (x - size, y - size, z - size);
  i->glVertex3f (x - size, y - size, z + size);
  i->glVertex3f (x + size, y - size, z + size);
  i->glVertex3f (x + size, y - size, z - size);
  i->glEnd ();
}

static void
gl_cube_init (GLCube * self) {
}

static void
gl_cube_class_init (GLCubeClass * klass) {
  Basegl3dClass *base_gl3d_class = (Basegl3dClass *) klass;

  base_gl3d_class->draw = gl_cube_draw;
}

G_DEFINE_TYPE (GLCube, gl_cube, G_TYPE_BASEGL3D)
/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (gl_cube);
