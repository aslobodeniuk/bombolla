lib_LTLIBRARIES = libbombolla-core.la

libbombolla_core_la_SOURCES = bombolla-core.c \
  bombolla-commands.c \
  bombolla-command-on.c \
  bombolla-command-set.c

libbombolla_core_la_CFLAGS = $(COMMON_CFLAGS) $(GOBJ_CFLAGS) $(ASAN_CFLAGS)
libbombolla_core_la_LIBADD = $(GOBJ_LIBS) $(GMODULE_LIBS)
libbombolla_core_la_LDFLAGS = $(ASAN_LDFLAGS)

noinst_HEADERS = bombolla-commands.h
EXTRA_DIST = ../../autogen.sh
