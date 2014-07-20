libogg:
	./compileOgg.sh
	
browser: src/*.js libogg
	mkdir -p build/
	./node_modules/.bin/browserify \
		--extension .coffee \
		--transform browserify-shim \
		--noparse build/libogg.js \
		--debug \
		. \
		| ./node_modules/.bin/exorcist build/ogg.js.map > build/ogg.js

clean:
	cd libogg && make clean
	rm -f libogg/configure
	rm -rf build

.PHONY: libogg
