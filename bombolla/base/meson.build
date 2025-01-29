lba_base = shared_library('lba-base',
                          include_directories : [include_directories('.')],
                          dependencies: [bombolla_dep, bmixin_dep],
                          sources: files(['i2d.c', 'i3d.c', 'lba-module-scanner.c']),
                         )

bombolla_basewindow = shared_library('lba-basewindow', 'lba-basewindow.c',
              dependencies: bombolla_dep)

bombolla_basedrawable = shared_library('lba-basedrawable', 'lba-basedrawable.c',
              dependencies: bombolla_dep)

lba_3d = shared_library('lba-3d',
               'lba-3d.c',
               dependencies: [bombolla_dep],
               link_with: [lba_base]
              )

lba_2d = shared_library('lba-2d',
               'lba-2d.c',
               dependencies: [bombolla_dep],
               link_with: [lba_base]
              )

lba_picture = shared_library('lba-picture',
               'lba-picture.c',
               dependencies: [bombolla_dep],
               link_with: [lba_base]
              )

lba_async = shared_library('lba-async',
               'lba-async.c',
               dependencies: [bombolla_dep],
               link_with: [lba_base]
              )
