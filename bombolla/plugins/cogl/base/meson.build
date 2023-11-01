bombolla_icogl = shared_library('lba-icogl', 'icogl.c',
                                dependencies: [bombolla_dep, cogl_dep])

lba_cogl = shared_library('lba-cogl',
               'lba-cogl.c',
               dependencies: [bombolla_dep, cogl_pango_dep],
               link_with: [lba_base, bombolla_icogl, bombolla_basedrawable]
              )
