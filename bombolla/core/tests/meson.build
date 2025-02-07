exe = executable('bombolla-core-test', 'bombolla-core-test.c',
                 dependencies : [bombolla_core_dep, asan_dep]
                )

env = environment()
core_plugin_path = join_paths(meson.project_build_root(), 'bombolla', 'plugins', 'core')
env.set ('LBA_PLUGINS_PATH', core_plugin_path)
env.set ('G_SLICE', 'always-malloc')

test('core', exe, env: env)
