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

typedef struct {
  gchar *expr;
  GObject *self;
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
  BombollaOnCommandCtx *ctx;

  /* Execute stored commands */
  LBA_LOG ("on something of %d parameters", n_param_values);

  ctx = closure->data;

  /* Now execute  */
  g_signal_emit_by_name (ctx->self, "execute", ctx->expr);
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

  g_free (ctx->expr);
  g_free (ctx);
}

static void
lba_command_on (GObject *core, GObject *obj, const char *signal, const char *expr) {
  GClosure *closure;
  BombollaOnCommandCtx *on_ctx;

  g_return_if_fail (obj != NULL);
  g_return_if_fail (signal != NULL);
  g_return_if_fail (expr != NULL);

  LBA_LOG ("Hello from on command (%s.%s --> [%s])", G_OBJECT_TYPE_NAME (obj),
           signal, expr);

  on_ctx = g_new0 (BombollaOnCommandCtx, 1);
  /* ref ?? */
  on_ctx->self = core;
  on_ctx->expr = g_strdup (expr);

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
  g_signal_connect_closure (obj, signal, closure,
                            /* execute after default handler = TRUE */
                            TRUE);

}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_COMMAND (on, LBA_COMMAND_SETUP_DEFAULT,
                                        G_TYPE_OBJECT, G_TYPE_STRING, G_TYPE_STRING);
