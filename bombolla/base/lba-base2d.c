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


#include "bombolla/base/lba-base2d.h"
#include "bombolla/lba-log.h"

typedef enum
{
  PROP_X = 1,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT
} Base2dProperty;


static void
base2d_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  Base2d *self = (Base2d *) object;

  switch ((Base2dProperty) property_id) {
    case PROP_X:
      self->x = g_value_get_uint (value);
      break;

    case PROP_Y:
      self->y = g_value_get_uint (value);
      break;

    case PROP_WIDTH:
      self->width = g_value_get_uint (value);
      break;

    case PROP_HEIGHT:
      self->height = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
base2d_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  Base2d *self = (Base2d *) object;

  switch ((Base2dProperty) property_id) {
    case PROP_X:
      g_value_set_uint (value, self->x);
      break;

    case PROP_Y:
      g_value_set_uint (value, self->y);
      break;

    case PROP_WIDTH:
      g_value_set_uint (value, self->width);
      break;

    case PROP_HEIGHT:
      g_value_set_uint (value, self->height);
      break;
      
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
base2d_init (Base2d * self)
{
}


static void
base2d_class_init (Base2dClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = base2d_set_property;
  object_class->get_property = base2d_get_property;

  g_object_class_install_property (object_class,
      PROP_X,
      g_param_spec_uint ("x",
          "X", "X coordinate",
          0, G_MAXUINT, 0,
          G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
      PROP_Y,
      g_param_spec_uint ("y",
          "Y", "Y coordinate",
          0, G_MAXUINT, 0,
          G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
      PROP_X,
      g_param_spec_uint ("width",
          "W", "Width",
          0, G_MAXUINT, 0,
          G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
      PROP_Y,
      g_param_spec_uint ("height",
          "H", "Height",
          0, G_MAXUINT, 0,
          G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

}


G_DEFINE_TYPE (Base2d, base2d, G_TYPE_BASE_DRAWABLE)
