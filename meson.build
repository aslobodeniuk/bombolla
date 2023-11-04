project('bombolla', 'c', 'vala',
        version: '0.0.0',
        meson_version: '>= 0.58',
        default_options: ['buildtype=debug',])

subdir('friendly-libs')

bombolla_dep = declare_dependency(
  include_directories : [include_directories('.'),
                         include_directories('friendly-libs')],
  dependencies: [dependency('gobject-2.0', version: '>=2.58'),
                 dependency('gio-2.0'),
                 gmo_dep],
  compile_args: ['-Wall', '-Werror', '-Wfatal-errors']
)

asan_dep = declare_dependency(
  compile_args: ['-fsanitize=address', '-fno-omit-frame-pointer', '-g', '-O0', '-fno-inline'],
  link_args: ['-fsanitize=address'],
)

cc = meson.get_compiler('c')
gmodule_dep = dependency('gmodule-2.0')

subdir('bombolla')
subdir('examples')

run_command('sh', '-c', 'cp Firulais .git/hooks/pre-commit')

devenv = environment()
devenv.prepend('PATH', meson.current_build_dir() / 'bombolla' / 'tools' / 'shell')
devenv.prepend('LBA_PLUGINS_PATH', meson.current_build_dir() / 'bombolla')
meson.add_devenv(devenv)
