dnl required version of autoconf
AC_PREREQ([2.63])

strictness="foreign"

AC_INIT([Bombolla-GObj-shell],[0.0.0])

AC_CONFIG_SRCDIR([.])
AC_CONFIG_MACRO_DIR([.])
AC_CONFIG_HEADERS([config.h])

dnl required version of automake
AM_INIT_AUTOMAKE([1.11 foreign])
AM_SILENT_RULES([yes])
dnl enable mainainer mode by default
AM_MAINTAINER_MODE([enable])

dnl check for tools (compiler etc.)
AC_PROG_CC

dnl required version of libtool
LT_PREREQ([2.2.6])
LT_INIT

dnl give error and exit if we don't have pkgconfig
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, [ ], [
  AC_MSG_ERROR([You need to have pkg-config installed!])
])

PKG_CHECK_MODULES(GOBJ, [gobject-2.0], [
  AC_SUBST(GOBJ_CFLAGS)
  AC_SUBST(GOBJ_LIBS)
], [
  AC_MSG_ERROR([GObject library not found])
])

PKG_CHECK_MODULES(GMODULE, [gmodule-2.0], [
  AC_SUBST(GMODULE_CFLAGS)
  AC_SUBST(GMODULE_LIBS)
], [
  AC_MSG_ERROR([GModule library not found])
])

PKG_CHECK_MODULES(SOUP, [libsoup-2.4], [
  AC_SUBST(SOUP_CFLAGS)
  AC_SUBST(SOUP_LIBS)
], [
  AC_MSG_ERROR([Soup library not found (e.g. libsoup2.4-dev in Ubuntu)])
])

PKG_CHECK_MODULES(GJS, [gjs-1.0], [
  AC_SUBST(GJS_CFLAGS)
  AC_SUBST(GJS_LIBS)
], [
  AC_MSG_ERROR([Gjs package not found (e.g. libgjs-dev in Ubuntu)])
])

dnl FIXME: ugly copy
PKG_CHECK_MODULES(COGL_PANGO_DEP, pangocairo >= 1.20)
AC_SUBST(COGL_PANGO_DEP_CFLAGS)
AC_SUBST(COGL_PANGO_DEP_LIBS)

dnl no idea about the required minimum version of Vala
AM_PROG_VALAC([0.35], [AC_MSG_RESULT([valac found])], [AC_MSG_ERROR([valac not found])])


ASAN_CFLAGS="-O0 -g -fsanitize=address -fno-omit-frame-pointer"
ASAN_LDFLAGS="-fsanitize=address"

COMMON_CFLAGS="-Wall -Werror -I\$(top_srcdir) $ASAN_CFLAGS"

BASECLASS_CFLAGS="$GOBJ_CFLAGS $COMMON_CFLAGS"
BASECLASS_LIBS="$GOBJ_LIBS"
PLUGIN_CFLAGS="-fpic $COMMON_CFLAGS"
PLUGIN_LDFLAGS="-module -avoid-version -no-undefined $ASAN_LDFLAGS"

# for now we keep plugins path to be a /lib directory. That's bad,
# and we'll fix it.
LBA_PLUGINS_PATH=${prefix}
AC_SUBST(LBA_PLUGINS_PATH)

AC_SUBST(COMMON_CFLAGS)

AC_SUBST(ASAN_CFLAGS)
AC_SUBST(ASAN_LDFLAGS)

AC_SUBST(BASECLASS_CFLAGS)
AC_SUBST(BASECLASS_LIBS)

AC_SUBST(PLUGIN_LDFLAGS)
AC_SUBST(PLUGIN_CFLAGS)
AC_SUBST(PLUGIN_LDFLAGS)

GLIB_TESTS

dnl configure glib
if test ! -f deps/meson-based/glib/_builddir;
then
mkdir deps/meson-based/glib/_builddir
fi

cd deps/meson-based/glib/_builddir && meson --prefix=${prefix} --libdir=lib-asan --default-library=both --buildtype=debug --backend=ninja --wrap-mode=nodownload -Dinternal_pcre=true -Dlibmount=disabled -Ddtrace=false -Diconv=libc -Db_sanitize=address
cd ../../../..

dnl call cogl/configure recursively
AC_CONFIG_SUBDIRS([deps/cogl])

LBA_COGL_CFLAGS="-DCOGL_ENABLE_EXPERIMENTAL_2_0_API -I\$(top_srcdir)/deps/cogl ${COGL_PANGO_DEP_CFLAGS}"
LBA_COGL_LIBS="${COGL_PANGO_DEP_LIBS} ${COGL_DEP_LIBS} -L\$(top_srcdir)/deps/cogl/cogl/.libs/ -lcogl -L\$(top_srcdir)/deps/cogl/cogl-pango/.libs/ -lcogl-pango"
AC_SUBST(LBA_COGL_CFLAGS)
AC_SUBST(LBA_COGL_LIBS)

AC_CONFIG_FILES([
Makefile
deps/Makefile
deps/meson-based/Makefile
bombolla/Makefile
bombolla/core/Makefile
bombolla/lba/Makefile
bombolla/lba/remote-object/Makefile
bombolla/lba/python/Makefile
bombolla/lba/js/Makefile
bombolla/shell/Makefile
bombolla/base/Makefile
examples/Makefile
examples/vala/Makefile
tests/Makefile
])
AC_OUTPUT
