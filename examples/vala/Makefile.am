lib_LTLIBRARIES = liblba-vala-example.la

liblba_vala_example_la_SOURCES = plugin-in-vala.vala register-plugin.c
liblba_vala_example_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS) -Wno-unused-variable
liblba_vala_example_la_LIBADD = $(GOBJ_LIBS)
liblba_vala_example_la_LDFLAGS = $(GOBJ_LIBS) $(PLUGIN_LDFLAGS)
