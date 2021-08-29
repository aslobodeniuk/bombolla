include $(top_srcdir)/glib-tap.mk

EXTRA_DIST += ../autogen.sh

# ---------------------- UNIT TESTS ----------------------
# test binaries
test_programs = bombolla-core-test
# data not distributed in the tarball
test_data =
# data distributed in the tarball
dist_test_data =

bombolla_core_test_SOURCES = bombolla-core-test.c
bombolla_core_test_CFLAGS = $(GOBJ_CFLAGS)
bombolla_core_test_LDADD = $(GOBJ_LIBS) \
  $(top_builddir)/bombolla/core/libbombolla-core.la
