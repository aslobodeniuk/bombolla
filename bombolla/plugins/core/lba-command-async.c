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
