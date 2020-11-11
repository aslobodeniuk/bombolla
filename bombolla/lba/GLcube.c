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

#include "bombolla/base/lba-base3d.h"
#include "bombolla/base/lba-base-opengl-interface.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

typedef struct _GLCube
{
  Base3d parent;

  BaseOpenGLInterface *i;
} GLCube;


typedef struct _GLCubeClass
{
  Base3dClass parent;
} GLCubeClass;


typedef enum
{
  PROP_GL_PROVIDER = 1
} GLCubeProperty;


static void
gl_cube_draw (BaseDrawable * base)
{
  GLCube *self = (GLCube *) base;
  Base3d *s3d = (Base3d *) base;
  BaseOpenGLInterface *i;
  double x, y, z;
  double size = 0.3;

  x = s3d->x;
  y = s3d->y;
  z = s3d->z;

  LBA_LOG ("draw (%f, %f, %f)", x, y, z);

  /* TODO: to base class */
  if (G_UNLIKELY (!self->i)) {
    GType t;
    BaseOpenGLInterface *i = NULL;
    LBA_LOG
        ("No OpenGL interface provided. Trying to query one from drawing scene");

    if (base->scene)
      for (t = G_OBJECT_TYPE (base->scene); t; t = g_type_parent (t)) {
        i = (BaseOpenGLInterface *)
            g_type_interface_peek (g_type_class_peek (t), G_TYPE_BASE_OPENGL);

        if (i) {
          LBA_LOG ("OpenGL interface provided by %s", g_type_name (t));
          break;
        }
      }

    if (G_LIKELY (i)) {
      self->i = i;
    } else {
      LBA_LOG ("No OpenGL interface provided. Rendering is impossible");
      return;
    }
  }

  i = self->i;

  /* Reset transformations */
  i->glMatrixMode(i->LBA_GL_MODELVIEW);
  i->glLoadIdentity();

  /* Rotate a bit */
  i->glRotatef(60.0, 1.0, 1.0, 1.0);
  i->glRotatef(60.0, 1.0, 0.0, 1.0 );
  i->glRotatef(60.0, 0.0, 1.0, 1.0 );
  
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
gl_cube_init (GLCube * self)
{
  LBA_LOG ("init");
}


static void
gl_cube_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  GLCube *self = (GLCube *) object;

  switch ((GLCubeProperty) property_id) {
    case PROP_GL_PROVIDER:
      self->i = (BaseOpenGLInterface *)
          g_type_interface_peek (g_type_class_peek (g_value_get_gtype (value)),
          G_TYPE_BASE_OPENGL);

      LBA_LOG ("Interface provider %sfound", self->i ? "" : "NOT ");
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
gl_cube_class_init (GLCubeClass * klass)
{
  BaseDrawableClass *base_class = (BaseDrawableClass *) klass;
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = gl_cube_set_property;

  LBA_LOG ("%p", base_class);
  base_class->draw = gl_cube_draw;

  /* FIXME: would be nice to move it to some base class */
  g_object_class_install_property (object_class,
      PROP_GL_PROVIDER,
      g_param_spec_gtype ("gl-provider",
          "GL provider", "Object, that provides Opengl interface",
          /* We don't know what plugin may provide us an interface yet */
          G_TYPE_NONE, G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE));
}


G_DEFINE_TYPE (GLCube, gl_cube, G_TYPE_BASE3D)

/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (gl_cube);
