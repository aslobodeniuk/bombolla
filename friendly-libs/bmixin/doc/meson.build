gnome = import('gnome')

gnome.gtkdoc('bmixin',
             main_sgml: 'bmixin-docs.xml',
             mkdb_args: ['--source-suffixes=h,cc',
                         '--xml-mode',
                         '--output-format=xml',
             ],
             content_files: ['usermanual.xml'],
             html_assets: ['../../../doc/logo/light.svg'],
             src_dir: ['friendly-libs/bmixin'],
             dependencies: bmixin_dep,
             install: true)
