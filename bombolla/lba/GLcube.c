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

#include "bombolla/base/lba-basedrawable.h"
#include "bombolla/base/lba-base-opengl-interface.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

GType gl_cube_get_type (void);

#define GLCUBE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), gl_cube_get_type , GLCubeClass))

/* ======================= Instance */
typedef struct _GLCube
{
  BaseDrawable parent;

  BaseOpenGLInterface *i;
} GLCube;


/* ======================= Class */
typedef struct _GLCubeClass
{
  BaseDrawableClass parent;
  guint PROP_GL_PROVIDER;
} GLCubeClass;


static void
gl_cube_draw (BaseDrawable * base)
{
  GLCube *self = (GLCube *) base;
  BaseOpenGLInterface *i;

  /* FIXME: 2 param */
  double x = 0.6;
  double y = 0.6;
  double z = 0.6;
  /* =============== */

  LBA_LOG ("draw");

  if (G_UNLIKELY (!self->i)) {
    LBA_LOG ("No OpenGL interface provided");
    return;
  }

  i = self->i;

  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.4, 0.3, 0.5);
    i->glVertex3f (x, y, z);
    i->glVertex3f (x, -y, z);
    i->glVertex3f (-x, y, z);
  }
  i->glEnd ();


  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.5, 0.3, 0.2);
    i->glVertex3f (-x, -y, z);
    i->glVertex3f (x, -y, z);
    i->glVertex3f (-x, y, z);
  }
  i->glEnd ();

  // FRONT
  // Using 4 trianges!
  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.1, 0.5, 0.3);
    i->glVertex3f (-x, y, -z);
    i->glVertex3f (0, 0, -z);
    i->glVertex3f (-x, -y, -z);
  }
  i->glEnd ();

  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.0, 0.5, 0.0);
    i->glVertex3f (-x, -y, -z);
    i->glVertex3f (0, 0, -z);
    i->glVertex3f (x, -y, -z);
  }
  i->glEnd ();

  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.1, 0.3, 0.3);
    i->glVertex3f (-x, y, -z);
    i->glVertex3f (x, y, -z);
    i->glVertex3f (0, 0, -z);
  }
  i->glEnd ();

  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.2, 0.2, 0.2);
    i->glVertex3f (0, 0, -z);
    i->glVertex3f (x, y, -z);
    i->glVertex3f (x, -y, -z);
  }
  i->glEnd ();

  // LEFT
  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.3, 0.5, 0.6);
    i->glVertex3f (-x, -y, -z);
    i->glVertex3f (-x, -y, z);
    i->glVertex3f (-x, y, -z);
  }
  i->glEnd ();

  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.5, 0.5, 0.5);
    i->glVertex3f (-x, y, z);
    i->glVertex3f (-x, -y, z);
    i->glVertex3f (-x, y, -z);
  }
  i->glEnd ();

  // RIGHT
  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.2, 0.2, 0.2);
    i->glVertex3f (x, y, z);
    i->glVertex3f (x, y, -z);
    i->glVertex3f (x, -y, z);
  }
  i->glEnd ();

  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.0, 0.0, 0.0);
    i->glVertex3f (x, -y, -z);
    i->glVertex3f (x, y, -z);
    i->glVertex3f (x, -y, z);
  }
  i->glEnd ();

  // TOP
  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.6, 0.0, 0.0);
    i->glVertex3f (x, y, z);
    i->glVertex3f (x, y, -z);
    i->glVertex3f (-x, y, -z);
  }
  i->glEnd ();

  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.6, 0.1, 0.2);
    i->glVertex3f (-x, y, z);
    i->glVertex3f (x, y, z);
    i->glVertex3f (-x, y, -z);
  }
  i->glEnd ();

  // BOTTOM
  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.4, 0.0, 0.4);
    i->glVertex3f (-x, -y, -z);
    i->glVertex3f (-x, -y, z);
    i->glVertex3f (x, -y, z);
  }
  i->glEnd ();

  i->glBegin (i->LBA_GL_TRIANGLES);
  {
    i->glColor3f (0.3, 0.0, 0.3);
    i->glVertex3f (x, -y, -z);
    i->glVertex3f (-x, -y, -z);
    i->glVertex3f (x, -y, z);
  }
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
  GLCubeClass *klass = GLCUBE_GET_CLASS (self);

  if (property_id == klass->PROP_GL_PROVIDER) {

    self->i = (BaseOpenGLInterface *)
        g_type_interface_peek (g_type_class_peek (g_value_get_gtype (value)),
        G_TYPE_BASE_OPENGL);

    LBA_LOG ("Interface provider %sfound", self->i ? "" : "NOT ");

    /* property handled */
    return;
  }

  /* Chain up to parent class */
  base_drawable_set_property (object,
      property_id, value, pspec);
}

/* =================== CLASS */

static void
gl_cube_class_init (GLCubeClass * klass)
{
  BaseDrawableClass *base_class = BASE_DRAWABLE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  guint base_class_n_properties;

  object_class->set_property = gl_cube_set_property;

  LBA_LOG ("%p", base_class);
  base_class->draw = gl_cube_draw;

  /* Install one more property independently from base class */
  g_free (g_object_class_list_properties (object_class,
          &base_class_n_properties));

  klass->PROP_GL_PROVIDER = base_class_n_properties++;

  g_object_class_install_property (object_class,
      klass->PROP_GL_PROVIDER,
      g_param_spec_gtype ("gl-provider",
          "GL provider", "Object, that provides Opengl interface",
          /* We don't know what plugin may provide us an interface yet */
          G_TYPE_NONE, G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE));
}


    G_DEFINE_TYPE (GLCube, gl_cube, G_TYPE_BASE_DRAWABLE)
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (gl_cube);
