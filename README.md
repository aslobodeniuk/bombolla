![Ubuntu build & unit tests](https://github.com/aslobodeniuk/bombolla/actions/workflows/ubuntu.yml/badge.svg)

# bombolla
![Screenshot](demo.png)

Bombolla is just a bunch of plugins used to play with the code and some technologies.
This project doesn't make sense at all actually.
But we do have some spinning cubes (almost).

## Hacking

If we say that a normal unix executable is a plugin, that is being loaded by an operating
system, then we can describe it's paradigm as "plugin = function".
Or to be more precise "every executable is a 'main' function plus one input and two output byte streams" (we mean stdin, stdout and strerr ofcource).

And we can use pipelining to connect this executables (= plugins = functions), and redirect their output.

```bash
sudo cat /my-cats-in-root-dir.txt | grep "Pancake" > /dev/usb/lp0
```

Bombolla's goal is to extend this paradigm by replacing "plugin = function" by "plugin = object". So, it's just a bunch of plugins. Http servers, OpenGL cubes, Python interpretators, etc. And one of possible ways (but not the only) to load and controll this plugins is through the "bombolla-shell".

## Developer's dreams for next 10-20 years

* To finally introduce bytestreams.
* To finish the idea of "Remote object" - a mirror of GObject that is connected to it's origin through the bytestream. So all the signals and properties are sincronized

* To implement some way to abstract grop of objects into a higher level one.
* To implement "states" - let the plugins register their own states (as much as they want) and state transitions therefore, that will be executed from the GMainLoop.


In the end people of the future will be connecting all this objects with some visual tool, abstract them with this tool into a higher level object, connecting them together by https, etc.

## Building

```bash
sudo apt-get install -y libglib2.0-dev libsoup2.4-dev libpango1.0-dev libgjs-dev libpython3-dev automake pkg-config autopoint gtk-doc-tools libx11-xcb-dev freeglut3-dev libxfixes-dev libxdamage-dev libxcomposite-dev libxrandr-dev libglew-dev indent valac meson ninja-build

git clone git@github.com:aslobodeniuk/bombolla.git

./autogen.sh

make install
```

## Running an example

```bash
build/bin/bombolla-shell -p build/lib/ -i examples/cogl
```

## FAQ

### Can I write a plugin in python?

Oh yes, my friend, you can!
see
```bash
cat examples/plugin-in-python

build/bin/bombolla-shell -i examples/plugin-in-python
```

### ¿Can I call bombolla scripts from python?

Oh yes, my friend, you can!
see
```bash
cat examples/python/use-from-python.py
```

### ¿Can I write a plugin in JavaScript?

You can, but for now it's crashing right after usage, because JS's garbage collector cleanups our plugin..

see
```bash
cat examples/js/plugin-in-js.js
```

### ¿Can I write a plugin in Vala?

YES!!!

see
```bash
cat examples/vala/plugin-in-vala.vala
```

### ¿Does Bombolla have bugs?

Yes, currently it's a heaven of bugs. Bombolla follows a very nice model of development to just go forward with a feature ignoring everything else, not freeing the memory, etc.

Keep in mind that speed of the development is limited to personal interest of some people, so "we" prefer to have features and bugs instead of the contrary.

But we already have few unit tests built with an address sanitizer.
Depending on the mood some of the bugs or leaks may get fixed from time to time.
