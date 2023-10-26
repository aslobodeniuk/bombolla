lba_base = shared_library('lba-base',
                          include_directories : [include_directories('.')],
                          dependencies: [dependency('gobject-2.0', version: '>=2.58'), gmo_dep, bombolla_dep],
                          sources: files(['i2d.c', 'i3d.c']),
                         )

bombolla_basewindow = shared_library('lba-basewindow', 'lba-basewindow.c',
              dependencies: bombolla_dep)

bombolla_basedrawable = shared_library('lba-basedrawable', 'lba-basedrawable.c',
              dependencies: bombolla_dep)

subdir('mutogenes')
