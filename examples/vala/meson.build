vala_example = library('lba-vala-example',
  sources: files(['plugin-in-vala.vala', 'register-plugin.c']),
  c_args: ['-Wno-unused-variable'],
  dependencies: bombolla_dep
)

subdir('test')
