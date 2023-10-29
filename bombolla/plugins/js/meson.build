shared_library('lba-gjs',
               'lba-gjs.c',
               dependencies: [bombolla_dep, dependency ('gjs-1.0')]
              )

async_string_input = shared_library('lba-async-string-input',
               'lba-async-string-input.c',
               dependencies: [bombolla_dep, gmo_dep]
              )

subdir ('tests')
