ogg.js
======

An Ogg demuxer for [Aurora.js](https://github.com/audiocogs/aurora.js), compiled with Emscripten.

## Usage

ogg.js isn't very useful without some plugins. Some examples of audio formats that can be stored in an 
Ogg container include:

* [Vorbis](https://github.com/audiocogs/vorbis.js)
* [Opus](https://github.com/audiocogs/opus.js)
* [FLAC](https://gitub.com/audiocogs/flac.js)

If you're using ogg.js and its associated codecs in a browser, you should either build your application 
with Browserify or include the prebuilt versions of aurora.js, ogg.js and the codecs you want to support
in your HTML page as `<script>` tags.
  
If you're using Node, you can simply require `av`, `ogg.js`, and the codecs you want to use.

## Building from source

1. Install [Emscripten](https://github.com/kripken/emscripten/wiki/Emscripten-SDK).
2. Clone git submodules
3. Run `npm install` to install dependencies
4. Run `make libogg` to configure and build libogg and the C wrapper. Run this again whenever you make changes to the C wrapper or a new version of libogg is released.
5. Run `make browser` to generate a browser version of ogg.js, or use browserify to build your application.

## License

libogg is available under its existing license, and the JavaScript and C wrapper code in this repo
for Aurora.js is licensed under MIT.
