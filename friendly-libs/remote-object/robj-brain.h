#ifndef _ROBJ_BRAIN_H
#  define _ROBJ_BRAIN_H

#  include <glib-object.h>

/* Property notify */
typedef struct {
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
RObjPN *robj_brain_lookup_pn (guint32 o_hash, guint32 pn_hash);

/* returns transfer-none */
RObjPN *robj_brain_learn_pn (guint32 o_hash, const gchar * pname,
                             const GValue * pval);

guint32 robj_brain_add_object (const gchar * name);

void
  robj_brain_forget_object (guint32 ohash);

#endif
