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

#ifndef __BASE2D_H__
#  define __BASE2D_H__

#  include <glib-object.h>
#  include <glib/gstdio.h>
#  include "bombolla/base/lba-basedrawable.h"

GType base2d_get_type (void);

#  define G_TYPE_BASE2D (base2d_get_type ())

#  define BASE2D_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),G_TYPE_BASE2D ,Base2dClass))
#  define BASE2D_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_BASE2D ,Base2dClass))

typedef struct _Base2d {
  BaseDrawable parent;

  guint x,
    y,
    width,
    height;

} Base2d;

typedef struct _Base2dClass {
  BaseDrawableClass parent;
} Base2dClass;

#endif
