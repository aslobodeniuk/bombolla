/* la Bombolla GObject shell.
 *
 * Copyright (C) 2023 Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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

#include <gmo/gmo.h>
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

typedef struct _LbaAsyncStringInput {

  const gchar *input_string;

  /* FIXME: will belong to the mutogene Async */
  GMutex lock;
  GCond cond;
  GSource *async_ctx;
} LbaAsyncStringInput;

typedef struct _LbaAsyncStringInputClass {
  void (*input_string) (GObject *, const gchar *);
} LbaAsyncStringInputClass;

GMO_DEFINE_MUTOGENE (lba_async_string_input, LbaAsyncStringInput);

enum {
  SIGNAL_INPUT_STRING,
  SIGNAL_HAVE_STRING,
  LAST_SIGNAL
};

static guint lba_async_string_input_signals[LAST_SIGNAL] = { 0 };

static void
lba_async_string_input_init (GObject * object, gpointer mutogene) {
}

static void
LbaAsync_async_cmd_free (gpointer gobject) {
  LbaAsyncStringInput *self = gmo_get_LbaAsyncStringInput (gobject);

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

  self = gmo_get_LbaAsyncStringInput (gobject);
  g_return_val_if_fail (self->input_string, G_SOURCE_REMOVE);

  LBA_LOG ("Emitting through the GMainContext: [%s]", self->input_string);

  g_signal_emit (gobject,
                 lba_async_string_input_signals[SIGNAL_HAVE_STRING],
                 0, self->input_string);

  self->input_string = NULL;
  return G_SOURCE_REMOVE;
}

void
lba_async_string_input_input_string (GObject * gobject, const gchar * input) {
  LbaAsyncStringInput *self;

  LBA_LOG ("String input: [%s]", input);

  self = gmo_get_LbaAsyncStringInput (gobject);
  // FIXME: lock, otherwise someone can overwrite self->input_string
  g_warn_if_fail (self->input_string == NULL);
  self->input_string = input;

  /* FIXME: will belong to the iAsync interface of the Async mutogene */
  LbaAsync_call_through_main_loop (self, lba_async_string_input_have_str, gobject);
}

static void
lba_async_string_input_class_init (GObjectClass * object_class, gpointer gmo_class) {

  LbaAsyncStringInputClass *klass = (LbaAsyncStringInputClass *) gmo_class;

  klass->input_string = lba_async_string_input_input_string;

  lba_async_string_input_signals[SIGNAL_INPUT_STRING] =
      g_signal_new ("input-string", G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    /* FIXME: resolve in gmo.h. We install signal to the object_class.
                     * GMO_CLASS_OFFSET() ?? */
                    (gmo_class - (gpointer) object_class) +
                    G_STRUCT_OFFSET (LbaAsyncStringInputClass, input_string),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__STRING,
                    G_TYPE_NONE, 1, G_TYPE_STRING, G_TYPE_NONE);

  lba_async_string_input_signals[SIGNAL_HAVE_STRING] =
      g_signal_new ("have-string", G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_LAST, 0,
                    NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING, G_TYPE_NONE);
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_async_string_input);
