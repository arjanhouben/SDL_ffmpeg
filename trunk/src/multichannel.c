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

SDL_ffmpegFile *audioFile[10];

SDL_ffmpegAudioFrame *audioFrame[10];

int playing[10];

inline int16_t clamp( int a, int b ) {
    int c = a + b;
    if( c > 0x7FFF ) return 0x7FFF;
    if( c < -0x7FFF ) return -0x7FFF;
    return c;
}

void audioCallback(void *udata, Uint8 *stream, int len) {

    int i, f;

    /* zero output data */
    memset(stream, 0, len);

    for(f=0; audioFile[f]; f++) {

        if( playing[f] ) {

            if( !audioFrame[f] ) audioFrame[f] = SDL_ffmpegCreateAudioFrame( audioFile[f], len );

            audioFrame[f]->size = 0;

            /* try to get a new frame */
            SDL_ffmpegGetAudioFrame( audioFile[f], audioFrame[f] );

            /* we could not receive a new frame, break from loop */
            if( !audioFrame[f] ) continue;

            /* add audio data to output */
            int16_t *dest = (int16_t*)stream;
            int16_t *src = (int16_t*)audioFrame[f]->buffer;

            i = len / 2;
            while( i-- ) {
                *dest = clamp( *dest, *src );
                dest++;
                src++;
            }
        }
    }

    return;
}

int main(int argc, char** argv) {

    SDL_AudioSpec specs;
    int i, f, done;

    /* check if we got an argument */
    if(argc < 2) {
        printf("usage: %s <file1> <file2> ...\n", argv[0]);
        return -1;
    }

    /* standard SDL initialization stuff */
    if(SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "problem initializing SDL: %s\n", SDL_GetError());
        return -1;
    }

    /* open window to capture keypresses */
    SDL_SetVideoMode(320, 240, 0, SDL_DOUBLEBUF|SDL_HWSURFACE);

    /* reset audiofile pointers */
    memset(audioFile, 0, sizeof(SDL_ffmpegFile*)*10);
    memset(audioFrame, 0, sizeof(SDL_ffmpegAudioFrame*)*10);
    memset(playing, 0, sizeof(int)*10);

    f = 0;
    for(i=1; i<argc && i<10; i++) {

        /* open file from argument */
        audioFile[f] = SDL_ffmpegOpen( argv[i] );

        if( !audioFile[f] ) {
            printf("error opening file \"%s\"\n", argv[i] );
            continue;
        }

        printf("added \"%s\" at key %i\n", argv[i], i);

        /* select the stream you want to decode (example just uses 0 as a default) */
        SDL_ffmpegSelectAudioStream( audioFile[f], 0 );

        /* we start our decode thread, this always tries to buffer in some frames */
        /* so we can enjoy smooth playback */
        SDL_ffmpegStartDecoding( audioFile[f] );

        f++;
    }

    /* check if at least one file could be opened */
    if( !audioFile[0] ) goto freeAndClose;

    /* get the audiospec which fits the selected audiostream, if no audiostream */
    /* is selected, default values are used (2 channel, 48Khz) */
    specs = SDL_ffmpegGetAudioSpec(audioFile[0], 512, audioCallback);

    /* Open the Audio device */
    if( SDL_OpenAudio(&specs, 0) < 0 ) {
        printf("Couldn't open audio: %s\n", SDL_GetError());
        goto freeAndClose;
    }

    /* we unpause the audio so our audiobuffer gets read */
    SDL_PauseAudio(0);

    done = 0;

    while( !done ) {

        /* just some standard SDL event stuff */
        SDL_Event event;
        while(SDL_PollEvent(&event)) {

            if( event.type == SDL_QUIT ) {
                done = 1;
                break;
            } else if( event.type == SDL_KEYDOWN ) {
                /* check al files, and play if needed */
                for(i=0; audioFile[i]; i++) {
                    if( event.key.keysym.sym == SDLK_1+i ) {
                        playing[i] = 1;
                    }
                }
            } else if( event.type == SDL_KEYUP ) {
                /* check al files, and play if needed */
                for(i=0; audioFile[i]; i++) {
                    if( event.key.keysym.sym == SDLK_1+i ) {
                        playing[i] = 0;
                        SDL_ffmpegSeek( audioFile[i], 0 );
                    }
                }
            }
        }

        /* we wish not to kill our poor cpu, so we give it some timeoff */
        SDL_Delay(5);
    }

    freeAndClose:

    /* stop audio callback */
    SDL_PauseAudio(1);

    /* after all is said and done, we should call this */
    for(i=0; audioFile[i]; i++) SDL_ffmpegFree( audioFile[i] );

    /* the SDL_Quit function offcourse... */
    SDL_Quit();

    return 0;
}
