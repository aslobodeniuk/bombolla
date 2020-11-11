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
#include "bombolla/lba-log.h"

typedef enum
{
  PROP_DRAWING_SCENE = 1
} BaseDrawableProperty;


static void
base_drawable_scene_on_draw_cb (GObject * scene, BaseDrawable * self)
{
  BaseDrawableClass *klass = BASE_DRAWABLE_GET_CLASS (self);
  if (G_LIKELY (klass->draw))
    klass->draw (self);
}


static void
base_drawable_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  BaseDrawable *self = (BaseDrawable *) object;

  switch ((BaseDrawableProperty) property_id) {
    case PROP_DRAWING_SCENE:
    {
      self->scene = g_value_get_object (value);

      g_signal_connect (self->scene, "on-draw",
          G_CALLBACK (base_drawable_scene_on_draw_cb), self);

      LBA_LOG ("drawing scene set");
    }
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
base_drawable_init (BaseDrawable * self)
{
}

static void
base_drawable_dispose (GObject * gobject)
{
}

/* =================== CLASS */

static void
base_drawable_class_init (BaseDrawableClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = base_drawable_dispose;
  object_class->set_property = base_drawable_set_property;

  g_object_class_install_property (object_class, PROP_DRAWING_SCENE,
      g_param_spec_object ("drawing-scene",
          "Drawing Scene", "Scene that triggers drawing of the object",
          /* TODO: can check type here: to have a signal */
          G_TYPE_OBJECT, G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE));
}


G_DEFINE_TYPE (BaseDrawable, base_drawable, G_TYPE_OBJECT)
