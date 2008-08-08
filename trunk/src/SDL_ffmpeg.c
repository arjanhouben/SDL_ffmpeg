/*******************************************************************************
*                                                                              *
*   SDL_ffmpeg is a library for basic multimedia functionality.                *
*   SDL_ffmpeg is based on ffmpeg.                                             *
*                                                                              *
*   Copyright (C) 2007  Arjan Houben                                           *
*                                                                              *
*   SDL_ffmpeg is free software: you can redistribute it and/or modify         *
*   it under the terms of the GNU Lesser General Public License as published   *
*	by the Free Software Foundation, either version 3 of the License, or any   *
*   later version.                                                             *
*                                                                              *
*   This program is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
*   GNU Lesser General Public License for more details.                        *
*                                                                              *
*   You should have received a copy of the GNU Lesser General Public License   *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
*                                                                              *
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include "avformat.h"

#include "SDL/SDL_ffmpeg.h"

int test = 0;

int __Y[256];
int __CrtoR[256];
int __CrtoG[256];
int __CbtoG[256];
int __CbtoB[256];

void initializeLookupTables() {

    float f;
    int i;

    for(i=0; i<256; i++) {

        f = (float)i;

		__Y[i] = (int)( 1.164 * (f-16.0) );

		__CrtoR[i] = (int)( 1.596 * (f-128.0) );

		__CrtoG[i] = (int)( 0.813 * (f-128.0) );
		__CbtoG[i] = (int)( 0.392 * (f-128.0) );

		__CbtoB[i] = (int)( 2.017 * (f-128.0) );
    }
}

int FFMPEG_init_was_called = 0;

int getAudioFrame( SDL_ffmpegFile*, AVPacket*, int16_t*, SDL_ffmpegAudioFrame*, int64_t );

int getVideoFrame( SDL_ffmpegFile*, AVPacket*, AVFrame*, SDL_ffmpegVideoFrame*, int64_t );

void convertYUV420PtoRGBA( AVFrame *YUV420P, SDL_Surface *RGB444, int interlaced );

void snowFill( SDL_Surface *frame );

SDL_ffmpegFile* SDL_ffmpegCreateFile() {

    /* create SDL_ffmpegFile pointer */
    SDL_ffmpegFile *file = (SDL_ffmpegFile*)malloc( sizeof(SDL_ffmpegFile) );
    if(!file) return 0;

    /* reset video streams */
    memset(file->vs, 0, sizeof(SDL_ffmpegStream*) * MAX_STREAMS );

    /* reset audio streams */
    memset(file->as, 0, sizeof(SDL_ffmpegStream*) * MAX_STREAMS );

    /* initialize variables with standard values */
    file->audioStream = 0;
    file->videoStream = 0;
    file->offset = 0;
    file->loopCount = 0;
    file->startTime = 0;
	file->mustSeek = 0;
    file->seekTo = 0;
    file->videoFrameInUse = 0;
	file->pendingVideoFrame = 0;
    file->audioFrameInUse = 0;
	file->pendingAudioFrame = 0;
    file->threadID = 0;

    return file;
}

void SDL_ffmpegFree(SDL_ffmpegFile* file) {

    int i,f;

    if( !file ) return;

    SDL_ffmpegStopDecoding(file);

    /* invalidate any frame currently in use */
    file->videoFrameInUse = 0;

    SDL_ffmpegFlush(file);

    for(i=0; i<MAX_STREAMS; i++) {
        if(file->vs[i]) {
            if( file->vs[i]->_ffmpeg ) avcodec_close( file->vs[i]->_ffmpeg->codec );
            for(f=0; f<SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES; f++) {
                SDL_FreeSurface( file->vs[i]->videoBuffer[f].buffer );
            }
            free( file->vs[i] );
        }
        if(file->as[i]) {
            if( file->as[i]->_ffmpeg ) avcodec_close( file->as[i]->_ffmpeg->codec );
            for(f=0; f<SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES; f++) {
                free( file->as[i]->audioBuffer[f].source );
            }
            free( file->as[i] );
        }
    }

    if(file->_ffmpeg) {
        av_close_input_file( file->_ffmpeg );
    }

    free(file);
}

SDL_ffmpegFile* SDL_ffmpegOpen(const char* filename) {

    SDL_ffmpegFile *file;
    size_t i,f;
    AVCodec *codec;

    /* register all codecs */
    if(!FFMPEG_init_was_called) {
        FFMPEG_init_was_called = 1;

        avcodec_register_all();
        av_register_all();
        initializeLookupTables();
    }

    /* open new ffmpegFile */
    file = SDL_ffmpegCreateFile();
    if(!file) return 0;

    /* information about format is stored in file->_ffmpeg */

    /* open the file */
    if(av_open_input_file( (AVFormatContext**)(&file->_ffmpeg), filename, 0, 0, 0) != 0) {
        fprintf(stderr, "could not open \"%s\"\n", filename);
        free(file);
        return 0;
    }

    /* retrieve format information */
    if(av_find_stream_info(file->_ffmpeg) < 0) {
        fprintf(stderr, "could not retrieve file info for \"%s\"\n", filename);
        free(file);
        return 0;
    }

    /* find the streams in the file */
    file->VStreams = 0;
    file->AStreams = 0;
    file->threadActive = 0;

    /* iterate through all the streams and store audio/video streams */
    for(i=0; i<file->_ffmpeg->nb_streams; i++) {

        if(file->_ffmpeg->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {

            /* if this is a packet of the correct type we create a new stream */
            SDL_ffmpegStream* stream = (SDL_ffmpegStream*)malloc( sizeof(SDL_ffmpegStream) );

            if(stream) {
                /* we set our stream to zero */
                memset(stream, 0, sizeof(SDL_ffmpegStream));

                /* save unique streamid */
                stream->id = i;

                /* the timeBase is what we use to calculate from/to pts */
                stream->timeBase = av_q2d(file->_ffmpeg->streams[i]->time_base) * 1000;

                /* save width, height and pixFmt of our outputframes */
                stream->width = file->_ffmpeg->streams[i]->codec->width;
                stream->height = file->_ffmpeg->streams[i]->codec->height;

                /* _ffmpeg holds data about streamcodec */
                stream->_ffmpeg = file->_ffmpeg->streams[i];

                /* get the correct decoder for this stream */
                codec = avcodec_find_decoder( stream->_ffmpeg->codec->codec_id );

                /* initialize empty audiobuffer */
                memset(stream->audioBuffer, 0, sizeof(SDL_ffmpegAudioFrame)*SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES);
                memset(stream->videoBuffer, 0, sizeof(SDL_ffmpegVideoFrame)*SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES);

                if(!codec) {
                    free(stream);
                    fprintf(stderr, "could not find codec\n");
                } else if(avcodec_open(file->_ffmpeg->streams[i]->codec, codec) < 0) {
                    free(stream);
                    fprintf(stderr, "could not open decoder\n");
                } else {

                    /* allocate videobuffer */
                    for(f=0; f<SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES; f++) {
                        stream->videoBuffer[f].buffer = SDL_CreateRGBSurface( 0,
                                                    stream->width, stream->height, 32,
                                                    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 );
                    }

                    /* copy metadata from AVStream into our stream */
                    stream->frameRate[0] = file->_ffmpeg->streams[i]->time_base.num;
                    stream->frameRate[1] = file->_ffmpeg->streams[i]->time_base.den;
                    memcpy(stream->language, file->_ffmpeg->streams[i]->language, 4);
                    stream->sampleRate = file->_ffmpeg->streams[i]->codec->sample_rate;
                    stream->channels = file->_ffmpeg->streams[i]->codec->channels;
                    memcpy(stream->codecName, file->_ffmpeg->streams[i]->codec->codec_name, 32);

                    file->vs[file->VStreams] = stream;
                    file->VStreams++;
                }
            }
        } else if(file->_ffmpeg->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {

            /* if this is a packet of the correct type we create a new stream */
            SDL_ffmpegStream* stream = (SDL_ffmpegStream*)malloc( sizeof(SDL_ffmpegStream) );

            if(stream) {
                /* we set our stream to zero */
                memset(stream, 0, sizeof(SDL_ffmpegStream));

                /* save unique streamid */
                stream->id = i;

                /* the timeBase is what we use to calculate from/to pts */
                stream->timeBase = av_q2d(file->_ffmpeg->streams[i]->time_base) * 1000;

                /* _ffmpeg holds data about streamcodec */
                stream->_ffmpeg = file->_ffmpeg->streams[i];

                /* get the correct decoder for this stream */
                codec = avcodec_find_decoder(file->_ffmpeg->streams[i]->codec->codec_id);

                if(!codec) {
                    free( stream );
                    fprintf(stderr, "could not find codec\n");
                } else if(avcodec_open(file->_ffmpeg->streams[i]->codec, codec) < 0) {
                    free( stream );
                    fprintf(stderr, "could not open decoder\n");
                } else {

                    /* allocate audiobuffer */
                    for(f=0; f<SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES; f++) {
                        stream->audioBuffer[f].source = (uint8_t*)malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE );
                    }

                    /* copy metadata from AVStream into our stream */
                    stream->frameRate[0] = file->_ffmpeg->streams[i]->time_base.num;
                    stream->frameRate[1] = file->_ffmpeg->streams[i]->time_base.den;
                    memcpy(stream->language, file->_ffmpeg->streams[i]->language, 4);
                    stream->sampleRate = file->_ffmpeg->streams[i]->codec->sample_rate;
                    stream->channels = file->_ffmpeg->streams[i]->codec->channels;
                    memcpy(stream->codecName, file->_ffmpeg->streams[i]->codec->codec_name, 32);

                    file->as[file->AStreams] = stream;
                    file->AStreams++;
                }
            }
        }
    }

    return file;
}

SDL_ffmpegVideoFrame* SDL_ffmpegGetVideoFrame(SDL_ffmpegFile* file) {

    SDL_ffmpegVideoFrame *f;

    if( !file || !file->videoStream || !file->loopCount || file->videoStream->endReached ) return 0;

	f = 0;

	/* previous frame can indicate which frame should come now */
	if( file->videoFrameInUse ) {

		if( file->videoFrameInUse->next && file->videoFrameInUse->next->filled ) {

			f = file->videoFrameInUse->next;
		}

	} else if( file->pendingVideoFrame ) {

		f = file->pendingVideoFrame;

	} else {

		return 0;
	}

    /* new frame was selected */
    if(f) {

        /* check if pts is suited for displaying */
        if( f->pts <= SDL_ffmpegGetPosition(file) ) {

			if( file->videoFrameInUse ) {

				/* release previous frame */
				file->videoFrameInUse->filled = 0;
			}

			#if 1
			/* check if we can skip this frame */
			while( !f->last && f->next && f->next->filled && f->next->pts <= SDL_ffmpegGetPosition(file) ) {

				/* current frame will not be used, flag it empty */
				f->filled = 0;
//printf("dropped frame\n");
				/* jump to next frame */
				f = f->next;
			}
			#endif

			/* selected frame is marked as being used by user */
			file->videoFrameInUse = f;

			if( f->last ) {

				f->last = 0;

			    /* last video frame found, check if audio is available/done */
			    if( !file->audioStream || file->audioStream->endReached ) {

			        file->videoStream->endReached = 0;
			        if(file->audioStream) file->audioStream->endReached = 0;

					/* check if file should loop */
					if( file->loopCount > 0 ) {
						file->loopCount--;
					}

					/* if loopCount is still non-zero, we loop */
					if( file->loopCount ) {

						file->startTime = SDL_GetTicks();

						file->offset = 0;
					}

			    } else {

			        file->videoStream->endReached = 1;
			    }
			}

		} else {

			/* we can't use this frame right now */
			f = 0;
		}
    }

    return f;
}

SDL_ffmpegStream* SDL_ffmpegGetAudioStream(SDL_ffmpegFile *file, int audioID) {

    /* check if we have any audiostreams */
    if( !file || !file->AStreams ) return 0;

    /* check if the requested id is possible */
    if(audioID >= file->AStreams) return 0;

    /* return audiostream linked to audioID */
    return file->as[audioID];
}

int SDL_ffmpegSelectAudioStream(SDL_ffmpegFile* file, int audioID) {

    /* check if we have any audiostreams */
    if( !file || !file->AStreams ) return -1;

    /* check if the requested id is possible */
    if(audioID >= file->AStreams) return -1;

    /* set current audiostream to stream linked to audioID */
    file->audioStream = file->as[audioID];

    return 0;
}

SDL_ffmpegStream* SDL_ffmpegGetVideoStream(SDL_ffmpegFile *file, int videoID) {

    /* check if we have any videostreams */
    if( !file || !file->VStreams ) return 0;

    /* check if the requested id is possible */
    if(videoID >= file->VStreams) return 0;

    /* return ausiostream linked to videoID */
    return file->vs[videoID];
}

int SDL_ffmpegSelectVideoStream(SDL_ffmpegFile* file, int videoID) {

    /* check if we have any videostreams */
    if( !file || !file->VStreams ) return -1;

    /* check if the requested id is possible */
    if(videoID >= file->VStreams) return -1;

    /* set current videostream to stream linked to videoID */
    file->videoStream = file->vs[videoID];

    return 0;
}

int SDL_ffmpegStartDecoding(SDL_ffmpegFile* file) {

    if( !file ) return -1;

    /* start a thread that continues to fill audio/video buffers */
    if(!file->threadID) file->threadID = SDL_CreateThread(SDL_ffmpegDecodeThread, file);

    return 0;
}

int SDL_ffmpegStopDecoding(SDL_ffmpegFile* file) {

    if( !file ) return -1;

    /* stop decode thread */
    file->threadActive = 0;
    if(file->threadID) SDL_WaitThread(file->threadID, 0);

    /* set threadID to zero, so we can check for concurrent threads */
    file->threadID = 0;

    return -1;
}

int SDL_ffmpegDecodeThread(void* data) {

    SDL_ffmpegFile *file;
	SDL_ffmpegVideoFrame *lastVideoFrame;
	SDL_ffmpegAudioFrame *lastAudioFrame;
    AVFrame *inFrame;
    int64_t seekPos,
            minimalTimestamp;
    int16_t *samples;
    int decode, a, streamLooped = 0;

    /* create a packet for our data */
    AVPacket pack;

    /* initialize packet */
    av_init_packet(&pack);

    /* if we got invalid data, return */
    if(!data) return -1;

    /* unpack the void pointer */
    file = (SDL_ffmpegFile*)data;

    /* flag this thread as active, used for stopping */
    file->threadActive = 1;

    /* allocate a frame */
    inFrame = avcodec_alloc_frame();
    if( !inFrame ) return -1;

    /* allocate temporary audiobuffer */
    samples = (int16_t*)malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE );
    if( !samples ) {
        return -1;
    }

    minimalTimestamp = AV_NOPTS_VALUE;

	lastVideoFrame = 0;
	lastAudioFrame = 0;

    while(file->threadActive) {

        /* check if a seek operation is pending */
        if(file->mustSeek) {

            /* convert milliseconds to AV_TIME_BASE units */
            seekPos = file->seekTo * (AV_TIME_BASE / 1000);

            if(file->_ffmpeg->start_time != AV_NOPTS_VALUE) {
                /* start_time is in AV_TIME_BASE fractional seconds */
                seekPos += file->_ffmpeg->start_time;
            }

            /* AVSEEK_FLAG_BACKWARD means we jump to the first keyframe before seekPos */
            if( av_seek_frame(file->_ffmpeg, -1, seekPos, AVSEEK_FLAG_BACKWARD) >= 0 ) {
                /*  seek succesfull
                    set some values in our file so we now were to start playing
                    offset is in milliseconds */
            }

            /* determine the reason for this seek action */
            if( streamLooped ) {

                minimalTimestamp = AV_NOPTS_VALUE;

            } else {

                file->offset = file->seekTo;
                file->startTime = SDL_GetTicks();

                minimalTimestamp = file->offset;

                SDL_ffmpegFlush(file);
            }

            file->mustSeek = 0;
        }

        /* read a packet from the file */
        decode = av_read_frame(file->_ffmpeg, &pack);

        /* if we did not get a packet, we seek to begin and try again */
        if(decode < 0) {

            streamLooped = 1;
            file->mustSeek = 1;
            file->seekTo = 0;

            /* last frame should be flagged as such */
            if( lastAudioFrame ) lastAudioFrame->last = 1;

			if( lastVideoFrame ) lastVideoFrame->last = 1;

            continue;
        } else {
            streamLooped = 0;
        }

        /* we got a packet, lets handle it */

        /* If it's a audio packet from our stream... */
        if( file->audioStream && pack.stream_index == file->audioStream->id ) {

            a = 0;
            /*  while thread is active, and buffer position is not empty or
                equal to the frame in use by user, we seek for an empty frame */
            while(  file->threadActive && ( file->audioStream->audioBuffer[a].size ||
                    &file->audioStream->audioBuffer[a] == file->audioFrameInUse ) ) {

                /* if we need to seek, drop current frame */
                if(file->mustSeek) goto freePacket;
                a++;
                if(a >= SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES) {
                    a = 0;
                    /* we tried all buffer options, wait a bit and try them again */
                    SDL_Delay(10);
				}
            }

            /* we got out of the loop, meaning we found an empty space in the audio buffer */

			/* write found audioFrame into empty audiobuffer place */
			if( getAudioFrame( file, &pack, samples, &file->audioStream->audioBuffer[a], minimalTimestamp ) ) {

                if( file->audioStream->flushed ) {
                    /* after a flush the lastAudioFrame is invalid */
                    lastAudioFrame = file->audioFrameInUse;
                    /* we continue with the last frame given to the user */
                    file->audioStream->flushed = 0;
                }

				if( lastAudioFrame ) lastAudioFrame->next = &file->audioStream->audioBuffer[a];

				lastAudioFrame = &file->audioStream->audioBuffer[a];

				if( !file->audioFrameInUse && !file->pendingAudioFrame ) {

					file->pendingAudioFrame = &file->audioStream->audioBuffer[a];

					file->pendingAudioFrame->next = 0;
				}
			}
        }

        /* If it's a video packet from our video stream... */
        if( file->videoStream && pack.stream_index == file->videoStream->id ) {

            a = 0;
            /*  while thread is active, and buffer position is not empty and not
                equal to the frame in use by user, we seek for an empty frame */
            while(  file->threadActive && ( file->videoStream->videoBuffer[a].filled ||
                    &file->videoStream->videoBuffer[a] == file->videoFrameInUse ) ) {

                /* if we need to seek, drop current frame */
                if(file->mustSeek) goto freePacket;
                a++;
                if(a >= SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES) {
                    a = 0;
                    /* we tried all buffer options, wait a bit and try them again */
                    SDL_Delay(10);				}
            }

            /* we got out of the loop, meaning we found an empty space in the video buffer */

			/* write found videoFrame into empty videobuffer place */
			if( getVideoFrame( file, &pack, inFrame, &file->videoStream->videoBuffer[a], minimalTimestamp ) ) {

                if( file->videoStream->flushed ) {
                    /* after a flush the lastAudioFrame is invalid */
                    lastVideoFrame = file->videoFrameInUse;
                    /* we continue with the last frame given to the user */
                    file->videoStream->flushed = 0;
                }

				if( lastVideoFrame ) lastVideoFrame->next = &file->videoStream->videoBuffer[a];

				lastVideoFrame = &file->videoStream->videoBuffer[a];

				if( !file->videoFrameInUse && !file->pendingVideoFrame ) {

					file->pendingVideoFrame = &file->videoStream->videoBuffer[a];

					file->pendingVideoFrame->next = 0;
				}
			}
        }

        freePacket:
        /* we can release the packet we reserved */
        av_free_packet(&pack);
    }

    av_free( samples );

    return 0;
}

int SDL_ffmpegSeek(SDL_ffmpegFile* file, int64_t timestamp) {
    file->mustSeek = 1;
    file->seekTo = timestamp;
    return 0;
}

int SDL_ffmpegSeekRelative(SDL_ffmpegFile *file, int64_t timestamp) {

    /* same thing as normal seek, just take into account the current position */
    return SDL_ffmpegSeek(file, SDL_ffmpegGetPosition(file) + timestamp);
}

int SDL_ffmpegFlush(SDL_ffmpegFile *file) {

    int i;

    /* check for file and permission to flush buffers */
    if( !file ) return -1;

    /* if we have a valid audio stream, we flush it */
    if( file->audioStream ) {

        for(i=0; i<SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES; i++) {

			/* frame which is in use by user should not be flagged free */
            if(file->audioFrameInUse != &file->audioStream->audioBuffer[i]) {
                file->audioStream->audioBuffer[i].size = 0;
                file->audioStream->audioBuffer[i].next = 0;
                file->audioStream->audioBuffer[i].last = 0;
            }
        }

        if(file->audioStream->_ffmpeg) {
            avcodec_flush_buffers( file->audioStream->_ffmpeg->codec );
        }

		file->pendingAudioFrame = 0;

        file->audioStream->flushed = 1;
    }

    /* if we have a valid video stream, we flush some more */
    if( file->videoStream ) {

        for(i=0; i<SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES; i++) {

			/* frame which is in use by user should not be flagged free */
			if(file->videoFrameInUse != &file->videoStream->videoBuffer[i]) {
				file->videoStream->videoBuffer[i].filled = 0;
                file->videoStream->videoBuffer[i].next = 0;
                file->videoStream->videoBuffer[i].last = 0;
			}
        }

        if(file->videoStream->_ffmpeg) {
            avcodec_flush_buffers( file->videoStream->_ffmpeg->codec );
        }

        file->videoStream->flushed = 1;

		file->pendingVideoFrame = 0;
    }

    return 0;
}

SDL_ffmpegAudioFrame* SDL_ffmpegGetAudioFrame(SDL_ffmpegFile *file) {

    SDL_ffmpegAudioFrame *f;

    if( !file || !file->audioStream || !file->loopCount || file->audioStream->endReached ) return 0;

	f = 0;

	/* previous frame can indicate which frame should come now */
	if( file->audioFrameInUse ) {

		if( file->audioFrameInUse->size ) return file->audioFrameInUse;

        if( file->audioFrameInUse->next && file->audioFrameInUse->next->size ) f = file->audioFrameInUse->next;

	} else if( file->pendingAudioFrame ) {

		f = file->pendingAudioFrame;

	} else {

		return 0;
	}

    /* new frame was selected */
    if(f) {

        /* check if the timestamp is ready to be played */
        if( f->pts <= SDL_ffmpegGetPosition(file) ) {

			if( file->audioFrameInUse ) {

				/* release previous frame */
				file->audioFrameInUse->size = 0;
			}

			#if 1
			while( !f->last && f->next && f->next->size && f->next->pts <= SDL_ffmpegGetPosition(file) ) {

				/* flag frame which we are not going to use as empty */
				f->size = 0;

				/* jump to next frame */
				f = f->next;
			}
			#endif

			/* selected frame is marked as being in use by user */
			file->audioFrameInUse = f;

			if( f->last ) {

				f->last = 0;

			    /* last audio frame found, check if video is available/done */
			    if( !file->videoStream || file->videoStream->endReached ) {

			        file->audioStream->endReached = 0;
			        if(file->videoStream) file->videoStream->endReached = 0;

					/* check if file should loop */
					if( file->loopCount > 0 ) {
						file->loopCount--;
					}

					/* if loopCount is still non-zero, we loop */
					if( file->loopCount ) {

						file->startTime = SDL_GetTicks();

						file->offset = 0;
					}

			    } else {

			        file->audioStream->endReached = 1;
			    }
			}

        } else {
            /* we can't show this frame at this moment */
            f = 0;
        }
    }

    /* return whatever we found */
    return f;
}

int64_t SDL_ffmpegGetPosition(SDL_ffmpegFile *file) {

    if( !file ) return 0;

    /* return the current playposition of our file */
    return SDL_GetTicks() - file->startTime + file->offset;
}

SDL_AudioSpec* SDL_ffmpegGetAudioSpec(SDL_ffmpegFile *file, int samples, SDL_ffmpegCallback callback) {

    /* create audio spec */
    SDL_AudioSpec *spec = (SDL_AudioSpec*)malloc( sizeof(SDL_AudioSpec) );

    if(spec) {
        spec->format = AUDIO_S16SYS;
        spec->samples = samples;
        spec->userdata = file;
        spec->callback = callback;
        spec->freq = 48000;
        spec->channels = 2;

        /* if we have a valid audiofile, we can use its data to create a
           more appropriate audio spec */
        if( file && file->audioStream ) {
            spec->freq = file->audioStream->sampleRate;
            spec->channels = file->audioStream->channels;
        }
    }

    return spec;
}

int64_t SDL_ffmpegGetDuration(SDL_ffmpegFile *file) {

    /* returns the duration of the entire file, please note that ffmpeg doesn't
       always get this value right! so don't bet your life on it... */
    return file->_ffmpeg->duration / (AV_TIME_BASE / 1000);
}

int SDL_ffmpegGetVideoSize(SDL_ffmpegFile *file, int *w, int *h) {

    if(!w || !h) return -1;

    /* if we have a valid video file selected, we use it
       if not, we send default values and return.
       by checking the return value you can check if you got a valid size */
    if( file->videoStream ) {
        *w = file->videoStream->width;
        *h = file->videoStream->height;
        return 0;
    }

    *w = 320;
    *h = 240;
    return -1;
}

int SDL_ffmpegValidAudio(SDL_ffmpegFile* file) {

    /* this function is used to check if we selected a valid audio stream */
    if(file->audioStream) return 1;

    return 0;
}

int SDL_ffmpegValidVideo(SDL_ffmpegFile* file) {

    /* this function is used to check if we selected a valid video stream */
    if(file->videoStream) return 1;

    return 0;
}

int SDL_ffmpegPlay(SDL_ffmpegFile *file, int64_t count) {

	/* if we are going to start, we may have to reset startTime */
    if( count ) {

		/* if current state is paused, we reset the starttime*/
		if( !file->loopCount ) file->startTime = SDL_GetTicks();

        /* -1 means we play indefenetely, otherwise we play count times */
        file->loopCount = count;

    } else {

        if( file->loopCount ) {
            /* we are about to stop playback, record current position as our offset */
            file->offset = SDL_ffmpegGetPosition(file);
        }

        /* reset loopCount to disable playback */
        file->loopCount = 0;

	}

    return 0;
}

int SDL_ffmpegGetState(SDL_ffmpegFile *file) {
    return file->loopCount != 0;
}

int getAudioFrame( SDL_ffmpegFile *file, AVPacket *pack, int16_t *samples, SDL_ffmpegAudioFrame *frame, int64_t minimalPTS ) {

    uint8_t *data;
    int size, len, audioSize;

    data = pack->data;
    size = pack->size;
    audioSize = AVCODEC_MAX_AUDIO_FRAME_SIZE * 2;

	/* calculate pts to determine wheter or not this frame should be stored */
	frame->pts = av_rescale((pack->dts-file->audioStream->_ffmpeg->start_time)*1000, file->audioStream->_ffmpeg->time_base.num, file->audioStream->_ffmpeg->time_base.den);

    /* don't decode packets which are too old anyway */
    if( frame->pts != AV_NOPTS_VALUE && frame->pts < minimalPTS ) {
        file->audioStream->_ffmpeg->codec->hurry_up = 1;
    } else {
        file->audioStream->_ffmpeg->codec->hurry_up = 0;
	}

	while(size > 0 && file->threadActive) {

		/* Decode the packet */
		len = avcodec_decode_audio2(file->audioStream->_ffmpeg->codec, samples, &audioSize, data, size);

		/* if error, or hurry state, we skip the frame */
		if( len <= 0 || !audioSize ) break;

		/* change pointers */
		data += len;
		size -= len;
	}

	if( !file->audioStream->_ffmpeg->codec->hurry_up ) {

		/* set buffer to start of allocated buffer */
        frame->buffer = frame->source;

        /* copy data to frame */
        memcpy(frame->buffer, samples, audioSize);

        /* write decoding time stamp */
        frame->dts = file->audioStream->dts++;

        /* set frame size so it can be used */
        frame->size = audioSize;
    }

    return !file->audioStream->_ffmpeg->codec->hurry_up;
}

int getVideoFrame( SDL_ffmpegFile* file, AVPacket *pack, AVFrame *inFrame, SDL_ffmpegVideoFrame *frame, int64_t minimalPTS ) {

    int got_frame;

    /* usefull when dealing with B frames */
    if(pack->dts == AV_NOPTS_VALUE) {
        /* if we did not get a valid timestamp, we make one up based on the last
           valid timestamp + the duration of a frame */
        frame->pts = file->videoStream->lastTimeStamp + file->videoStream->timeBase;
    } else {
        /* write timestamp into the buffer */
        frame->pts = av_rescale((pack->dts-file->videoStream->_ffmpeg->start_time)*1000, file->videoStream->_ffmpeg->time_base.num, file->videoStream->_ffmpeg->time_base.den);
    }

	if( frame->pts != AV_NOPTS_VALUE && frame->pts < minimalPTS ) {
		file->videoStream->_ffmpeg->codec->hurry_up = 1;
    } else {
        file->videoStream->_ffmpeg->codec->hurry_up = 0;
    }

    got_frame = 0;

    /* Decode the packet */
    avcodec_decode_video(file->videoStream->_ffmpeg->codec, inFrame, &got_frame, pack->data, pack->size);

    /* if we did not get a frame or we need to hurry, we return */
    if( got_frame && !file->videoStream->_ffmpeg->codec->hurry_up ) {
		if( file->videoStream->_ffmpeg->codec->pix_fmt == PIX_FMT_YUV420P ) {
			/* convert YUV to RGB data */
			convertYUV420PtoRGBA( inFrame, frame->buffer, inFrame->interlaced_frame );
		} else {
			/* could not find a valid conversion */
			printf("couldn't convert image format to RGB\n");
			/* write digital 'snow' into output */
			snowFill( frame->buffer );
		}

		/* write decoding time stamp */
		frame->dts = file->videoStream->dts++;

		/* we write the lastTimestamp we got */
		file->videoStream->lastTimeStamp = frame->pts;

		/* flag this frame as not-empty */
		frame->filled = 1;
	}

    return frame->filled;
}

inline int clamp0_255(int x) {
	x &= (~x) >> 31;
	x -= 255;
	x &= x >> 31;
	return x + 255;
}

void convertYUV420PtoRGBA( AVFrame *YUV420P, SDL_Surface *OUTPUT, int interlaced ) {

    uint8_t *Y, *U, *V;
	uint32_t *RGBA = OUTPUT->pixels;
    int x, y, mod;

    for(y=0; y<OUTPUT->h; y++){

        Y = YUV420P->data[0] + YUV420P->linesize[0] * y;
        U = YUV420P->data[1] + YUV420P->linesize[1] * (y/2);
        V = YUV420P->data[2] + YUV420P->linesize[2] * (y/2);

		/* make sure we deinterlace before upsampling */
		if( interlaced ) {
			mod = y % 4;
			/* on scanline 2 and 3 we need to look at different lines */
			if( mod == 1 ) {
				U += YUV420P->linesize[1];
				V += YUV420P->linesize[2];
			} else if( mod == 2 ) {
				U -= YUV420P->linesize[1];
				V -= YUV420P->linesize[2];
			}
		}

        for(x=0; x<OUTPUT->w; x++){

			/* shift components to the correct place in pixel */
			*RGBA =   clamp0_255( __Y[*Y] + __CrtoR[*V] )							| /* red */
					( clamp0_255( __Y[*Y] - __CrtoG[*V] - __CbtoG[*U] )	<<  8 )		| /* green */
					( clamp0_255( __Y[*Y] + __CbtoB[*U] )				<< 16 )		| /* blue */
					0xFF000000;

			/* goto next pixel */
			RGBA++;

            /* full resolution luma, so we increment at every pixel */
            Y++;

			/* quarter resolution chroma, increment every other pixel */
            U+=x&1;
			V+=x&1;
        }
    }
}

void snowFill( SDL_Surface *frame ) {
    uint8_t *RGB = frame->pixels;
    int i, t;
    for(i=0; i<frame->w*frame->h; i++) {
        t = (int)(((float)rand() / (float)RAND_MAX) * 255.0);
        *RGB = t;   RGB++;
        *RGB = t;   RGB++;
        *RGB = t;   RGB++;
    }
}
