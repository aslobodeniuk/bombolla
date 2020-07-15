#ifndef _BOMBOLLA_PLUGIN_SYSTEM
#define _BOMBOLLA_PLUGIN_SYSTEM
#include <glib-object.h>

typedef GType (*lBaPluginSystemGetGtypeFunc) (void);


#define BOMBOLLA_PLUGIN_SYSTEM_ENTRY_   bombolla_plugin_system_get_gtype
#define BOMBOLLA_PLUGIN_SYSTEM_ENTRY   G_STRINGIFY (BOMBOLLA_PLUGIN_SYSTEM_ENTRY_)

#define BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE(name)                      \
  GType BOMBOLLA_PLUGIN_SYSTEM_ENTRY_ (void)                            \
  {                                                                     \
    return name##_get_type ();                                          \
  }

#endif
