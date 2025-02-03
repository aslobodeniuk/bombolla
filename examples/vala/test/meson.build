exe = executable(
  'test-plugin-in-vala', 'test-plugin-in-vala.c',
  dependencies : [bombolla_core_dep, vala_example_test_dep, asan_dep],
)

env = environment()
env.set ('LBA_PLUGINS_PATH', meson.project_build_root())
env.set ('G_SLICE', 'always-malloc')
  
test('vala', exe, env: env)
