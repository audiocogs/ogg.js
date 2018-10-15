#!/bin/bash
set -e

# configure libogg
cd ogg
if [ ! -f configure ]; then
  # generate configuration script
  ./autogen.sh

  # -O20 and -04 cause problems
  # see https://github.com/kripken/emscripten/issues/264
  sed -i 's/-O20/-O2/g' configure
  sed -i 's/-O4/-O2/g' configure
fi

# finally, run configuration script
emconfigure ./configure --prefix="`pwd`" --disable-shared --enable-static

# compile libogg
emmake make
emmake make -i install

# compile wrapper
cd ..
mkdir -p build
emcc -O3 -I ogg/include -s WASM=0 -s RESERVED_FUNCTION_POINTERS=50 -s EXPORTED_FUNCTIONS="['_AVOggInit', '_AVOggRead', '_AVOggDestroy']" src/ogg.c ogg/lib/libogg.a -o build/libogg.js --memory-init-file 0
echo "module.exports = Module" >> build/libogg.js
