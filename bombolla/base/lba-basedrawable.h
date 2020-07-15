#ifndef __BASE_DRAWABLE_H__
#define __BASE_DRAWABLE_H__

#include <glib-object.h>
#include <glib/gstdio.h>

GType base_drawable_get_type (void);

#define G_TYPE_BASE_DRAWABLE (base_drawable_get_type ())           
#define BASE_DRAWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),BASE_DRAWABLE,BaseDrawableClass))

typedef struct _BaseDrawable
{
  GObject parent;

  /* instance members */
  int width;
  int height;
  int x_pos;
  int y_pos;
} BaseDrawable;


typedef struct _BaseDrawableClass
{
  GObjectClass parent;

  /* Actions */
  void (*draw) (BaseDrawable *);

} BaseDrawableClass;

#endif
