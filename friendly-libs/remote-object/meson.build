robj_internal_dep = declare_dependency (
  sources: files([#'robj-portal.c',
                  'robj-protocol.c', 'robj-brain.c']),
  dependencies: [dependency('gobject-2.0', version: '>=2.58')],
  compile_args: ['-Wall', '-Werror', '-Wfatal-errors', '-Wimplicit-fallthrough'],
  include_directories : [include_directories('..')]
)

robj_lib = shared_library('lba-remote-object',
                            dependencies: robj_internal_dep,
                           )

robj_dep = declare_dependency (
  link_with: robj_lib,
  include_directories : [include_directories('..')]
)

subdir('tests')
