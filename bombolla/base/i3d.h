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
#ifndef _LBA_I3D

typedef struct _LbaI3DInterface {
  GTypeInterface g_iface;

  void (*xyz) (GObject *, gdouble * x, gdouble * y, gdouble * z);
} LbaI3DInterface;

typedef LbaI3DInterface LbaI3D;

#  define LBA_I3D lba_i3d_get_type ()
GType lba_i3d_get_type (void);

#endif
