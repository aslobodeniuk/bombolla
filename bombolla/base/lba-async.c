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
#include "bombolla/base/i2d.h"

typedef struct _LbaAsync {
  /* TODO: move to GMOMutogene struct */
  GObject *mami;
  /* -------------------------------- */

  GMutex lock;
  GCond cond;
  GSource *async_ctx;
} LbaAsync;

typedef struct _LbaAsyncClass {
} LbaAsyncClass;

/* TODO: this mixin should always go as the last in the inheiritance tree.
 * We should add some kind of flag to it's type that would provide that feature. */
GMO_DEFINE_MUTOGENE (lba_async, LbaAsync);

static void
lba_async_cmd_free (gpointer gobject) {
  LbaAsync *self = (LbaAsync *) gobject;

  g_mutex_lock (&self->lock);
  g_source_unref (self->async_ctx);
  self->async_ctx = NULL;
  g_cond_broadcast (&self->cond);
  g_mutex_unlock (&self->lock);
}

static void
lba_async_call_through_main_loop (LbaAsync * self, GSourceFunc cmd, gpointer data) {
  g_warn_if_fail (self->async_ctx == NULL);

  if (g_main_context_is_owner (NULL)) {
    LBA_LOG ("Already in the MainContext. Calling synchronously");
    cmd (data);
  } else {
    self->async_ctx = g_idle_source_new ();
    g_source_set_priority (self->async_ctx, G_PRIORITY_HIGH);
    g_source_set_callback (self->async_ctx, cmd, data, lba_async_cmd_free);

    /* Attach the source and wait for it to finish */
    g_mutex_lock (&self->lock);
    g_source_attach (self->async_ctx, NULL);
    g_cond_wait (&self->cond, &self->lock);
    g_mutex_unlock (&self->lock);
  }
}

static gboolean
lba_async_dispose_cmd (gpointer ptr) {
  LbaAsync *self = (LbaAsync *) ptr;

  GMO_CHAINUP (self->mami, lba_async, GObject)->dispose (self->mami);
  return G_SOURCE_REMOVE;
}

static void
lba_async_dispose (GObject * gobject) {
  LbaAsync *self = gmo_get_LbaAsync (gobject);

  LBA_LOG ("Disposing [%s]", g_type_name (G_OBJECT_TYPE (gobject)));
  lba_async_call_through_main_loop (self, lba_async_dispose_cmd, self);
}

static gboolean
lba_async_finalize_cmd (gpointer ptr) {
  LbaAsync *self = (LbaAsync *) ptr;

  GMO_CHAINUP (self->mami, lba_async, GObject)->finalize (self->mami);
  return G_SOURCE_REMOVE;
}

static void
lba_async_finalize (GObject * gobject) {
  LbaAsync *self = gmo_get_LbaAsync (gobject);

  LBA_LOG ("Finalizing [%s]", g_type_name (G_OBJECT_TYPE (gobject)));
  lba_async_call_through_main_loop (self, lba_async_finalize_cmd, self);

  /* Now can clean our own stuff */
  g_mutex_clear (&self->lock);
  g_cond_clear (&self->cond);
  g_warn_if_fail (self->async_ctx == NULL);
}

static void
lba_async_class_init (GObjectClass * gobject_class, LbaAsyncClass * self_class) {

  gobject_class->dispose = lba_async_dispose;
  gobject_class->finalize = lba_async_finalize;
}

static void
lba_async_init (GObject * object, LbaAsync * self) {
  self->mami = object;

  g_mutex_init (&self->lock);
  g_cond_init (&self->cond);
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_async);
