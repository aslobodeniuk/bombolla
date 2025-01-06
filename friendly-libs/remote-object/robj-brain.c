#include "robj-brain.h"

static struct {
  GMutex lock;
  GHashTable *map;
} robj_objects;

#define LOCK_OBJECTS(x) g_mutex_lock(&((x)->lock));
#define UNLOCK_OBJECTS(x) g_mutex_unlock(&((x)->lock));

#define LOCK_OBJECT(x) g_mutex_lock (&((x)->lock))
#define UNLOCK_OBJECT(x) g_mutex_unlock (&((x)->lock))

typedef struct {
  GMutex lock;
  GHashTable *pns;
  GHashTable *sigs;
  gchar *name;
} RObjBrainObject;

static RObjBrainObject *
robj_brain_peek_object (guint32 o_hash) {
  RObjBrainObject *obj = NULL;

  LOCK_OBJECTS (&robj_objects);
  obj = (RObjBrainObject *)
      g_hash_table_lookup (robj_objects.map, GUINT_TO_POINTER (o_hash));
  UNLOCK_OBJECTS (&robj_objects);

  return obj;
}

static RObjPN *
robj_brain_peek_pn (RObjBrainObject * obj, guint32 pn_hash) {
  RObjPN *pn;

  LOCK_OBJECT (obj);
  pn = (RObjPN *)
      g_hash_table_lookup (obj->pns, GUINT_TO_POINTER (pn_hash));
  UNLOCK_OBJECT (obj);

  return pn;
}

RObjPN *
robj_brain_lookup_pn (guint32 o_hash, guint32 pn_hash) {
  RObjBrainObject *obj;

  obj = robj_brain_peek_object (o_hash);
  if (G_UNLIKELY (obj == NULL)) {
    g_critical ("Object with hash %u not found", o_hash);
    return NULL;
  }

  return robj_brain_peek_pn (obj, pn_hash);
}

RObjPN *
robj_brain_learn_pn (guint32 o_hash, const gchar * pname, const GValue * pval) {
  gboolean didnt_exist;
  RObjPN *pn = NULL;
  RObjBrainObject *obj = robj_brain_peek_object (o_hash);

  g_return_val_if_fail (pname != NULL, NULL);
  g_return_val_if_fail (pval != NULL, NULL);
  g_return_val_if_fail (NULL != obj, NULL);

  pn = g_new (RObjPN, 1);
  g_mutex_init (&pn->lock);
  pn->pname = g_strdup (pname);
  pn->pval = (GValue) G_VALUE_INIT;
  g_value_init (&pn->pval, G_VALUE_TYPE (pval));

  /* For a refcounted things such as GBytes, is it racy for the brain
   * to remember a reference on the GBytes from a real object?
   * Not itself: if your property provides GBytes, and this GBytes is
   * not a deep copy, then changing this GBytes from any code is racy,
   * and reading them from any other code is racy too, UNLESS this value
   * is not supposed to ever change. */
  g_value_copy (pval, &pn->pval);

  pn->o_hash = o_hash;
  pn->pn_hash = GUINT32_TO_BE (g_str_hash (pname));

  /* Remember this PN in the brain */
  LOCK_OBJECT (obj);
  didnt_exist = g_hash_table_insert (obj->pns, GUINT_TO_POINTER (pn->pn_hash), pn);
  UNLOCK_OBJECT (obj);

  g_return_val_if_fail (didnt_exist, NULL);
  return pn;
}

static void
robj_brain_destroy_pn (gpointer data) {
  RObjPN *pn = (RObjPN *) data;

  g_return_if_fail (pn != NULL);
  g_return_if_fail (pn->pname != NULL);

  g_mutex_clear (&pn->lock);
  g_free (pn->pname);
  g_value_unset (&pn->pval);

  g_free (pn);
}

static void
robj_brain_destroy_obj (gpointer data) {
  RObjBrainObject *obj = (RObjBrainObject *) data;

  g_return_if_fail (obj->name != NULL);

  g_hash_table_unref (obj->pns);
  g_mutex_clear (&obj->lock);
  g_free (obj->name);
  g_free (obj);
}

guint32
robj_brain_add_object (const gchar * name) {
  RObjBrainObject *obj;
  G_GNUC_UNUSED gboolean didnt_exist;
  guint32 ohash;

  g_return_val_if_fail (name != NULL, 0);

  ohash = GUINT32_TO_BE (g_str_hash (name));
  obj = g_new (RObjBrainObject, 1);
  obj->name = g_strdup (name);
  obj->pns = g_hash_table_new_full (NULL, NULL, NULL, robj_brain_destroy_pn);
  g_mutex_init (&obj->lock);

  // obj->sigs = TODO

  LOCK_OBJECTS (&robj_objects);
  if (G_UNLIKELY (robj_objects.map == NULL)) {
    /* The very first object. FIXME: when are we going to
     * destroy the table? Probably we have to add brain_init
     * function */
    robj_objects.map = g_hash_table_new_full (NULL,
                                              NULL, NULL, robj_brain_destroy_obj);
  }

  didnt_exist =
      g_hash_table_insert (robj_objects.map, GUINT_TO_POINTER (ohash), obj);
  UNLOCK_OBJECTS (&robj_objects);

  g_return_val_if_fail (didnt_exist, 0);
  return ohash;
}

void
robj_brain_forget_object (guint32 ohash) {
  G_GNUC_UNUSED gboolean found;

  LOCK_OBJECTS (&robj_objects);
  found = g_hash_table_remove (robj_objects.map, GUINT_TO_POINTER (ohash));
  UNLOCK_OBJECTS (&robj_objects);

  g_warn_if_fail (found);
}
