exe = executable('bombolla-core-test', 'bombolla-core-test.c',
                 dependencies : [bombolla_core_dep, asan_dep]
                )

env = environment()
core_plugin_path = join_paths(meson.project_build_root(), 'bombolla', 'core')
env.set ('LBA_PLUGINS_PATH', core_plugin_path)
  
test('core', exe, env: env)
