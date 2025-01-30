/* la Bombolla GObject shell
 *
 * Copyright (c) 2025, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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

#include <bmixin/bmixin.h>
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include "lba-picture.h"

typedef struct _LbaPicture {
  BMixinInstance i;
  
  gchar *format;
  GValue data;
  GRecMutex lock;
} LbaPicture;

BM_DEFINE_MIXIN (lba_picture, LbaPicture);

gpointer
lba_picture_get (GObject *obj, gchar fmt[16], guint *w, guint *h)
{
  LbaPicture *self = bm_get_LbaPicture (obj);

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (fmt != NULL, NULL);
  g_return_val_if_fail (w != NULL, NULL);
  g_return_val_if_fail (h != NULL, NULL);

  if (G_UNLIKELY (self->format == NULL))
    return NULL;

  if (3 != sscanf (self->format, "%s w(%u) h(%u)", fmt, w, h)) {
    g_critical ("Bad format %s", self->format);
    return NULL;
  }

  return g_value_dup_boxed (&self->data);
}

void
lba_picture_set (GObject *obj, const gchar fmt[16], guint w, guint h, gpointer data)
{
  LbaPicture *self = bm_get_LbaPicture (obj);

  g_return_if_fail (self != NULL);
  g_return_if_fail (fmt != NULL);
  g_return_if_fail (w != 0);
  g_return_if_fail (h != 0);
  g_return_if_fail (data != NULL);

  /* TODO: LBA_TYPE_BOXED with type discovery */
  /* g_return_val_if_fail (G_TYPE_IS_BOXED (data), FALSE); */

  g_rec_mutex_lock (&self->lock);
  g_value_take_boxed (&self->data, data);
  g_free (self->format);
  /* NOTE: noify only if have changed */
  self->format = g_strdup_printf ("%s w(%u) h(%u)", fmt, w, h);
  g_rec_mutex_unlock (&self->lock);

  g_object_notify (obj, "format");
  g_object_notify (obj, "data");
}

typedef enum {
  PROP_DATA = 1,
  PROP_FORMAT,
  N_PROPERTIES
} LbaPictureProperty;

static void
lba_picture_set_property (GObject * object,
                           guint property_id, const GValue * value,
                           GParamSpec * pspec) {
  LbaPicture *self = bm_get_LbaPicture (object);
  
  g_rec_mutex_lock (&self->lock);
  switch ((LbaPictureProperty) property_id) {
  case PROP_FORMAT:
    g_free (self->format);
    self->format = g_value_dup_string (value);
    break;
  case PROP_DATA:
    g_value_copy (value, &self->data);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
  g_rec_mutex_unlock (&self->lock);
}

static void
lba_picture_get_property (GObject * object,
                           guint property_id, GValue * value, GParamSpec * pspec) {
  LbaPicture *self = bm_get_LbaPicture (object);

  g_rec_mutex_lock (&self->lock);
  switch ((LbaPictureProperty) property_id) {
  case PROP_DATA:
    g_value_copy (&self->data, value);
    break;
  case PROP_FORMAT:
    g_value_set_string (value, self->format);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
  g_rec_mutex_unlock (&self->lock);
}

static void
lba_picture_init (GObject * object, LbaPicture * self) {
  LbaPictureClass * mc = BM_GET_CLASS (self, LbaPictureClass);
  
  g_rec_mutex_init (&self->lock);
  g_value_init (&self->data, mc->data_type);
}

static void
lba_picture_finalize (GObject * gobject) {
  LbaPicture *self = bm_get_LbaPicture (gobject);

  g_value_unset (&self->data);
  g_clear_pointer (&self->format, g_free);
  g_rec_mutex_clear (&self->lock);

  BM_CHAINUP (self, GObject)->finalize (gobject);
}

static void
lba_picture_class_init (GObjectClass * gobj_class, LbaPictureClass * mixin_class) {
  gobj_class->set_property = lba_picture_set_property;
  gobj_class->get_property = lba_picture_get_property;
  gobj_class->finalize = lba_picture_finalize;

  if (mixin_class->data_type == 0)
    mixin_class->data_type = G_TYPE_BYTES;

  g_return_if_fail (G_TYPE_IS_BOXED (mixin_class->data_type));
  
  {
    GParamFlags maybe_writable = 0;

    if (mixin_class->writable_properties)
      maybe_writable |= G_PARAM_WRITABLE;
    
    g_object_class_install_property
        (gobj_class,
            PROP_DATA,
            g_param_spec_boxed ("data",
                "Data", "Data",
                mixin_class->data_type,
                G_PARAM_STATIC_STRINGS | G_PARAM_READABLE | maybe_writable));

    g_object_class_install_property (gobj_class, PROP_FORMAT,
        g_param_spec_string ("format",
            "Format",
            "Format",
            "rgba8888 w(128) h(128)",
            G_PARAM_STATIC_STRINGS |
            G_PARAM_READABLE | maybe_writable));
  }
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_picture);
