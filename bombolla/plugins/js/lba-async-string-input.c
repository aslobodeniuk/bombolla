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

#include <bmixin/bmixin.h>
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

typedef struct _LbaAsyncStringInput {
  BMixinInstance i;

  const gchar *input_string;

  /* FIXME: will belong to the mixin Async */
  GMutex lock;
  GCond cond;
  GSource *async_ctx;
} LbaAsyncStringInput;

typedef struct _LbaAsyncStringInputClass {
  BMixinClass c;
  void (*input_string) (GObject *, const gchar *);
} LbaAsyncStringInputClass;

BM_DEFINE_MIXIN (lba_async_string_input, LbaAsyncStringInput);

enum {
  SIGNAL_INPUT_STRING,
  SIGNAL_HAVE_STRING,
  LAST_SIGNAL
};

static guint lba_async_string_input_signals[LAST_SIGNAL] = { 0 };

static void
lba_async_string_input_init (GObject * object, LbaAsyncStringInput * mixin) {
}

static void
LbaAsync_async_cmd_free (gpointer gobject) {
  LbaAsyncStringInput *self = bm_get_LbaAsyncStringInput (gobject);

  g_mutex_lock (&self->lock);
  g_source_unref (self->async_ctx);
  self->async_ctx = NULL;
  g_cond_broadcast (&self->cond);
  g_mutex_unlock (&self->lock);
}

static void
LbaAsync_call_through_main_loop (LbaAsyncStringInput * self, GSourceFunc cmd,
                                 gpointer data) {
  g_warn_if_fail (self->async_ctx == NULL);

  self->async_ctx = g_idle_source_new ();
  g_source_set_priority (self->async_ctx, G_PRIORITY_HIGH);
  g_source_set_callback (self->async_ctx, cmd, data, LbaAsync_async_cmd_free);

  /* Attach the source and wait for it to finish */
  g_mutex_lock (&self->lock);
  g_source_attach (self->async_ctx, NULL);
  g_cond_wait (&self->cond, &self->lock);
  g_mutex_unlock (&self->lock);
}

static gboolean
lba_async_string_input_have_str (gpointer gobject) {
  LbaAsyncStringInput *self;

  self = bm_get_LbaAsyncStringInput (gobject);
  g_return_val_if_fail (self->input_string, G_SOURCE_REMOVE);

  LBA_LOG ("Emitting through the GMainContext: [%s]", self->input_string);

  g_signal_emit (gobject,
                 lba_async_string_input_signals[SIGNAL_HAVE_STRING],
                 0, self->input_string);

  self->input_string = NULL;
  return G_SOURCE_REMOVE;
}

static void
lba_async_string_input_input_string (GObject * gobject, const gchar * input) {
  LbaAsyncStringInput *self;

  LBA_LOG ("String input: [%s]", input);

  self = bm_get_LbaAsyncStringInput (gobject);
  // FIXME: lock, otherwise someone can overwrite self->input_string
  g_warn_if_fail (self->input_string == NULL);
  self->input_string = input;

  /* FIXME: will belong to the iAsync interface of the Async mixin */
  LbaAsync_call_through_main_loop (self, lba_async_string_input_have_str, gobject);
}

static void
lba_async_string_input_class_init (GObjectClass * object_class,
                                   LbaAsyncStringInputClass * bm_class) {

  bm_class->input_string = lba_async_string_input_input_string;

  lba_async_string_input_signals[SIGNAL_INPUT_STRING] =
      g_signal_new ("input-string", G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    BM_CLASS_VFUNC_OFFSET (bm_class, input_string),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__STRING,
                    G_TYPE_NONE, 1, G_TYPE_STRING, G_TYPE_NONE);

  lba_async_string_input_signals[SIGNAL_HAVE_STRING] =
      g_signal_new ("have-string", G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_LAST, 0,
                    NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING, G_TYPE_NONE);
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_async_string_input);
