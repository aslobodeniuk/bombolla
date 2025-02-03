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

#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include "bombolla/base/lba-module-scanner.h"
#include "lba-expr-parser.h"
#include <bmixin/bmixin.h>
#include <glib/gstdio.h>
#include <gmodule.h>
#include <string.h>
#include <stdlib.h>

/* DEPRECATED: */
#include "commands/lba-commands.h"
/* ------------- */

enum {
  SIGNAL_EXECUTE,
  SIGNAL_ADD,
  SIGNAL_PICK,
  LAST_SIGNAL
};

static guint lba_core_signals[LAST_SIGNAL] = { 0 };

typedef struct _LbaCore {
  BMixinInstance i;
  BombollaContext *ctx;

  gboolean started;
  GThread *mainloop_thr;
  GMainContext *mainctx;
  GMainLoop *mainloop;
  GMutex lock;
  GCond cond;

  GMutex async_cmd_guard;
  GList *async_cmds;
} LbaCore;

typedef struct _LbaCoreClass {
  BMixinClass c;
  void (*execute) (GObject *, const gchar *);
  void (*add) (GObject *, GObject *, const gchar *);
  GObject *(*pick) (GObject *, const gchar *);
} LbaCoreClass;

BM_DEFINE_MIXIN (lba_core, LbaCore, BM_ADD_DEP (lba_module_scanner));

/* HACK: Needed to use LBA_LOG */
static const gchar *global_lba_plugin_name = "LbaCore";

static gpointer
lba_core_mainloop (gpointer data) {
  LbaCore *self = (LbaCore *) data;

  LBA_LOG ("starting the mainloop");

  /* FIXME: for the beginning we use default context */
//  self->mainctx = g_main_context_new ();
//  g_main_context_acquire (self->mainctx);

  /* Proccessing events here until quit event arrives */
  g_main_loop_run (self->mainloop);
  self->started = FALSE;
  LBA_LOG ("mainloop stopped");
  return NULL;
}

/* Callback proccessed in the main loop.
 * Simply makes it quit. */
gboolean
lba_core_quit_msg (gpointer data) {
  LbaCore *self = (LbaCore *) data;

  g_main_loop_quit (self->mainloop);
  return G_SOURCE_REMOVE;
}

static void
lba_core_stop (LbaCore *self) {
  GSource *s;

  /* Send quit message to the main loop */
  s = g_idle_source_new ();
  g_source_set_priority (s, G_PRIORITY_HIGH);

  g_source_set_callback (s, lba_core_quit_msg, self, NULL);
  g_source_attach (s, NULL);

  /* Wait for main loop to quit */
  g_thread_join (self->mainloop_thr);
  self->mainloop_thr = NULL;

  g_source_destroy (s);
  g_source_unref (s);

  g_main_loop_unref (self->mainloop);
  self->mainloop = NULL;
  self->started = FALSE;
}

static void
lba_core_init (GObject *object, LbaCore *self) {
  g_mutex_init (&self->async_cmd_guard);
  g_mutex_init (&self->lock);
  g_cond_init (&self->cond);

  if (!self->started) {
    /* Start the main loop */
    g_mutex_lock (&self->lock);
    /* FIXME: custom MainContext would be nice */
    self->mainloop = g_main_loop_new (NULL, TRUE);
    self->mainloop_thr = g_thread_new ("LbaCoreMainLoop", lba_core_mainloop, self);
    while (!g_main_loop_is_running (self->mainloop))
      g_usleep (1);             // FIXME: wait properly
    /* Done */
    self->started = TRUE;
    g_mutex_unlock (&self->lock);
  }
}

void lba_core_sync_with_async_cmds (gpointer core);

static void
lba_core_dispose (GObject *gobject) {
  LbaCore *self = bm_get_LbaCore (gobject);

  if (self->async_cmds) {
    lba_core_sync_with_async_cmds (self);
  }

  if (self->ctx) {
    if (self->ctx->bindings) {
      // The bindings actually belong to the object, and are
      // automatically destroyed when the objects are destroyed
      g_hash_table_unref (self->ctx->bindings);
    }
    if (self->ctx->objects) {
      g_hash_table_remove_all (self->ctx->objects);
      g_hash_table_unref (self->ctx->objects);
    }

    g_free (self->ctx);
    self->ctx = NULL;
  }

  if (self->started) {
    lba_core_stop (self);
  }

  LBA_LOG ("Stopped. Disposing");
  BM_CHAINUP (self, GObject)->dispose (gobject);
}

static void
lba_core_finalize (GObject *gobject) {
  LbaCore *self = bm_get_LbaCore (gobject);

  LBA_LOG ("Finalize");

  g_mutex_clear (&self->async_cmd_guard);
  g_mutex_clear (&self->lock);
  g_cond_clear (&self->cond);

  BM_CHAINUP (self, GObject)->finalize (gobject);
}

static void
lba_core_load_module (GObject *gobj, const gchar *module_filename) {
  GModule *module = NULL;
  gpointer ptr;
  lBaPluginSystemGetGtypeFunc get_type_f;
  GType plugin_gtype;

  g_return_if_fail (module_filename);

  module = g_module_open (module_filename, G_MODULE_BIND_LOCAL);
  if (!module) {
    LBA_LOG ("Failed to load plugin '%s': %s", module_filename, g_module_error ());
    return;
  }

  if (!g_module_symbol (module, BOMBOLLA_PLUGIN_SYSTEM_ENTRY, &ptr)) {
    LBA_LOG ("File '%s' is not a bombolla plugin", module_filename);
    g_module_close (module);
    return;
  }

  /* If module is from plugin system - we don't want to unload it */
  g_module_make_resident (module);

  get_type_f = (lBaPluginSystemGetGtypeFunc) ptr;
  plugin_gtype = get_type_f ();

  /* This function does nothing important, only prints everything
   * it can about the GType it has. It could output something like a
   * dot graph actually, or so. */
  LBA_LOG ("Found plugin: type = [%s] file = [%s]", g_type_name (plugin_gtype),
           module_filename);
}

GType lba_core_object_get_type (void);

static void
DEPRECATED_lba_core_eval_expression (gpointer selv, const char *expr, guint len) {
  LbaCore *self = (LbaCore *) selv;
  const BombollaCommand *command;

  guint exprlen = 0;

  expr = DEPRECATED_lba_expr_parser_find_next (expr, len, &exprlen);
  if (G_UNLIKELY (expr == NULL)) {
    LBA_LOG ("Empty expression");
    /* Just a sanity check */
    g_assert (exprlen == 0);
    return;
  }

  /* So now find the command: */
  for (command = commands; command->name != NULL; command++) {
    if (g_str_has_prefix (expr, command->name)) {
      if (!command->parse (self->ctx, expr, len)) {
        g_error ("Command parse error");
      }

      return;
    }
  }

  g_error ("Unknown command in the beginning of [%s]", expr);
}

static void
lba_expr_node_action_ref (GNode *node, gpointer p) {
  LbaExprNode *parent = (LbaExprNode *) node->parent->data;

  /* NOTE: this seems to be always executed sinchronously, while
   * we are just building the action chain. So probably it's impossible
   * to collide with other happening on '&en->actionrefs'. However JUST IN CASE
   * we increase here atomically too. */
  g_atomic_int_inc (&parent->actionrefs);
}

static void lba_expr_node_action_unref (GNode * node, LbaCore * self);

static void
lba_expr_node_action (gpointer data, LbaCore *self) {
  int p;
  guint signal_id;
  GSignalQuery query;
  LbaExprNode *en = (LbaExprNode *) data;
  LbaExprNode *cen;

  LBA_LOG ("Actioning [%s]", en->str);
  /* Ok, so we are actioning a node, all of which children have
   * GValue ready. We know it has children.
   *
   * So now we check the first child: it has to be a command.
   */
  cen = (LbaExprNode *) g_node_first_child (en->node)->data;
  LBA_LOG ("Checking command [%s]", cen->str);

  signal_id = g_signal_lookup (cen->str, lba_core_object_get_type ());
  if (!signal_id) {
    LBA_LOG ("No command '%s', using FIXME ones", cen->str);
    /* Expression already has no quotes */
    DEPRECATED_lba_core_eval_expression (self, en->str, strlen (en->str));
    goto unref;
  }

  g_signal_query (signal_id, &query);

  if (G_TYPE_NONE != query.return_type)
    g_value_init (&en->value, query.return_type);

  /* Prepare array of gvalues to exec the signal */
  GValue instance_and_params[64] = { 0 };
  g_assert (query.n_params + 1 < G_N_ELEMENTS (instance_and_params));

  /* Set LbaCore as a first parameter */
  //  instance_and_params[0] = G_VALUE_INIT;
  g_value_init (&instance_and_params[0], lba_core_object_get_type ());
  g_value_set_object (&instance_and_params[0], BM_GET_GOBJECT (self));

  /* Must be exactly action + params */
  g_assert (g_node_n_children (en->node) == query.n_params + 1);

  /* So we already have our GValues of the children ready. Now the question
   * is if we can transform them to the values of GTypes of the signal params */
  for (p = 0; p < query.n_params; p++) {
    GValue *dst = &instance_and_params[p + 1];

    cen = cen->node->next->data;

    /* Where should these lines be? */
    /* Maybe when we unref ?? So unref == value is ready */
    /* ------------------------------- */
    if (cen->type == LBA_EXPR_NODE_IS_STONE || cen->type == LBA_EXPR_NODE_IS_STRING) {
      g_value_init (&cen->value, G_TYPE_STRING);
      g_value_set_string (&cen->value, cen->str);
    }
    /* ------------------------------ */

    LBA_LOG ("Param[%d] (%s) will be set from <-- (%s)", p,
             g_type_name (query.param_types[p]), G_VALUE_TYPE_NAME (&cen->value));

    if (G_VALUE_TYPE (&cen->value) == G_TYPE_STRING
        && query.param_types[p] == G_TYPE_OBJECT) {
      g_value_init (dst, G_TYPE_OBJECT);
      g_value_set_object (dst,
                          g_hash_table_lookup (self->ctx->objects,
                                               g_value_get_string (&cen->value)));
    } else {
      g_assert (TRUE == g_value_type_transformable (G_VALUE_TYPE (&cen->value),
                                                    query.param_types[p]));

      g_value_init (dst, query.param_types[p]);
      /* Finally try to transform */
      g_assert (TRUE == g_value_transform (&cen->value, dst));
    }
  }

  /* Now we can happily emit the signal */
  LBA_LOG ("Executing the signal %s of %d params", query.signal_name,
           query.n_params);
  g_signal_emitv (instance_and_params, signal_id, 0, &en->value);
  /* Now release the values */
  for (p = 0; p < query.n_params + 1; p++)
    g_value_unset (&instance_and_params[p]);

unref:
  /* And now the tastiest thing:
   * Most likely our action is a child of another action that needs the
   * result of ours as a parameter. So we are going to "chainup" - decrease
   * the parent's actionref */
  if (en->node->parent->parent != NULL)
    lba_expr_node_action_unref (en->node, self);
}

static void
lba_expr_node_action_unref (GNode *node, LbaCore *self) {
  g_assert (node->parent != NULL);

  LbaExprNode *parent = (LbaExprNode *) node->parent->data;

  g_assert (parent->actionrefs > 0);

  /* This means all the components of the action are ready, so
   * we can trigger the action on the parent */
  if (g_atomic_int_dec_and_test (&parent->actionrefs))
    lba_expr_node_action (parent->node->data, self);
}

static void
lba_expr_node_ref_children (GNode *node, gpointer data) {
  G_GNUC_UNUSED LbaExprNode *en = (LbaExprNode *) node->data;

  /* Empty expressions are not supported */

  lba_expr_node_action_ref (node, NULL);
  g_node_children_foreach (node, G_TRAVERSE_ALL, lba_expr_node_ref_children, NULL);
}

static gboolean
lba_core_eval_expr_node (GNode *node, gpointer data) {
  LbaCore *self = (LbaCore *) data;
  LbaExprNode *en = (LbaExprNode *) node->data;

  /* So we are in the node. It might be either ((expression)), or something,
   * such as command (most likely) */

  /* If it's a command, capture parameters with node->next */
  /* TODO: If it has "." then it might be a property or a signal (find out) */

  switch (en->type) {
  case LBA_EXPR_NODE_IS_EXPR:
    /* Empty expressions are not supported */
    g_assert (node->children != NULL);

    break;

    /* Set refs to all the children (NOT recursively) */
    g_node_children_foreach (node, G_TRAVERSE_ALL, lba_expr_node_action_ref, NULL);

    break;
  case LBA_EXPR_NODE_IS_STONE:
    /* Eval to GValue and decrease the ref. Could it hot be referenced?
     * only it it doesn't have parent, that is handled too. */
    g_assert (node->children == NULL);
    lba_expr_node_action_unref (node, self);
    break;
  default:
    g_error ("Unexpected node type: %d", en->type);
  }

  return FALSE;
}

static void
lba_expr_root_list_expr (GNode *node, gpointer p) {
  LbaCore *self = (LbaCore *) p;
  LbaExprNode *en = (LbaExprNode *) node->data;

  LBA_LOG ("ROOT LIST element: [%s]", en->str);
  LBA_LOG ("Building refs");
  /* NOTE: we don't ref the ROOT, nor unref it, because there's nothing to action */
  g_node_children_foreach (node, G_TRAVERSE_ALL, lba_expr_node_ref_children, NULL);

  LBA_LOG ("Starting the unref chain");
  /* unreffing */
  g_node_traverse (node,
                   G_POST_ORDER, G_TRAVERSE_ALL, -1, lba_core_eval_expr_node, self);
}

static GObject *
lba_core_pick (GObject *gobject, const gchar *name) {
  LbaCore *self = bm_get_LbaCore (gobject);

  g_return_val_if_fail (name != NULL, NULL);

  return g_hash_table_lookup (self->ctx->objects, name);
}

static void
lba_core_add (GObject *gobject, GObject *newcomer, const gchar *name) {
  LbaCore *self = bm_get_LbaCore (gobject);

  g_return_if_fail (G_IS_OBJECT (newcomer));

  if (g_hash_table_lookup (self->ctx->objects, name)) {
    g_warning ("variable '%s' already exists", name);
    return;
  }

  if (g_object_is_floating (newcomer))
    g_object_ref_sink (newcomer);
  g_hash_table_insert (self->ctx->objects, (gpointer) g_strdup (name),
                       g_object_ref (newcomer));

  LBA_LOG ("Added '%s' of type '%s'", name, G_OBJECT_TYPE_NAME (newcomer));
}

static void
lba_core_execute (GObject *gobject, const gchar *commands) {
  LbaCore *self = bm_get_LbaCore (gobject);

  /* Proccessing commands */
  if (G_UNLIKELY (!commands))
    return;

  /* ================================= FIXMA: horrible */
  if (!self->ctx) {
    self->ctx = g_new0 (BombollaContext, 1);
    self->ctx->self = (GObject *) self;
    self->ctx->objects =
        g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

    // The bindings actually belong to the object, and are
    // automatically destroyed when the objects are destroyed.
    // The only point of storing them is the "unbind" command

    /* NOTE: we could have a list of bindings attached to each object */
    self->ctx->bindings =
        g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  }

  /* ========================================= */

  LBA_LOG ("Going to exec: [%s]", commands);

  /* Root and only the root node is handled differently, since it
   * actually IS a list of expressions:
   * -------------------------
   * (expr1 (bla (foo) (bar)))
   * (expr2)
   * (etc)
   * -------------------------
   * And we MUST execute them one after another.
   */
  LBA_LOG ("building the tree");
  GNode *tree = lba_expr_parser_sniff (LBA_EXPR_NODE_IS_LIST,
                                       commands, strlen (commands));

  LBA_LOG ("start executing (%d children)", g_node_n_children (tree));
  g_node_children_foreach (tree, G_TRAVERSE_ALL, lba_expr_root_list_expr, self);

  lba_expr_node_destroy (tree);
}

typedef struct _LbaCoreAsyncCmd {
  gchar *command;
  LbaCore *core;
  GSource *source;

  GMutex lock;
  GCond cond;
  gboolean done;
} LbaCoreAsyncCmd;

static void
lba_core_async_cmd_free (gpointer data) {
  LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) data;

  g_free (ctx->command);
  g_source_unref (ctx->source);
  g_mutex_clear (&ctx->lock);
  g_cond_clear (&ctx->cond);
  g_free (ctx);
}

static void
lba_core_async_cmd_done (gpointer data) {
  LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) data;

  g_mutex_lock (&ctx->lock);
  ctx->done = TRUE;
  g_cond_broadcast (&ctx->cond);
  g_mutex_unlock (&ctx->lock);
}

static gboolean
lba_core_async_cmd (gpointer data) {
  LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) data;

  lba_core_execute (BM_GET_GOBJECT (ctx->core), ctx->command);

  return G_SOURCE_REMOVE;
}

void
lba_core_sync_with_async_cmds (gpointer core) {
  LbaCore *self = (LbaCore *) core;
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

void
lba_core_shedule_async_script (GObject *obj, gchar *command) {
  LbaCore *self = (LbaCore *) obj;
  LbaCoreAsyncCmd *ctx = g_new0 (LbaCoreAsyncCmd, 1);

  LBA_LOG ("Shedulling command [%s] for async execution", command);

  ctx->core = self;
  ctx->command = command;
  ctx->source = g_idle_source_new ();
  g_mutex_init (&ctx->lock);
  g_cond_init (&ctx->cond);

  g_source_set_priority (ctx->source, G_PRIORITY_DEFAULT);

  g_source_set_callback (ctx->source, lba_core_async_cmd, ctx,
                         lba_core_async_cmd_done);

  self->async_cmds = g_list_append (self->async_cmds, ctx);
  g_source_attach (ctx->source, NULL);
}

G_LOCK_DEFINE_STATIC (singleton_lock);
static GObject *singleton_object;

static void
lba_core_reset_singleton (gpointer data, GObject *where_the_object_was) {
  LBA_LOG ("Reset singleton to NULL");
  G_LOCK (singleton_lock);
  singleton_object = NULL;
  G_UNLOCK (singleton_lock);
}

static void
lba_core_constructed (GObject *gobject) {
  LbaCore *self = bm_get_LbaCore (gobject);
  static int scanned;

  if (!scanned) {
    /* Hack to avoid installing the commands multiple times */
    BM_CHAINUP (self, GObject)->constructed (gobject);
    scanned = 1;
  }
}

static GObject *
lba_core_constructor (GType type, guint n_cp, GObjectConstructParam *cp) {
  GObjectClass *chain_up =
      (GObjectClass *) g_type_class_peek_parent (g_type_class_peek (type));

  LBA_LOG ("Constructing");

  if (G_UNLIKELY (n_cp != 0 && singleton_object != NULL))
    g_warning ("Will ignore the construct properties, this object is a singleton!");

  G_LOCK (singleton_lock);
  if (singleton_object == NULL) {
    LBA_LOG ("New instance of a singleton");
    singleton_object = chain_up->constructor (type, n_cp, cp);
    g_object_weak_ref (singleton_object, lba_core_reset_singleton, NULL);
  } else {
    LBA_LOG ("Singleton already found");
    singleton_object = g_object_ref (singleton_object);
  }
  G_UNLOCK (singleton_lock);

  return singleton_object;
}

static void
lba_core_class_init (GObjectClass *object_class, LbaCoreClass *klass) {
  LbaModuleScannerClass *lms_class;

  object_class->dispose = lba_core_dispose;
  object_class->finalize = lba_core_finalize;
  object_class->constructor = lba_core_constructor;
  object_class->constructed = lba_core_constructed;

  klass->execute = lba_core_execute;
  klass->add = lba_core_add;
  klass->pick = lba_core_pick;

  lba_core_signals[SIGNAL_EXECUTE] =
      g_signal_new ("execute", G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    BM_CLASS_VFUNC_OFFSET (klass, execute),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);

  lba_core_signals[SIGNAL_ADD] =
      g_signal_new ("add", G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    BM_CLASS_VFUNC_OFFSET (klass, add),
                    NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_STRING);

  lba_core_signals[SIGNAL_PICK] =
      g_signal_new ("pick", G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    BM_CLASS_VFUNC_OFFSET (klass, pick),
                    NULL, NULL, NULL, G_TYPE_OBJECT, 1, G_TYPE_STRING);

  lba_core_init_convertion_functions ();

  lms_class = BM_CLASS_LOOKUP_MIXIN (klass, LbaModuleScanner);
  lms_class->plugin_path_env = "LBA_PLUGINS_PATH";
  lms_class->plugin_prefix = "liblba-";
  lms_class->plugin_suffix = G_MODULE_SUFFIX;
  lms_class->have_file = lba_core_load_module;
}

/* GObject entry point */
GType
lba_core_object_get_type (void) {
  static GType ret;

  return ret ? ret : (ret =
                      bm_register_mixed_type ("LbaCoreObject", G_TYPE_OBJECT,
                                              lba_core_get_type (), NULL));
}
