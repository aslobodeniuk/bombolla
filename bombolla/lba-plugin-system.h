#ifndef _MY_PLUGIN_SYSTEM
#define _MY_PLUGIN_SYSTEM
#include <glib-object.h>

typedef GType (*MyPluginSystemGetGtypeFunc) (void);

#define MY_PLUGIN_SYSTEM_PROVIDE_GTYPE(name)                            \
  GType my_plugin_system_get_gtype (void)                               \
  {                                                                     \
    g_printf ("hello from '" #name "'\n");                              \
    return name##_get_type ();                                          \
  }

#endif
