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
  void (*proccess_command) (gpointer self, const gchar * expr, guint len);
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
    ctx->proccess_command (ctx->self, (const gchar *)l->data,
                           strlen ((const gchar *)l->data));
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

static void
lba_command_on_append (gpointer ctx_ptr, const gchar *command, guint len) {
  BombollaOnCommandCtx *ctx = ctx_ptr;

  /* FIXME: */
  if (g_str_has_prefix (command, "on ")) {
    g_error ("setting 'on' while defining another is not supported for now");
  }

  ctx->commands_list = g_slist_append (ctx->commands_list, g_strndup (command, len));
}

gboolean
lba_command_on (BombollaContext *ctx, const gchar *expr, guint len) {
  gchar *objname,
   *signal_name;
  GObject *obj;
  char **tmp = NULL;
  gboolean ret = FALSE;
  gchar **tokens = FIXME_adapt_to_old (expr, len);

  tmp = g_strsplit (tokens[1], ".", 2);

  if (FALSE == (tmp && tmp[0] && tmp[1])) {
    g_warning ("wrong syntax");
    goto done;
  }

  objname = tmp[0];
  signal_name = tmp[1];
  g_assert (signal_name[0]);
  if (signal_name[strlen (signal_name) - 1] == '\n')
    signal_name[strlen (signal_name) - 1] = 0;

  obj = g_hash_table_lookup (ctx->objects, objname);

  if (!obj) {
    g_warning ("object %s not found", objname);
    goto done;
  }

  {
    GClosure *closure;
    BombollaOnCommandCtx *on_ctx;

    on_ctx = g_new0 (BombollaOnCommandCtx, 1);
    /* ref ?? */
    on_ctx->self = ctx->self;
    on_ctx->proccess_command = ctx->proccess_command;

    {
      guint exprlen;
      const gchar *e;
      guint total = len;

      for (e = expr;;) {
        const gchar *p = e;

        e = lba_expr_parser_find_next (e, total, &exprlen);
        if (!e)
          break;

        //LBA_LOG
        g_message ("ON: Append expression [%.*s]", exprlen, e);
        lba_command_on_append (ctx->self, e, exprlen);

        g_assert (len <= total);
        e += exprlen + 1;
        total -= (e - p);
      }
    }

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

  ret = TRUE;
done:
  if (tmp) {
    g_strfreev (tmp);
  }
  g_strfreev (tokens);
  return ret;
}
