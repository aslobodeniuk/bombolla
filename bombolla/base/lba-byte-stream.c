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

#include "bombolla/base/lba-byte-stream.h"
#include "bombolla/lba-log.h"

enum {
  SIGNAL_ON_HAVE_DATA,
  LAST_SIGNAL
};

static guint lba_byte_stream_signals[LAST_SIGNAL] = { 0 };

gboolean
lba_byte_stream_write_gbytes (LbaByteStream * self, GBytes * data, GError ** err) {
  LbaByteStreamClass *klass = LBA_BYTE_STREAM_GET_CLASS (self);

  if (klass->write) {
    return klass->write (self, data, err);
  }

  g_critical ("Class doesn't have a write function");
  return FALSE;
}

/* This calls are unneccessary: stream can open on first writing, and close
 * on object's destruction */
gboolean
lba_byte_stream_open (LbaByteStream * self, GError ** err) {
  LbaByteStreamClass *klass = LBA_BYTE_STREAM_GET_CLASS (self);

  if (klass->open) {
    self->opened = klass->open (self, err);
  }

  return self->opened;
}

void
lba_byte_stream_close (LbaByteStream * self) {
  LbaByteStreamClass *klass = LBA_BYTE_STREAM_GET_CLASS (self);

  if (klass->close) {
    klass->close (self);
  }

  self->opened = FALSE;
}

static gboolean
lba_byte_stream_write_passthrough (LbaByteStream * self, GBytes * data,
                                   GError ** err) {
  LBA_LOG ("passthrough");
  g_signal_emit (self, lba_byte_stream_signals[SIGNAL_ON_HAVE_DATA], 0, data);
  g_bytes_unref (data);
  return TRUE;
}

static void
lba_byte_stream_init (LbaByteStream * self) {
}

static void
lba_byte_stream_dispose (GObject * gobject) {
  LbaByteStream *self = (LbaByteStream *) gobject;
  LbaByteStreamClass *klass = LBA_BYTE_STREAM_GET_CLASS (self);

  if (self->opened && klass->close) {
    klass->close (self);
  }
}

static void
lba_byte_stream_class_init (LbaByteStreamClass * klass) {
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = lba_byte_stream_dispose;

  lba_byte_stream_signals[SIGNAL_ON_HAVE_DATA] =
      g_signal_new ("on-have-data", G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    0, NULL, NULL,
                    g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1, G_TYPE_BYTES);

  klass->write = lba_byte_stream_write_passthrough;
}

G_DEFINE_TYPE (LbaByteStream, lba_byte_stream, G_TYPE_OBJECT)
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_byte_stream);
