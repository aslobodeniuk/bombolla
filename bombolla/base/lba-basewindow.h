#ifndef __BASE_WINDOW_H__
#define __BASE_WINDOW_H__

#include <glib-object.h>
#include <glib/gstdio.h>

GType base_window_get_type (void);

#define G_TYPE_BASE_WINDOW (base_window_get_type ())           
#define BASE_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),G_TYPE_BASE_WINDOW ,BaseWindowClass))
#define BASE_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_BASE_WINDOW ,BaseWindowClass))


typedef struct _BaseWindow
{
  GObject parent;

  /* instance members */
  int width;
  int height;
  int x_pos;
  int y_pos;

  gchar *title;
  gboolean opened;
} BaseWindow;


typedef struct _BaseWindowClass
{
  GObjectClass parent;
  
  /* Events */
  void (*on_display) (BaseWindow *);

  /* Actions */
  void (*open) (BaseWindow *);
  void (*close) (BaseWindow *);
  void (*request_redraw) (BaseWindow *);

} BaseWindowClass;

void base_window_notify_display (BaseWindow * self);

#endif
