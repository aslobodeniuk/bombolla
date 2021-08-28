bin_PROGRAMS = bombolla-shell

bombolla_shell_SOURCES = bombolla-shell.c \
  bombolla-core.c \
  commands/bombolla-commands.c \
  commands/bombolla-command-on.c \
  commands/bombolla-command-set.c

bombolla_shell_CFLAGS = $(GOBJ_CFLAGS)
bombolla_shell_LDADD = $(GOBJ_LIBS) $(GMODULE_LIBS)

EXTRA_DIST = ../../autogen.sh
