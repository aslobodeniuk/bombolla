/* la Bombolla GObject shell.
 *
 * Copyright (C) 2023 Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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
#ifndef _LBA_ICOGL

#  include <cogl/cogl.h>
#  include <glib-object.h>

typedef struct _LbaICoglInterface {
  GTypeInterface g_iface;

  void (*paint) (GObject * obj, CoglFramebuffer * fb, CoglPipeline * pipeline);
  void (*reopen) (GObject * base, CoglFramebuffer * fb, CoglPipeline * pipeline,
                  CoglContext * ctx);
} LbaICoglInterface;

typedef LbaICoglInterface LbaICogl;

#  define LBA_ICOGL lba_icogl_get_type()
GType lba_icogl_get_type (void);

#endif
