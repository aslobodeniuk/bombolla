project('bombolla', 'c', 'vala',
        version: '0.0.0',
        meson_version: '>= 0.56',
        default_options: ['buildtype=debug',])

bombolla_dep = declare_dependency(
  include_directories : [include_directories('.')],
  dependencies: dependency('gobject-2.0'),
  compile_args: ['-Wall', '-Werror']
)

cc = meson.get_compiler('c')
gmodule_dep = dependency('gmodule-2.0')

run_command('sh', '-c', 'cp jihn .git/hooks/pre-commit')

subdir('bombolla')
subdir('examples')
