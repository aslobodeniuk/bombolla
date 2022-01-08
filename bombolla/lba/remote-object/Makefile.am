MAINTAINERCLEANFILES = Makefile.in

lib_LTLIBRARIES = liblba-remote-object-server.la

liblba_remote_object_server_la_DEPENDENCIES = \
   $(top_builddir)/bombolla/base/liblba-byte-stream.la
liblba_remote_object_server_la_SOURCES = lba-remote-object-server.c
liblba_remote_object_server_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
liblba_remote_object_server_la_LIBADD = $(GOBJ_LIBS) -L$(top_builddir)/bombolla/base/.libs/ -llba-byte-stream
liblba_remote_object_server_la_LDFLAGS = $(PLUGIN_LDFLAGS)


lib_LTLIBRARIES = liblba-remote-object-client.la

liblba_remote_object_client_la_DEPENDENCIES = \
  $(top_builddir)/bombolla/base/liblba-byte-stream.la
liblba_remote_object_client_la_SOURCES = lba-remote-object-client.c lba-remote-object.c lba-remote-object-protocol.c
liblba_remote_object_client_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
liblba_remote_object_client_la_LIBADD = $(GOBJ_LIBS) -L$(top_builddir)/bombolla/base/.libs/ -llba-byte-stream
liblba_remote_object_client_la_LDFLAGS = $(PLUGIN_LDFLAGS)

noinst_HEADERS = \
lba-remote-object-private.h \
lba-remote-object-protocol.h

EXTRA_DIST = ../../autogen.sh

