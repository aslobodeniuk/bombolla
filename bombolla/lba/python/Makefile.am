MAINTAINERCLEANFILES = Makefile.in

# FIXME: get python flags interactively using python3-config --ldflags --embed
# Also it seems that we need a plugin per version
PYTHON_CFLAGS=-I/usr/include/python3.8 -I/usr/include/python3.8  -Wno-unused-result -Wsign-compare -g -fdebug-prefix-map=/build/python3.8-CoVRmP/python3.8-3.8.10=. -specs=/usr/share/dpkg/no-pie-compile.specs -fstack-protector -Wformat -Werror=format-security  -DNDEBUG -g -fwrapv -O3 -Wall

PYTHON_LIBS=-L/usr/lib/python3.8/config-3.8-x86_64-linux-gnu -L/usr/lib -lpython3.8 -lcrypt -lpthread -ldl  -lutil -lm -lm

lib_LTLIBRARIES = libLbaPythonScanner.la

libLbaPythonScanner_la_SOURCES = LbaPythonScanner.c
libLbaPythonScanner_la_CFLAGS = $(GOBJ_CFLAGS) $(PLUGIN_CFLAGS) $(PYTHON_CFLAGS)
libLbaPythonScanner_la_LIBADD = $(GOBJ_LIBS)
libLbaPythonScanner_la_LDFLAGS = $(PLUGIN_LDFLAGS) $(PYTHON_LIBS)

EXTRA_DIST = ../../autogen.sh
