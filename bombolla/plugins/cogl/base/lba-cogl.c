/* la Bombolla GObject shell
 *
 * Copyright (c) 2024, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "bombolla/lba-log.h"
#include "icogl.h"
#include <bombolla/base/lba-basedrawable.h>
#include <bmixin/bmixin.h>

typedef struct _LbaCogl {
  BMixinInstance p;
  CoglFramebuffer *fb;
  CoglPipeline *pipeline;
  CoglContext *ctx;

  GMutex lock;
  gboolean closing;
} LbaCogl;

typedef struct _LbaCoglClass {
  BMixinClass c;
  int dummy;
} LbaCoglClass;

static void lba_cogl_icogl_init (LbaICogl * iface);

/* *INDENT-OFF* */ 
BM_DEFINE_MIXIN (lba_cogl, LbaCogl,
    BM_ADD_IFACE (lba, cogl, icogl),
    BM_ADD_DEP (base_drawable));
/* *INDENT-ON* */ 

static void
lba_cogl_get_ctx (GObject * obj, CoglContext ** ctx, CoglPipeline ** pipeline) {
  LbaCogl *self = bm_get_LbaCogl (obj);

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
lba_cogl_draw (BaseDrawable * obj) {
  LbaCogl *self = bm_get_LbaCogl (obj);
  LbaICogl *iface;

  LBA_LOG ("draw");
  iface = G_TYPE_INSTANCE_GET_INTERFACE (obj, LBA_ICOGL, LbaICogl);

  if (!iface->paint)
    return;

  g_mutex_lock (&self->lock);
  if (G_UNLIKELY (self->closing))
    goto done;

  if (!self->fb || !self->pipeline) {
    LBA_LOG ("Incompatible drawing scene: needs COGL framebuffer and pipeline");
    g_mutex_unlock (&self->lock);
    return;
  }

  iface->paint (G_OBJECT (obj), self->fb, self->pipeline);

done:
  g_mutex_unlock (&self->lock);
}

static void
lba_cogl_scene_reopen (GObject * scene, gpointer user_data) {
  LbaICogl *iface;
  LbaCogl *self = bm_get_LbaCogl (user_data);

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
lba_cogl_init (GObject * obj, LbaCogl * self) {
  g_mutex_init (&self->lock);
  g_signal_connect (obj, "notify::drawing-scene",
                    G_CALLBACK (lba_cogl_has_drawing_scene), NULL);
}

static void
lba_cogl_finalize (GObject * gobject) {
  LbaCogl *self = bm_get_LbaCogl (gobject);

  LBA_LOG ("finalize");
  /* Sync for case of closing while doing things */
  g_mutex_lock (&self->lock);
  self->closing = TRUE;
  g_mutex_unlock (&self->lock);

  g_mutex_clear (&self->lock);

  BM_CHAINUP (gobject, lba_cogl, GObject)->dispose (gobject);
}

static void
lba_cogl_class_init (GObjectClass * gobject_class, LbaCoglClass * mixin_class) {
  BaseDrawableClass *base_drawable_class = (BaseDrawableClass *) gobject_class;

  base_drawable_class->draw = lba_cogl_draw;
  gobject_class->finalize = lba_cogl_finalize;
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_cogl);
