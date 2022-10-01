# FIXME: get python flags interactively using python3-config --ldflags --embed
# Also it seems that we need a plugin per version
python_cflags = ['-I/usr/include/python3.8',
                 '-Wno-unused-result',
                 '-Wsign-compare',
                 '-fdebug-prefix-map=/build/python3.8-CoVRmP/python3.8-3.8.10=.',
                 '-specs=/usr/share/dpkg/no-pie-compile.specs',
                 '-fstack-protector',
                 '-DNDEBUG',
                 '-fwrapv']

python_deps = [cc.find_library('python3.8')]
python_deps += cc.find_library('crypt')
python_deps += cc.find_library('pthread')
python_deps += cc.find_library('dl')
python_deps += cc.find_library('util')
python_deps += cc.find_library('m')

shared_library('lba-python-scanner',
               'lba-python-scanner.c',
               dependencies: [bombolla_dep, python_deps, gmodule_dep],
               c_args: python_cflags,
              )
