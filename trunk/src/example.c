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

#include "SDL/SDL.h"
#include "SDL/SDL_ffmpeg.h"

#include <string.h>

/* simple way of syncing, just for example purposes */
int64_t sync = 0;

void audioCallback( void *udata, Uint8 *stream, int len ) {

    /* unpack our void pointer */
    SDL_ffmpegFile *file = (SDL_ffmpegFile*)udata;

    /* create an audio frame to store data received from SDL_ffmpegGetAudioFrame */
    SDL_ffmpegAudioFrame *frame = SDL_ffmpegCreateAudioFrame( file, len );

    /* try to get a new frame */
    if( SDL_ffmpegGetAudioFrame( file, frame ) ) {

        /* copy received data to output */
        memcpy( stream, frame->buffer, frame->size );

        /* store time, so we sync the video frames to this time */
        sync = frame->pts;

    } else {

        /* could not get frame, just set output to zero */
        memset( stream, 0, len );
    }

    /* clean up frame */
    SDL_ffmpegFreeFrame( frame );

    return;
}

int main(int argc, char** argv) {

    SDL_ffmpegFile          *file = 0;
    SDL_ffmpegStream        *str = 0;
	SDL_Surface             *screen = 0;
	SDL_ffmpegVideoFrame    *frame = 0;
	SDL_AudioSpec           specs;
    int                     w, h, done, mouseState = 0,
                            x, y, useAudio = 0;
	int64_t                 time;

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
        fprintf( stderr, "error opening file\n" );
        SDL_Quit();
        return -1;
    }

    /* select the stream you want to decode (example just uses 0 as a default) */
    SDL_ffmpegSelectVideoStream( file, 0 );

    /* if no audio can be selected, audio will not be used in this example */
    useAudio = !SDL_ffmpegSelectAudioStream( file, 0 );

    /* get the audiospec which fits the selected audiostream, if no audiostream
       is selected, default values are used (2 channel, 48Khz) */
    specs = SDL_ffmpegGetAudioSpec( file, 512, audioCallback );

    /* we get the size from our active video stream, if no active video stream
       exists, width and height are set to zero */
    SDL_ffmpegGetVideoSize( file, &w, &h );

    /* Open the Video device */
    screen = SDL_SetVideoMode( w, h, 0, SDL_DOUBLEBUF|SDL_HWSURFACE );
    if( !screen ) {
        fprintf( stderr, "Couldn't open video: %s\n", SDL_GetError() );
        SDL_Quit();
        return -1;
    }

    /* create a video frame which will be used to receive the video data.
       If you want to receive YCbCr data, you have to define the second parameter
       with the format you would like to receive and the last parameter needs to
       be a pointer to the SDL_surface as returned by SDL_SetVideoMode */
    frame = SDL_ffmpegCreateVideoFrame( file, SDL_YUY2_OVERLAY, screen );

    /* create a SDL_Rect for blitting of image data */
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = w;
    rect.h = h;

    /* Open the Audio device */
    if( useAudio && SDL_OpenAudio( &specs, 0 ) < 0 ) {
        fprintf( stderr, "Couldn't open audio: %s\n", SDL_GetError() );
        SDL_Quit();
        return -1;
    }

    /* we start our decode thread, this always tries to buffer in some frames
       so we can enjoy smooth playback */
    SDL_ffmpegStartDecoding( file );

    /* we unpause the audio so our audiobuffer gets read */
    if( useAudio ) SDL_PauseAudio( 0 );

    done = 0;

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

                SDL_GetMouseState( &x, &y );
                /* by clicking you turn on the stream, seeking to the percentage
                   in time, based on the x-position you clicked on */
                time = (int64_t)(((double)x / (double)w) * SDL_ffmpegDuration( file ));

                /* we seek to time (milliseconds) */
                SDL_ffmpegSeek( file, time );

                /* invalidate current frame */
                if( frame ) frame->ready = 0;
            }
        }

        if( frame ) {

            /* check if frame is ready */
            if( !frame->ready ) {

                /* not ready, try to get a new frame */
                SDL_ffmpegGetVideoFrame( file, frame );

            } else if( !useAudio || frame->pts <= sync ) {

                /* frame ready and in sync, or no audio present */

                if( frame->overlay ) {

                    /* blit overlay */
                    SDL_DisplayYUVOverlay( frame->overlay, &rect );

                } else if( frame->surface ) {

                    /* blit RGB surface */
                    SDL_BlitSurface( frame->surface, 0, screen, 0 );

                    /* flip screen */
                    SDL_Flip( screen );
                }

                /* frame is displayed, make sure we don't show it again */
                frame->ready = 0;
            }
        }

        /* we wish not to kill our poor cpu, so we give it some timeoff */
        SDL_Delay( 1 );
    }

    /* stop audio callback */
    if( useAudio ) SDL_PauseAudio( 1 );

    /* after all is said and done, we should call this */
    SDL_ffmpegFree( file );

    /* the SDL_Quit function offcourse... */
    SDL_Quit();

    return 0;
}
