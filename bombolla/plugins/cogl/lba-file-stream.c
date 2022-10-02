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

typedef struct _LbaFileStream {
  GObject parent;
  GInputStream *stream;
  gchar *path;
  GFile *file;
} LbaFileStream;

typedef struct _LbaFileStreamClass {
  GObjectClass parent;
} LbaFileStreamClass;

typedef enum {
  PROP_STREAM = 1,
  PROP_PATH,
  N_PROPERTIES
} LbaFileStreamProperty;

G_DEFINE_TYPE (LbaFileStream, lba_file_stream, G_TYPE_OBJECT);

static void
lba_file_stream_path_update (LbaFileStream * self, const gchar * path) {
  g_free (self->path);
  self->path = g_strdup (path);
  if (self->stream) {
    g_object_unref (self->stream);
  }
  if (self->file) {
    g_object_unref (self->file);
  }
  self->file = g_file_new_for_path (self->path);
  /* TODO: use g_file_read_async () */
  self->stream = (GInputStream *) g_file_read (self->file, NULL, NULL);
  LBA_ASSERT (self->stream);
  LBA_LOG ("Opened file %s for reading", self->path);
}

static void
lba_file_stream_set_property (GObject * object,
                              guint property_id, const GValue * value,
                              GParamSpec * pspec) {
  LbaFileStream *self = (LbaFileStream *) object;

  switch ((LbaFileStreamProperty) property_id) {
  case PROP_PATH:
    lba_file_stream_path_update (self, g_value_get_string (value));
    break;

  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_file_stream_get_property (GObject * object,
                              guint property_id, GValue * value,
                              GParamSpec * pspec) {
  LbaFileStream *self = (LbaFileStream *) object;

  switch ((LbaFileStreamProperty) property_id) {
  case PROP_PATH:
    g_value_set_string (value, self->path);
    break;
  case PROP_STREAM:
    g_value_set_object (value, self->stream);
    break;

  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_file_stream_init (LbaFileStream * self) {
  lba_file_stream_path_update (self, "/dev/random");
}

static void
lba_file_stream_dispose (GObject * gobject) {
  LbaFileStream *self = (LbaFileStream *) gobject;

  if (self->stream) {
    g_object_unref (self->stream);
  }
  if (self->file) {
    g_object_unref (self->file);
  }

  g_free (self->path);

  G_OBJECT_CLASS (lba_file_stream_parent_class)->dispose (gobject);
}

static void
lba_file_stream_class_init (LbaFileStreamClass * klass) {
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->dispose = lba_file_stream_dispose;
  gobj_class->set_property = lba_file_stream_set_property;
  gobj_class->get_property = lba_file_stream_get_property;

  g_object_class_install_property (gobj_class, PROP_STREAM,
                                   g_param_spec_object ("stream",
                                                        "stream",
                                                        "stream",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READABLE));

  g_object_class_install_property (gobj_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "Path",
                                                        "Path",
                                                        "/dev/random",
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_file_stream);
