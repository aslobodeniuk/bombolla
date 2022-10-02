/* la Bombolla GObject shell.
 * Copyright (C) 2022 Aleksandr Slobodeniuk
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

#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include <gio/gio.h>

typedef struct _LbaPicture {
  GObject parent;
  GInputStream *input_stream;
  guint w;
  guint h;
  GBytes *data;
  gchar *format;
} LbaPicture;

typedef struct _LbaPictureClass {
  GObjectClass parent;
} LbaPictureClass;

typedef enum {
  PROP_INPUT_STREAM = 1,
  PROP_H,
  PROP_W,
  PROP_DATA,
  PROP_FORMAT,
  N_PROPERTIES
} LbaPictureProperty;

enum {
  SIGNAL_ON_UPDATE,
  LAST_SIGNAL
};

static guint lba_picture_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (LbaPicture, lba_picture, G_TYPE_OBJECT);

static guint
lba_picture_get_data_size (LbaPicture * self) {
  return self->w * self->h * 4;
}

static void
lba_picture_have_data_cb (GObject * obj, GAsyncResult * result, gpointer user_data) {
  GBytes *bytes;
  GError *error = NULL;
  LbaPicture *self = (LbaPicture *) user_data;
  GInputStream *in = (GInputStream *) obj;

  bytes = g_input_stream_read_bytes_finish (in, result, &error);

  LBA_ASSERT (bytes != NULL);

  if (error) {
    g_error ("%s", error->message);
  }

  LBA_LOG ("Bytes read %d. Expecting %d", (int)g_bytes_get_size (bytes),
           lba_picture_get_data_size (self));
  LBA_ASSERT (g_bytes_get_size (bytes) == lba_picture_get_data_size (self));

  if (self->data) {
    g_bytes_unref (self->data);
  }
  self->data = bytes;

  g_signal_emit (self, lba_picture_signals[SIGNAL_ON_UPDATE], 0);

  /* TODO: read again? Also, how to block?
   * we can't block here */
}

static void
lba_picture_set_property (GObject * object,
                          guint property_id, const GValue * value,
                          GParamSpec * pspec) {
  LbaPicture *self = (LbaPicture *) object;

  switch ((LbaPictureProperty) property_id) {
  case PROP_FORMAT:
    g_free (self->format);
    self->format = g_value_dup_string (value);
    break;

  case PROP_W:
    self->w = g_value_get_uint (value);
    break;
  case PROP_H:
    self->h = g_value_get_uint (value);
    break;
  case PROP_INPUT_STREAM:
    if (self->input_stream) {
      g_object_unref (self->input_stream);
    }

    self->input_stream = g_value_get_object (value);

    if (self->input_stream) {
      g_input_stream_read_bytes_async (self->input_stream,
                                       lba_picture_get_data_size (self),
                                       G_PRIORITY_DEFAULT, NULL,
                                       lba_picture_have_data_cb, self);
    }

    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_picture_get_property (GObject * object,
                          guint property_id, GValue * value, GParamSpec * pspec) {
  LbaPicture *self = (LbaPicture *) object;

  switch ((LbaPictureProperty) property_id) {
  case PROP_INPUT_STREAM:
    g_value_set_object (value, self->input_stream);
    break;
  case PROP_W:
    g_value_set_uint (value, self->w);
    break;
  case PROP_H:
    g_value_set_uint (value, self->h);
    break;
  case PROP_DATA:
    g_value_set_boxed (value, self->data);
    break;
  case PROP_FORMAT:
    g_value_set_string (value, self->format);
    break;

  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_picture_init (LbaPicture * self) {
}

static void
lba_picture_dispose (GObject * gobject) {
  LbaPicture *self = (LbaPicture *) gobject;

  g_free (self->format);

  if (self->input_stream) {
    g_object_unref (self->input_stream);
  }
  G_OBJECT_CLASS (lba_picture_parent_class)->dispose (gobject);
}

static void
lba_picture_class_init (LbaPictureClass * klass) {
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->dispose = lba_picture_dispose;
  gobj_class->set_property = lba_picture_set_property;
  gobj_class->get_property = lba_picture_get_property;

  lba_picture_signals[SIGNAL_ON_UPDATE] =
      g_signal_new ("on-update", G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  g_object_class_install_property (gobj_class, PROP_INPUT_STREAM,
                                   g_param_spec_object ("input-stream",
                                                        "input-stream",
                                                        "input-stream",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property
      (gobj_class,
       PROP_W,
       g_param_spec_uint ("width",
                          "W", "Width",
                          2, 2048,
                          32,
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property
      (gobj_class,
       PROP_H,
       g_param_spec_uint ("height",
                          "H", "Height",
                          2, 2048,
                          32,
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property
      (gobj_class,
       PROP_DATA,
       g_param_spec_boxed ("data",
                           "Data", "Data",
                           G_TYPE_BYTES, G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));

  g_object_class_install_property (gobj_class, PROP_FORMAT,
                                   g_param_spec_string ("format",
                                                        "Format",
                                                        "Format",
                                                        "rgba8888",
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_picture);
