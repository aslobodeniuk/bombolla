/* la Bombolla GObject shell.
 * Copyright (C) 2020 Aleksandr Slobodeniuk
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

#ifndef __BASE_WINDOW_H__
#  define __BASE_WINDOW_H__

#  include <glib-object.h>
#  include <glib/gstdio.h>

GType base_window_get_type (void);

#  define G_TYPE_BASE_WINDOW (base_window_get_type ())
#  define BASE_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),G_TYPE_BASE_WINDOW ,BaseWindowClass))
#  define BASE_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_BASE_WINDOW ,BaseWindowClass))

typedef struct _BaseWindow {
  GObject parent;

  /* instance members */
  int width;
  int height;
  int x_pos;
  int y_pos;

  gchar *title;
  gboolean opened;
} BaseWindow;

typedef struct _BaseWindowClass {
  GObjectClass parent;

  /* Events */
  void (*on_display) (BaseWindow *);

  /* Actions */
  void (*open) (BaseWindow *);
  void (*close) (BaseWindow *);
  void (*request_redraw) (BaseWindow *);

} BaseWindowClass;

void base_window_notify_display (BaseWindow * self);

#endif
