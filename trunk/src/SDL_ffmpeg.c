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

#include "swscale.h"
#include "avformat.h"

#include "SDL/SDL_ffmpeg.h"

int FFMPEG_init_was_called = 0;

int getAudioFrame( SDL_ffmpegFile*, AVPacket*, int16_t*, SDL_ffmpegAudioFrame* );

int getVideoFrame( SDL_ffmpegFile*, AVPacket*, AVFrame*, AVFrame*, SDL_ffmpegVideoFrame* );

SDL_ffmpegFile* SDL_ffmpegCreateFile() {

    /* create SDL_ffmpegFile pointer */
    SDL_ffmpegFile *file = (SDL_ffmpegFile*)malloc( sizeof(SDL_ffmpegFile) );
    if(!file) return 0;

    /* create a semaphore for every file */
    file->decode = SDL_CreateSemaphore(1);

    /* allocate room for VStreams */
    file->vs = (SDL_ffmpegStream**)malloc( sizeof(SDL_ffmpegStream*) * MAX_STREAMS );
    if(!file->vs) {
        SDL_DestroySemaphore( file->decode );
        free( file );
        return 0;
    }
    memset(file->vs, 0, sizeof(SDL_ffmpegStream*) * MAX_STREAMS );

    /* allocate room for AStreams */
    file->as = (SDL_ffmpegStream**)malloc( sizeof(SDL_ffmpegStream*) * MAX_STREAMS );
    if(!file->as) {
        SDL_DestroySemaphore( file->decode );
        free( file->vs );
        free( file );
        return 0;
    }
    memset(file->as, 0, sizeof(SDL_ffmpegStream*) * MAX_STREAMS );

    /* initialize variables with standard values */
    file->audioStream = 0;
    file->videoStream = 0;
    file->offset = 0;
    file->videoOffset = 0;
    file->startTime = 0;
    file->pause = 1;
    file->mustSeek = 0;
    file->seekTo = 0;
    file->videoFrameInUse = 0;
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

    SDL_DestroySemaphore( file->decode );

    for(i=0; i<MAX_STREAMS; i++) {
        if(file->vs[i]) {
            SDL_DestroySemaphore( file->vs[i]->sem );
            if( file->vs[i]->_conversion ) sws_freeContext( file->vs[i]->_conversion );
            if( file->vs[i]->_ffmpeg ) avcodec_close( file->vs[i]->_ffmpeg );
            for(f=0; f<SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES; f++) {
                SDL_FreeSurface( file->vs[i]->videoBuffer[f].buffer );
            }
            free( file->vs[i] );
        }
        if(file->as[i]) {
            SDL_DestroySemaphore( file->as[i]->sem );
            if( file->as[i]->_conversion ) sws_freeContext( file->as[i]->_conversion );
            if( file->as[i]->_ffmpeg ) avcodec_close( file->as[i]->_ffmpeg );
            free( file->as[i] );
        }
    }

    free( file->vs );

    free( file->as );

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
        fprintf(stderr, "could not retrieve video stream info");
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
                stream->pixFmt = PIX_FMT_RGB24;

                /* _ffmpeg holds data about streamcodec */
                stream->_ffmpeg = file->_ffmpeg->streams[i]->codec;

                /* get the correct decoder for this stream */
                codec = avcodec_find_decoder((stream->_ffmpeg)->codec_id);

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
                                                    stream->width, stream->height, 24,
                                                    0x0000FF, 0x00FF00, 0xFF0000, 0 );
                    }

                    /* copy metadata from AVStream into our stream */
                    stream->frameRate[0] = file->_ffmpeg->streams[i]->time_base.num;
                    stream->frameRate[1] = file->_ffmpeg->streams[i]->time_base.den;
                    memcpy(stream->language, file->_ffmpeg->streams[i]->language, 4);
                    stream->sampleRate = file->_ffmpeg->streams[i]->codec->sample_rate;
                    stream->channels = file->_ffmpeg->streams[i]->codec->channels;
                    memcpy(stream->codecName, file->_ffmpeg->streams[i]->codec->codec_name, 32);

                    /* create conversion context for current stream */
                    stream->_conversion = sws_getContext( stream->width, stream->height,
                                                          stream->_ffmpeg->pix_fmt,
                                                          stream->width, stream->height,
                                                          stream->pixFmt, 0, 0, 0, 0 );

                    file->vs[file->VStreams] = stream;
                    file->VStreams++;

                    /* create semaphore for thread-safe use */
                    stream->sem = SDL_CreateSemaphore(1);
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
                stream->_ffmpeg = file->_ffmpeg->streams[i]->codec;

                stream->width = 0;
                stream->height = 0;
                stream->pixFmt = PIX_FMT_RGB24;

                /* get the correct decoder for this stream */
                codec = avcodec_find_decoder(file->_ffmpeg->streams[i]->codec->codec_id);

                if(!codec) {
                    free( stream );
                    fprintf(stderr, "could not find codec\n");
                } else if(avcodec_open(file->_ffmpeg->streams[i]->codec, codec) < 0) {
                    free( stream );
                    fprintf(stderr, "could not open decoder\n");
                } else {

                    /* copy metadata from AVStream into our stream */
                    stream->frameRate[0] = file->_ffmpeg->streams[i]->time_base.num;
                    stream->frameRate[1] = file->_ffmpeg->streams[i]->time_base.den;
                    memcpy(stream->language, file->_ffmpeg->streams[i]->language, 4);
                    stream->sampleRate = file->_ffmpeg->streams[i]->codec->sample_rate;
                    stream->channels = file->_ffmpeg->streams[i]->codec->channels;
                    memcpy(stream->codecName, file->_ffmpeg->streams[i]->codec->codec_name, 32);

                    stream->_conversion = 0;

                    file->as[file->AStreams] = stream;
                    file->AStreams++;

                    /* create semaphore for thread-safe use */
                    stream->sem = SDL_CreateSemaphore(1);
                }
            }
        }
    }

    return file;
}

SDL_ffmpegVideoFrame* SDL_ffmpegGetVideoFrame(SDL_ffmpegFile* file) {

    SDL_ffmpegVideoFrame *f;
    int i;
    int64_t pos;

    if( !file || !file->videoStream || file->pause ) return 0;

    /* working on videobuffer should always be done from inside semaphore */
    SDL_SemWait(file->videoStream->sem);

    /* save position, saves cpu cycles */
    pos = SDL_ffmpegGetPosition(file);
    f = 0;
    /* seek through all buffer positions */
    for(i=0; i<SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES; i++) {
        /* check if buffer position is filled */
        if( file->videoStream->videoBuffer[i].filled &&
            &file->videoStream->videoBuffer[i] != file->videoFrameInUse ) {
            /* if no frame has been selected, or decoding timestamp is lower */
            if(!f || f->dts > file->videoStream->videoBuffer[i].dts) {
                /* check if timestamp is suited for displaying */
                if(file->videoStream->videoBuffer[i].pts <= pos) {
                    /* all checks succeeded, supply this frame to user */
                    f = &file->videoStream->videoBuffer[i];
                }
            }
        }
    }

    SDL_SemPost(file->videoStream->sem);

    /* new frame was selected */
    if(f) {
        /* release previous frame */
        if( file->videoFrameInUse ) {
            /* frame is released */
            file->videoFrameInUse->filled = 0;
        }
        /* selected frame is marked as being used by user */
        file->videoFrameInUse = f;
    }

    return f;
}

SDL_ffmpegStream* SDL_ffmpegGetAudioStream(SDL_ffmpegFile *file, int audioID) {

    /* check if we have any audiostreams */
    if( !file || !file->AStreams ) return 0;

    /* check if the requested id is possible */
    if(audioID >= file->AStreams) return 0;

    /* return ausiostream linked to audioID */
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

    SDL_ffmpegFile* file;
    AVFrame *inFrameRGB, *inFrame;
    int64_t seekPos;
    uint8_t *inVideoBuffer;
    int16_t *samples;
    int decode, a;

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

    /* allocate another frame for unknown->RGB conversion */
    inFrameRGB = avcodec_alloc_frame();
    if( !inFrameRGB ) return -1;

    inVideoBuffer = 0;

    if(file->videoStream) {
        /* allocate buffer */
        inVideoBuffer = (uint8_t*)malloc(  avpicture_get_size(  file->videoStream->pixFmt,
                                                                file->videoStream->width,
                                                                file->videoStream->height) );
        if( !inVideoBuffer ) {
            av_free( inFrameRGB );
            return -1;
        }

        /* put buffer into our reserved frame */
        avpicture_fill( (AVPicture*)inFrameRGB,
                        inVideoBuffer,
                        file->videoStream->pixFmt,
                        file->videoStream->width,
                        file->videoStream->height);
    }

    /* allocate temporary audiobuffer */
    samples = (int16_t*)malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE );
    if( !samples ) {
        av_free( inFrameRGB );
        if(inVideoBuffer) av_free( inVideoBuffer );
        return -1;
    }

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
                file->offset = seekPos / (AV_TIME_BASE / 1000);
                file->startTime = SDL_GetTicks();
            }

            SDL_ffmpegFlush(file);

            file->mustSeek = 0;
        }

        /* read a packet from the file */
        decode = av_read_frame(file->_ffmpeg, &pack);

        /* if we did not get a packet, we wait a bit and try again */
        if(decode < 0) {
            /* thread is idle */
            SDL_Delay(10);
            continue;
        }

        /* we got a packet, lets handle it */

        /* If it's a audio packet from our stream... */
        if( file->audioStream && pack.stream_index == file->audioStream->id ) {

            a = 0;
            while(file->threadActive && file->audioStream->audioBuffer[a].size) {
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

            /* we enter the audio semaphore */
            SDL_SemWait(file->audioStream->sem);

                /* write found audioFrame into empty audiobuffer place */
                getAudioFrame( file, &pack, samples, &file->audioStream->audioBuffer[a] );

            /* we leave the audio semaphore */
            SDL_SemPost(file->audioStream->sem);

        }

        /* If it's a video packet from our video stream... */
        if( file->videoStream && pack.stream_index == file->videoStream->id ) {

            a = 0;
            while(file->threadActive && file->videoStream->videoBuffer[a].filled) {
                /* if we need to seek, drop current frame */
                if(file->mustSeek) goto freePacket;
                a++;
                if(a >= SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES) {
                    a = 0;
                    /* we tried all buffer options, wait a bit and try them again */
                    SDL_Delay(10);
                }
            }

            /* we got out of the loop, meaning we found an empty space in the video buffer */

            /* we enter the video semaphore */
            SDL_SemWait(file->videoStream->sem);

                /* write found audioFrame into empty audiobuffer place */
                getVideoFrame( file, &pack, inFrameRGB, inFrame, &file->videoStream->videoBuffer[a] );

            /* we leave the audio semaphore */
            SDL_SemPost(file->videoStream->sem);

        }

        freePacket:
        /* we can release the packet we reserved */
        av_free_packet(&pack);
    }

    if(inVideoBuffer) av_free( inVideoBuffer );
    av_free( inFrameRGB );
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

        /* flush audiobuffer from semaphore, be thread-safe! */
        SDL_SemWait(file->audioStream->sem);

        for(i=0; i<SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES; i++) {

            if(file->audioStream->audioBuffer[i].size) {
                free( file->audioStream->audioBuffer[i].buffer );
                file->audioStream->audioBuffer[i].size = 0;
            }
        }

        if(file->audioStream->_ffmpeg) {
            avcodec_flush_buffers( file->audioStream->_ffmpeg );
        }

        SDL_SemPost(file->audioStream->sem);
    }

    /* if we have a valid video stream, we flush some more */
    if( file->videoStream ) {

        /* again, be thread safe! */
        SDL_SemWait(file->videoStream->sem);

        for(i=0; i<SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES; i++) {

            if(file->videoStream->videoBuffer[i].filled) {
                /* frame which is in use by user should not be flagged free */
                if(file->videoFrameInUse != &file->videoStream->videoBuffer[i]) {
                    file->videoStream->videoBuffer[i].filled = 0;
                }
            }
        }

        if(file->videoStream->_ffmpeg) {
            avcodec_flush_buffers( file->videoStream->_ffmpeg );
        }

        SDL_SemPost(file->videoStream->sem);
    }

    return 0;
}

SDL_ffmpegAudioFrame* SDL_ffmpegGetAudioFrame(SDL_ffmpegFile *file) {

    SDL_ffmpegAudioFrame *f;
    int64_t pos;
    int i;

    if( !file || !file->audioStream || file->pause ) return 0;

    /* working on audiobuffer should always be done from inside semaphore */
    SDL_SemWait(file->audioStream->sem);

    /* save cpu by saving pos */
    pos = SDL_ffmpegGetPosition(file);
    f = 0;
    for(i=0; i<SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES; i++) {

        if(file->audioStream->audioBuffer[i].size) {
            if(!f || f->timestamp > file->audioStream->audioBuffer[i].timestamp) {
                if(file->audioStream->audioBuffer[i].timestamp <= pos) {
                    f = &file->audioStream->audioBuffer[i];
                }
            }
        }
    }

    /* if there was no frame found, release semaphore */
    if(!f) SDL_SemPost(file->audioStream->sem);

    return f;
}

int SDL_ffmpegReleaseAudio(SDL_ffmpegFile *file, SDL_ffmpegAudioFrame *frame, int len) {

    /* no audio, means no releasing */
    if( !file || !file->audioStream ) return -1;

    /* we move the correct amount of data or invalidate the frame entirely */
    frame->size -= len;

    if(frame->size > 0) {
        /* move memory so we can use this frame again */
        memmove( frame->buffer, frame->buffer+len, frame->size );
    } else {
        /* free allocated buffer */
        free( frame->buffer );
        /* frame should be 0, but better safe than sorry.. */
        frame->size = 0;
    }

    /* work on audiodata is done, so we release the semaphore */
    if(frame) SDL_SemPost(file->audioStream->sem);

    return 0;
}

int64_t SDL_ffmpegGetPosition(SDL_ffmpegFile *file) {

    if( !file ) return 0;

    /* return the current playposition of our file */
    return (SDL_GetTicks() + file->offset) - file->startTime;
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

int SDL_ffmpegPause(SDL_ffmpegFile *file, int state) {

    /* by putting 0 into state, we play the file
       this behaviour is analogue to SDL audio */
    file->pause = state;

    if(!file->pause) {
        file->startTime = SDL_GetTicks();
    }

    return 0;
}

int SDL_ffmpegGetState(SDL_ffmpegFile *file) {
    return file->pause;
}

int getAudioFrame( SDL_ffmpegFile *file, AVPacket *pack, int16_t *samples, SDL_ffmpegAudioFrame *frame ) {

    uint8_t *data;
    int size, len, audioSize;

    data = pack->data;
    size = pack->size;
    audioSize = AVCODEC_MAX_AUDIO_FRAME_SIZE * 2;

    frame->timestamp = pack->pts * file->audioStream->timeBase;

    if(SDL_ffmpegGetPosition(file) > frame->timestamp) {
        file->audioStream->_ffmpeg->hurry_up = 1;
    } else {
        file->audioStream->_ffmpeg->hurry_up = 0;
    }

    while(size > 0 && file->threadActive) {

        /* Decode the packet */
        len = avcodec_decode_audio2(file->audioStream->_ffmpeg, samples, &audioSize, data, size);

        /* if error, or hurry state, we skip the frame */
        if(len <= 0 || !audioSize || (file->audioStream->_ffmpeg)->hurry_up) return -1;

        /* change pointers */
        data += len;
        size -= len;
    }

    frame->size = audioSize;
    frame->buffer = (uint8_t*)malloc( audioSize );

    memcpy(frame->buffer, samples, audioSize);

    return 0;
}

int getVideoFrame( SDL_ffmpegFile* file, AVPacket *pack, AVFrame *inFrameRGB, AVFrame *inFrame, SDL_ffmpegVideoFrame *frame ) {

    int got_frame;

    /* usefull when dealing with B frames */
    if(pack->dts == AV_NOPTS_VALUE) {
        /* if we did not get a valid timestamp, we make one up based on the last
           valid timestamp + the duration of a frame */
        frame->pts = file->videoStream->lastTimeStamp + file->videoStream->timeBase;
    } else {
        /* write timestamp into the buffer */
        frame->pts = file->videoStream->timeBase * pack->dts;
    }

    if(SDL_ffmpegGetPosition(file) > frame->pts) {
        file->videoStream->_ffmpeg->hurry_up = 1;
    } else {
        file->videoStream->_ffmpeg->hurry_up = 0;
    }

    got_frame = 0;

    /* Decode the packet */
    avcodec_decode_video(file->videoStream->_ffmpeg, inFrame, &got_frame, pack->data, pack->size);

    /* if we did not get a frame or we need to hurry, we return */
    if( !got_frame || file->videoStream->_ffmpeg->hurry_up ) return -1;

    /* replace image pointer so data is copied to our frame */
    inFrameRGB->data[0] = frame->buffer->pixels;

    /* we convert whatever type of data we got to RGB24 */
    sws_scale( file->videoStream->_conversion, inFrame->data,
               inFrame->linesize, 0, file->videoStream->height,
               inFrameRGB->data, inFrameRGB->linesize);

    /* write decoding time stamp */
    frame->dts = file->videoStream->dts++;

    /* flag this frame as not-empty */
    frame->filled = 1;

    /* we write the lastTimestamp we got */
    file->videoStream->lastTimeStamp = frame->pts;

    return 0;
}
