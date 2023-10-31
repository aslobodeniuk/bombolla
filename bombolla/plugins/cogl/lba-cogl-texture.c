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

#include "base/icogl.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

typedef struct _LbaCoglTexture {
  GObject parent;

  CoglTexture *texture;
  GRecMutex lock;
  struct {
    guint64 last_cookie;
    guint64 cookie;
    GBytes *data;
    guint w;
    guint h;
    GObject *obj;
  } pic;
} LbaCoglTexture;

typedef struct _LbaCoglTextureClass {
  GObjectClass parent;

  void (*set) (LbaCoglTexture * self, GObject * obj);
  void (*unset) (LbaCoglTexture * self, GObject * obj);
} LbaCoglTextureClass;

typedef enum {
  PROP_PICTURE_OBJECT = 1,
  N_PROPERTIES
} LbaCoglTextureProperty;

G_DEFINE_TYPE (LbaCoglTexture, lba_cogl_texture, G_TYPE_OBJECT);

static void
lba_cogl_texture_picture_update_cb (GObject * pic, LbaCoglTexture * self) {
  const gchar *format;
  GBytes *pic_data;
  guint w,
    h;

  LBA_LOCK (self);
  g_object_get (pic, "width", &w, "height", &h,
                "data", &pic_data, "format", &format, NULL);

  LBA_LOG ("Picture update: w=%d, h=%d", w, h);

  /* TODO: GTypeEnum format */
  LBA_ASSERT (0 == g_strcmp0 (format, "rgba8888"));
  LBA_ASSERT (w != 0 && h != 0);
  LBA_ASSERT (pic_data != NULL);
  LBA_ASSERT (g_bytes_get_size (pic_data) >= w * h * 4);

  if (self->pic.data) {
    g_bytes_unref (self->pic.data);
  }

  self->pic.data = pic_data;
  self->pic.w = w;
  self->pic.w = w;
  self->pic.last_cookie = self->pic.cookie;
  self->pic.cookie++;

  /* Now update drawing scene?? */

  LBA_UNLOCK (self);
}

static void
lba_cogl_texture_set_property (GObject * object,
                               guint property_id, const GValue * value,
                               GParamSpec * pspec) {
  LbaCoglTexture *self = (LbaCoglTexture *) object;

  switch ((LbaCoglTextureProperty) property_id) {
  case PROP_PICTURE_OBJECT:
    LBA_LOCK (self);
    if (self->pic.obj) {
      g_object_unref (self->pic.obj);
    }

    self->pic.obj = g_value_get_object (value);

    if (self->pic.obj) {
      g_signal_connect (self->pic.obj, "on-update",
                        G_CALLBACK (lba_cogl_texture_picture_update_cb), self);
      lba_cogl_texture_picture_update_cb (self->pic.obj, self);
    }
    LBA_UNLOCK (self);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_cogl_texture_get_property (GObject * object,
                               guint property_id, GValue * value,
                               GParamSpec * pspec) {
  LbaCoglTexture *self = (LbaCoglTexture *) object;

  switch ((LbaCoglTextureProperty) property_id) {
  case PROP_PICTURE_OBJECT:
    LBA_LOCK (self);
    g_value_set_object (value, self->pic.obj);
    LBA_UNLOCK (self);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_cogl_texture_unset (LbaCoglTexture * self, GObject * obj_3d) {
  LbaICogl *iface;
  CoglContext *cogl_ctx;
  CoglPipeline *cogl_pipeline;

  /* NOTE: called in GL thread */
  if (!obj_3d)
    return;

  iface = G_TYPE_INSTANCE_GET_INTERFACE (obj_3d, LBA_ICOGL, LbaICogl);
  iface->get_ctx (obj_3d, &cogl_ctx, &cogl_pipeline);

  g_assert (cogl_ctx && cogl_pipeline);

  LBA_LOCK (self);
  cogl_pipeline_set_layer_texture (cogl_pipeline, 0, NULL);
  LBA_UNLOCK (self);
}

static void
lba_cogl_texture_set (LbaCoglTexture * self, GObject * obj_3d) {
  LbaICogl *iface;
  CoglContext *cogl_ctx;
  CoglPipeline *cogl_pipeline;

  /* NOTE: called in GL thread */
  if (!obj_3d)
    return;

  iface = G_TYPE_INSTANCE_GET_INTERFACE (obj_3d, LBA_ICOGL, LbaICogl);
  iface->get_ctx (obj_3d, &cogl_ctx, &cogl_pipeline);

  g_assert (cogl_ctx && cogl_pipeline);

  LBA_LOCK (self);
  if (!self->texture || self->pic.cookie != self->pic.last_cookie) {
    g_clear_pointer (&self->texture, cogl_object_unref);
    self->texture = cogl_texture_2d_new_from_data (cogl_ctx,
                                                   self->pic.w, self->pic.h,
                                                   COGL_PIXEL_FORMAT_RGBA_8888, 0,
                                                   g_bytes_get_data (self->pic.data,
                                                                     NULL), NULL);
  }

  cogl_pipeline_set_layer_texture (cogl_pipeline, 0, self->texture);
  LBA_UNLOCK (self);
}

static void
lba_cogl_texture_init_picture (LbaCoglTexture * self) {
#define DEFAULT_PICTURE_SIZE_BYTES (4 * 32 * 32)
  int i;
  static uint8_t test_rgba_tex[DEFAULT_PICTURE_SIZE_BYTES];

  self->pic.w = 32;
  self->pic.h = 32;

  for (i = 0; i < DEFAULT_PICTURE_SIZE_BYTES; i++) {
    test_rgba_tex[i] = g_random_int_range (0, 256);
  }

  self->pic.data = g_bytes_new_static (test_rgba_tex, DEFAULT_PICTURE_SIZE_BYTES);
}

static void
lba_cogl_texture_init (LbaCoglTexture * self) {
  g_rec_mutex_init (&self->lock);
  lba_cogl_texture_init_picture (self);
}

static void
lba_cogl_texture_dispose (GObject * gobject) {
  LbaCoglTexture *self = (LbaCoglTexture *) gobject;

  if (self->texture) {
    cogl_object_unref (self->texture);
  }

  if (self->pic.obj) {
    g_object_unref (self->pic.obj);
  }

  if (self->pic.data) {
    g_bytes_unref (self->pic.data);
  }

  g_rec_mutex_clear (&self->lock);
  G_OBJECT_CLASS (lba_cogl_texture_parent_class)->dispose (gobject);
}

static void
lba_cogl_texture_class_init (LbaCoglTextureClass * klass) {
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->dispose = lba_cogl_texture_dispose;
  gobj_class->set_property = lba_cogl_texture_set_property;
  gobj_class->get_property = lba_cogl_texture_get_property;

  g_object_class_install_property (gobj_class, PROP_PICTURE_OBJECT,
                                   g_param_spec_object ("picture",
                                                        "picture", "picture",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  klass->set = lba_cogl_texture_set;
  klass->unset = lba_cogl_texture_unset;

  g_signal_new ("set", G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                G_STRUCT_OFFSET (LbaCoglTextureClass, set), NULL, NULL,
                NULL, G_TYPE_NONE, 1, G_TYPE_OBJECT);

  g_signal_new ("unset", G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                G_STRUCT_OFFSET (LbaCoglTextureClass, unset), NULL, NULL,
                NULL, G_TYPE_NONE, 1, G_TYPE_OBJECT);

}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_cogl_texture);
