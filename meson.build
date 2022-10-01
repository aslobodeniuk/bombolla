project('bombolla', 'c', 'vala',
        version: '0.0.0',
        meson_version: '>= 0.56',
        default_options: ['buildtype=debug',])

bombolla_dep = declare_dependency(
  include_directories : [include_directories('.')],
  dependencies: dependency('gobject-2.0'),
  compile_args: ['-Wall', '-Werror']
)

asan_dep = declare_dependency(
  compile_args: ['-fsanitize=address', '-fno-omit-frame-pointer', '-g'],
  link_args: ['-fsanitize=address'],
)

cc = meson.get_compiler('c')
gmodule_dep = dependency('gmodule-2.0')

subdir('bombolla')
subdir('examples')

run_command('sh', '-c', 'cp jihn .git/hooks/pre-commit')
