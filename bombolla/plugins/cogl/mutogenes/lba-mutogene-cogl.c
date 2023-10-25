/* la Bombolla GObject shell.
 * Copyright (C) 2023 Alexander Slobodeniuk
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

#include "bombolla/lba-log.h"
#include "bombolla/base/i3d.h"
#include "../base/icogl.h"
#include <bombolla/base/lba-basedrawable.h>
#include <gmo/gmo.h>

static const gchar *global_lba_plugin_name = "LbaCogl";
typedef struct _LbaCogl {
  CoglFramebuffer *fb;
  CoglPipeline *pipeline;
  CoglContext *ctx;

  GMutex lock;
} LbaCogl;

typedef struct _LbaCoglClass {
  int dummy;
} LbaCoglClass;

static void lba_cogl_icogl_init (LbaICogl * iface);

GMO_DEFINE_MUTOGENE (lba_cogl, LbaCogl, GMO_ADD_IFACE (lba, cogl, icogl));

static void
lba_cogl_get_ctx (GObject * obj, CoglContext ** ctx, CoglPipeline ** pipeline) {
  LbaCogl *self = gmo_get_LbaCogl (obj);

  if (ctx)
    *ctx = self->ctx;

  if (pipeline)
    *pipeline = self->pipeline;
}

static void
lba_cogl_icogl_init (LbaICogl * iface) {
  /* The rest will be implemented by the children */
  iface->get_ctx = lba_cogl_get_ctx;
}

static void
lba_cogl_draw (GObject * obj) {
  LbaCogl *self = gmo_get_LbaCogl (obj);
  LbaICogl *iface;

  LBA_LOG ("draw");
  iface = G_TYPE_INSTANCE_GET_INTERFACE (obj, LBA_ICOGL, LbaICogl);

  if (!iface->paint)
    return;

  g_mutex_lock (&self->lock);

  if (!self->fb || !self->pipeline) {
    LBA_LOG ("Incompatible drawing scene: needs COGL framebuffer and pipeline");
    g_mutex_unlock (&self->lock);
    return;
  }

  iface->paint (obj, self->fb, self->pipeline);

  g_mutex_unlock (&self->lock);
}

static void
lba_cogl_scene_reopen (GObject * scene, gpointer user_data) {
  LbaICogl *iface;
  LbaCogl *self = gmo_get_LbaCogl (user_data);

  LBA_LOG ("reopen");
  iface = G_TYPE_INSTANCE_GET_INTERFACE (user_data, LBA_ICOGL, LbaICogl);

  g_mutex_lock (&self->lock);

  g_object_get (scene,
                "cogl-framebuffer", &self->fb, "cogl-pipeline", &self->pipeline,
                "cogl-ctx", &self->ctx, NULL);

  if (iface->reopen && self->fb && self->pipeline && self->ctx) {
    iface->reopen (user_data, self->fb, self->pipeline, self->ctx);
  }

  g_mutex_unlock (&self->lock);
}

void
lba_cogl_has_drawing_scene (GObject * gobject, GParamSpec * pspec,
                            gpointer user_data) {
  BaseDrawable *drawable = (BaseDrawable *) gobject;

  /* Cache pipeline/fb from the drawing scene that must be LbaCoglWindow */

  if (!drawable->scene)
    return;

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
                          G_CALLBACK (lba_cogl_scene_reopen), gobject);

  /* Update also at this moment */
  lba_cogl_scene_reopen (G_OBJECT (drawable->scene), gobject);

  /* Request redraw: if we've just added something new */
  g_signal_emit_by_name (G_OBJECT (drawable->scene), "request-redraw", NULL);
}

static void
lba_cogl_init (GObject * obj, gpointer mutogene) {
  LbaCogl *self = mutogene;

  g_mutex_init (&self->lock);
  g_signal_connect (obj, "notify::drawing-scene",
                    G_CALLBACK (lba_cogl_has_drawing_scene), NULL);
}

static void
lba_cogl_dispose (GObject * gobject) {
  LbaCogl *self = gmo_get_LbaCogl (gobject);

  g_mutex_clear (&self->lock);

  GMO_CHAINUP (gobject, lba_cogl, GObject)->dispose (gobject);
}

static void
lba_cogl_class_init (GObjectClass * gobject_class, gpointer mutogene) {
  BaseDrawableClass *base_drawable_class = (BaseDrawableClass *) gobject_class;

  base_drawable_class->draw = (void (*)(BaseDrawable *))lba_cogl_draw;
  gobject_class->dispose = lba_cogl_dispose;
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_cogl);
