/* la Bombolla GObject shell.
 * Copyright (C) 2021 Aleksandr Slobodeniuk
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

#include "bombolla/lba-log.h"
#include "bombolla/core/bombolla-commands.h"

typedef struct {
  GSList *commands_list;
  GObject *self;
    gboolean (*proccess_command) (GObject * self, const gchar * command);
} BombollaOnCommandCtx;

/* Callback for "on" command. It's designed to use parameters
 * of the signal, but for now we just ignore them.
 * So, it executes the commands, stored for that "on" instance. */
static void
lba_command_on_cb (GClosure * closure,
                   GValue * return_value,
                   guint n_param_values,
                   const GValue * param_values,
                   gpointer invocation_hint, gpointer marshal_data) {
  gpointer user_data;
  GSList *l;
  BombollaOnCommandCtx *ctx;

  /* Execute stored commands */
  LBA_LOG ("on something of %d parameters\n", n_param_values);

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
lba_command_on_marshal (GClosure * closure,
                        GValue * return_value,
                        guint n_param_values,
                        const GValue * param_values,
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
lba_command_on_finish (gpointer data, GClosure * closure) {
  BombollaOnCommandCtx *ctx = data;

  /* unref ctx->self */
  g_slist_free_full (ctx->commands_list, g_free);
  g_free (ctx);
}

gboolean
lba_command_on_append (gpointer ctx_ptr, const gchar * command) {
  BombollaOnCommandCtx *ctx = ctx_ptr;

  if (g_str_has_prefix (command, "end")) {
    return FALSE;
  }

  ctx->commands_list = g_slist_append (ctx->commands_list, g_strdup (command));
  return TRUE;
}

gboolean
lba_command_on (BombollaContext * ctx, gchar ** tokens) {
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

    g_printf ("on %s.%s () ...\n", objname, signal_name);

    on_ctx = g_new0 (BombollaOnCommandCtx, 1);
    ctx->capturing_on_command = on_ctx;

    /* ref ?? */
    on_ctx->self = ctx->self;
    on_ctx->proccess_command = ctx->proccess_command;

    /* User data are the lines we will execute. */
    closure =
        g_cclosure_new (G_CALLBACK (lba_command_on_cb), on_ctx,
                        lba_command_on_finish);

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
  ret = TRUE;
done:
  if (tmp) {
    g_strfreev (tmp);
  }
  return ret;
}
