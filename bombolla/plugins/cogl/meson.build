cogl_dep = declare_dependency (
  compile_args: ['-DCOGL_ENABLE_EXPERIMENTAL_2_0_API'],
  dependencies: [dependency('cogl-2.0-experimental')]
)

cogl_pango_dep = declare_dependency (
  dependencies: [cogl_dep, dependency('cogl-pango-2.0-experimental')]
)

subdir('base')

# TODO: link all that into one plugin

shared_library('lba-cogl-window',
               'lba-cogl-window.c',
               dependencies: [bombolla_dep, cogl_dep],
               link_with: bombolla_basewindow
              )

shared_library('lba-cogl-texture',
               'lba-cogl-texture.c',
               dependencies: [bombolla_dep, cogl_dep],
               link_with: bombolla_icogl
              )

lba_cogl = shared_library('lba-cogl',
               'lba-cogl.c',
               dependencies: [bombolla_dep, cogl_pango_dep],
               link_with: [lba_base, bombolla_icogl, bombolla_basedrawable]
              )

shared_library('lba-cogl-cube',
               'lba-cogl-cube.c',
               dependencies: [bombolla_dep, cogl_pango_dep],
               link_with: [lba_base, bombolla_icogl, lba_cogl, lba_3d]
              )

shared_library('lba-cogl-label',
               'lba-cogl-label.c',
               dependencies: [bombolla_dep, cogl_pango_dep],
               link_with: [lba_base, bombolla_icogl, lba_cogl, lba_2d]
              )
