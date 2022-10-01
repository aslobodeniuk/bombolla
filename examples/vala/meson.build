vala_example_test_dep = declare_dependency(
  sources: files(['plugin-in-vala.vala', 'register-plugin.c']),
  compile_args: ['-Wno-unused-variable'],
  dependencies: bombolla_dep
)

vala_example = library(
  'lba-vala-example',
  dependencies: vala_example_test_dep
)

subdir('test')
