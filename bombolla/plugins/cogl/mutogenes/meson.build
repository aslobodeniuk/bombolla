lba_cogl = shared_library('lba-mutogene-cogl',
               'lba-mutogene-cogl.c',
               dependencies: [bombolla_dep, cogl_pango_dep],
               link_with: [lba_base, bombolla_icogl, bombolla_basedrawable]
              )

shared_library('lba-mutogene-cogl-cube',
               'lba-mutogene-cogl-cube.c',
               dependencies: [bombolla_dep, cogl_pango_dep],
               link_with: [lba_base, bombolla_icogl, lba_cogl, lba_3d]
              )

shared_library('lba-cogl-label',
               'lba-cogl-label.c',
               dependencies: [bombolla_dep, cogl_pango_dep],
               link_with: [lba_base, bombolla_icogl, lba_cogl, lba_2d]
              )
