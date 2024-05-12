exe = executable('bmixin-test-wolf', ['bmixin-test-wolf.c',
                                      'wolf.c',
                                      'dog.c',
                                      'hairy.c',
                                      'forest_matter.c',
                                      'animal.c',
                                      'forest_animal.c',
                                      'has_tail.c',
                                      'wolf2.c',
                                     ],
                 dependencies : [bmixin_internal_dep, asan_dep]
                )

env = environment()
env.set ('G_SLICE', 'always-malloc')

test('bmixin-test-wolf', exe, env: env)
