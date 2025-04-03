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

#define MAGIC "lba-core-bindings-ht"

static GHashTable *
lba_get_bindings_ht (GObject *core) {
  /* FIXME: ht must be thread-safe */
  GHashTable *ht = g_object_get_data (core, MAGIC);

  if (G_UNLIKELY (NULL == ht)) {
    // The bindings actually belong to the object, and are
    // automatically destroyed when the objects are destroyed.
    // The only point of storing them is the "unbind" command        
    ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    g_object_set_data_full (core, MAGIC, ht, (GDestroyNotify) g_hash_table_unref);
  }

  return ht;
}

static gboolean
lba_command_bind (GObject *core, GObject *obj1, const gchar *prop1,
                  GObject *obj2, const gchar *prop2) {
  GParamSpec *pspec1;
  GParamSpec *pspec2;
  gboolean ret = FALSE;
  GBindingFlags flags = G_BINDING_SYNC_CREATE | G_BINDING_DEFAULT;
  GBinding *binding;
  gchar *binding_name = NULL;
  GHashTable *ht;

  /* Now we figure out the binding flags:
   * If one of the properties is read-only, we bind in one direction */
  pspec1 = g_object_class_find_property (G_OBJECT_GET_CLASS (obj1), prop1);

  if (!pspec1) {
    g_warning ("property [%s] not found", prop1);
    goto done;
  }

  pspec2 = g_object_class_find_property (G_OBJECT_GET_CLASS (obj2), prop2);

  if (!pspec2) {
    g_warning ("property [%s] not found", prop2);
    goto done;
  }

  ht = lba_get_bindings_ht (core);
  // ???
  binding_name = g_strdup_printf ("%p.%s_%p.%s", obj1, prop1, obj2, prop2);

  if (((pspec1->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)
      && ((pspec2->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)) {
    /* easy case: both are rw. Do bidirectional binding */
    LBA_LOG ("Adding bidirectional binding [%s]<-->[%s]", prop1, prop2);
    flags |= G_BINDING_BIDIRECTIONAL;
  } else if ((pspec1->flags & G_PARAM_READABLE)
             && (pspec2->flags & G_PARAM_WRITABLE)) {
    LBA_LOG ("Adding monodirectional binding [%s]--->[%s]", prop1, prop2);
  } else if ((pspec2->flags & G_PARAM_READABLE)
             && (pspec1->flags & G_PARAM_WRITABLE)) {

    LBA_LOG ("Adding monodirectional binding [%s]<---[%s]", prop1, prop2);
    binding = g_object_bind_property (obj2, prop2, obj1, prop1, flags);

    /* TODO: we will need it for "unbind" command */
    {
      g_hash_table_insert (ht, binding_name, binding);
      LBA_LOG ("Added binding %s", binding_name);
      binding_name = NULL;
    }
    ret = TRUE;
    goto done;
  } else {
    g_warning ("Couldn't bind property [%s](%c%c) to [%s](%c%c)", prop1,
               (pspec1->flags & G_PARAM_READABLE) ? 'r' : '_',
               (pspec1->flags & G_PARAM_WRITABLE) ? 'w' : '_', prop2,
               (pspec2->flags & G_PARAM_READABLE) ? 'r' : '_',
               (pspec2->flags & G_PARAM_WRITABLE) ? 'w' : '_');
    goto done;
  }

  binding = g_object_bind_property (obj1, prop1, obj2, prop2, flags);
  /* TODO: we will need it for "unbind" command */
  g_hash_table_insert (ht, binding_name, binding);
  LBA_LOG ("Added binding %s", binding_name);
  binding_name = NULL;
  ret = TRUE;
done:
  g_free (binding_name);
  return ret;
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_COMMAND (bind, LBA_COMMAND_SETUP_DEFAULT,
                                        /* obj1 */
                                        G_TYPE_OBJECT,
                                        /* prop1 */
                                        G_TYPE_STRING,
                                        /* obj2 */
                                        G_TYPE_OBJECT,
                                        /* prop2 */
                                        G_TYPE_STRING);
