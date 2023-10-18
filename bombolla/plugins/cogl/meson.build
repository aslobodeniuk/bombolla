cogl_dep = declare_dependency (
  compile_args: ['-DCOGL_ENABLE_EXPERIMENTAL_2_0_API'],
  dependencies: [dependency('cogl-2.0-experimental')]
)

cogl_pango_dep = declare_dependency (
  dependencies: [cogl_dep, dependency('cogl-pango-2.0-experimental')]
)

subdir('base')
subdir('mutogenes')

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
