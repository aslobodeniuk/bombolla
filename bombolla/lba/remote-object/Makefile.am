MAINTAINERCLEANFILES = Makefile.in

lib_LTLIBRARIES = libLbaRemoteObject.la

libLbaRemoteObject_la_SOURCES = LbaRemoteObject.c
libLbaRemoteObject_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
libLbaRemoteObject_la_LIBADD = $(GOBJ_LIBS)
libLbaRemoteObject_la_LDFLAGS = $(PLUGIN_LDFLAGS)

EXTRA_DIST = ../../autogen.sh
