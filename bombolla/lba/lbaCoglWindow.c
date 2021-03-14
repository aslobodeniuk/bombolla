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

#include "bombolla/base/lba-basewindow.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

#include "cogl/cogl/cogl.h"

typedef struct _LbaCoglWindow
{
  BaseWindow parent;

  CoglContext *ctx;
  CoglFramebuffer *fb;
  CoglPipeline *pipeline;

  unsigned int redraw_idle;
  CoglBool is_dirty;
  CoglBool draw_ready;

} LbaCoglWindow;


typedef struct _LbaCoglWindowClass
{
  BaseWindowClass parent;
} LbaCoglWindowClass;


static gboolean
lba_cogl_window_paint_cb (void *user_data)
{
  LbaCoglWindow *self = (LbaCoglWindow *) user_data;

  LBA_LOG ("repainting..");

  self->redraw_idle = 0;
  self->is_dirty = FALSE;
  self->draw_ready = FALSE;

  cogl_framebuffer_clear4f (self->fb, COGL_BUFFER_BIT_COLOR, 0, 0, 0, 1);

  /* Now let friend objects draw something */
  base_window_notify_display ((BaseWindow *) self);

  /* And swap buffers */
  cogl_onscreen_swap_buffers (self->fb);

  return G_SOURCE_REMOVE;
}


static void
lba_cogl_window_close (BaseWindow * base)
{
//  LbaCoglWindow *self = (LbaCoglWindow *) base;

  /* Tell main loop to close the window */
  LBA_LOG ("TODO");
}


static void
lba_cogl_window_maybe_redraw (LbaCoglWindow * self)
{
  if (self->is_dirty && self->draw_ready && self->redraw_idle == 0) {
    /* We'll draw on idle instead of drawing immediately so that
     * if Cogl reports multiple dirty rectangles we won't
     * redundantly draw multiple frames */
    self->redraw_idle = g_idle_add (lba_cogl_window_paint_cb, self);
  }
}


static void
lba_cogl_window_frame_event_cb (CoglOnscreen * onscreen,
    CoglFrameEvent event, CoglFrameInfo * info, void *user_data)
{
  LbaCoglWindow *self = (LbaCoglWindow *) user_data;

  if (event == COGL_FRAME_EVENT_SYNC) {
    self->draw_ready = TRUE;
    lba_cogl_window_maybe_redraw (self);
  }
}


static void
lba_cogl_window_request_redraw (BaseWindow * base)
{
  LbaCoglWindow *self = (LbaCoglWindow *) base;

  self->is_dirty = TRUE;
  lba_cogl_window_maybe_redraw (self);
}


static void
lba_cogl_window_dirty_cb (CoglOnscreen * onscreen,
    const CoglOnscreenDirtyInfo * info, void *user_data)
{
  lba_cogl_window_request_redraw ((BaseWindow *) user_data);
}

/* Fixme: defaults are not set */
static void
lba_cogl_window_open (BaseWindow * base)
{
  LbaCoglWindow *self = (LbaCoglWindow *) base;

  CoglOnscreen *onscreen;
  CoglError *error = NULL;
  GSource *cogl_source;

  self->redraw_idle = 0;
  self->is_dirty = FALSE;
  self->draw_ready = TRUE;

  self->ctx = cogl_context_new (NULL, &error);
  if (!self->ctx) {
    LBA_LOG ("Failed to create context: %s\n", error->message);
    return;
  }

  onscreen = cogl_onscreen_new (self->ctx, base->width, base->height);

  cogl_onscreen_show (onscreen);
  self->fb = onscreen;

  cogl_onscreen_set_resizable (onscreen, TRUE);

  self->pipeline = cogl_pipeline_new (self->ctx);

  cogl_source = cogl_glib_source_new (self->ctx, G_PRIORITY_DEFAULT);

  g_source_attach (cogl_source, NULL);

  cogl_onscreen_add_frame_callback (self->fb, lba_cogl_window_frame_event_cb,
      self, NULL);
  cogl_onscreen_add_dirty_callback (self->fb, lba_cogl_window_dirty_cb, self,
      NULL);

  /* TODO/base: x_pos, y_pos, title */
}


static void
lba_cogl_window_init (LbaCoglWindow * self)
{
}

/* =================== CLASS */
static void
lba_cogl_window_class_init (LbaCoglWindowClass * klass)
{
  BaseWindowClass *base_class = BASE_WINDOW_CLASS (klass);

  base_class->open = lba_cogl_window_open;
  base_class->close = lba_cogl_window_close;
  base_class->request_redraw = lba_cogl_window_request_redraw;
}


G_DEFINE_TYPE (LbaCoglWindow, lba_cogl_window, G_TYPE_BASE_WINDOW)

/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_cogl_window);
