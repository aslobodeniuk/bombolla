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

#include "lba-base-cogl3d.h"
#include "bombolla/lba-log.h"

static const gchar *global_lba_plugin_name = "BaseCogl3d";

G_DEFINE_TYPE (BaseCogl3d, base_cogl3d, G_TYPE_BASE3D);

static void
base_cogl3d_draw (BaseDrawable * base) {
  BaseCogl3d *self = (BaseCogl3d *) base;
  BaseCogl3dClass *klass = BASE_COGL3D_GET_CLASS (self);

  LBA_LOG ("draw");

  /* Now let the child draw */
  if (klass->paint) {
    g_mutex_lock (&self->lock);

    if (!self->fb || !self->pipeline) {
      LBA_LOG ("Incompatible drawing scene: needs COGL framebuffer and pipeline");
      g_mutex_unlock (&self->lock);
      return;
    }

    klass->paint (self, self->fb, self->pipeline);

    g_mutex_unlock (&self->lock);
  }
}

static void
base_cogl3d_scene_reopen (GObject * scene, gpointer user_data) {
  BaseCogl3d *self = (BaseCogl3d *) user_data;
  BaseCogl3dClass *klass = BASE_COGL3D_GET_CLASS (self);

  LBA_LOG ("reopen");

  g_mutex_lock (&self->lock);

  g_object_get (scene,
                "cogl-framebuffer", &self->fb, "cogl-pipeline", &self->pipeline,
                "cogl-ctx", &self->ctx, NULL);

  if (klass->reopen && self->fb && self->pipeline && self->ctx) {
    klass->reopen (self, self->fb, self->pipeline, self->ctx);
  }

  g_mutex_unlock (&self->lock);
}

void
base_cogl3d_has_drawing_scene (GObject * gobject, GParamSpec * pspec,
                               gpointer user_data) {
  BaseDrawable *drawable = (BaseDrawable *) gobject;
  BaseCogl3d *self = (BaseCogl3d *) gobject;

  /* Cache pipeline/fb from the drawing scene that must be LbaCoglWindow */

  if (drawable->scene) {
    GObjectClass *scene_instance_class = G_OBJECT_GET_CLASS (drawable->scene);

    /* Check if scene has properties we need */
    if (!g_object_class_find_property (scene_instance_class, "cogl-framebuffer")
        || !g_object_class_find_property (scene_instance_class, "cogl-pipeline")
        || !g_object_class_find_property (scene_instance_class, "cogl-ctx")) {
      LBA_LOG ("Incompatible drawing scene: "
               "must have 'cogl-framebuffer', 'cogl-pipeline' and 'cogl-ctx' parameters");
      return;
    }

    /* On each reopen we must update */
    g_signal_connect_after (drawable->scene, "open",
                            G_CALLBACK (base_cogl3d_scene_reopen), self);

    /* Update also at this moment */
    base_cogl3d_scene_reopen (G_OBJECT (drawable->scene), self);

    /* Request redraw: if we've just added something new */
    g_signal_emit_by_name (G_OBJECT (drawable->scene), "request-redraw", NULL);
  }
}

static void
base_cogl3d_init (BaseCogl3d * self) {
  g_mutex_init (&self->lock);

  g_signal_connect (self, "notify::drawing-scene",
                    G_CALLBACK (base_cogl3d_has_drawing_scene), self);
}

static void
base_cogl3d_dispose (GObject * gobject) {
  BaseCogl3d *self = (BaseCogl3d *) gobject;

  g_mutex_clear (&self->lock);

  /* Always chain up */
  G_OBJECT_CLASS (base_cogl3d_parent_class)->dispose (gobject);
}

static void
base_cogl3d_class_init (BaseCogl3dClass * klass) {
  BaseDrawableClass *base_drawable_class = (BaseDrawableClass *) klass;
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  base_drawable_class->draw = base_cogl3d_draw;

  gobject_class->dispose = base_cogl3d_dispose;
}
