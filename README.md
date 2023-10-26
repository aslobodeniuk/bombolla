![Ubuntu build & unit tests](https://github.com/aslobodeniuk/bombolla/actions/workflows/ubuntu.yml/badge.svg)
# bombolla
![logo](doc/logo/dark1.svg)

Currently bombolla is a space to play with technologies.
It is some kind of object-oriented shell/DSL that allows to manipulate plugins.

![Screenshot](doc/demo.png)
## Building

```bash
sudo apt-get install -qq libglib2.0-dev libsoup2.4-dev libgjs-dev libpython3-dev pkg-config indent valac ninja-build libcogl-pango-dev python3-pip python3-setuptools python3-wheel
sudo pip install meson

git clone git@github.com:aslobodeniuk/bombolla.git
cd bombolla
./Firulais
```

## Running an example

```bash
build/bombolla/tools/shell/bombolla-shell -p build/bombolla/ -i examples/cogl
```

## FAQ

### Can I write a plugin in python?

Yes, check
```bash
cat examples/plugin-in-python

build/bombolla/tools/shell/bombolla-shell -i examples/plugin-in-python
```

### ¿Can I call bombolla scripts from python?

Yes, check
```bash
cat examples/python/use-from-python.py
python3 examples/python/use-from-python.py
```

### ¿Can I write a plugin in JavaScript?

Yes, check
```bash
cat examples/js/lba-plugin-in-js.js
build/bombolla/tools/shell/bombolla-shell -i examples/plugin-in-js
```

### ¿Can I write a plugin in Vala?

Yes, check
```bash
cat examples/vala/plugin-in-vala.vala
build/bombolla/tools/shell/bombolla-shell -i examples/plugin-in-vala
```

### So, ok I can write in many languages. Can I connect them in between?

Yes. There's some example in which we create some rough app, that parses an RSS feed,
renders it natively with Cogl, but also it's controlled by a UI written in JS.
(Warning, Frankenstein example is still very rough)
```bash
LBA_PYTHON_PLUGINS_PATH=$(pwd)/examples/frankenstein-news LBA_JS_PLUGINS_PATH=$(pwd)/examples/js gdb --args build/bombolla/tools/shell/bombolla-shell -p build/bombolla/ -i examples/frankenstein-news/frank.lba
```
