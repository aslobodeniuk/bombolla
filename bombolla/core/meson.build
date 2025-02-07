src = files(['lba-core.c',
	     'lba-commands-fund.c',
	     'lba-boxed.c',
	     'lba-expr-node.c',
	     'lba-expr-parser.c',
             'commands/lba-commands.c',
             'commands/lba-command-set.c'])

# To share statically with the test that will build it with asan
bombolla_core_dep = declare_dependency(
  sources: src,
  dependencies: [bombolla_dep, gmodule_dep],
  link_with: [lba_base]
)

bombolla_core = shared_library('bombolla-core',
                               dependencies: bombolla_core_dep
                              )

subdir ('tests')
