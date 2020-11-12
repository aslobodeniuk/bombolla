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


#ifndef __BASEGL3D_H__
#define __BASEGL3D_H__

#include <glib-object.h>
#include <glib/gstdio.h>
#include "bombolla/base/lba-base3d.h"
#include "bombolla/base/lba-base-opengl-interface.h"

GType basegl3d_get_type (void);

#define G_TYPE_BASEGL3D (basegl3d_get_type ())

#define BASE_GL3D_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),G_TYPE_BASEGL3D ,Basegl3dClass))
#define BASE_GL3D_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_BASEGL3D ,Basegl3dClass))

typedef struct _Basegl3d
{
  Base3d parent;

  BaseOpenGLInterface *i;

} Basegl3d;


typedef struct _Basegl3dClass
{
  Base3dClass parent;

  void (*draw) (Basegl3d *, BaseOpenGLInterface *);
} Basegl3dClass;


#endif
