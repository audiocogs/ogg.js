#!/bin/bash

# configure libogg
cd libogg
if [ ! -f configure ]; then
  # generate configuration script
  ./autogen.sh
  
  # -O20 and -04 cause problems
  # see https://github.com/kripken/emscripten/issues/264
  sed -i '' 's/-O20/-O2/g' configure
  sed -i '' 's/-O4/-O2/g' configure
  
  # finally, run configuration script
  emconfigure ./configure --prefix="`pwd`" --disable-static
fi

# compile libogg
emmake make
emmake make install

# compile wrapper
cd ..
mkdir -p build
emcc -O3 -s RESERVED_FUNCTION_POINTERS=50 -s EXPORTED_FUNCTIONS="['_AVOggInit', '_AVOggRead', '_AVOggDestroy']" -I libogg/include -Llibogg/lib -logg src/ogg.c -o build/libogg.js
echo "module.exports = Module" >> build/libogg.js
