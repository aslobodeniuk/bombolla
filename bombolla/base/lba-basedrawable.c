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


enum
{
  SIGNAL_DRAW,
  LAST_SIGNAL
};

static guint base_drawable_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[BASE_DRAWABLE_N_PROPERTIES] = { NULL, };

static void
base_drawable_scene_on_draw_cb (GObject * scene, BaseDrawable * self)
{
  BaseDrawableClass *klass = BASE_DRAWABLE_GET_CLASS (self);
  if (G_LIKELY (klass->draw))
    klass->draw (self);
}


void
base_drawable_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  BaseDrawable *self = (BaseDrawable *) object;

  switch ((BaseDrawableProperty) property_id) {
    case PROP_WIDTH:
      self->width = g_value_get_uint (value);
      break;

    case PROP_HEIGHT:
      self->height = g_value_get_uint (value);
      break;

    case PROP_X_POS:
      self->x_pos = g_value_get_uint (value);
      break;

    case PROP_Y_POS:
      self->y_pos = g_value_get_uint (value);
      break;

    case PROP_DRAWING_SCENE:
    {
      GObject *scene;

      scene = g_value_get_object (value);

      g_signal_connect (scene, "on-draw",
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
base_drawable_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  BaseDrawable *self = (BaseDrawable *) object;

  switch ((BaseDrawableProperty) property_id) {
    case PROP_WIDTH:
      g_value_set_uint (value, self->width);
      break;

    case PROP_HEIGHT:
      g_value_set_uint (value, self->height);
      break;

    case PROP_X_POS:
      g_value_set_uint (value, self->x_pos);
      break;

    case PROP_Y_POS:
      g_value_set_uint (value, self->y_pos);
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
  object_class->get_property = base_drawable_get_property;

  obj_properties[PROP_WIDTH] =
      g_param_spec_uint ("width",
      "Drawable width", "Drawable width", 0 /* minimum value */ ,
      G_MAXUINT /* maximum value */ ,
      500 /* default value */ ,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  obj_properties[PROP_HEIGHT] =
      g_param_spec_uint ("height",
      "Drawable height", "Drawable height", 0 /* minimum value */ ,
      G_MAXUINT /* maximum value */ ,
      500 /* default value */ ,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  obj_properties[PROP_X_POS] =
      g_param_spec_uint ("x-pos",
      "Drawable position X", "Drawable position X", 0 /* minimum value */ ,
      G_MAXUINT /* maximum value */ ,
      100 /* default value */ ,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  obj_properties[PROP_Y_POS] =
      g_param_spec_uint ("y-pos",
      "Drawable position Y", "Drawable position Y", 0 /* minimum value */ ,
      G_MAXUINT /* maximum value */ ,
      100 /* default value */ ,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  obj_properties[PROP_Y_POS] =
      g_param_spec_uint ("y-pos",
      "Drawable position Y", "Drawable position Y", 0 /* minimum value */ ,
      G_MAXUINT /* maximum value */ ,
      100 /* default value */ ,
      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  obj_properties[PROP_DRAWING_SCENE] =
      g_param_spec_object ("drawing-scene",
      "Drawing Scene", "Scene that triggers drawing of the object",
      /* TODO: can check type here: to have a signal */
      G_TYPE_OBJECT, G_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE);


  g_object_class_install_properties (object_class,
      BASE_DRAWABLE_N_PROPERTIES, obj_properties);

  /* Useless */
#if 1
  base_drawable_signals[SIGNAL_DRAW] =
      g_signal_new ("draw", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (BaseDrawableClass, draw), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE);
#endif
}


G_DEFINE_TYPE (BaseDrawable, base_drawable, G_TYPE_OBJECT)
