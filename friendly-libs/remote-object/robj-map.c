#include "robj-map.h"

typedef struct {
  GMutex lock;
  GHashTable *pns;
  GHashTable *sigs;
  gchar *name;
} RObjMapObject;

static RObjMapObject *
robj_map_peek_object (RObjMap * map, guint32 o_hash) {
  RObjMapObject *obj = NULL;

  g_mutex_lock (&map->lock);
  obj = (RObjMapObject *)
      g_hash_table_lookup (map->objects, GUINT_TO_POINTER (o_hash));
  g_mutex_unlock (&map->lock);

  return obj;
}

static RObjPN *
robj_map_peek_pn (RObjMapObject * obj, guint32 pn_hash) {
  RObjPN *pn;

  g_mutex_lock (&obj->lock);
  pn = (RObjPN *)
      g_hash_table_lookup (obj->pns, GUINT_TO_POINTER (pn_hash));
  g_mutex_unlock (&obj->lock);

  return pn;
}

RObjPN *
robj_map_lookup_pn (RObjMap * map, guint32 o_hash, guint32 pn_hash) {
  RObjMapObject *obj;

  obj = robj_map_peek_object (map, o_hash);
  if (G_UNLIKELY (obj == NULL)) {
    g_critical ("Object with hash %u not found", o_hash);
    return NULL;
  }

  return robj_map_peek_pn (obj, pn_hash);
}

RObjPN *
robj_map_new_pn (RObjMap * map, guint32 o_hash, const gchar * pname,
                 const GValue * pval) {
  gboolean didnt_exist;
  RObjPN *pn = NULL;
  RObjMapObject *obj = robj_map_peek_object (map, o_hash);

  g_return_val_if_fail (pname != NULL, NULL);
  g_return_val_if_fail (pval != NULL, NULL);
  g_return_val_if_fail (NULL != obj, NULL);

  pn = g_new (RObjPN, 1);
  g_mutex_init (&pn->lock);
  pn->pname = g_strdup (pname);
  pn->pval = (GValue) G_VALUE_INIT;
  g_value_init (&pn->pval, G_VALUE_TYPE (pval));
  pn->map = map;

  /* For a refcounted things such as GBytes, is it racy for the map
   * to remember a reference on the GBytes from a real object?
   * Not itself: if your property provides GBytes, and this GBytes is
   * not a deep copy, then changing this GBytes from any code is racy,
   * and reading them from any other code is racy too, UNLESS this value
   * is not supposed to ever change. */
  g_value_copy (pval, &pn->pval);

  pn->o_hash = o_hash;
  pn->pn_hash = GUINT32_TO_BE (g_str_hash (pname));

  /* Remember this PN in the map */
  g_mutex_lock (&obj->lock);
  didnt_exist = g_hash_table_insert (obj->pns, GUINT_TO_POINTER (pn->pn_hash), pn);
  g_mutex_unlock (&obj->lock);

  g_return_val_if_fail (didnt_exist, NULL);
  return pn;
}

static void
robj_map_destroy_pn (gpointer data) {
  RObjPN *pn = (RObjPN *) data;

  g_return_if_fail (pn != NULL);
  g_return_if_fail (pn->pname != NULL);

  g_mutex_clear (&pn->lock);
  g_free (pn->pname);
  g_value_unset (&pn->pval);

  g_free (pn);
}

static void
robj_map_destroy_obj (gpointer data) {
  RObjMapObject *obj = (RObjMapObject *) data;

  g_return_if_fail (obj->name != NULL);

  g_hash_table_unref (obj->pns);
  g_mutex_clear (&obj->lock);
  g_free (obj->name);
  g_free (obj);
}

guint32
robj_map_add_object (RObjMap * map, const gchar * name) {
  RObjMapObject *obj;
  G_GNUC_UNUSED gboolean didnt_exist;
  guint32 ohash;

  g_return_val_if_fail (name != NULL, 0);

  ohash = GUINT32_TO_BE (g_str_hash (name));
  obj = g_new (RObjMapObject, 1);
  obj->name = g_strdup (name);
  obj->pns = g_hash_table_new_full (NULL, NULL, NULL, robj_map_destroy_pn);
  g_mutex_init (&obj->lock);

  // obj->sigs = TODO

  g_mutex_lock (&map->lock);
  didnt_exist = g_hash_table_insert (map->objects, GUINT_TO_POINTER (ohash), obj);
  g_mutex_unlock (&map->lock);

  g_return_val_if_fail (didnt_exist, 0);
  return ohash;
}

void
robj_map_remove_object (RObjMap * map, guint32 ohash) {
  G_GNUC_UNUSED gboolean found;

  g_mutex_lock (&map->lock);
  found = g_hash_table_remove (map->objects, GUINT_TO_POINTER (ohash));
  g_mutex_unlock (&map->lock);

  g_warn_if_fail (found);
}

void
robj_map_init (RObjMap * map) {
  g_mutex_init (&map->lock);
  map->objects = g_hash_table_new_full (NULL, NULL, NULL, robj_map_destroy_obj);
}

void
robj_map_clear (RObjMap * map) {
  g_mutex_clear (&map->lock);
  g_hash_table_unref (map->objects);
}
