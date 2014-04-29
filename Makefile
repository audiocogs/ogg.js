.FAKE : all clean cleanswf swf js jsdemo flash demo

all : jsdemo demo

js : build/ogvjs.js

flash : build/ogvswf.js

demo : build/demo/index.html

jsdemo : build/jsdemo/index.html

clean:
	rm -rf build
	rm -f libogg/configure
	rm -f libvorbis/configure
	rm -f libtheora/configure

build/js/root/lib/libogg.a : configureOgg.sh compileOggJs.sh
	test -d build || mkdir build
	./configureOgg.sh
	./compileOggJs.sh

build/js/root/lib/libvorbis.a : build/js/root/lib/libogg.a configureVorbis.sh compileVorbisJs.sh
	test -d build || mkdir build
	./configureVorbis.sh
	./compileVorbisJs.sh

build/js/root/lib/libtheoradec.a : build/js/root/lib/libogg.a configureTheora.sh compileTheoraJs.sh
	test -d build || mkdir build
	./configureTheora.sh
	./compileTheoraJs.sh

build/js/ogv-libs.js : src/ogv-libs.c src/ogv-libs-mixin.js build/js/root/lib/libogg.a build/js/root/lib/libtheoradec.a build/js/root/lib/libvorbis.a compileOgvJs.sh
	test -d build || mkdir build
	./compileOgvJs.sh

build/OgvJsCodec.js : src/OgvJsCodec.js.in build/js/ogv-libs.js
	 cpp -E -w -P -CC -nostdinc src/OgvJsCodec.js.in > build/OgvJsCodec.js

build/YCbCr-shaders.h : src/YCbCr-vertex.glsl src/YCbCr-fragment.glsl file2def.js
	test -d build || mkdir build
	node file2def.js src/YCbCr-vertex.glsl YCBCR_VERTEX_SHADER > build/YCbCr-shaders.h
	node file2def.js src/YCbCr-fragment.glsl YCBCR_FRAGMENT_SHADER >> build/YCbCr-shaders.h

build/YCbCrFrameSink.js : src/YCbCrFrameSink.js.in build/YCbCr-shaders.h
	 cpp -E -w -P -CC -nostdinc -Ibuild src/YCbCrFrameSink.js.in > build/YCbCrFrameSink.js

build/ogvjs.js : src/ogvjs.js.in src/StreamFile.js src/AudioFeeder.js src/YCbCr.js build/YCbCrFrameSink.js src/OgvJsPlayer.js build/OgvJsCodec.js
	 cpp -E -w -P -CC -nostdinc -Ibuild src/ogvjs.js.in > build/ogvjs.js

build/ogvjs.js.gz : build/ogvjs.js
	 7z -tgzip -mx=9 -so a dummy.gz build/ogvjs.js > build/ogvjs.js.gz || gzip -9 -c build/ogvjs.js > build/ogvjs.js.gz

# The player demo, with the JS and Flash builds
build/demo/index.html : src/demo/index.html.in src/demo/demo.css src/demo/demo.js src/demo/motd.js src/demo/minimal.html \
                        build/ogvjs.js build/ogvjs.js.gz \
                        src/dynamicaudio.swf build/ogv.swf build/ogvswf.js \
                        src/cortado.jar src/CortadoPlayer.js
	test -d build/demo || mkdir build/demo
	cpp -E -w -P -CC -nostdinc -DWITH_JS -DWITH_FLASH src/demo/index.html.in > build/demo/index.html
	
	cp src/demo/demo.css build/demo/demo.css
	cp src/demo/demo.js build/demo/demo.js
	cp src/demo/motd.js build/demo/motd.js
	cp src/dynamicaudio.swf build/demo/dynamicaudio.swf
	cp src/demo/minimal.html build/demo/minimal.html
	
	test -d build/demo/lib || mkdir build/demo/lib
	cp build/ogvjs.js build/demo/lib/ogvjs.js
	cp build/ogvjs.js.gz build/demo/lib/ogvjs.js.gz
	cp build/ogvswf.js build/demo/lib/ogvswf.js
	cp build/ogv.swf build/demo/lib/ogv.swf
	cp src/cortado.jar build/demo/lib/cortado.jar
	cp src/CortadoPlayer.js build/demo/lib/CortadoPlayer.js

# The player demo, JS only without the Flash build
build/jsdemo/index.html : src/demo/index.html.in src/demo/demo.css src/demo/demo.js src/demo/motd.js src/demo/minimal.html \
                          build/ogvjs.js build/ogvjs.js.gz src/dynamicaudio.swf \
                          src/cortado.jar src/CortadoPlayer.js
	test -d build/jsdemo || mkdir build/jsdemo
	cpp -E -w -P -CC -nostdinc -DWITH_JS -DWITHOUT_FLASH src/demo/index.html.in > build/jsdemo/index.html
	
	cp src/demo/demo.css build/jsdemo/demo.css
	cp src/demo/demo.js build/jsdemo/demo.js
	cp src/demo/motd.js build/jsdemo/motd.js
	cp src/dynamicaudio.swf build/jsdemo/dynamicaudio.swf
	cp src/demo/minimal.html build/jsdemo/minimal.html
	
	test -d build/jsdemo/lib || mkdir build/jsdemo/lib
	cp build/ogvjs.js build/jsdemo/lib/ogvjs.js
	cp build/ogvjs.js.gz build/jsdemo/lib/ogvjs.js.gz
	cp src/cortado.jar build/jsdemo/lib/cortado.jar
	cp src/CortadoPlayer.js build/jsdemo/lib/CortadoPlayer.js


# There is a Flash shim for audio on Internet Explorer which doesn't
# have Web Audio API.
#
# The .swf build artifact is in the source tree so you don't have to
# figure out how to install the Apache Flex SDK when you've already
# gone to the trouble of setting up emscripten.
#
# Get SDK binaries from http://flex.apache.org/ and install them somewhere
# in your PATH.
#
# To rebuild the .swf, run 'make cleanswf' then 'make swf'
#
swf : src/dynamicaudio.swf

cleanswf:
	rm -f src/dynamicaudio.swf

src/dynamicaudio.swf : src/dynamicaudio.as
	mxmlc -o src/dynamicaudio.swf -file-specs src/dynamicaudio.as


# And the Flash version of the decoder...

build/flash/root/lib/libogg.a : configureOgg.sh compileOggFlash.sh
	test -d build || mkdir build
	./configureOgg.sh
	./compileOggFlash.sh

build/flash/root/lib/libvorbis.a : build/flash/root/lib/libogg.a configureVorbis.sh compileVorbisFlash.sh
	test -d build || mkdir build
	./configureVorbis.sh
	./compileVorbisFlash.sh

build/flash/root/lib/libtheoradec.a : build/flash/root/lib/libogg.a configureTheora.sh compileTheoraFlash.sh
	test -d build || mkdir build
	./configureTheora.sh
	./compileTheoraFlash.sh

build/flash/ogv-libs.swc : src/ogv-libs.c src/ogv-libs-mixin-flash.c src/YCbCr.h src/YCbCr.c build/flash/root/lib/libogg.a build/flash/root/lib/libtheoradec.a build/flash/root/lib/libvorbis.a compileOgvFlash.sh
	test -d build || mkdir build
	./compileOgvFlash.sh

build/YCbCr-vertex.glsl.out : src/YCbCr-vertex.glsl
	./glsl2agal/bin/glsl2agalopt -optimize -v -e src/YCbCr-vertex.glsl
	mv src/YCbCr-vertex.glsl.out build/YCbCr-vertex.glsl.out

build/YCbCr-fragment.glsl.out : src/YCbCr-fragment.glsl
	./glsl2agal/bin/glsl2agalopt -optimize -f -e src/YCbCr-fragment.glsl
	mv src/YCbCr-fragment.glsl.out build/YCbCr-fragment.glsl.out

build/YCbCr-shaders-agal.h : file2def.js build/YCbCr-fragment.glsl.out build/YCbCr-vertex.glsl.out
	test -d build || mkdir build
	node file2def.js build/YCbCr-vertex.glsl.out YCBCR_VERTEX_SHADER > build/YCbCr-shaders-agal.h
	node file2def.js build/YCbCr-fragment.glsl.out YCBCR_FRAGMENT_SHADER >> build/YCbCr-shaders-agal.h

build/YCbCrFrameSink.as : src/YCbCrFrameSink.as.in build/YCbCr-shaders-agal.h
	 cpp -E -w -P -CC -nostdinc -Ibuild src/YCbCrFrameSink.as.in > build/YCbCrFrameSink.as

build/ogv.swf : src/ogv.as src/OgvCodec.as build/YCbCrFrameSink.as build/flash/ogv-libs.swc
	mxmlc -o build/ogv.swf -static-link-runtime-shared-libraries -library-path=build/flash/ogv-libs.swc -source-path+=build src/ogv.as

build/ogvswf-version.js : build/ogv.swf
	echo 'OgvSwfPlayer.buildDate = "'`date -u`'";' > build/ogvswf-version.js

build/ogvswf.js : src/OgvSwfPlayer.js build/ogvswf-version.js
	cat src/OgvSwfPlayer.js build/ogvswf-version.js > build/ogvswf.js
