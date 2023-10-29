exe = executable('lba-async-string-input-test', 'lba-async-string-input-test.c',
                 dependencies : [bombolla_core_dep, gmo_dep, asan_dep],
                 link_with : [async_string_input]
                )

env = environment()
core_plugin_path = join_paths(meson.project_build_root(), 'bombolla', 'plugins', 'js')
env.set ('LBA_PLUGINS_PATH', core_plugin_path)
env.set ('G_SLICE', 'always-malloc')

test('js', exe, env: env)
