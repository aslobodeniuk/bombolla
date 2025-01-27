/* la Bombolla GObject shell
 *
 * Copyright (c) 2024, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "base/icogl.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

typedef struct _LbaCoglTexture {
  GObject parent;

  CoglTexture *texture;
  GRecMutex lock;
  GObject *scene;
  struct {
    guint64 last_cookie;
    guint64 cookie;
    GBytes *data;
    guint w;
    guint h;
    CoglPixelFormat format;
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
  PROP_DRAWING_SCENE,
  N_PROPERTIES
} LbaCoglTextureProperty;

G_DEFINE_TYPE (LbaCoglTexture, lba_cogl_texture, G_TYPE_OBJECT);

/* transfer-full */
static CoglPixelFormat
lba_cogl_texture_format_to_cogl (gchar *format)
{
  static const struct
  {
    const gchar *str;
    CoglPixelFormat cogl;
  } bunch[] = {
    { "argb8888", COGL_PIXEL_FORMAT_ARGB_8888 },
    { "rgba8888", COGL_PIXEL_FORMAT_RGBA_8888 }
  };

  CoglPixelFormat ret = COGL_PIXEL_FORMAT_RGBA_8888;
  int i;
  
  for (i = 0; i < G_N_ELEMENTS (bunch); i++)
    if (g_strcmp0 (bunch[i].str, format)) {
      ret = bunch[i].cogl;
    }
  
  g_free (format);

  return ret;
}

static void
lba_cogl_texture_picture_update_cb (GObject * pic,
                                    GParamSpec * pspec, LbaCoglTexture * self) {
  gchar *format;
  GBytes *pic_data;
  guint w,
    h;

  LBA_LOCK (self);
  g_object_get (pic, "width", &w, "height", &h,
                "data", &pic_data, "format", &format, NULL);

  LBA_LOG ("Picture update: w=%d, h=%d, format=%s", w, h, format);

  /* TODO: GTypeEnum format */
  LBA_ASSERT (w != 0 && h != 0);
  LBA_ASSERT (pic_data != NULL);

  if (self->pic.data) {
    g_bytes_unref (self->pic.data);
  }

  self->pic.format = lba_cogl_texture_format_to_cogl (format);
  self->pic.data = pic_data;
  self->pic.w = w;
  self->pic.h = h;
  self->pic.last_cookie = self->pic.cookie;
  self->pic.cookie++;

  /* Now request update the drawing scene */
  g_signal_emit_by_name (self->scene, "request-redraw", NULL);
  LBA_UNLOCK (self);
}

static void
lba_cogl_texture_set_property (GObject * object,
                               guint property_id, const GValue * value,
                               GParamSpec * pspec) {
  LbaCoglTexture *self = (LbaCoglTexture *) object;

  switch ((LbaCoglTextureProperty) property_id) {
  case PROP_DRAWING_SCENE:
    LBA_LOCK (self);
    self->scene = g_value_get_object (value);
    LBA_UNLOCK (self);
    break;

  case PROP_PICTURE_OBJECT:
    LBA_LOCK (self);
    if (self->pic.obj) {
      g_object_unref (self->pic.obj);
    }

    self->pic.obj = g_value_dup_object (value);

    if (self->pic.obj) {
      g_signal_connect (self->pic.obj, "notify::data",
                        G_CALLBACK (lba_cogl_texture_picture_update_cb), self);
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
  if (!obj_3d || !self->pic.data)
    return;

  iface = G_TYPE_INSTANCE_GET_INTERFACE (obj_3d, LBA_ICOGL, LbaICogl);
  iface->get_ctx (obj_3d, &cogl_ctx, &cogl_pipeline);

  g_assert (cogl_ctx && cogl_pipeline);

  LBA_LOCK (self);
  if (!self->texture || self->pic.cookie != self->pic.last_cookie) {    
    g_clear_pointer (&self->texture, cogl_object_unref);
    self->texture = cogl_texture_2d_new_from_data (cogl_ctx,
                                                   self->pic.w, self->pic.h,
        self->pic.format, 0,
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
  self->pic.format = COGL_PIXEL_FORMAT_RGBA_8888;

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

  g_object_class_install_property (gobj_class, PROP_DRAWING_SCENE,
                                   g_param_spec_object ("drawing-scene",
                                                        "Drawing Scene",
                                                        "Scene that triggers drawing of the object",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_WRITABLE));

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
