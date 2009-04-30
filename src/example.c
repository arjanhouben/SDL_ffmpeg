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

#include "SDL.h"
#include "SDL_ffmpeg.h"

#include <string.h>

/* as an example we create an audio buffer consisting of BUF_SIZE frames */
#define BUF_SIZE 10

/* pointer to file we will be opening */
SDL_ffmpegFile *file = 0;

/* create a buffer for audio frames */
SDL_ffmpegAudioFrame *audioFrame[BUF_SIZE];

/* simple way of syncing, just for example purposes */
uint64_t sync = 0,
         offset = 0;

/* returns the current position the file should be at */
uint64_t getSync() {
    if( file ) {
        if( SDL_ffmpegValidAudio(file) ) {
            return sync;
        }
        if( SDL_ffmpegValidVideo(file) ) {
            return ( SDL_GetTicks() % SDL_ffmpegDuration(file) ) + offset;
        }
    }
    return 0;
}

/* use a mutex to prevent errors due to multithreading */
SDL_mutex *mutex = 0;

void audioCallback( void *data, Uint8 *stream, int length ) {

    /* lock mutex, so audioFrame[0] will not be changed from another thread */
    SDL_LockMutex( mutex );

    if( audioFrame[0]->size == length ) {

        /* update sync */
        sync = audioFrame[0]->pts;

        /* copy the data to the output */
        memcpy( stream, audioFrame[0]->buffer, audioFrame[0]->size );

        /* mark data as used */
        audioFrame[0]->size = 0;

        /* move frames in buffer */
        SDL_ffmpegAudioFrame *f = audioFrame[0];
        for(int i=1; i<BUF_SIZE; i++) audioFrame[i-1] = audioFrame[i];
        audioFrame[BUF_SIZE-1] = f;

    } else {

        /* no data available, just set output to zero */
        memset( stream, 0, length );
    }

    /* were done with the audio frame, release lock */
    SDL_UnlockMutex( mutex );

    return;
}

int main(int argc, char** argv) {

    /* check if we got an argument */
    if(argc < 2) {
        printf( "usage: \"%s\" \"filename\"\n", argv[0] );
        return -1;
    }

    /* standard SDL initialization stuff */
    if( SDL_Init( SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_TIMER ) < 0 ) {
        fprintf( stderr, "problem initializing SDL: %s\n", SDL_GetError() );
        return -1;
    }

    /* open file from arg[1] */
    file = SDL_ffmpegOpen( argv[1] );
    if( !file ) {
        SDL_ffmpegPrintErrors( stderr );
        SDL_Quit();
        return -1;
    }

    /* initialize the mutex */
    mutex = SDL_CreateMutex();

    /* select the stream you want to decode (example just uses 0 as a default) */
    SDL_ffmpegSelectVideoStream( file, 0 );

    /* if no audio can be selected, audio will not be used in this example */
    SDL_ffmpegSelectAudioStream( file, 0 );

    /* get the audiospec which fits the selected audiostream, if no audiostream
       is selected, default values are used (2 channel, 48Khz) */
    SDL_AudioSpec specs = SDL_ffmpegGetAudioSpec( file, 512, audioCallback );

    /* we get the size from our active video stream, if no active video stream
       exists, width and height are set to zero */
    int w, h;
    SDL_ffmpegGetVideoSize( file, &w, &h );

    /* Open the Video device */
    SDL_Surface *screen = SDL_SetVideoMode( w, h, 0, SDL_DOUBLEBUF|SDL_HWSURFACE );
    if( !screen ) {
        fprintf( stderr, "Couldn't open video: %s\n", SDL_GetError() );
        SDL_Quit();
        return -1;
    }

    /* create a video frame which will be used to receive the video data.
       If you want to receive YCbCr data, you have to define the second parameter
       with the format you would like to receive and the last parameter needs to
       be a pointer to the SDL_surface as returned by SDL_SetVideoMode */
    SDL_ffmpegVideoFrame *videoFrame = SDL_ffmpegCreateVideoFrame( file, SDL_YUY2_OVERLAY, screen );

    /* create a SDL_Rect for blitting of image data */
    SDL_Rect rect;
    rect.x = 0;    rect.y = 0;
    rect.w = w;    rect.h = h;

    /* check if a valid audio stream was selected */
    if( SDL_ffmpegValidAudio(file) ) {

        /* Open the Audio device */
        if( SDL_OpenAudio( &specs, 0 ) < 0 ) {
            fprintf( stderr, "Couldn't open audio: %s\n", SDL_GetError() );
            SDL_Quit();
            return -1;
        }

        /* calculate frame size ( 2 bytes per sample ) */
        int frameSize = specs.channels * specs.samples * 2;

        /* prepare audio buffer */
        for(int i=0; i<BUF_SIZE; i++) {

            /* create frame */
            audioFrame[i] = SDL_ffmpegCreateAudioFrame( file, frameSize );

            /* check if we got a frame */
            if( !audioFrame[i] ) {
                /* no frame could be created, this is fatal */
                goto CLEANUP_DATA;
            }

            /* fill frame with data */
            SDL_ffmpegGetAudioFrame( file, audioFrame[i] );
        }

        /* we unpause the audio so our audiobuffer gets read */
        SDL_PauseAudio( 0 );
    }

    int done = 0,
        mouseState = 0,
        time = SDL_GetTicks();

    while( !done ) {

        /* just some standard SDL event stuff */
        SDL_Event event;
        while( SDL_PollEvent( &event ) ) {

            if( event.type == SDL_QUIT ) {
                done = 1;
                break;
            }

            if( event.type == SDL_MOUSEBUTTONUP ) mouseState = 0;

            if( event.type == SDL_MOUSEBUTTONDOWN ) mouseState = 1;

            if( mouseState ) {

                /* parse events */
                SDL_PumpEvents();

                int x, y;
                SDL_GetMouseState( &x, &y );
                /* by clicking you turn on the stream, seeking to the percentage
                   in time, based on the x-position you clicked on */
                uint64_t time = (uint64_t)(((double)x / (double)w) * SDL_ffmpegDuration( file ));


                /* lock mutex when working on data which is shared with the audiocallback */
                SDL_LockMutex( mutex );

                /* invalidate current video frame */
                if( videoFrame ) videoFrame->ready = 0;

                /* invalidate buffered audio frames */
                if( SDL_ffmpegValidAudio(file) ) {
                    for(int i=0; i<BUF_SIZE; i++) {
                        audioFrame[i]->size = 0;
                    }
                }

                /* we seek to time (milliseconds) */
                SDL_ffmpegSeek( file, time );

                /* store new offset */
                offset = time - (getSync() - offset);

                /* we release the mutex so the new data can be handled */
                SDL_UnlockMutex( mutex );
            }
        }

        /* check if we need to decode audio data */
        if( SDL_ffmpegValidAudio(file) ) {

            /* lock mutex when working on data which is shared with the audiocallback */
            SDL_LockMutex( mutex );

            /* fill empty spaces in audio buffer */
            for(int i=0; i<BUF_SIZE; i++) {
                /* check if frame is empty */
                if( !audioFrame[i]->size ) {
                    /* fill frame with new data */
                    SDL_ffmpegGetAudioFrame( file, audioFrame[i] );
                }
            }

            SDL_UnlockMutex( mutex );
        }

        if( videoFrame ) {

            /* check if video frame is ready */
            if( !videoFrame->ready ) {

                /* not ready, try to get a new frame */
                SDL_ffmpegGetVideoFrame( file, videoFrame );

            } else if( videoFrame->pts <= getSync() ) {

                /* video frame ready and in sync */

                if( videoFrame->overlay ) {

                    /* blit overlay */
                    SDL_DisplayYUVOverlay( videoFrame->overlay, &rect );

                } else if( videoFrame->surface ) {

                    /* blit RGB surface */
                    SDL_BlitSurface( videoFrame->surface, 0, screen, 0 );

                    /* flip screen */
                    SDL_Flip( screen );
                }

                /* video frame is displayed, make sure we don't show it again */
                videoFrame->ready = 0;
            }
        }

        if( SDL_GetTicks() - time < 5 ) {
            /* we wish not to kill our poor cpu, so we give it some timeoff */
            SDL_Delay( 5 );
        }
        time = SDL_GetTicks();
    }

    CLEANUP_DATA:

    /* cleanup audio related data */
    if( SDL_ffmpegValidAudio(file) ) {

        /* stop audio callback */
        SDL_PauseAudio( 1 );

        /* clean up frames */
        for(int i=0; i<BUF_SIZE; i++) {
            SDL_ffmpegFreeAudioFrame( audioFrame[i] );
        }
    }

    /* cleanup video data */
    if( videoFrame ) {
        SDL_ffmpegFreeVideoFrame( videoFrame );
    }

    /* after all is said and done, we should call this */
    SDL_ffmpegFree( file );

    /* print any errors which may have been encountered during this program */
    SDL_ffmpegPrintErrors( stderr );

    /* the SDL_Quit function offcourse... */
    SDL_Quit();

    return 0;
}
