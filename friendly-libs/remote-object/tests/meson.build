exe = executable('robj-test-protocol', ['robj-test-protocol.c'],
                 dependencies : [robj_internal_dep, asan_dep])

env = environment()
env.set ('G_SLICE', 'always-malloc')

test('robj-test-protocol', exe, env: env)
