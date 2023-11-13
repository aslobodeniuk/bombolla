shared_library('lba-python',
               'lba-python.c',
               dependencies: [bombolla_dep, gmodule_dep, cc.find_library ('dl'),
                              dependency ('python3').partial_dependency(compile_args : true)],
               link_with: [lba_base]
              )
