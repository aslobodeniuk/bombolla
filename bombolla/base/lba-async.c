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

#include <bmixin/bmixin.h>
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include "bombolla/base/i2d.h"

static GQuark toggle_refs_qrk;

typedef struct {
  GType type;
  guint n_construct_properties;
  GObjectConstructParam *construct_properties;
  GObjectClass *parent_klass;
  GObject *return_object;
} LbaAsyncConstructorInfo;

typedef struct _LbaAsync {
  /* TODO: move to BMixin struct */
  GObject *mami;
  /* -------------------------------- */

  GMutex lock;
  GCond cond;
  GSource *async_ctx;

  /* Special case: GObject constructor */
  LbaAsyncConstructorInfo *constructor_info;

  /* Another dirty hack */
  gpointer obj_toggle_refs;
} LbaAsync;

typedef struct _LbaAsyncClass {
  gboolean dirty_hack_constructing_now;
} LbaAsyncClass;

/* TODO: this mixin should always go as the last in the inheiritance tree.
 * We should add some kind of flag to it's type that would provide that feature. */
static void
lba_async_base_init (gpointer g_class) {
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  LBA_LOG ("base init for %s (%p) "
           "constructor = %p, dispose = %p, finalize = %p",
           G_OBJECT_CLASS_NAME (gobject_class),
           g_class,
           gobject_class->constructor,
           gobject_class->dispose, gobject_class->finalize);
}

static void lba_async_class_init (GObjectClass *, LbaAsyncClass *);
static void lba_async_init (GObject *, LbaAsync *);
static LbaAsync *bm_get_LbaAsync (gpointer gobject);
static LbaAsyncClass *bm_class_get_LbaAsync (gpointer gobject);
static void
lba_async_proxy_class_init (gpointer class, gpointer p) {
  lba_async_class_init (class, bm_class_get_LbaAsync (class));
}

static void
lba_async_proxy_init (GTypeInstance * instance, gpointer p) {
  lba_async_init (G_OBJECT (instance), bm_get_LbaAsync (instance));
}

static const BMParams lba_async_bmixin_params[] = { };

static BMInfo lba_async_info = {
  .info = {
           .base_init = lba_async_base_init,
           .class_init = lba_async_proxy_class_init,
           .instance_init = lba_async_proxy_init,
           .class_size = sizeof (LbaAsyncClass),
           .instance_size = sizeof (LbaAsync)
            },
  .params = lba_async_bmixin_params,
  .num_params = G_N_ELEMENTS (lba_async_bmixin_params)
};

GType
lba_async_get_type (void) {
  static gsize g_define_type_id = 0;

  if (g_once_init_enter (&g_define_type_id)) {
    g_define_type_id = bm_register_mixin ("LbaAsync", &lba_async_info);
    lba_async_info.type = g_define_type_id;
    g_type_set_qdata (lba_async_info.type, bmixin_info_qrk (), &lba_async_info);
  }
  return g_define_type_id;
}

static LbaAsync *
bm_get_LbaAsync (gpointer gobject) {
  return bm_instance_get_mixin (gobject, lba_async_info.type);
}

static LbaAsyncClass *
bm_class_get_LbaAsync (gpointer class) {
  return bm_class_get_mixin (class, lba_async_info.type);
}

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
lba_async_call_through_main_loop (LbaAsync * self, GSourceFunc cmd) {
  g_warn_if_fail (self->async_ctx == NULL);
  if (g_main_context_is_owner (NULL)) {
    LBA_LOG ("Already in the MainContext. Calling synchronously");
    cmd (self);
  } else {
    self->async_ctx = g_idle_source_new ();
    g_source_set_priority (self->async_ctx, G_PRIORITY_HIGH);
    g_source_set_callback (self->async_ctx, cmd, self, lba_async_cmd_free);

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

  BM_CHAINUP (self->mami, lba_async, GObject)->dispose (self->mami);
  return G_SOURCE_REMOVE;
}

static void
lba_async_dispose (GObject * gobject) {
  LbaAsync *self = bm_get_LbaAsync (gobject);

  LBA_LOG ("Schedulling [%s]->dispose", G_OBJECT_TYPE_NAME (gobject));
  lba_async_call_through_main_loop (self, lba_async_dispose_cmd);
}

static gboolean
lba_async_finalize_cmd (gpointer ptr) {
  LbaAsync *self = (LbaAsync *) ptr;

  BM_CHAINUP (self->mami, lba_async, GObject)->finalize (self->mami);
  return G_SOURCE_REMOVE;
}

static void
lba_async_finalize (GObject * gobject) {
  LbaAsync *self = bm_get_LbaAsync (gobject);

  LBA_LOG ("Scheduling [%s]->finalize", G_OBJECT_TYPE_NAME (gobject));
  lba_async_call_through_main_loop (self, lba_async_finalize_cmd);

  /* Now can clean our own stuff */
  g_mutex_clear (&self->lock);
  g_cond_clear (&self->cond);
  g_warn_if_fail (self->async_ctx == NULL);
}

static void
  lba_async_toggle_notify (gpointer data, GObject * object, gboolean is_last_ref);

static gboolean
lba_async_toggle_notify_cmd (gpointer ptr) {
  LbaAsync *self = (LbaAsync *) ptr;

  g_object_remove_toggle_ref (self->mami, lba_async_toggle_notify, self);

  if (self->obj_toggle_refs) {
    if (g_object_get_qdata (self->mami, toggle_refs_qrk)) {
      g_critical ("Can't restore the initial toggle refs, because there're"
                  " other ones... May leak this object..");
    } else {
      LBA_LOG ("Warning: Dirtiest hack!! restoring the original toggle refs");
      g_object_set_qdata (self->mami, toggle_refs_qrk, self->obj_toggle_refs);
      self->obj_toggle_refs = NULL;
    }
  }

  return G_SOURCE_REMOVE;
}

static void
lba_async_toggle_notify (gpointer data, GObject * object, gboolean is_last_ref) {
  LbaAsync *self = (LbaAsync *) data;

  LBA_LOG ("(%s) is_last_ref = %d", G_OBJECT_TYPE_NAME (object), is_last_ref);

  if (is_last_ref) {
    g_object_ref (object);
    lba_async_call_through_main_loop (self, lba_async_toggle_notify_cmd);
    /* REMEMBER: this can trigger dispose/finalize */
    g_object_unref (object);
  }
}

static gboolean
lba_async_constructed_cmd (gpointer ptr) {
  LbaAsync *self = (LbaAsync *) ptr;

  BM_CHAINUP (self->mami, lba_async, GObject)->constructed (self->mami);
  return G_SOURCE_REMOVE;
}

static void
lba_async_constructed (GObject * gobject) {
  LbaAsync *self = bm_get_LbaAsync (gobject);

  /* We want to set the very first toggle ref. To do that we
   * just store all the previous toggle refs here, hoping to restore
   * them back. If someone adds a toggle ref after that, we are screwed,
   * because our toggle ref will never be triggered.. if only when that one
   * removes the ref..
   * But in any case our ref MUST be the first one */
  self->obj_toggle_refs = g_object_steal_qdata (gobject, toggle_refs_qrk);
  LBA_LOG ("obj_toggle_refs = %p", self->obj_toggle_refs);
  g_object_add_toggle_ref (gobject, lba_async_toggle_notify, self);

  LBA_LOG ("Scheduling [%s]->constructed", G_OBJECT_TYPE_NAME (gobject));
  lba_async_call_through_main_loop (self, lba_async_constructed_cmd);
}

static gboolean
lba_async_constructor_cmd (gpointer ptr) {
  LbaAsync *self = (LbaAsync *) ptr;
  LbaAsyncConstructorInfo *info = self->constructor_info;

  g_return_val_if_fail (info, G_SOURCE_REMOVE);
  g_return_val_if_fail (info->type, G_SOURCE_REMOVE);
  g_return_val_if_fail (info->parent_klass, G_SOURCE_REMOVE);
  g_return_val_if_fail (info->n_construct_properties == 0
                        || info->construct_properties, G_SOURCE_REMOVE);
  g_return_val_if_fail (info->return_object == NULL, G_SOURCE_REMOVE);

  info->return_object =
      info->parent_klass->constructor (info->type,
                                       info->n_construct_properties,
                                       info->construct_properties);
  g_return_val_if_fail (info->return_object, G_SOURCE_REMOVE);
  return G_SOURCE_REMOVE;
}

static GObject *
lba_async_constructor (GType type, guint n_construct_properties,
                       GObjectConstructParam * construct_properties) {
  GObjectClass *parent_klass;
  GObject *ret;
  LbaAsyncClass *aklass = bm_class_get_LbaAsync (g_type_class_peek (type));

  parent_klass =
      g_type_class_peek
      (g_type_parent (bm_type_peek_mixed_type (type, lba_async_info.type)));

  LBA_LOG ("Type: %s class ptr=(%p)", g_type_name (type), g_type_class_peek (type));
  LBA_LOG ("Parent: %s class_ptr=(%p)", G_OBJECT_CLASS_NAME (parent_klass),
           parent_klass);
  g_assert (parent_klass->constructor != lba_async_constructor);

  if (g_main_context_is_owner (NULL)) {
    /* NOTE: here comes a dirty hack to workaround a bug somethere between gjs
     * and JS gi bindings. One of that codes in the constructor doesn't chain
     * up as we would like, and instead of chaining up to the parent class
     * (that would be a parent class of the class written in JS, e.g.
     * GtkWindow), it chains up back to us :/. So we enter into an infinite
     * recursion. With Python it doesn't happen.. .*/

    if (aklass->dirty_hack_constructing_now) {
      parent_klass = g_type_class_peek_parent (parent_klass);
      LBA_LOG ("Dirty hacking gjs/gi constructor: will chain up to '%s'",
               G_OBJECT_CLASS_NAME (parent_klass));
    }

    aklass->dirty_hack_constructing_now = TRUE;
    ret = parent_klass->constructor (type, n_construct_properties,
                                     construct_properties);
    /* Yes, that is a dirty hack. We also (correctly) assume that all this
     * always runs in the same thread. */
    aklass->dirty_hack_constructing_now = FALSE;
  } else {
    /* Here we need a tricky chainup. Async and without an instance.
     * For now we are choosing a quick hack. */
    LbaAsync tmp = { 0 };
    tmp.constructor_info = g_new0 (LbaAsyncConstructorInfo, 1);
    tmp.constructor_info->type = type;
    tmp.constructor_info->n_construct_properties = n_construct_properties;
    /* NOTE: will crash if construct_properties point on stack, will have to
     * do a copy then.. */
    tmp.constructor_info->construct_properties = construct_properties;
    tmp.constructor_info->parent_klass = parent_klass;

    lba_async_init (NULL, &tmp);
    aklass->dirty_hack_constructing_now = TRUE;
    lba_async_call_through_main_loop (&tmp, lba_async_constructor_cmd);
    aklass->dirty_hack_constructing_now = FALSE;

    /* can't do finalize */
    g_mutex_clear (&tmp.lock);
    g_cond_clear (&tmp.cond);
    ret = tmp.constructor_info->return_object;
    g_free (tmp.constructor_info);
  }

  g_warn_if_fail (ret);
  return ret;
}

static void
lba_async_class_init (GObjectClass * gobject_class, LbaAsyncClass * self_class) {

  if (G_UNLIKELY (!toggle_refs_qrk))
    toggle_refs_qrk = g_quark_from_static_string ("GObject-toggle-references");

  self_class->dirty_hack_constructing_now = FALSE;

  /* seldom overidden */
  /* overridable methods */
  /* void       (*set_property)         (GObject        *object, */
  /*                                        guint           property_id, */
  /*                                        const GValue   *value, */
  /*                                        GParamSpec     *pspec); */
  /* void       (*get_property)         (GObject        *object, */
  /*                                        guint           property_id, */
  /*                                        GValue         *value, */
  /*                                        GParamSpec     *pspec); */

  /* void       (*dispatch_properties_changed) (GObject      *object, */
  /*                                   guint         n_pspecs, */
  /*                                   GParamSpec  **pspecs); */
  /* signals */
  /* void            (*notify)                  (GObject *object, */
  /*                               GParamSpec *pspec); */

  gobject_class->constructor = lba_async_constructor;
  gobject_class->constructed = lba_async_constructed;
  gobject_class->dispose = lba_async_dispose;
  gobject_class->finalize = lba_async_finalize;

  /* TODO:
   * --------------------------------------------
   GParamSpec ** g_object_class_list_properties (GObjectClass *oclass, guint *n_properties);

   + foreach:

   void g_object_class_override_property (GObjectClass *oclass, guint property_id, const gchar *name);

   +
   Then chainup though parent_klass->get/set_property
   ----------------------------------------------
   guint *
   g_signal_list_ids (GType itype,
   guint *n_ids);

   void
   g_signal_query (guint signal_id,
   GSignalQuery *query);

   void
   g_signal_override_class_handler (const gchar *signal_name,
   GType instance_type,
   GCallback class_handler);

   g_signal_chain_from_overridden_handler ()
   * */
}

static void
lba_async_init (GObject * object, LbaAsync * self) {
  self->mami = object;

  g_mutex_init (&self->lock);
  g_cond_init (&self->cond);
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_async);
