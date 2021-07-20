#!/bin/sh
# you can either set the environment variables AUTOCONF, AUTOHEADER, AUTOMAKE,
# ACLOCAL, AUTOPOINT and/or LIBTOOLIZE to the right versions, or leave them
# unset and get the defaults

if test ! -f cogl/autogen.sh;
then
  echo "+ Setting up COGL submodule"
  git submodule update --init
fi

cd cogl
echo "patching cogl to build on Ubuntu focal.."
git apply ../patches/0001-fix-build-on-Ubuntu-focal.patch 2>/dev/null && echo "ok"
NOCONFIGURE=1 ./autogen.sh
cd ..

autoreconf --force --install || {
 echo 'autogen.sh failed';
 exit 1;
}

./configure --prefix=$(pwd)/build $@ || {
 echo 'configure failed';
 exit 1;
}

echo
echo "Now type 'make' to compile this module."
echo
