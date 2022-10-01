src = files(['bombolla-core.c',
             'bombolla-commands.c',
             'bombolla-command-on.c',
             'bombolla-command-set.c'])

bombolla_core = shared_library('bombolla-core',
               sources: src,
               dependencies: [bombolla_dep, gmodule_dep]
              )

subdir ('tests')
