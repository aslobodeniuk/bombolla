exe = executable('test-plugin-in-vala', 'test-plugin-in-vala.c',
                 dependencies : [bombolla_dep],
                 link_with: [bombolla_core, vala_example]
                )

env = environment()
examples_plugin_path = join_paths(meson.project_build_root(), 'examples', 'vala')
env.set ('LBA_PLUGINS_PATH', examples_plugin_path)
  
test('vala', exe, env: env)
