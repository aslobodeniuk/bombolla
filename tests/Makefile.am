MAINTAINERCLEANFILES = \
Makefile.in \
bombolla-core-test \
bombolla-core-test.trs \
test-plugin-in-vala \
test-plugin-in-vala.log \
test-plugin-in-vala.trs \
test-suite.log

include $(top_srcdir)/glib-tap.mk
# This one is needed to properly identify memleaks
AM_TESTS_ENVIRONMENT += G_SLICE=always-malloc
# -----------------------------------------------

EXTRA_DIST += ../autogen.sh

# ---------------------- UNIT TESTS ----------------------
# test binaries
test_programs = bombolla-core-test test-plugin-in-vala
# data not distributed in the tarball
test_data =
# data distributed in the tarball
dist_test_data =

bombolla_core_test_SOURCES = bombolla-core-test.c
bombolla_core_test_CFLAGS = $(GOBJ_CFLAGS) $(ASAN_CFLAGS)
bombolla_core_test_LDADD = $(GOBJ_LIBS) \
  $(top_builddir)/bombolla/core/libbombolla-core.la
bombolla_core_test_LDFLAGS = $(ASAN_LDFLAGS)

# Test plugin in vala
test_plugin_in_vala_SOURCES = test-plugin-in-vala.c
test_plugin_in_vala_CFLAGS = $(GOBJ_CFLAGS) $(ASAN_CFLAGS)
test_plugin_in_vala_LDADD = $(GOBJ_LIBS) \
  $(top_builddir)/bombolla/core/libbombolla-core.la \
  $(top_builddir)/examples/vala/liblba-vala-example.la
test_plugin_in_vala_LDFLAGS = $(ASAN_LDFLAGS)
