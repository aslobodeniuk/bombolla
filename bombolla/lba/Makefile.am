lib_LTLIBRARIES =

# GlutWindow
lib_LTLIBRARIES += libGlutWindow.la
libGlutWindow_la_DEPENDENCIES = \
$(top_builddir)/bombolla/base/liblba-basewindow.la \
$(top_builddir)/bombolla/base/liblba-base-opengl-interface.la
libGlutWindow_la_SOURCES = GlutWindow.c
libGlutWindow_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
libGlutWindow_la_LIBADD = $(GOBJ_LIBS) -lGLU -lGLEW -lglut -lGL -L$(top_builddir)/bombolla/base/.libs/ -llba-basewindow -llba-base-opengl-interface
libGlutWindow_la_LDFLAGS = $(PLUGIN_LDFLAGS)

# GLcube
lib_LTLIBRARIES += libGLcube.la
libGLcube_la_DEPENDENCIES = \
$(top_builddir)/bombolla/base/liblba-base3d.la \
$(top_builddir)/bombolla/base/liblba-basegl3d.la \
$(top_builddir)/bombolla/base/liblba-basedrawable.la \
$(top_builddir)/bombolla/base/liblba-base-opengl-interface.la

libGLcube_la_SOURCES = GLcube.c
libGLcube_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
libGLcube_la_LIBADD = $(GOBJ_LIBS) -L$(top_builddir)/bombolla/base/.libs/ -llba-basedrawable -llba-base-opengl-interface -llba-base3d -llba-basegl3d
libGLcube_la_LDFLAGS = $(PLUGIN_LDFLAGS)

# GLlight
lib_LTLIBRARIES += libGLlight.la
libGLlight_la_DEPENDENCIES = \
$(top_builddir)/bombolla/base/liblba-base3d.la \
$(top_builddir)/bombolla/base/liblba-basegl3d.la \
$(top_builddir)/bombolla/base/liblba-basedrawable.la \
$(top_builddir)/bombolla/base/liblba-base-opengl-interface.la

libGLlight_la_SOURCES = GLlight.c
libGLlight_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
libGLlight_la_LIBADD = $(GOBJ_LIBS) -L$(top_builddir)/bombolla/base/.libs/ -llba-basedrawable -llba-base-opengl-interface -llba-base3d -llba-basegl3d
libGLlight_la_LDFLAGS = $(PLUGIN_LDFLAGS)

# GLtexture
lib_LTLIBRARIES += libGLtexture.la
libGLtexture_la_DEPENDENCIES = \
$(top_builddir)/bombolla/base/liblba-base2d.la \
$(top_builddir)/bombolla/base/liblba-basegl2d.la \
$(top_builddir)/bombolla/base/liblba-basedrawable.la \
$(top_builddir)/bombolla/base/liblba-base-opengl-interface.la

libGLtexture_la_SOURCES = GLtexture.c
libGLtexture_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
libGLtexture_la_LIBADD = $(GOBJ_LIBS) -L$(top_builddir)/bombolla/base/.libs/ -llba-basedrawable -llba-base-opengl-interface -llba-base2d -llba-basegl2d
libGLtexture_la_LDFLAGS = $(PLUGIN_LDFLAGS)

# HTTPServer
lib_LTLIBRARIES += libhttpserver.la
libhttpserver_la_SOURCES = httpserver.c
libhttpserver_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS) $(SOUP_CFLAGS)
libhttpserver_la_LIBADD = $(GOBJ_LIBS) $(SOUP_LIBS)
libhttpserver_la_LDFLAGS = $(PLUGIN_LDFLAGS)

# FooBarParser
lib_LTLIBRARIES += libFooBarParser.la
libFooBarParser_la_SOURCES = FooBarParser.c
libFooBarParser_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS)
libFooBarParser_la_LIBADD = $(GOBJ_LIBS)
libFooBarParser_la_LDFLAGS = $(PLUGIN_LDFLAGS)

EXTRA_DIST = ../../autogen.sh
