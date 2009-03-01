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

/**
    @mainpage
    @version 1.0.0
    @author Arjan Houben

    SDL_ffmpeg is designed with ease of use in mind.
    Even the beginning programmer should be able to use this library
    so he or she can use multimedia in his/her program.
**/

#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include "libavformat/avformat.h"

#include "SDL/SDL_ffmpeg.h"

/**
\cond
*/

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

int getAudioFrame( SDL_ffmpegFile*, AVPacket*, SDL_ffmpegAudioFrame* );

int getVideoFrame( SDL_ffmpegFile*, AVPacket*, SDL_ffmpegVideoFrame* );

void convertYUV420PtoRGBA( AVFrame *YUV420P, SDL_Surface *RGB444, int interlaced );

void convertYUV420PtoYUY2( AVFrame *YUV420P, SDL_Overlay *YUY2, int interlaced, int scaled );

int SDL_ffmpegDecodeThread(void* data);

SDL_ffmpegFile* SDL_ffmpegCreateFile() {

    /* create SDL_ffmpegFile pointer */
    SDL_ffmpegFile *file = (SDL_ffmpegFile*)malloc( sizeof(SDL_ffmpegFile) );
    if(!file) return 0;

    memset( file, 0, sizeof(SDL_ffmpegFile) );

    /* standard preload size is 1MB */
    file->preloadSize = 1024 * 1024;

    return file;
}

/**
\endcond
*/

/** \brief  Use this to free an SDL_ffmpegFile.

            This function stops the decoding thread if needed
            and flushes the buffers before releasing the memory.
            It also invalidates any frames aquired by
            SDL_ffmpegGetVideoFrame( SDL_ffmpegFile* ) or SDL_ffmpegGetAudioFrame( SDL_ffmpegFile* ).
\param      file SDL_ffmpegFile on which an action is required
*/
void SDL_ffmpegFree( SDL_ffmpegFile *file ) {

    if( !file ) return;

    SDL_ffmpegStopDecoding(file);

    SDL_ffmpegFlush(file);

    SDL_ffmpegStream *s = file->vs;
    while( s ) {

        SDL_ffmpegStream *old = s;

        s = s->next;

        SDL_DestroyMutex( old->mutex );

        av_free( old->decodeFrame );

        if( old->_ffmpeg ) avcodec_close( old->_ffmpeg->codec );

        free( old );
    }

    s = file->as;
    while( s ) {

        SDL_ffmpegStream *old = s;

        s = s->next;

        SDL_DestroyMutex( old->mutex );

        av_free( old->sampleBuffer );

        if( old->_ffmpeg ) avcodec_close( old->_ffmpeg->codec );

        free( old );
    }

    if(file->_ffmpeg) av_close_input_file( file->_ffmpeg );

    free(file);
}


/** \brief  Use this to free an SDL_ffmpegAudioFrame.

            This releases all buffers which where allocated in SDL_ffmpegCreateAudioFrame
\param      frame SDL_ffmpegAudioFrame which needs to be deleted
*/
void SDL_ffmpegFreeFrame(SDL_ffmpegAudioFrame* frame) {

    av_free( frame->buffer );

    free( frame );
}


/** \brief  Use this to open the multimedia file of your choice.

            This function is used to open a multimedia file.

\param      filename string containing the location of the file
\returns    a pointer to a SDL_ffmpegFile structure, or NULL if a file could not be opened
*/
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

    /* iterate through all the streams and store audio/video streams */
    for(i=0; i<file->_ffmpeg->nb_streams; i++) {

        if( file->_ffmpeg->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO ) {

            /* if this is a packet of the correct type we create a new stream */
            SDL_ffmpegStream* stream = (SDL_ffmpegStream*)malloc( sizeof(SDL_ffmpegStream) );

            if(stream) {

                /* we set our stream to zero */
                memset(stream, 0, sizeof(SDL_ffmpegStream));

                /* save unique streamid */
                stream->id = i;

                /* _ffmpeg holds data about streamcodec */
                stream->_ffmpeg = file->_ffmpeg->streams[i];

                /* get the correct decoder for this stream */
                codec = avcodec_find_decoder( stream->_ffmpeg->codec->codec_id );

                if(!codec) {
                    free(stream);
                    fprintf(stderr, "could not find codec\n");
                } else if(avcodec_open(file->_ffmpeg->streams[i]->codec, codec) < 0) {
                    free(stream);
                    fprintf(stderr, "could not open decoder\n");
                } else {

                    stream->mutex = SDL_CreateMutex();

                    stream->decodeFrame = avcodec_alloc_frame();

                    SDL_ffmpegStream **s = &file->vs;
                    while( *s ) {
                        *s = (*s)->next;
                    }

                    *s = stream;

                    file->videoStreams++;
                }
            }
        } else if( file->_ffmpeg->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO ) {

            /* if this is a packet of the correct type we create a new stream */
            SDL_ffmpegStream* stream = (SDL_ffmpegStream*)malloc( sizeof(SDL_ffmpegStream) );

            if(stream) {
                /* we set our stream to zero */
                memset(stream, 0, sizeof(SDL_ffmpegStream));

                /* save unique streamid */
                stream->id = i;

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

                    stream->mutex = SDL_CreateMutex();

                    stream->sampleBuffer = av_malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE * sizeof( int16_t ) );
                    stream->sampleBufferSize = 0;
                    stream->sampleBufferOffset = 0;
                    stream->sampleBufferTime = AV_NOPTS_VALUE;

                    SDL_ffmpegStream **s = &file->as;
                    while( *s ) {
                        *s = (*s)->next;
                    }

                    *s = stream;

                    file->audioStreams++;
                }
            }
        }
    }

    return file;
}


/** \brief  Use this to create a SDL_ffmpegAudioFrame

            With this frame, you can receve audio data from the stream using
            SDL_ffmpegGetAudioFrame.
\param      file SDL_ffmpegFile for which a frame needs to be created
\returns    Pointer to SDL_ffmpegAudioFrame, or NULL if no frame could be created
*/
SDL_ffmpegAudioFrame* SDL_ffmpegCreateAudioFrame( SDL_ffmpegFile *file, uint32_t bytes ) {

    if( !file || !file->audioStream || !bytes ) return 0;

    /* allocate new frame */
    SDL_ffmpegAudioFrame *frame = (SDL_ffmpegAudioFrame*)malloc( sizeof( SDL_ffmpegAudioFrame ) );
    memset( frame, 0, sizeof( SDL_ffmpegAudioFrame ) );

    /* set capacity of new frame */
    frame->capacity = bytes;

    /* allocate buffer */
    frame->buffer = av_malloc( bytes );

    /* initialize a non-valid timestamp */
    frame->pts = AV_NOPTS_VALUE;

    return frame;
}


/** \brief  Use this to create a SDL_ffmpegVideoFrame

            With this frame, you can receve video frames from the stream using
            SDL_ffmpegGetVideoFrame.
\param      file SDL_ffmpegFile for which a frame needs to be created
\param      file SDL_ffmpegFile for which a frame needs to be created
\returns    Pointer to SDL_ffmpegVideoFrame, or NULL if no frame could be created
*/
SDL_ffmpegVideoFrame* SDL_ffmpegCreateVideoFrame( const SDL_ffmpegFile *file, const uint32_t format, SDL_Surface *screen ) {

    if( !file || !file->videoStream ) return 0;

    SDL_ffmpegVideoFrame *frame = malloc( sizeof(SDL_ffmpegVideoFrame) );
    memset( frame, 0, sizeof(SDL_ffmpegVideoFrame) );

    if( format == SDL_YUY2_OVERLAY && screen ) {

        frame->overlay = SDL_CreateYUVOverlay( file->videoStream->_ffmpeg->codec->width, file->videoStream->_ffmpeg->codec->height, SDL_YUY2_OVERLAY, screen );
    }

    if( !format ) {

        frame->surface = SDL_CreateRGBSurface( 0, file->videoStream->_ffmpeg->codec->width, file->videoStream->_ffmpeg->codec->height, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 );
    }
}


/** \brief  Use this to get a pointer to a SDL_ffmpegVideoFrame.

            If you receive a frame, it is valid until you receive a new frame, or
            until the file is freed, using SDL_ffmpegFree( SDL_ffmpegFile* ).
\param      file SDL_ffmpegFile from which the information is required
\returns    Pointer to SDL_ffmpegVideoFrame, or NULL if no frame was available.
*/
int SDL_ffmpegGetVideoFrame( SDL_ffmpegFile* file, SDL_ffmpegVideoFrame *frame ) {

    if( !frame || !file || !file->videoStream || file->videoStream->endReached ) return 0;

    /* assume current frame is empty */
    frame->ready = 0;

    SDL_LockMutex( file->videoStream->mutex );

    SDL_ffmpegPacket *pack = file->videoStream->buffer;

    while( pack && !frame->ready ) {

        /* when a frame is received, frame->ready will be set */
        getVideoFrame( file, pack->data, frame );

        /* store used packet for cleaning */
        SDL_ffmpegPacket *used = pack;

        pack = pack->next;

        /* destroy used packet */
        av_free_packet( used->data );
        free( used );
    }

    /* next packet will be our starting point */
    file->videoStream->buffer = pack;

    SDL_UnlockMutex( file->videoStream->mutex );

    return frame->ready;
}


/** \brief  Get the desired audio stream from file.

            This returns a pointer to the requested stream. With this stream pointer you can
            get information about the stream, like language, samplerate, size etc.
            Based on this information you can choose the stream you want to use.
\param      file SDL_ffmpegFile from which the information is required
\param      audioID is the stream you whish to select.
\returns    Pointer to SDL_ffmpegStream, or NULL if selected stream could not be found
*/
SDL_ffmpegStream* SDL_ffmpegGetAudioStream(SDL_ffmpegFile *file, uint32_t audioID) {

    int i;
    SDL_ffmpegStream *s;

    /* check if we have any audiostreams */
    if( !file || !file->audioStreams ) return 0;

    /* check if the requested id is possible */
    if( audioID >= file->audioStreams ) return 0;

    s = file->as;

    /* return audiostream linked to audioID */
    for(i=0; i<audioID && s; i++) s = s->next;

    return s;
}


/** \brief  Select an audio stream from file.

            Use this function to select an audio stream for decoding.
            Using SDL_ffmpegGetAudioStream you can get information about the streams.
            Based on that you can chose the stream you want.
\param      file SDL_ffmpegFile on which an action is required
\param      audioID is the stream you whish to select. negative values de-select any audio stream.
\returns    -1 on error, otherwise 0
*/
int SDL_ffmpegSelectAudioStream(SDL_ffmpegFile* file, int audioID) {

    int i;

    /* check if we have any audiostreams */
    if( !file || !file->audioStreams ) return -1;

    /* check if the requested id is possible */
    if( audioID >= file->audioStreams ) return -1;

    if( audioID < 0 ) {

        /* reset audiostream */
        file->audioStream = 0;

    } else {

        /* set current audiostream to stream linked to audioID */
        file->audioStream = file->as;

        for(i=0; i<audioID && file->audioStream; i++) file->audioStream = file->audioStream->next;
    }

    return 0;
}


/** \brief  Get the desired video stream from file.

            This returns a pointer to the requested stream. With this stream pointer you can
            get information about the stream, like language, samplerate, size etc.
            Based on this information you can choose the stream you want to use.
\param      file SDL_ffmpegFile from which the information is required
\param      videoID is the stream you whish to select.
\returns    Pointer to SDL_ffmpegStream, or NULL if selected stream could not be found
*/
SDL_ffmpegStream* SDL_ffmpegGetVideoStream(SDL_ffmpegFile *file, uint32_t videoID) {

    int i;
    SDL_ffmpegStream *s;

    /* check if we have any audiostreams */
    if( !file || !file->videoStreams ) return 0;

    /* check if the requested id is possible */
    if( videoID >= file->videoStreams ) return 0;

    s = file->vs;

    /* return audiostream linked to audioID */
    for(i=0; i<videoID && s; i++) s = s->next;

    return s;
}


/** \brief  Select a video stream from file.

            Use this function to select a video stream for decoding.
            Using SDL_ffmpegGetVideoStream you can get information about the streams.
            Based on that you can chose the stream you want.
\param      file SDL_ffmpegFile on which an action is required
\param      videoID is the stream you whish to select.
\returns    -1 on error, otherwise 0
*/
int SDL_ffmpegSelectVideoStream(SDL_ffmpegFile* file, int videoID) {

    int i;

    /* check if we have any videostreams */
    if( !file || !file->videoStreams ) return -1;

    /* check if the requested id is possible */
    if( videoID >= file->videoStreams ) return -1;

    if( videoID < 0 ) {

        /* reset videostream */
        file->videoStream = 0;

    } else {

        /* set current videostream to stream linked to videoID */
        file->videoStream = file->vs;

        /* keep searching for correct videostream */
        for(i=0; i<videoID && file->videoStream; i++) file->videoStream = file->videoStream->next;

        /* check if pixel format is supported */
        if( file->videoStream->_ffmpeg->codec->pix_fmt != PIX_FMT_YUV420P &&
            file->videoStream->_ffmpeg->codec->pix_fmt != PIX_FMT_YUVJ420P ) {
            printf("unsupported pixel format [%i]\n", file->videoStream->_ffmpeg->codec->pix_fmt);
            file->videoStream = 0;
        }
    }

    return 0;
}

/** \brief  Starts decoding the file.

            After you call this function, a thread starts filling the buffers with the
            streams you selected. If you only want audio, you shouldn't select a video
            stream and vice versa. Doing so would mean extra work for the cpu.
\param      file SDL_ffmpegFile on which an action is required
\returns    -1 on error, otherwise 0
*/
int SDL_ffmpegStartDecoding(SDL_ffmpegFile* file) {

    if( !file ) return -1;

    /* start a thread that continues to fill audio/video buffers */
    if(!file->threadID) file->threadID = SDL_CreateThread(SDL_ffmpegDecodeThread, file);

    return 0;
}


/** \brief  Stops decoding the file.

            This stops the decodethread, this doesn't flush the buffers so you can use the
            data in them, even after you called this function.
\param      file SDL_ffmpegFile on which an action is required
\returns    -1 on error, otherwise 0
*/
int SDL_ffmpegStopDecoding(SDL_ffmpegFile* file) {

    if( !file ) return -1;

    /* stop decode thread */
    file->threadActive = 0;
    if(file->threadID) SDL_WaitThread(file->threadID, 0);

    /* set threadID to zero, so we can check for concurrent threads */
    file->threadID = 0;

    return -1;
}

/** \brief  Seek to a certain point in file.

            Tries to seek to specified point in file.
\param      file SDL_ffmpegFile on which an action is required
\param      timestamp is represented in milliseconds.
\returns    -1 on error, otherwise 0
*/
int SDL_ffmpegSeek(SDL_ffmpegFile* file, uint64_t timestamp) {

    if( !file || SDL_ffmpegGetDuration( file ) < timestamp ) return -1;

    /* set new timestamp */
    file->seekPosition = timestamp;

    /* flush buffers */
    SDL_ffmpegFlush( file );

    return 0;
}

/** \brief  Seek to a relative point in file.

            Tries to seek to new location, based on current location in file.
\param      file SDL_ffmpegFile on which an action is required
\param      timestamp is represented in milliseconds.
\returns    -1 on error, otherwise 0
*/
int SDL_ffmpegSeekRelative(SDL_ffmpegFile *file, int64_t timestamp) {

    /* same thing as normal seek, just take into account the current position */
    return SDL_ffmpegSeek(file, SDL_ffmpegGetPosition(file) + timestamp);
}

/**
\cond
*/
int SDL_ffmpegFlush(SDL_ffmpegFile *file) {

    int i;

    /* check for file and permission to flush buffers */
    if( !file ) return -1;

    /* if we have a valid audio stream, we flush it */
    if( file->audioStream ) {

        SDL_LockMutex( file->audioStream->mutex );

        SDL_ffmpegPacket *pack = file->audioStream->buffer;

        while( pack ) {

            SDL_ffmpegPacket *old = pack;

            pack = pack->next;

            av_free( old->data );
            free( old );
        }

        file->audioStream->buffer = 0;

        /* flush internal ffmpeg buffers */
        if( file->audioStream->_ffmpeg ) {

            avcodec_flush_buffers( file->audioStream->_ffmpeg->codec );
        }

        SDL_UnlockMutex( file->audioStream->mutex );
    }

    /* if we have a valid video stream, we flush some more */
    if( file->videoStream ) {

        SDL_LockMutex( file->videoStream->mutex );

        SDL_ffmpegPacket *pack = file->videoStream->buffer;

        while( pack ) {

            SDL_ffmpegPacket *old = pack;

            pack = pack->next;

            av_free( old->data );
            free( old );
        }

        file->videoStream->buffer = 0;

        /* flush internal ffmpeg buffers */
        if(file->videoStream->_ffmpeg) avcodec_flush_buffers( file->videoStream->_ffmpeg->codec );

        SDL_UnlockMutex( file->videoStream->mutex );
    }

    return 0;
}
/**
\endcond
*/


/** \brief  Use this to get a pointer to a SDL_ffmpegAudioFrame.

            If you receive a frame, it is valid until you receive a new frame, or
            until the file is freed, using SDL_ffmpegFree( SDL_ffmpegFile* ).
            I you use data from the frame, you should adjust the size member by
            the amount of data used in bytes. This is needed so that SDL_ffmpeg can
            calculate the next frame.
\param      file SDL_ffmpegFile from which the information is required
\returns    Pointer to SDL_ffmpegAudioFrame, or NULL if no frame was available.
*/
int SDL_ffmpegGetAudioFrame( SDL_ffmpegFile *file, SDL_ffmpegAudioFrame *frame ) {

    if( !frame || !file || !file->audioStream || file->audioStream->endReached ) return 0;

    SDL_LockMutex( file->audioStream->mutex );

    SDL_ffmpegPacket *pack = file->audioStream->buffer;

    while( pack && frame->size < frame->capacity ) {

        /* keep collecting data until frame has reached capacity
           getAudioFrame will return true if data from pack was used */
        if( getAudioFrame( file, pack->data, frame ) ) {

            /* store used packet for cleaning */
            SDL_ffmpegPacket *used = pack;

            pack = pack->next;

            /* destroy used packet */
            av_free_packet( used->data );
            free( used );
        }
    }

    /* next packet will be our starting point */
    file->audioStream->buffer = pack;

    SDL_UnlockMutex( file->audioStream->mutex );

    return ( frame->size == frame->capacity );
}


/** \brief  Returns the current position of the file in milliseconds.

\param      file SDL_ffmpegFile from which the information is required
\returns    -1 on error, otherwise the length of the file in milliseconds
*/
int64_t SDL_ffmpegGetPosition(SDL_ffmpegFile *file) {

    if( !file ) return -1;

    /* return the current playposition of our file */
    return 0;
}


/** \brief  This can be used to get a SDL_AudioSpec based on values found in file

            This returns a SDL_AudioSpec, if you have selected a valid audio
            stream. Otherwise, default values are used. 0 if valid audiostream was found, 1 there was no valid audio stream found.
\param      file SDL_ffmpegFile from which the information is required
\param      samples Amount of samples required every time the callback is called.
            Lower values mean less latency, but please note that SDL has a minimal value.
\param      callback Pointer to callback function
\returns    SDL_AudioSpec with values ready in according to the selected audio stream.
            If no valid audio stream was available, all values of returned SDL_AudioSpec are set to 0
*/
SDL_AudioSpec SDL_ffmpegGetAudioSpec(SDL_ffmpegFile *file, int samples, SDL_ffmpegCallback callback) {

    /* create audio spec */
    SDL_AudioSpec spec;

    memset(&spec, 0, sizeof(SDL_AudioSpec));

    /* if we have a valid audiofile, we can use its data to create a
       more appropriate audio spec */
    if( file && file->audioStream ) {

        spec.format = AUDIO_S16SYS;
        spec.samples = samples;
        spec.userdata = file;
        spec.callback = callback;
        spec.freq = file->audioStream->_ffmpeg->codec->sample_rate;
        spec.channels = file->audioStream->_ffmpeg->codec->channels;
    }

    return spec;
}


/** \brief  Returns the Duration of the file in milliseconds.

            Please note that this value is guestimated by FFmpeg, it may differ from
            actual playing time.
\param      file SDL_ffmpegFile from which the information is required
\returns    -1 on error, otherwise the length of the file in milliseconds
*/
uint64_t SDL_ffmpegGetDuration(SDL_ffmpegFile *file) {

    if( !file ) return -1;
    /* returns the duration of the entire file, please note that ffmpeg doesn't
       always get this value right! so don't bet your life on it... */
    return file->_ffmpeg->duration / (AV_TIME_BASE / 1000);
}

/** \brief  retreive the width/height of a frame beloning to file

            With this function you can get the width and height of a frame, belonging to
            your file. If there is no (valid) videostream selected w and h default
            to 320x240. Please not that you will have to make sure the pointers are
            allocated.

\param      file SDL_ffmpegFile from which the information is required
\param      w width
\param      h height
\returns    -1 on error, otherwise 0
*/
int SDL_ffmpegGetVideoSize(SDL_ffmpegFile *file, int *w, int *h) {

    if(!w || !h) return -1;

    /* if we have a valid video file selected, we use it
       if not, we send default values and return.
       by checking the return value you can check if you got a valid size */
    if( file->videoStream ) {
        *w = file->videoStream->_ffmpeg->codec->width;
        *h = file->videoStream->_ffmpeg->codec->height;
        return 0;
    }

    *w = 320;
    *h = 240;
    return -1;
}


/** \brief  This is used to check if a valid audio stream is selected.

\param      file SDL_ffmpegFile from which the information is required
\returns    -1 on error, otherwise 0
*/
int SDL_ffmpegValidAudio(SDL_ffmpegFile* file) {

    /* this function is used to check if we selected a valid audio stream */
    if(file->audioStream) return 1;

    return 0;
}


/** \brief  This is used to check if a valid video stream is selected.

\param      file SDL_ffmpegFile from which the information is required
\returns    -1 on error, otherwise 0
*/
int SDL_ffmpegValidVideo(SDL_ffmpegFile* file) {

    /* this function is used to check if we selected a valid video stream */
    if(file->videoStream) return 1;

    return 0;
}


/** \brief  This is used to check the amount of data which is preloaded.

\param      file SDL_ffmpegFile from which the information is required
\returns    the amount of bytes preloaded by file
*/
int SDL_ffmpegPreloaded(SDL_ffmpegFile *file) {

    int size = 0;

    if( file->audioStream ) {

        SDL_LockMutex( file->audioStream->mutex );

        SDL_ffmpegPacket *pack = file->audioStream->buffer;

        while( pack ) {

            size += pack->data->size;

            pack = pack->next;
        }

        SDL_UnlockMutex( file->audioStream->mutex );
    }

    if( file->videoStream ) {

        SDL_LockMutex( file->videoStream->mutex );

        SDL_ffmpegPacket *pack = file->videoStream->buffer;

        while( pack ) {

            size += pack->data->size;

            pack = pack->next;
        }

        SDL_UnlockMutex( file->videoStream->mutex );
    }

    return size;
}

/**
\cond
*/
int SDL_ffmpegDecodeThread(void* data) {

    SDL_ffmpegFile *file;
	SDL_ffmpegVideoFrame *lastVideoFrame;
	SDL_ffmpegAudioFrame *lastAudioFrame;
    AVFrame *inFrame;
    int64_t seekPos;
    int16_t *samples;
    int decode, a, i, streamLooped = 0, checkBufferedFrame = 0, packetNeedsHandling = 0;
    AVPacket *pack;

    /* if we got invalid data, return */
    if(!data) return -1;

    /* unpack the void pointer */
    file = (SDL_ffmpegFile*)data;

    /* flag this thread as active, used for stopping */
    file->threadActive = 1;

    /* allocate a frame */
    inFrame = avcodec_alloc_frame();
    if( !inFrame ) return -1;

    /* allocate temporary buffer */
    samples = (int16_t*)av_malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE * sizeof(int16_t) );
    if( !samples ) {
        return -1;
    }

    file->minimalTimestamp = 0;

	lastVideoFrame = 0;
	lastAudioFrame = 0;

    while( file->threadActive ) {

        /* check if a seek operation is pending */
        if( file->seekPosition >= 0 ) {

            /* convert milliseconds to AV_TIME_BASE units */
            seekPos = file->seekPosition * (AV_TIME_BASE / 1000);

            /* AVSEEK_FLAG_BACKWARD means we jump to the first keyframe before seekPos */
            av_seek_frame(file->_ffmpeg, -1, seekPos, AVSEEK_FLAG_BACKWARD);

            /* determine the reason for this seek action */
            if( streamLooped ) {

                file->minimalTimestamp = 0;

            } else {

                file->minimalTimestamp = file->seekPosition;

                SDL_ffmpegFlush(file);
            }

            file->seekPosition = -1;
        }

        if( SDL_ffmpegPreloaded( file ) >= file->preloadSize ) {

            SDL_Delay(1);

            continue;
        }

        /* create a packet for our data */
        AVPacket *pack = av_malloc( sizeof(AVPacket) );

        /* initialize packet */
        av_init_packet( pack );

        /* read a packet from the file */
        decode = av_read_frame( file->_ffmpeg, pack );

        /* if we did not get a packet, we seek to begin and try again */
        if( decode < 0 ) {

            streamLooped = 1;
            file->seekPosition = 0;

            /* first lets check if there is still a frame in the buffer */
            if( checkBufferedFrame && file->videoStream && file->videoStream->id >= 0 ) {

//                /* prepare packet to check for buffered frame */
//                pack.data = 0;
//                pack.size = 0;
//                pack.stream_index = file->videoStream->id;
//
//                /* make sure we only check once for a buffered frame */
//                checkBufferedFrame = 0;

            } else {

                streamLooped = 1;
                file->seekPosition = 0;

                /* last frame should be flagged as such */
                if( lastAudioFrame ) lastAudioFrame->last = 1;

                if( lastVideoFrame ) lastVideoFrame->last = 1;

                continue;
            }

        } else {

            streamLooped = 0;
            checkBufferedFrame = 1;

            /* packet needs to be handled */
            packetNeedsHandling = 1;
        }

        /* we got a packet, lets handle it */

        /* try to allocate the packet */
        if( av_dup_packet( pack ) ) {

            /* error allocating packet */
            av_free_packet( pack );

        } else {

            /* store pointer to packet */
            SDL_ffmpegPacket *temp = (SDL_ffmpegPacket*)malloc( sizeof(SDL_ffmpegPacket) );
            temp->data = pack;
            temp->next = 0;

            /* If it's an audio packet from our stream... */
            if( file->audioStream && pack->stream_index == file->audioStream->id ) {

                /* store pointer to packet */
                SDL_ffmpegPacket *temp = (SDL_ffmpegPacket*)malloc( sizeof(SDL_ffmpegPacket) );
                temp->data = pack;
                temp->next = 0;

                SDL_LockMutex( file->audioStream->mutex );

                SDL_ffmpegPacket **p = &file->audioStream->buffer;

                /* find first empty position in the buffer */
                while( *p ) p = &((*p)->next);

                *p = temp;

                SDL_UnlockMutex( file->audioStream->mutex );
            }

            else

            /* If it's a video packet from our video stream... */
            if( file->videoStream && pack->stream_index == file->videoStream->id ) {

                /* store pointer to packet */
                SDL_ffmpegPacket *temp = (SDL_ffmpegPacket*)malloc( sizeof(SDL_ffmpegPacket) );
                temp->data = pack;
                temp->next = 0;

                SDL_LockMutex( file->videoStream->mutex );

                SDL_ffmpegPacket **p = &file->videoStream->buffer;

                /* find first empty position in the buffer */
                while( *p ) p = &((*p)->next);

                *p = temp;

                SDL_UnlockMutex( file->videoStream->mutex );

            } else {

                av_free_packet( pack );
            }
        }

        SDL_Delay(0);
    }

    return 0;
}

int getAudioFrame( SDL_ffmpegFile *file, AVPacket *pack, SDL_ffmpegAudioFrame *frame ) {

    uint8_t *data;
    int size,
        len,
        audioSize;

    data = pack->data;
    size = pack->size;
    audioSize = AVCODEC_MAX_AUDIO_FRAME_SIZE * sizeof( int16_t );

    /* check if there is still data in the buffer */
    if( file->audioStream->sampleBufferSize ) {

        /* set new pts */
        if( !frame->size ) frame->pts = file->audioStream->sampleBufferTime;

        /* calculate free space in frame */
        int fs = frame->capacity - frame->size;

        /* check the amount of data which needs to be copied */
        if( fs < file->audioStream->sampleBufferSize ) {

            /* copy data from sampleBuffer into frame buffer until frame buffer is full */
            memcpy( frame->buffer+frame->size, file->audioStream->sampleBuffer+file->audioStream->sampleBufferOffset, fs );

            /* mark the amount of bytes still in the buffer */
            file->audioStream->sampleBufferSize -= fs;

            /* move offset accordingly */
            file->audioStream->sampleBufferOffset += fs;

            /* update framesize */
            frame->size = frame->capacity;

        } else {

            /* copy data from sampleBuffer into frame buffer until sampleBuffer is empty */
            memcpy( frame->buffer+frame->size, file->audioStream->sampleBuffer+file->audioStream->sampleBufferOffset, file->audioStream->sampleBufferSize );

            /* update framesize */
            frame->size += file->audioStream->sampleBufferSize;

            /* at this point, samplebuffer should have been handled */
            file->audioStream->sampleBufferSize = 0;

            /* no more data in buffer, reset offset */
            file->audioStream->sampleBufferOffset = 0;
        }

        /* return 0 to signal caller that 'pack' was not used */
        if( frame->size == frame->capacity ) return 0;
    }

    file->audioStream->_ffmpeg->codec->hurry_up = 0;

    /* calculate pts to determine wheter or not this frame should be stored */
    file->audioStream->sampleBufferTime = av_rescale( (pack->dts-file->audioStream->_ffmpeg->start_time)*1000, file->audioStream->_ffmpeg->time_base.num, file->audioStream->_ffmpeg->time_base.den );

    /* don't decode packets which are too old anyway */
    if( file->audioStream->sampleBufferTime != AV_NOPTS_VALUE && file->audioStream->sampleBufferTime < file->minimalTimestamp ) {

        file->audioStream->_ffmpeg->codec->hurry_up = 1;
    }

	while( size > 0 && file->threadActive ) {

		/* Decode the packet */
		len = avcodec_decode_audio2( file->audioStream->_ffmpeg->codec, (int16_t*)file->audioStream->sampleBuffer, &audioSize, data, size );

		/* if an error occured, we skip the frame */
		if( len <= 0 || !audioSize ) break;

		/* change pointers */
		data += len;
		size -= len;
	}

	if( !file->audioStream->_ffmpeg->codec->hurry_up ) {

        /* set new pts */
        if( !frame->size ) frame->pts = file->audioStream->sampleBufferTime;

        /* room in frame */
        int fs = frame->capacity - frame->size;

        /* check if there is room at all */
        if( fs ) {

            /* check the amount of data which needs to be copied */
            if( fs < audioSize ) {

                /* copy data from sampleBuffer into frame buffer until frame buffer is full */
                memcpy( frame->buffer+frame->size, file->audioStream->sampleBuffer, fs );

                /* mark the amount of bytes still in the buffer */
                file->audioStream->sampleBufferSize = audioSize - fs;

                /* set the offset so the remaining data can be found */
                file->audioStream->sampleBufferOffset = fs;

                /* update framesize */
                frame->size = frame->capacity;

            } else {

                /* copy data from sampleBuffer into frame buffer until sampleBuffer is empty */
                memcpy( frame->buffer+frame->size, file->audioStream->sampleBuffer, audioSize );

                /* mark the amount of bytes still in the buffer */
                file->audioStream->sampleBufferSize = 0;

                /* reset buffer offset */
                file->audioStream->sampleBufferOffset = 0;

                /* update framesize */
                frame->size += audioSize;
            }

        } else {

            /* no room in frame, mark samplebuffer as full */
            file->audioStream->sampleBufferSize = audioSize;

            /* reset buffer offset */
            file->audioStream->sampleBufferOffset = 0;
        }
    }

    /* pack was used, return 1 */
    return 1;
}

int getVideoFrame( SDL_ffmpegFile* file, AVPacket *pack, SDL_ffmpegVideoFrame *frame ) {

    int got_frame;

    /* usefull when dealing with B frames */
    if(pack->dts == AV_NOPTS_VALUE) {
        /* if we did not get a valid timestamp, we make one up based on the last
           valid timestamp + the duration of a frame */
//        frame->pts = file->videoStream->lastTimeStamp + file->videoStream->timeBase;
    } else {
        /* write timestamp into the buffer */
        frame->pts = av_rescale((pack->dts-file->videoStream->_ffmpeg->start_time)*1000, file->videoStream->_ffmpeg->time_base.num, file->videoStream->_ffmpeg->time_base.den);
    }

	if( frame->pts != AV_NOPTS_VALUE && frame->pts < file->minimalTimestamp ) {

		file->videoStream->_ffmpeg->codec->hurry_up = 1;

    } else {

        file->videoStream->_ffmpeg->codec->hurry_up = 0;
    }

    got_frame = 0;

    /* Decode the packet */
    avcodec_decode_video( file->videoStream->_ffmpeg->codec, file->videoStream->decodeFrame, &got_frame, pack->data, pack->size);

    /* if we did not get a frame or we need to hurry, we return */
    if( got_frame && !file->videoStream->_ffmpeg->codec->hurry_up ) {

        int scaled = 1;

        /* if YUV data is scaled in the range of 8 - 235 instead of 0 - 255, we need to take this into account */
        if( file->videoStream->_ffmpeg->codec->pix_fmt == PIX_FMT_YUVJ420P ||
            file->videoStream->_ffmpeg->codec->pix_fmt == PIX_FMT_YUVJ422P ||
            file->videoStream->_ffmpeg->codec->pix_fmt == PIX_FMT_YUVJ444P ) scaled = 0;

        /* convert YUV 420 to YUYV 422 data */
        if( frame->overlay && frame->overlay->format == SDL_YUY2_OVERLAY ) {

            convertYUV420PtoYUY2( file->videoStream->decodeFrame, frame->overlay, file->videoStream->decodeFrame->interlaced_frame, scaled );
        }

        /* convert YUV to RGB data */
        if( frame->surface ) {

			convertYUV420PtoRGBA( file->videoStream->decodeFrame, frame->surface, file->videoStream->decodeFrame->interlaced_frame );
        }

		/* we write the lastTimestamp we got */
		file->videoStream->lastTimeStamp = frame->pts;

		/* flag this frame as ready */
		frame->ready = 1;
	}

    return frame->ready;
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
    int x, y;

    for(y=0; y<OUTPUT->h; y++){

        Y = YUV420P->data[0] + YUV420P->linesize[0] * y;
        U = YUV420P->data[1] + YUV420P->linesize[1] * (y/2);
        V = YUV420P->data[2] + YUV420P->linesize[2] * (y/2);

		/* make sure we deinterlace before upsampling */
		if( interlaced ) {
            /* y & 3 means y % 3, but this should be faster */
			/* on scanline 2 and 3 we need to look at different lines */
			if( y & 3 == 1 ) {
				U += YUV420P->linesize[1];
				V += YUV420P->linesize[2];
			} else if( y & 3 == 2 ) {
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
            U += x&1;
			V += x&1;
        }
    }
}

#define SCALE(i) (uint8_t)(((float)(i-8)/(255-16))*255)

void convertYUV420PtoYUY2scanline( const uint8_t *Y, const uint8_t *U, const uint8_t *V, uint32_t *YUVpacked, int w, int scaled ) {

    /* devide width by 2 */
    w >>= 1;

    if( scaled ) {

        while( w-- ) {

            /* Y0 U0 Y1 V0 */
            *YUVpacked = SCALE(*Y) | (SCALE(*U) << 8) | (SCALE(*(++Y)) << 16) | (SCALE(*V) << 24);
            YUVpacked++;
            Y++;
            U++;
            V++;
        }

    } else {

        while( w-- ) {

            /* Y0 U0 Y1 V0 */
            *YUVpacked = (*Y) | ((*U) << 8) | ((*(++Y)) << 16) | ((*V) << 24);
            YUVpacked++;
            Y++;
            U++;
            V++;
        }

    }
}

void convertYUV420PtoYUY2( AVFrame *YUV420P, SDL_Overlay *YUY2, int interlaced, int scaled ) {

    int y;

    const uint8_t   *Y = YUV420P->data[0],
                    *U = YUV420P->data[1],
                    *V = YUV420P->data[2];

    uint8_t *YUVpacked = YUY2->pixels[0];

    SDL_LockYUVOverlay( YUY2 );

    if( interlaced ) {

        /* handle 4 lines per loop */
        for(y=0; y<(YUY2->h>>2); y++){

            /* line 0 */
            convertYUV420PtoYUY2scanline( Y, U, V, (uint32_t*)YUVpacked, YUY2->w, scaled );
            YUVpacked += YUY2->pitches[0];
            Y += YUV420P->linesize[0];
            U += YUV420P->linesize[1];
            V += YUV420P->linesize[2];

            /* line 1 */
            convertYUV420PtoYUY2scanline( Y, U, V, (uint32_t*)YUVpacked, YUY2->w, scaled );
            YUVpacked += YUY2->pitches[0];
            Y += YUV420P->linesize[0];
            U -= YUV420P->linesize[1];
            V -= YUV420P->linesize[2];

            /* line 2 */
            convertYUV420PtoYUY2scanline( Y, U, V, (uint32_t*)YUVpacked, YUY2->w, scaled );
            YUVpacked += YUY2->pitches[0];
            Y += YUV420P->linesize[0];
            U += YUV420P->linesize[1];
            V += YUV420P->linesize[2];

            /* line 3 */
            convertYUV420PtoYUY2scanline( Y, U, V, (uint32_t*)YUVpacked, YUY2->w, scaled );
            YUVpacked += YUY2->pitches[0];
            Y += YUV420P->linesize[0];
            U += YUV420P->linesize[1];
            V += YUV420P->linesize[2];
        }

    } else {

        /* handle 2 lines per loop */
        for(y=0; y<(YUY2->h>>1); y++){

            /* line 0 */
            convertYUV420PtoYUY2scanline( Y, U, V, (uint32_t*)YUVpacked, YUY2->w, scaled );
            YUVpacked += YUY2->pitches[0];
            Y += YUV420P->linesize[0];
            U += YUV420P->linesize[1];
            V += YUV420P->linesize[2];

            /* line 1 */
            convertYUV420PtoYUY2scanline( Y, U, V, (uint32_t*)YUVpacked, YUY2->w, scaled );
            YUVpacked += YUY2->pitches[0];
            Y += YUV420P->linesize[0];
        }

    }

    SDL_UnlockYUVOverlay( YUY2 );
}

/**
\endcond
*/
