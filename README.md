![Ubuntu build & unit tests](https://github.com/aslobodeniuk/bombolla/actions/workflows/ubuntu.yml/badge.svg)

# bombolla
![Screenshot](demo.png)

Bombolla is just a bunch of plugins used to play with the code and some technologies.

## Building

```bash
sudo apt-get install -qq libglib2.0-dev libsoup2.4-dev libgjs-dev libpython3-dev pkg-config indent valac ninja-build libcogl-pango-dev python3-pip python3-setuptools python3-wheel
sudo pip install meson

git clone git@github.com:aslobodeniuk/bombolla.git

meson build
ninja -C build
```

## Running an example

```bash
build/bombolla/tools/shell/bombolla-shell -p build/bombolla/plugins/cogl -i examples/cogl
```

## FAQ

### Can I write a plugin in python?

Yes, check
```bash
cat examples/plugin-in-python

build/bombolla/tools/shell/bombolla-shell -p build/bombolla/plugins/python/ -i examples/plugin-in-python
```

### ¿Can I call bombolla scripts from python?

Yes, check
```bash
cat examples/python/use-from-python.py
```

### ¿Can I write a plugin in JavaScript?

You can, but for now it's crashing right after usage, because JS's garbage collector cleanups our plugin..

check
```bash
cat examples/js/plugin-in-js.js
```

### ¿Can I write a plugin in Vala?

Yes, check
```bash
cat examples/vala/plugin-in-vala.vala
```
