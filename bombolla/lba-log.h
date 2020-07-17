#ifndef _BOMBOLLA_LOG
#define _BOMBOLLA_LOG
#include <glib/gstdio.h>
#include "bombolla/lba-plugin-system.h"

GType bombolla_plugin_system_get_gtype ();

#define LBA_LOG(form, ...) do {                                         \
    g_printf ("[%s] " form "\n",                                        \
        g_type_name ( BOMBOLLA_PLUGIN_SYSTEM_ENTRY_ ()),  ##__VA_ARGS__); \
  } while (0)


#endif
