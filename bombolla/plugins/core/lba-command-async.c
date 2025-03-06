/* la Bombolla GObject shell.
 * Copyright (C) 2025 Aleksandr Slobodeniuk
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

#include <bombolla/lba-log.h>
#include <bmixin/bmixin.h>

typedef struct _LbaCoreAsyncCtx {
  GList *async_cmds;
} LbaCoreAsyncCtx;

/*
  g_mutex_clear (&self->async_cmd_guard);
*/

typedef struct _LbaCoreAsyncCmd {
  gchar *command;
  GSource *source;

  GMutex lock;
  GCond cond;
  gboolean done;
} LbaCoreAsyncCmd;

static void
lba_command_async_cmd_free (gpointer data) {
  LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) data;

  g_free (ctx->command);
  g_source_unref (ctx->source);
  g_mutex_clear (&ctx->lock);
  g_cond_clear (&ctx->cond);
  g_free (ctx);
}

static void
lba_command_async_cmd_done (gpointer data) {
  LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) data;

  g_mutex_lock (&ctx->lock);
  ctx->done = TRUE;
  g_cond_broadcast (&ctx->cond);
  g_mutex_unlock (&ctx->lock);

  /* Now remove from the async list */
}

static gboolean
lba_command_async_cmd (gpointer data) {
  LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) data;

  lba_command_execute (BM_GET_GOBJECT (ctx->core), ctx->command);

  return G_SOURCE_REMOVE;
}

#define MAGIC "lba-core-async-commands"

static void
lba_command_async_await_all (gpointer ptr) {

}

static void
lba_command_async_add_context (GObject *core, LbaCoreAsyncCmd *ctx) {
  GList *l = g_object_get_data (core, MAGIC);

  if (G_UNLIKELY (l == NULL)) {
    g_object_set_data (core, MAGIC, l, lba_command_async_await_all);
  }

  /* FIXME */
  g_list_append (l, ctx);
}

static void
lba_command_async (GObject *core, const gchar *command) {
  LbaCoreAsyncCmd *ctx = g_new0 (LbaCoreAsyncCmd, 1);

  LBA_LOG ("Shedulling command [%s] for async execution", command);

  ctx->command = command;
  ctx->source = g_idle_source_new ();
  g_mutex_init (&ctx->lock);
  g_cond_init (&ctx->cond);

  g_source_set_priority (ctx->source, G_PRIORITY_DEFAULT);

  g_source_set_callback (ctx->source, lba_command_async_cmd, ctx,
                         lba_command_async_cmd_done);

  lba_command_async_add_context (core, ctx);
  g_source_attach (ctx->source, NULL);
}

/* FIXME: move to plugin. This context self->async_cmds can be
 * attached. This is way more clear. */
static void
lba_command_sync (GObject *gobj) {
  LbaCore *self = bm_get_LbaCore (gobj);
  GList *it;

  /* To sync we do:
   * 1. copy a snap of the list of the async commands.
   * 2. iterate on this snap waiting for each. */

  /* FIXME:
   * Might be better to just send a new empty GSource and wait for it??
   * It looses few CPU cycles, but saves a lot of code lines.
   */
  g_mutex_lock (&self->async_cmd_guard);
  for (it = self->async_cmds; it != NULL; it = it->next) {
    LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) it->data;

    g_mutex_lock (&ctx->lock);
    while (!ctx->done) {
      g_cond_wait (&ctx->cond, &ctx->lock);
    }
    g_mutex_unlock (&ctx->lock);
  }
  g_list_free_full (self->async_cmds, lba_core_async_cmd_free);
  self->async_cmds = NULL;
  g_mutex_unlock (&self->async_cmd_guard);
}

/* FIXME: add 2 commands: sync and async */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_COMMAND (async, LBA_COMMAND_SETUP_DEFAULT,
                                        G_TYPE_STRING);
