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
} LbaAsync;

typedef struct _LbaAsyncClass {
} LbaAsyncClass;

/* TODO: this mixin should always go as the last in the inheiritance tree.
 * We should add some kind of flag to it's type that would provide that feature. */
BM_DEFINE_MIXIN (lba_async, LbaAsync);

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

static gboolean
lba_async_constructed_cmd (gpointer ptr) {
  LbaAsync *self = (LbaAsync *) ptr;

  BM_CHAINUP (self->mami, lba_async, GObject)->constructed (self->mami);
  return G_SOURCE_REMOVE;
}

static void
lba_async_constructed (GObject * gobject) {
  LbaAsync *self = bm_get_LbaAsync (gobject);

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
      info->parent_klass->constructor (info->type, info->n_construct_properties,
                                       info->construct_properties);
  g_return_val_if_fail (info->return_object, G_SOURCE_REMOVE);
  return G_SOURCE_REMOVE;
}

static GObject *
lba_async_constructor (GType type, guint n_construct_properties,
                       GObjectConstructParam * construct_properties) {
  GObjectClass *parent_klass;

  parent_klass =
      g_type_class_peek (g_type_parent
                         (bm_type_peek_mixed_type (type, lba_async_info.type)));

  LBA_LOG ("Parent: %s", G_OBJECT_CLASS_NAME (parent_klass));
  g_assert (parent_klass->constructor != lba_async_constructor);

  if (g_main_context_is_owner (NULL)) {
    return parent_klass->constructor (type, n_construct_properties,
                                      construct_properties);
  } else {
    GObject *ret;

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
    lba_async_call_through_main_loop (&tmp, lba_async_constructor_cmd);

    /* can't do finalize */
    g_mutex_clear (&tmp.lock);
    g_cond_clear (&tmp.cond);
    ret = tmp.constructor_info->return_object;
    g_free (tmp.constructor_info);

    g_warn_if_fail (ret != NULL);
    return ret;
  }
}

static void
lba_async_class_init (GObjectClass * gobject_class, LbaAsyncClass * self_class) {

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
