noinst_HEADERS =
lib_LTLIBRARIES =

# BaseWindow
lib_LTLIBRARIES += liblba-basewindow.la
noinst_HEADERS += lba-basewindow.h
liblba_basewindow_la_SOURCES = lba-basewindow.c
liblba_basewindow_la_CFLAGS = $(BASECLASS_CFLAGS)
liblba_basewindow_la_LIBADD = $(BASECLASS_LIBS)

# BaseDrawable
lib_LTLIBRARIES += liblba-basedrawable.la
noinst_HEADERS += lba-basedrawable.h
liblba_basedrawable_la_SOURCES = lba-basedrawable.c
liblba_basedrawable_la_CFLAGS = $(BASECLASS_CFLAGS)
liblba_basedrawable_la_LIBADD = $(BASECLASS_LIBS)

# BaseOpenGLInterface
lib_LTLIBRARIES += liblba-base-opengl-interface.la
noinst_HEADERS += lba-base-opengl-interface.h
liblba_base_opengl_interface_la_SOURCES = lba-base-opengl-interface.c
liblba_base_opengl_interface_la_CFLAGS = $(BASECLASS_CFLAGS)
liblba_base_opengl_interface_la_LIBADD = $(BASECLASS_LIBS)

# Base3d
lib_LTLIBRARIES += liblba-base3d.la
noinst_HEADERS += lba-base3d.h
liblba_base3d_la_DEPENDENCIES = \
$(top_builddir)/bombolla/base/liblba-basedrawable.la

liblba_base3d_la_SOURCES = lba-base3d.c
liblba_base3d_la_CFLAGS = $(BASECLASS_CFLAGS)
liblba_base3d_la_LIBADD = $(BASECLASS_LIBS) -llba-basedrawable

# BaseCogl3d
lib_LTLIBRARIES += liblba-base-cogl3d.la
noinst_HEADERS += lba-base-cogl3d.h
liblba_base_cogl3d_la_DEPENDENCIES = \
$(top_builddir)/bombolla/base/liblba-basedrawable.la \
$(top_builddir)/bombolla/base/liblba-base3d.la

liblba_base_cogl3d_la_SOURCES = lba-base-cogl3d.c
liblba_base_cogl3d_la_CFLAGS = -DCOGL_ENABLE_EXPERIMENTAL_2_0_API -I$(top_srcdir)/cogl \
    $(BASECLASS_CFLAGS)
liblba_base_cogl3d_la_LIBADD = $(BASECLASS_LIBS) -llba-basedrawable -llba-base3d $(COGL_DEP_LIBS) -L$(top_builddir)/cogl/cogl/.libs/ -lcogl

# Basegl3d
lib_LTLIBRARIES += liblba-basegl3d.la
noinst_HEADERS += lba-basegl3d.h
liblba_basegl3d_la_DEPENDENCIES = \
$(top_builddir)/bombolla/base/liblba-basedrawable.la \
$(top_builddir)/bombolla/base/liblba-base3d.la \
$(top_builddir)/bombolla/base/liblba-base-opengl-interface.la

liblba_basegl3d_la_SOURCES = lba-basegl3d.c
liblba_basegl3d_la_CFLAGS = $(BASECLASS_CFLAGS)
liblba_basegl3d_la_LIBADD = $(BASECLASS_LIBS) -llba-basedrawable -llba-base-opengl-interface -llba-base3d

# Base2d
lib_LTLIBRARIES += liblba-base2d.la
noinst_HEADERS += lba-base2d.h
liblba_base2d_la_DEPENDENCIES = \
$(top_builddir)/bombolla/base/liblba-basedrawable.la

liblba_base2d_la_SOURCES = lba-base2d.c
liblba_base2d_la_CFLAGS = $(BASECLASS_CFLAGS)
liblba_base2d_la_LIBADD = $(BASECLASS_LIBS) -llba-basedrawable

# Basegl2d
lib_LTLIBRARIES += liblba-basegl2d.la
noinst_HEADERS += lba-basegl2d.h
liblba_basegl2d_la_DEPENDENCIES = \
$(top_builddir)/bombolla/base/liblba-basedrawable.la \
$(top_builddir)/bombolla/base/liblba-base2d.la \
$(top_builddir)/bombolla/base/liblba-base-opengl-interface.la

liblba_basegl2d_la_SOURCES = lba-basegl2d.c
liblba_basegl2d_la_CFLAGS = $(BASECLASS_CFLAGS)
liblba_basegl2d_la_LIBADD = $(BASECLASS_LIBS) -llba-basedrawable -llba-base-opengl-interface -llba-base2d

EXTRA_DIST = ../../autogen.sh
