#include $(top_srcdir)/glib-tap.mk

lib_LTLIBRARIES = libbombolla-core.la

libbombolla_core_la_SOURCES = bombolla-core.c \
  bombolla-commands.c \
  bombolla-command-on.c \
  bombolla-command-set.c

libbombolla_core_la_CFLAGS = $(GOBJ_CFLAGS)
libbombolla_core_la_LIBADD = $(GOBJ_LIBS) $(GMODULE_LIBS)

noinst_HEADERS = bombolla-commands.h
EXTRA_DIST = ../../autogen.sh

# ---------------------- UNIT TESTS ----------------------
#installed_testdir = $(top_builddir)/bombolla/core
#installed_test_metadir = $(top_builddir)/bombolla/core
# test binaries
#test_programs = bombolla-core-test
# data not distributed in the tarball
#test_data =
# data distributed in the tarball
#dist_test_data =

#bombolla_core_test_SOURCES = bombolla-core-test.c
#bombolla_core_test_CFLAGS = $(GOBJ_CFLAGS)
#bombolla_core_test_LDADD = $(GOBJ_LIBS) \
#  $(top_builddir)/bombolla/core/libbombolla-core.la
