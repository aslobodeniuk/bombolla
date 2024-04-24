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
