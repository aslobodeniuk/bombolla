# To share statically with the test that will build it with asan
gmo_dep = declare_dependency (
  sources: files(['gmo.c']),
  dependencies: [dependency('gobject-2.0', version: '>=2.58')],
  compile_args: ['-Wall', '-Werror', '-Wfatal-errors']
)
