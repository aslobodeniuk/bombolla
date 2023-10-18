bombolla_basewindow = shared_library('lba-basewindow', 'lba-basewindow.c',
              dependencies: bombolla_dep)

bombolla_basedrawable = shared_library('lba-basedrawable', 'lba-basedrawable.c',
              dependencies: bombolla_dep)

bombolla_icogl = shared_library('lba-icogl', 'icogl.c',
                                dependencies: [bombolla_dep, cogl_dep],
                                c_args: ['-DCOGL_ENABLE_EXPERIMENTAL_2_0_API'])
