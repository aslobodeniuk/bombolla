MAINTAINERCLEANFILES = Makefile.in

lib_LTLIBRARIES = liblba-remote-object-server.la

liblba_remote_object_server_la_SOURCES = lba-remote-object-server.c
liblba_remote_object_server_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
liblba_remote_object_server_la_LIBADD = $(GOBJ_LIBS)
liblba_remote_object_server_la_LDFLAGS = $(PLUGIN_LDFLAGS)

EXTRA_DIST = ../../autogen.sh
