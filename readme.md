ogg.js
======

libogg compiled to JavaScript with Emscripten for [Aurora.js](https://github.com/audiocogs/aurora.js).

## Building

1. Install [Emscripten](https://github.com/kripken/emscripten/wiki/Emscripten-SDK).
2. Clone git submodules
3. Run `npm install` to install dependencies
4. Run `make libogg` to configure and build libogg and the C wrapper. Run this again whenever you make changes to the C wrapper or a new version of libogg is released.
5. Run `make browser` to generate a JavaScript file with libogg, or use browserify to build your application.

## License

libogg is available under its existing license, and the JavaScript and C wrapper code in this repo
for Aurora.js is licensed under MIT.
