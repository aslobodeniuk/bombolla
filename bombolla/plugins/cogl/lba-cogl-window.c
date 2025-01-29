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

#include <bombolla/base/lba-basewindow.h>
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

#include <cogl/cogl.h>

typedef struct _LbaCoglWindow {
  BaseWindow parent;

  CoglContext *ctx;
  CoglFramebuffer *fb;
  CoglPipeline *pipeline;

  unsigned int redraw_idle;
  CoglBool is_dirty;
  CoglBool draw_ready;

  GRecMutex lock;
  gboolean stopping;

} LbaCoglWindow;

typedef struct _LbaCoglWindowClass {
  BaseWindowClass parent;
} LbaCoglWindowClass;

static gboolean
lba_cogl_window_paint_cb (void *user_data) {
  LbaCoglWindow *self = (LbaCoglWindow *) user_data;

  LBA_LOG ("repainting..");

  LBA_LOCK (self);
  if (self->stopping) {
    LBA_LOG ("Stopping..");
    goto cleanup;
  }

  self->redraw_idle = 0;
  self->is_dirty = FALSE;
  self->draw_ready = FALSE;

  cogl_framebuffer_clear4f (self->fb, COGL_BUFFER_BIT_COLOR | COGL_BUFFER_BIT_DEPTH,
                            0, 0, 0, 1);

  /* Now let friend objects draw something */
  base_window_notify_display ((BaseWindow *) self);

  /* And swap buffers */
  cogl_onscreen_swap_buffers (self->fb);
cleanup:
  LBA_UNLOCK (self);
  return G_SOURCE_REMOVE;
}

static void
lba_cogl_window_close (BaseWindow *base) {
//  LbaCoglWindow *self = (LbaCoglWindow *) base;

  /* Tell main loop to close the window */
  LBA_LOG ("TODO");
}

static void
lba_cogl_window_maybe_redraw (LbaCoglWindow *self) {
  if (self->is_dirty && self->draw_ready && self->redraw_idle == 0) {
    /* We'll draw on idle instead of drawing immediately so that
     * if Cogl reports multiple dirty rectangles we won't
     * redundantly draw multiple frames */
    self->redraw_idle = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                                         lba_cogl_window_paint_cb,
                                         g_object_ref (self), g_object_unref);
  }
}

static void
lba_cogl_window_frame_event_cb (CoglOnscreen *onscreen,
                                CoglFrameEvent event, CoglFrameInfo *info,
                                void *user_data) {
  LbaCoglWindow *self = (LbaCoglWindow *) user_data;

  if (event == COGL_FRAME_EVENT_SYNC) {
    self->draw_ready = TRUE;
    lba_cogl_window_maybe_redraw (self);
  }
}

static void
lba_cogl_window_request_redraw (BaseWindow *base) {
  LbaCoglWindow *self = (LbaCoglWindow *) base;

  self->is_dirty = TRUE;
  lba_cogl_window_maybe_redraw (self);
}

static void
lba_cogl_window_dirty_cb (CoglOnscreen *onscreen,
                          const CoglOnscreenDirtyInfo *info, void *user_data) {
  lba_cogl_window_request_redraw ((BaseWindow *) user_data);
}

/* Fixme: defaults are not set */
static void
lba_cogl_window_open (BaseWindow *base) {
  LbaCoglWindow *self = (LbaCoglWindow *) base;

  CoglOnscreen *onscreen;
  CoglError *error = NULL;
  GSource *cogl_source;

  LBA_LOCK (self);
  if (self->stopping) {
    LBA_LOG ("Stopping..");
    goto cleanup;
  }
  self->redraw_idle = 0;
  self->is_dirty = FALSE;
  self->draw_ready = TRUE;

  self->ctx = cogl_context_new (NULL, &error);
  if (!self->ctx) {
    LBA_LOG ("Failed to create context: %s\n", error->message);
    goto cleanup;
  }

  onscreen = cogl_onscreen_new (self->ctx, base->width, base->height);

  cogl_onscreen_show (onscreen);
  self->fb = onscreen;

  cogl_onscreen_set_resizable (onscreen, TRUE);

  self->pipeline = cogl_pipeline_new (self->ctx);

  cogl_source = cogl_glib_source_new (self->ctx, G_PRIORITY_DEFAULT);

  /* FIXME: this is a 3d-only thing: */
  /* ====================================================== */
  {
    // The proper design should be:
    // -- LbaBaseWindow - width, height, title, position, blah. Open/Close signals.
    // -- LbaX11Window mixin - key events (??). Implements Open.
    //    The window doesn't know if it's going to be OpenGL or something else..
    //
    // -- LbaCoglFrameBuffer - ctx, pipeline, framebuffer. Also implements Open ??
    //    Other use of the FrameBuffer is rendering to a texture that has width/height..
    //    So FrameBuffer should actually export the Texture interface ??
    
    int framebuffer_width;
    int framebuffer_height;

    framebuffer_width = cogl_framebuffer_get_width (self->fb);
    framebuffer_height = cogl_framebuffer_get_height (self->fb);

    LBA_LOG ("framebuffer_width = %d, framebuffer_height = %d", framebuffer_width,
             framebuffer_height);

    cogl_framebuffer_set_viewport (self->fb, 0, 0,
                                   framebuffer_width, framebuffer_height);

    {
      CoglDepthState depth_state;

      /* Needed for proper 3d rendering */
      cogl_depth_state_init (&depth_state);
      cogl_depth_state_set_test_enabled (&depth_state, TRUE);

      cogl_pipeline_set_depth_state (self->pipeline, &depth_state, NULL);
    }
  }
  /* ========================================================= */

  g_source_attach (cogl_source, NULL);

  cogl_onscreen_add_frame_callback (self->fb, lba_cogl_window_frame_event_cb,
                                    self, NULL);
  cogl_onscreen_add_dirty_callback (self->fb, lba_cogl_window_dirty_cb, self, NULL);

cleanup:
  LBA_UNLOCK (self);
}

static void
lba_cogl_window_init (LbaCoglWindow *self) {
  g_rec_mutex_init (&self->lock);
}

typedef enum {
  PROP_COGL_FRAMEBUFFER = 1,
  PROP_COGL_PIPELINE,
  PROP_COGL_CTX,
  N_PROPERTIES
} LbaCoglWindowProperty;

static void
lba_cogl_window_get_property (GObject *object,
                              guint property_id, GValue *value, GParamSpec *pspec) {
  LbaCoglWindow *self = (LbaCoglWindow *) object;

  switch ((LbaCoglWindowProperty) property_id) {
  case PROP_COGL_FRAMEBUFFER:
    g_value_set_pointer (value, self->fb);
    break;

  case PROP_COGL_PIPELINE:
    g_value_set_pointer (value, self->pipeline);
    break;

  case PROP_COGL_CTX:
    g_value_set_pointer (value, self->ctx);
    break;

  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_cogl_window_finalize (GObject *obj) {
  LbaCoglWindow *self = (LbaCoglWindow *) obj;

  LBA_LOCK (self);
  self->stopping = TRUE;
  LBA_UNLOCK (self);
  g_rec_mutex_clear (&self->lock);
}

static void
lba_cogl_window_class_init (LbaCoglWindowClass *klass) {
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);
  BaseWindowClass *base_class = BASE_WINDOW_CLASS (klass);

  base_class->open = lba_cogl_window_open;
  base_class->close = lba_cogl_window_close;
  base_class->request_redraw = lba_cogl_window_request_redraw;

  gobj_class->get_property = lba_cogl_window_get_property;
  gobj_class->finalize = lba_cogl_window_finalize;

  g_object_class_install_property (gobj_class,
                                   PROP_COGL_FRAMEBUFFER,
                                   g_param_spec_pointer ("cogl-framebuffer",
                                                         "Cogl Framebuffer",
                                                         "Cogl Framebuffer",
                                                         G_PARAM_STATIC_STRINGS |
                                                         G_PARAM_READABLE));

  g_object_class_install_property (gobj_class,
                                   PROP_COGL_PIPELINE,
                                   g_param_spec_pointer ("cogl-pipeline",
                                                         "Cogl Pipeline",
                                                         "Cogl Pipeline",
                                                         G_PARAM_STATIC_STRINGS |
                                                         G_PARAM_READABLE));

  g_object_class_install_property (gobj_class,
                                   PROP_COGL_CTX,
                                   g_param_spec_pointer ("cogl-ctx", "Cogl Ctx",
                                                         "Cogl Context",
                                                         G_PARAM_STATIC_STRINGS |
                                                         G_PARAM_READABLE));
}

G_DEFINE_TYPE (LbaCoglWindow, lba_cogl_window, G_TYPE_BASE_WINDOW)
/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_cogl_window);
