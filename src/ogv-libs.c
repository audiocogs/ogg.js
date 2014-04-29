#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#include <ogg/ogg.h>

#include <vorbis/codec.h>

#include <theora/theoradec.h>



/* never forget that globals are a one-way ticket to Hell */
/* Ogg and codec state for demux/decode */
ogg_sync_state    oggSyncState;
ogg_page          oggPage;

/* Video decode state */
ogg_stream_state  theoraStreamState;
th_info           theoraInfo;
th_comment        theoraComment;
th_setup_info    *theoraSetupInfo;
th_dec_ctx       *theoraDecoderContext;

int              theora_p=0;
int              theora_processing_headers;

/* single frame video buffering */
int          videobuf_ready=0;
ogg_int64_t  videobuf_granulepos=-1;
double       videobuf_time=0;

int          audiobuf_ready=0;
ogg_int64_t  audiobuf_granulepos=0; /* time position of last sample */

int          raw=0;

/* Audio decode state */
int              vorbis_p = 0;
int              vorbis_processing_headers;
ogg_stream_state vo;
vorbis_info      vi;
vorbis_dsp_state vd;
vorbis_block     vb;
vorbis_comment   vc;
int          crop=0;

int got_sigint=0;
static void sigint_handler (int signal) {
  got_sigint = 1;
}

// Callbacks
extern void OgvJsLoadedMetadata();

extern void OgvJsInitVideo(int frameWidth, int frameHeight,
                           int hdec, int vdec,
                           double fps,
                           int picWidth, int picHeight,
                           int picX, int picY);
extern void OgvJsOutputFrameReady();
extern void OgvJsOutputFrame(unsigned char *bufferY, int strideY,
                             unsigned char *bufferCb, int strideCb,
                             unsigned char *bufferCr, int strideCr,
                             int width, int height,
                             int hdec, int vdec,
                             double timestamp);

extern void OgvJsInitAudio(int channels, int rate);
extern void OgvJsOutputAudioReady();
extern void OgvJsOutputAudio(float **buffers, int channels, int sampleCount);

/*Write out the planar YUV frame, uncropped.*/
static void video_write(void){
    th_ycbcr_buffer ycbcr;
    th_decode_ycbcr_out(theoraDecoderContext,ycbcr);

    int hdec = !(theoraInfo.pixel_fmt & 1);
    int vdec = !(theoraInfo.pixel_fmt & 2);
    
	OgvJsOutputFrame(ycbcr[0].data, ycbcr[0].stride,
	                 ycbcr[1].data, ycbcr[1].stride,
	                 ycbcr[2].data, ycbcr[2].stride,
	                 theoraInfo.frame_width, theoraInfo.frame_height,
	                 hdec, vdec,
	                 videobuf_time);
}

/* dump the theora comment header */
static int dump_comments(th_comment *_tc){
  int   i;
  int   len;
  printf("Encoded by %s\n",_tc->vendor);
  if(_tc->comments){
    printf("theora comment header:\n");
    for(i=0;i<_tc->comments;i++){
      if(_tc->user_comments[i]){
        len=_tc->comment_lengths[i]<INT_MAX?_tc->comment_lengths[i]:INT_MAX;
        printf("\t%.*s\n",len,_tc->user_comments[i]);
      }
    }
  }
  return 0;
}

/* helper: push a page into the appropriate steam */
/* this can be done blindly; a stream won't accept a page
                that doesn't belong to it */
static int queue_page(ogg_page *page){
  if (theora_p) ogg_stream_pagein(&theoraStreamState,page);
  if (vorbis_p) ogg_stream_pagein(&vo, page);
  return 0;
}


  ogg_packet oggPacket;
  ogg_packet audioPacket;
  ogg_packet videoPacket;

  int frames = 0;

enum AppState {
	STATE_BEGIN,
	STATE_HEADERS,
	STATE_DECODING
} appState;
int process_audio, process_video;

void OgvJsInit(int process_audio_flag, int process_video_flag) {
	// Allow the caller to specify whether we want audio, video, or both.
	// Or neither, but that won't be very useful.
	process_audio = process_audio_flag;
	process_video = process_video_flag;

	appState = STATE_BEGIN;
	
  /* start up Ogg stream synchronization layer */
  ogg_sync_init(&oggSyncState);

  /* init supporting Vorbis structures needed in header parsing */
  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);

  /* init supporting Theora structures needed in header parsing */
  th_comment_init(&theoraComment);
  th_info_init(&theoraInfo);
}

static void processHeaders();

static void processBegin() {
	if (ogg_page_bos(&oggPage)) {
		printf("Packet is at start of a bitstream\n");
		int got_packet;
	
		// Initialize a stream state object...
		ogg_stream_state test;
		ogg_stream_init(&test,ogg_page_serialno(&oggPage));
		ogg_stream_pagein(&test, &oggPage);

		// Peek at the next packet, since th_decode_headerin() will otherwise
		// eat the first Theora video packet...
		got_packet = ogg_stream_packetpeek(&test, &oggPacket);
		if (!got_packet) {
			return;
		}
  
		/* identify the codec: try theora */
		if(process_video && !theora_p && (theora_processing_headers = th_decode_headerin(&theoraInfo,&theoraComment,&theoraSetupInfo,&oggPacket))>=0){

			/* it is theora -- save this stream state */
			printf("found theora stream!\n");
			memcpy(&theoraStreamState, &test, sizeof(test));
			theora_p = 1;
			
			if (theora_processing_headers == 0) {
				printf("Saving first video packet for later!\n");
			} else {
				ogg_stream_packetout(&theoraStreamState, NULL);
			}
		} else if (process_audio && !vorbis_p && (vorbis_processing_headers = vorbis_synthesis_headerin(&vi,&vc,&oggPacket)) == 0) {
			// it's vorbis!
			// save this as our audio stream...
			printf("found vorbis stream! %d\n", vorbis_processing_headers);
			memcpy(&vo, &test, sizeof(test));
			vorbis_p = 1;
			
			// ditch the processed packet...
			ogg_stream_packetout(&vo, NULL);
		} else {
			printf("already have stream, or not theora or vorbis packet\n");
			/* whatever it is, we don't care about it */
			ogg_stream_clear(&test);
		}
	} else {
		printf("Moving on to header decoding...\n");
		// Not a bitstream start -- move on to header decoding...
		appState = STATE_HEADERS;
		//processHeaders();
	}
}

static void processHeaders() {
	if ((theora_p && theora_processing_headers) || (vorbis_p && vorbis_p < 3)) {
		printf("processHeaders pass... %d %d %d\n", theora_p, theora_processing_headers, vorbis_p);
		int ret;

		/* look for further theora headers */
		if (theora_p && theora_processing_headers) {
			printf("checking theora headers...\n");
			ret = ogg_stream_packetpeek(&theoraStreamState, &oggPacket);
			if (ret < 0) {
				printf("Error reading theora headers: %d.\n", ret);
				exit(1);
			}
			if (ret > 0) {
				printf("Checking another theora header packet...\n");
				theora_processing_headers = th_decode_headerin(&theoraInfo, &theoraComment, &theoraSetupInfo, &oggPacket);
				if (theora_processing_headers == 0) {
				  // We've completed the theora header
				  printf("Completed theora header.\n");
				  theora_p = 3;
				} else {
					printf("Still parsing theora headers...\n");
					ogg_stream_packetout(&theoraStreamState, NULL);
				}
			}
			if (ret == 0) {
				printf("No theora header packet...\n");
			}
		}
		
		if (vorbis_p && (vorbis_p < 3)) {
			printf("checking vorbis headers...\n");
			ret = ogg_stream_packetpeek(&vo, &oggPacket);
			if (ret < 0) {
				printf("Error reading vorbis headers: %d.\n", ret);
				exit(1);
			}
			if (ret > 0) {
				printf("Checking another vorbis header packet...\n");
				vorbis_processing_headers = vorbis_synthesis_headerin(&vi, &vc, &oggPacket);
				if (vorbis_processing_headers == 0) {
					printf("Completed another vorbis header (of 3 total)...\n");
					vorbis_p++;
				} else {
					printf("Invalid vorbis header?\n");
					exit(1);
				}
				ogg_stream_packetout(&vo, NULL);
			}
			if (ret == 0) {
				printf("No vorbis header packet...\n");
			}
		}		
    } else {
	  /* and now we have it all.  initialize decoders */
	  printf("theora_p is %d; vorbis_p is %d\n", theora_p, vorbis_p);
	  if(theora_p){
	  printf("SETTING UP THEORA DECODER CONTEXT\n");
		dump_comments(&theoraComment);
		theoraDecoderContext=th_decode_alloc(&theoraInfo,theoraSetupInfo);
		printf("Ogg logical stream %lx is Theora %dx%d %.02f fps video\n"
		 "Encoded frame content is %dx%d with %dx%d offset\n",
		 theoraStreamState.serialno,theoraInfo.frame_width,theoraInfo.frame_height,
		 (double)theoraInfo.fps_numerator/theoraInfo.fps_denominator,
		 theoraInfo.pic_width,theoraInfo.pic_height,theoraInfo.pic_x,theoraInfo.pic_y);

			int hdec = !(theoraInfo.pixel_fmt & 1);
			int vdec = !(theoraInfo.pixel_fmt & 2);
			OgvJsInitVideo(theoraInfo.frame_width, theoraInfo.frame_height,
			               hdec, vdec,
			               (float)theoraInfo.fps_numerator / theoraInfo.fps_denominator,
			               theoraInfo.pic_width, theoraInfo.pic_height,
			               theoraInfo.pic_x, theoraInfo.pic_y);
	  }

		if (vorbis_p) {
			vorbis_synthesis_init(&vd,&vi);
			vorbis_block_init(&vd,&vb);
			printf("Ogg logical stream %lx is Vorbis %d channel %ld Hz audio.\n",
			   vo.serialno,vi.channels,vi.rate);
			
			OgvJsInitAudio(vi.channels, vi.rate);
		}

		  appState = STATE_DECODING;
		  printf("Done with headers step\n");
		  OgvJsLoadedMetadata();
	}
}

static int needData = 1;

static void processDecoding() {
	needData = 0;
	if (theora_p && !videobuf_ready) {
		/* theora is one in, one out... */
		if (ogg_stream_packetpeek(&theoraStreamState, &videoPacket) > 0 ){
			videobuf_ready=1;
			OgvJsOutputFrameReady();
		} else {
		  	needData = 1;
		}
	}
	
	if (vorbis_p && !audiobuf_ready) {
		if (ogg_stream_packetpeek(&vo, &audioPacket) > 0) {
			audiobuf_ready = 1;
			OgvJsOutputAudioReady();
		} else {
			needData = 1;
		}
	}
}

int OgvJsDecodeFrame() {
	if (ogg_stream_packetout(&theoraStreamState, &videoPacket) <= 0) {
		printf("Theora packet didn't come out of stream\n");
		return 0;
	}
	videobuf_ready=0;
	int ret = th_decode_packetin(theoraDecoderContext,&videoPacket,&videobuf_granulepos);
	if (ret == 0){
		double t = th_granule_time(theoraDecoderContext,videobuf_granulepos);
		if (t > 0) {
			videobuf_time = t;
		} else {
			// For some reason sometimes we get a bunch of 0s out of th_granule_time
			videobuf_time += 1.0 / ((double)theoraInfo.fps_numerator / theoraInfo.fps_denominator);
		}
		frames++;
		video_write();
		return 1;
	} else if (ret == TH_DUPFRAME) {
		// Duplicated frame, advance time
		videobuf_time += 1.0 / ((double)theoraInfo.fps_numerator / theoraInfo.fps_denominator);
		frames++;
		video_write();
		return 1;
	} else {
		printf("Theora decoder failed mysteriously? %d\n", ret);
		return 0;
	}
}

int OgvJsDecodeAudio() {
	int packetRet = 0;
	audiobuf_ready = 0;
	int foundSome = 0;
	if (ogg_stream_packetout(&vo, &audioPacket) > 0) {
		int ret = vorbis_synthesis(&vb, &audioPacket);
		if (ret == 0) {
			foundSome = 1;
			vorbis_synthesis_blockin(&vd,&vb);

			float **pcm;
			int sampleCount = vorbis_synthesis_pcmout(&vd, &pcm);
			OgvJsOutputAudio(pcm, vi.channels, sampleCount);

			vorbis_synthesis_read(&vd, sampleCount);
		} else {
			printf("Vorbis decoder failed mysteriously? %d", ret);
		}
	}
	
	return foundSome;
}

void OgvJsReceiveInput(char *buffer, int bufsize) {
	if (bufsize > 0) {
		char *dest = ogg_sync_buffer(&oggSyncState, bufsize);
		memcpy(dest, buffer, bufsize);
		ogg_sync_wrote(&oggSyncState, bufsize);
	}
}

int OgvJsProcess() {
	if (needData) {
		if (ogg_sync_pageout(&oggSyncState, &oggPage) > 0) {
			queue_page(&oggPage);
		} else {
			// out of data!
			return 0;
		}
	}
	if (appState == STATE_BEGIN) {
		processBegin();
	} else if (appState == STATE_HEADERS) {
		processHeaders();
	} else if (appState == STATE_DECODING) {
		processDecoding();
	}
	return 1;
}


void OgvJsDestroy() {
  if(theora_p){
    ogg_stream_clear(&theoraStreamState);
    th_decode_free(theoraDecoderContext);
    th_comment_clear(&theoraComment);
    th_info_clear(&theoraInfo);
  }
  ogg_sync_clear(&oggSyncState);
}

