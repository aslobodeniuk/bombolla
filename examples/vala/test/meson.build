exe = executable(
  'test-plugin-in-vala', 'test-plugin-in-vala.c',
  dependencies : [bombolla_core_dep, vala_example_test_dep, asan_dep],
)

env = environment()
examples_plugin_path = join_paths(meson.project_build_root(), 'examples', 'vala')
env.set ('LBA_PLUGINS_PATH', examples_plugin_path)
env.set ('G_SLICE', 'always-malloc')
  
test('vala', exe, env: env)
