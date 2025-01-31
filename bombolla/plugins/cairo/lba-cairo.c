/* la Bombolla GObject shell
 *
 * Copyright (c) 2025, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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

#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include "bombolla/base/lba-picture.h"
#include <cairo.h>

typedef struct _LbaCairo {
  BMixinInstance i;

  cairo_surface_t *surface;
} LbaCairo;

typedef struct _LbaCairoClass {
  BMixinClass c;
} LbaCairoClass;

BM_DEFINE_MIXIN (lba_cairo, LbaCairo, BM_ADD_DEP (lba_picture));

static void
lba_cairo_init (GObject *obj, LbaCairo *self) {

  const char fmt[16] = "argb8888";
  guint w = 512;
  guint h = 512;

  /* Cairo */
  self->surface =
      /* todo fmt lba <--> cairo */
      cairo_image_surface_create (CAIRO_FORMAT_ARGB32, h, w);

  {
    cairo_t *cr = cairo_create (self->surface);

    /* --------------------------------------- how to?? */
    cairo_select_font_face (cr, "serif", CAIRO_FONT_SLANT_NORMAL,
                            CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, 32.0);
    cairo_set_source_rgb (cr, 0.0, 0.0, 1.0);
    cairo_move_to (cr, 10.0, 50.0);
    cairo_show_text (cr, "Hello from Cairo!!");

    cairo_rectangle (cr, 10.0, 70.0, 20.0, 90.0);
    cairo_fill (cr);
    /* ---------------------------------------- */

    cairo_destroy (cr);

    /* Must do after modifying the surface */
    cairo_surface_flush (self->surface);
    cairo_surface_mark_dirty (self->surface);
  }

  lba_picture_set (obj, fmt, w, h,
                   g_bytes_new_with_free_func (cairo_image_surface_get_data
                                               (self->surface), w * h * 4,
                                               /* TODO need to block and unblock when the surface have been rendered
                                                * (in the OpenGL thread). So we don't modify the surface meanwhile. */
                                               NULL, NULL));
}

static void
lba_cairo_dispose (GObject *gobject) {
  LbaCairo *self = bm_get_LbaCairo (gobject);

  g_clear_pointer (&self->surface, cairo_surface_destroy);

  BM_CHAINUP (self, GObject)->dispose (gobject);
}

static void
lba_cairo_class_init (GObjectClass *gobj_class, LbaCairoClass *klass) {
  gobj_class->dispose = lba_cairo_dispose;
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_cairo);
