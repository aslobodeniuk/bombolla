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
#include "bombolla/lba-log.h"

typedef enum
{
  PROP_GL_PROVIDER = 1
} Basegl3dProperty;


static void
basegl3d_draw (BaseDrawable * base)
{
  Basegl3d *self = (Basegl3d *) base;
  Basegl3dClass *klass = BASE_GL3D_GET_CLASS (self);

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

  /* Now let the child draw */
  if (G_LIKELY (klass->draw))
    klass->draw (self, self->i);
}


static void
basegl3d_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  Basegl3d *self = (Basegl3d *) object;

  switch ((Basegl3dProperty) property_id) {
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
basegl3d_init (Basegl3d * self)
{
}


static void
basegl3d_class_init (Basegl3dClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  BaseDrawableClass *base_drawable_class = (BaseDrawableClass *) klass;

  base_drawable_class->draw = basegl3d_draw;

  object_class->set_property = basegl3d_set_property;

  g_object_class_install_property (object_class,
      PROP_GL_PROVIDER,
      g_param_spec_gtype ("gl-provider",
          "GL provider", "Object, that provides Opengl interface",
          /* We don't know what plugin may provide us an interface yet */
          G_TYPE_NONE, G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE));
}


G_DEFINE_TYPE (Basegl3d, basegl3d, G_TYPE_BASE3D)
