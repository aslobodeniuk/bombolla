/* la Bombolla GObject shell.
 * Copyright (C) 2021 Aleksandr Slobodeniuk
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

#include "bombolla/base/lba-base-cogl3d.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

typedef struct _LbaCoglTexture
{
  GObject parent;

  CoglTexture *texture;
} LbaCoglTexture;


typedef struct _LbaCoglTextureClass
{
  GObjectClass parent;

  void (*set) (LbaCoglTexture * self, BaseCogl3d * obj_3d);
} LbaCoglTextureClass;


static void
lba_cogl_texture_init (LbaCoglTexture * self)
{
}

typedef enum
{
  PROP_PICTURE_OBJECT = 1,
  /* TODO: color */
  N_PROPERTIES
} LbaCoglTextureProperty;

enum
{
  SIGNAL_SET,
  LAST_SIGNAL
};

static void
lba_cogl_texture_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
//  LbaCoglTexture *self = (LbaCoglTexture *) object;

  switch ((LbaCoglTextureProperty) property_id) {
    case PROP_PICTURE_OBJECT:
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
lba_cogl_texture_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
//  LbaCoglTexture *self = (LbaCoglTexture *) object;

  switch ((LbaCoglTextureProperty) property_id) {
    case PROP_PICTURE_OBJECT:
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
lba_cogl_texture_set (LbaCoglTexture * self, BaseCogl3d * obj_3d)
{
  if (!obj_3d)
    return;

  if (!self->texture) {
    int i;
#define w 32
#define h 32
    static uint8_t test_rgba_tex[4 * w * h];

    for (i = 0; i < 4 * w * h; i++) {
      test_rgba_tex[i] = g_random_int_range (0, 256);
    }

    self->texture = cogl_texture_2d_new_from_data (obj_3d->ctx,
        w, h, COGL_PIXEL_FORMAT_RGBA_8888, 0,
        (const uint8_t *) &test_rgba_tex, NULL);
  }

  cogl_pipeline_set_layer_texture (obj_3d->pipeline, 0, self->texture);
}

static void
lba_cogl_texture_class_init (LbaCoglTextureClass * klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->set_property = lba_cogl_texture_set_property;
  gobj_class->get_property = lba_cogl_texture_get_property;

  g_object_class_install_property (gobj_class, PROP_PICTURE_OBJECT,
      g_param_spec_pointer ("picture",
          "picture", "picture",
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  klass->set = lba_cogl_texture_set;

  g_signal_new ("set", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (LbaCoglTextureClass, set), NULL, NULL,
      NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);
}


G_DEFINE_TYPE (LbaCoglTexture, lba_cogl_texture, G_TYPE_OBJECT)

/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_cogl_texture);
