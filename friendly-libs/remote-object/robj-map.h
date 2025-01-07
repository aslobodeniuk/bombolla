#ifndef _ROBJ_BRAIN_H
#  define _ROBJ_BRAIN_H

#  include <glib-object.h>

typedef struct {
  GMutex lock;
  GHashTable *objects;
} RObjMap;

void robj_map_init (RObjMap * map);
void robj_map_clear (RObjMap * map);

/* Property notify */
typedef struct {
  RObjMap *map;

  GMutex lock;
  /* Property name */
  gchar *pname;
  /* Current value of the property */
  GValue pval;
  /* Hash of the object name (BE). */
  guint32 o_hash;
  /* Hash of the property name (BE). */
  guint32 pn_hash;
} RObjPN;

/* transfer-none */
RObjPN *robj_map_lookup_pn (RObjMap * map, guint32 o_hash, guint32 pn_hash);

/* returns transfer-none */
RObjPN *robj_map_new_pn (RObjMap * map, guint32 o_hash, const gchar * pname,
                         const GValue * pval);

guint32 robj_map_add_object (RObjMap * map, const gchar * name);

void robj_map_remove_object (RObjMap * map, guint32 ohash);

#endif
