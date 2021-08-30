lib_LTLIBRARIES = libbombolla-core.la

libbombolla_core_la_SOURCES = bombolla-core.c \
  bombolla-commands.c \
  bombolla-command-on.c \
  bombolla-command-set.c

libbombolla_core_la_CFLAGS = $(GOBJ_CFLAGS)
libbombolla_core_la_LIBADD = $(GOBJ_LIBS) $(GMODULE_LIBS)

noinst_HEADERS = bombolla-commands.h
EXTRA_DIST = ../../autogen.sh
