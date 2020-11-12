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

# Base3d
lib_LTLIBRARIES += liblba-base3d.la
noinst_HEADERS += lba-base3d.h
liblba_base3d_la_DEPENDENCIES = \
$(top_builddir)/bombolla/base/liblba-basedrawable.la

liblba_base3d_la_SOURCES = lba-base3d.c
liblba_base3d_la_CFLAGS = $(BASECLASS_CFLAGS)
liblba_base3d_la_LIBADD = $(BASECLASS_LIBS) -llba-basedrawable

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

# BaseOpenGLInterface
lib_LTLIBRARIES += liblba-base-opengl-interface.la
noinst_HEADERS += lba-base-opengl-interface.h
liblba_base_opengl_interface_la_SOURCES = lba-base-opengl-interface.c
liblba_base_opengl_interface_la_CFLAGS = $(BASECLASS_CFLAGS)
liblba_base_opengl_interface_la_LIBADD = $(BASECLASS_LIBS)

EXTRA_DIST = ../../autogen.sh
