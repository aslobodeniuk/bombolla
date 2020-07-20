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


#ifndef __BASE_DRAWABLE_H__
#define __BASE_DRAWABLE_H__

#include <glib-object.h>
#include <glib/gstdio.h>

GType base_drawable_get_type (void);

#define G_TYPE_BASE_DRAWABLE (base_drawable_get_type ())

#define BASE_DRAWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),G_TYPE_BASE_DRAWABLE ,BaseDrawableClass))
#define BASE_DRAWABLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_BASE_DRAWABLE ,BaseDrawableClass))

typedef struct _BaseDrawable
{
  GObject parent;

  /* instance members */
  int width;
  int height;
  int x_pos;
  int y_pos;
} BaseDrawable;


typedef struct _BaseDrawableClass
{
  GObjectClass parent;

  /* Actions */
  void (*draw) (BaseDrawable *);

} BaseDrawableClass;


typedef enum
{
  PROP_WIDTH = 1,
  PROP_HEIGHT,
  PROP_X_POS,
  PROP_Y_POS,
  PROP_DRAWING_SCENE,
  BASE_DRAWABLE_N_PROPERTIES
} BaseDrawableProperty;

void
base_drawable_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);

#endif
