# To share statically with the test that will build it with asan
bmixin_internal_dep = declare_dependency (
  sources: files(['bmixin.c']),
  dependencies: [dependency('gobject-2.0', version: '>=2.58')],
  compile_args: ['-Wall', '-Werror', '-Wfatal-errors', '-Wimplicit-fallthrough'],
  include_directories : [include_directories('..')]
)
# note: __attribute__ ((fallthrough));
bmixin_lib = shared_library('bmixin',
                            dependencies: bmixin_internal_dep,
                           )

bmixin_dep = declare_dependency (
  link_with: bmixin_lib,
  include_directories : [include_directories('..')]
)

gnome = import('gnome')

gnome.gtkdoc('bmixin',
             main_sgml: 'bmixin-docs.xml',
             mkdb_args: ['--source-suffixes=h,cc',
                         '--xml-mode',
                         '--output-format=xml',
             ],
             content_files: ['usermanual.xml'],
             html_assets: ['../../doc/logo/light.svg'],
             src_dir: ['friendly-libs/bmixin'],
             dependencies: bmixin_dep,
             install: true)
