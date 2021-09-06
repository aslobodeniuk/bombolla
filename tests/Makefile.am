include $(top_srcdir)/glib-tap.mk

EXTRA_DIST += ../autogen.sh

# ---------------------- UNIT TESTS ----------------------
# test binaries
test_programs = bombolla-core-test test-plugin-in-vala
# data not distributed in the tarball
test_data =
# data distributed in the tarball
dist_test_data =

bombolla_core_test_SOURCES = bombolla-core-test.c
bombolla_core_test_CFLAGS = $(GOBJ_CFLAGS)
bombolla_core_test_LDADD = $(GOBJ_LIBS) \
  $(top_builddir)/bombolla/core/libbombolla-core.la

# Test plugin in vala
test_plugin_in_vala_SOURCES = test-plugin-in-vala.c
test_plugin_in_vala_CFLAGS = $(GOBJ_CFLAGS)
test_plugin_in_vala_LDADD = $(GOBJ_LIBS) \
  $(top_builddir)/bombolla/core/libbombolla-core.la \
  $(top_builddir)/examples/vala/liblba-vala-example.la
