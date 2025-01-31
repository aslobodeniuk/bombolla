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
#include "lba-commands.h"

/* HACK: Needed to use LBA_LOG */
static const gchar *global_lba_plugin_name = "LbaCore";

typedef struct {
  GSList *commands_list;
  GObject *self;
    gboolean (*proccess_command) (gpointer self, const gchar * command);
} BombollaOnCommandCtx;

/* Callback for "on" command. It's designed to use parameters
 * of the signal, but for now we just ignore them.
 * So, it executes the commands, stored for that "on" instance. */
static void
lba_command_on_cb (GClosure *closure,
                   GValue *return_value,
                   guint n_param_values,
                   const GValue *param_values,
                   gpointer invocation_hint, gpointer marshal_data) {
  gpointer user_data;
  GSList *l;
  BombollaOnCommandCtx *ctx;

  /* Execute stored commands */
  LBA_LOG ("on something of %d parameters", n_param_values);

  /* Closure data is our pointer to the list */
  user_data = closure->data;

  /* take user_data param, and execute it as a list of strings */
  ctx = user_data;

  /* Now execute line by line everything */
  for (l = ctx->commands_list; l; l = l->next) {
    ctx->proccess_command (ctx->self, (const gchar *)l->data);
  }
}

static void
lba_command_on_marshal (GClosure *closure,
                        GValue *return_value,
                        guint n_param_values,
                        const GValue *param_values,
                        gpointer invocation_hint, gpointer marshal_data) {
  /* this is our "passthrough" marshaller. We could also just omit
   * the closure's callback and do what we want here, given that it's passthrough, but
   * that is a hack */

  GClosureMarshal callback;
  GCClosure *cc = (GCClosure *) closure;

  callback = (GClosureMarshal) (marshal_data ? marshal_data : cc->callback);

  callback (closure,
            return_value,
            n_param_values, param_values, invocation_hint, marshal_data);
}

static void
lba_command_on_destroy (gpointer data, GClosure *closure) {
  BombollaOnCommandCtx *ctx = data;

  /* unref ctx->self */
  g_slist_free_full (ctx->commands_list, g_free);
  g_free (ctx);
}

gboolean
lba_command_on_append (gpointer ctx_ptr, const gchar *command) {
  BombollaOnCommandCtx *ctx = ctx_ptr;

  if (0 == g_strcmp0 (command, "end")) {
    g_printf ("}\n");
    return FALSE;
  }

  if (g_str_has_prefix (command, "on ")) {
    g_warning ("setting 'on' while defining another is not allowed");
    return TRUE;
  }

  ctx->commands_list = g_slist_append (ctx->commands_list, g_strdup (command));
  return TRUE;
}

gboolean
lba_command_on (BombollaContext *ctx, gchar **tokens) {
  const gchar *objname,
   *signal_name;
  GObject *obj;
  char **tmp = NULL;
  gboolean ret = FALSE;

  tmp = g_strsplit (tokens[1], ".", 2);

  if (FALSE == (tmp && tmp[0] && tmp[1])) {
    g_warning ("wrong syntax");
    goto done;
  }

  if (ctx->capturing_on_command) {
    g_warning ("forbidden to call on from on");
    goto done;
  }

  objname = tmp[0];
  signal_name = tmp[1];

  obj = g_hash_table_lookup (ctx->objects, objname);

  if (!obj) {
    g_warning ("object %s not found", objname);
    goto done;
  }

  {
    GClosure *closure;
    BombollaOnCommandCtx *on_ctx;

    on_ctx = g_new0 (BombollaOnCommandCtx, 1);
    ctx->capturing_on_command = on_ctx;

    /* ref ?? */
    on_ctx->self = ctx->self;
    on_ctx->proccess_command = ctx->proccess_command;

    /* User data are the lines we will execute. */
    closure =
        g_cclosure_new (G_CALLBACK (lba_command_on_cb), on_ctx,
                        lba_command_on_destroy);

    /* Set our custom passthrough marshaller */
    g_closure_set_marshal (closure, lba_command_on_marshal);
    /* memory management: alive while object is alive,
     * invalidated if object is freed */
    g_object_watch_closure (obj, closure);

    /* Connect our super closure. */
    g_signal_connect_closure (obj, signal_name, closure,
                              /* execute after default handler = TRUE */
                              TRUE);

    /* unref closure ?? */
  }

  /* All the next commands until "end" will be stored as is in
   * ctx->on_commands_list */
  g_printf ("On %s.%s () {\n", objname, signal_name);
  ret = TRUE;
done:
  if (tmp) {
    g_strfreev (tmp);
  }
  return ret;
}
