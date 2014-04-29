mergeInto(LibraryManager.library, {
	
	OgvJsLoadedMetadata: function() {
		OgvJsLoadedMetadataCallback();
	},
	
	OgvJsInitVideo: function(frameWidth, frameHeight,
	                         hdec, vdec,
                             fps,
                             picWidth, picHeight,
                             picX, picY) {
		OgvJsInitVideoCallback({
			codec: "Theora",
			frameWidth: frameWidth,
			frameHeight: frameHeight,
			hdec: hdec,
			vdec: vdec,
			fps: fps,
			picWidth: picWidth,
			picHeight: picHeight,
			picX: picX,
			picY: picY
		});
	},

	OgvJsOutputFrameReady: function(videoPosition) {
		OgvJsOutputFrameReadyCallback(videoPosition);
	},
	
	OgvJsOutputFrame: function(bufferY, strideY,
	                           bufferCb, strideCb,
	                           bufferCr, strideCr,
	                           width, height,
	                           hdec, vdec,
	                           timestamp) {
		
		// Create typed array views of the source buffers from the emscripten heap:
		var HEAPU8 = Module.HEAPU8,
			widthColor = width >> hdec,
			heightColor = height >> vdec,
			countBytesY = strideY * height,
			countBytesCb = strideCb * heightColor,
			countBytesCr = strideCr * heightColor,
			bytesY = HEAPU8.subarray(bufferY, bufferY + countBytesY),
			bytesCb = HEAPU8.subarray(bufferCb, bufferCb + countBytesCb),
			bytesCr = HEAPU8.subarray(bufferCr, bufferCr + countBytesCr);

		// And queue up the output buffer!
		OgvJsFrameCallback({
			bytesY: bytesY,
			bytesCb: bytesCb,
			bytesCr: bytesCr,
			strideY: strideY,
			strideCb: strideCb,
			strideCr: strideCr,
			width: width,
			height: height,
			hdec: hdec,
			vdec: vdec,
			timestamp: timestamp
		});
	},
	
	OgvJsInitAudio: function(channels, rate) {
		OgvJsInitAudioCallback({
			codec: "Vorbis",
			channels: channels,
			rate: rate
		});
	},
	
	OgvJsOutputAudioReady: function() {
		OgvJsOutputAudioReadyCallback();
	},
	
	OgvJsOutputAudio: function(buffers, channels, sampleCount) {
		if (buffers == 0) {
			OgvJsAudioCallback(null);
			return;
		}
		
		// buffers is an array of pointers to float arrays for each channel
		var HEAPU32 = Module.HEAPU32;
		var HEAPF32 = Module.HEAPF32;
		
		var outputBuffers = [];
		if (buffers != 0) {
			var inPtr, inArray, outArray, i;
			for (var channel = 0; channel < channels; channel++) {
				inPtr = HEAPU32[buffers / 4 + channel];
				inArray = HEAPF32.subarray(inPtr / 4, inPtr / 4 + sampleCount);
				outArray = new Float32Array(inArray);
				outputBuffers.push(outArray);
			}
		}

		OgvJsAudioCallback(outputBuffers);
	}

});
