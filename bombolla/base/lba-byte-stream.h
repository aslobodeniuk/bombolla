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

#ifndef __LBA_BYTE_STREAM_H__
#  define __LBA_BYTE_STREAM_H__

#  include <glib-object.h>
#  include <glib/gstdio.h>

GType lba_byte_stream_get_type (void);

#  define G_TYPE_LBA_BYTE_STREAM (lba_byte_stream_get_type ())
#  define LBA_BYTE_STREAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),G_TYPE_LBA_BYTE_STREAM ,LbaByteStreamClass))
#  define LBA_BYTE_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_LBA_BYTE_STREAM ,LbaByteStreamClass))
#  define LBA_IS_BYTE_STREAM(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_LBA_BYTE_STREAM))

typedef struct _LbaByteStream {
  GObject parent;
  gboolean opened;
} LbaByteStream;

typedef struct _LbaByteStreamClass {
  GObjectClass parent;

  /* Actions */
    gboolean (*write) (LbaByteStream *, GBytes * data, GError ** err);
    gboolean (*open) (LbaByteStream *, GError ** err);
  void (*close) (LbaByteStream *);
} LbaByteStreamClass;

gboolean
lba_byte_stream_write_gbytes (LbaByteStream *, GBytes * data, GError ** err);

/* This calls are unneccessary: stream can open on first writing, and close
 * on object's destruction */
gboolean lba_byte_stream_open (LbaByteStream *, GError ** err);
void
  lba_byte_stream_close (LbaByteStream *);

#endif
