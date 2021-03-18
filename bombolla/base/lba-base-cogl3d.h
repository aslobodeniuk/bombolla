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


#ifndef __BASE_COGL3D_H__
#define __BASE_COGL3D_H__

#include <glib-object.h>
#include <cogl/cogl.h>

#include "bombolla/base/lba-base3d.h"

GType base_cogl3d_get_type (void);

#define G_TYPE_BASE_COGL3D (base_cogl3d_get_type ())
#define G_OBJECT_IS_COGL3D(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_BASE_COGL3D))

#define BASE_COGL3D_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), G_TYPE_BASE_COGL3D, BaseCogl3dClass))
#define BASE_COGL3D_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_BASE_COGL3D, BaseCogl3dClass))

typedef struct _BaseCogl3d
{
  Base3d parent;

  CoglFramebuffer *fb;
  CoglPipeline *pipeline;
  CoglContext *ctx;
} BaseCogl3d;


typedef struct _BaseCogl3dClass
{
  Base3dClass parent;

  void (*paint) (BaseCogl3d *, CoglFramebuffer *fb, CoglPipeline *pipeline);
  void (*reopen) (BaseCogl3d *, CoglFramebuffer *fb, CoglPipeline *pipeline,
      CoglContext *ctx);
} BaseCogl3dClass;


#endif
