MAINTAINERCLEANFILES = Makefile.in

lib_LTLIBRARIES = libLbaGjs.la

libLbaGjs_la_SOURCES = LbaGjs.c
libLbaGjs_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS) $(GJS_CFLAGS)
libLbaGjs_la_LIBADD = $(GOBJ_LIBS) $(GMODULE_LIBS) $(GJS_LIBS)
libLbaGjs_la_LDFLAGS = $(PLUGIN_LDFLAGS)

EXTRA_DIST = ../../autogen.sh
