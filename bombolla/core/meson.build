src = files(['bombolla-core.c',
             'bombolla-commands.c',
             'bombolla-command-on.c',
             'bombolla-command-set.c'])

# To share statically with the test that will build it with asan
bombolla_core_dep = declare_dependency(
  sources: src,
  dependencies: [bombolla_dep, gmodule_dep, gmo_dep]
)

bombolla_core = shared_library('bombolla-core',
                               dependencies: bombolla_core_dep
                              )

subdir ('tests')
