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

#ifndef __BASEGL2D_H__
#  define __BASEGL2D_H__

#  include <glib-object.h>
#  include <glib/gstdio.h>
#  include "bombolla/base/lba-base2d.h"
#  include "bombolla/base/lba-base-opengl-interface.h"

GType basegl2d_get_type (void);

#  define G_TYPE_BASEGL2D (basegl2d_get_type ())

#  define BASE_GL2D_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),G_TYPE_BASEGL2D ,Basegl2dClass))
#  define BASE_GL2D_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_BASEGL2D ,Basegl2dClass))

typedef struct _Basegl2d {
  Base2d parent;

  BaseOpenGLInterface *i;

} Basegl2d;

typedef struct _Basegl2dClass {
  Base2dClass parent;

  void (*draw) (Basegl2d *, BaseOpenGLInterface *);
} Basegl2dClass;

#endif
