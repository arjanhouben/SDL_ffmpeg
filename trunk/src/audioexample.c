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

void audioCallback(void *udata, Uint8 *stream, int len) {

    /* unpack our void pointer  */
    SDL_ffmpegFile *file = (SDL_ffmpegFile*)udata;

    int bytesUsed;

    SDL_ffmpegAudioFrame *frame;

	while(len > 0) {

        /* try to get a new frame */
        frame = SDL_ffmpegGetAudioFrame(file);

        /* we could not receive a new frame, break from loop */
        if(!frame) break;

        if(frame->size <= len) {
            /* this frame is smaller or equal to the amount of data needed. */
            bytesUsed = frame->size;
        } else {
            /* this frame has more data than needed */
            bytesUsed = len;
        }

        /* copy the correct amount of data */
        memcpy(stream, frame->buffer, bytesUsed);

        /* adjust the needed length accordingly */
        len -= bytesUsed;

        /* adjust stream offset */
        stream += bytesUsed;

        /* adjust size of frame to prevent reusing the same data */
        frame->size -= bytesUsed;

        /* adjust buffer of frame for the same reason */
        frame->buffer += bytesUsed;
    }

    return;
}

int main(int argc, char** argv) {

    SDL_ffmpegFile* audioFile;
    SDL_ffmpegStream *str;
    SDL_AudioSpec specs;
    int s, done;

    /* check if we got an argument */
    if(argc < 2) {
        printf("usage: \"%s\" \"filename\"\n", argv[0]);
        return -1;
    }

    /* standard SDL initialization stuff */
    if(SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "problem initializing SDL: %s\n", SDL_GetError());
        return -1;
    }

    /* open file from arg[1] */
    audioFile = SDL_ffmpegOpen(argv[1]);
    if(!audioFile) {
        printf("error opening file\n");
        return -1;
    }

    /* print some info on detected stream to output */
    for(s = 0; s<audioFile->audioStreams; s++) {
        str = SDL_ffmpegGetAudioStream(audioFile, s);

        printf("Info on audiostream #%i:\n", s);
        printf("\tChannels: %i\n",      str->channels);
        if(strlen(str->language)) printf("\tLanguage: %s\n",      str->language);
        printf("\tSampleRate: %i\n",    str->sampleRate);
    }

    /* select the stream you want to decode (example just uses 0 as a default) */
    SDL_ffmpegSelectAudioStream(audioFile, 0);

    /* get the audiospec which fits the selected audiostream, if no audiostream */
    /* is selected, default values are used (2 channel, 48Khz) */
    specs = SDL_ffmpegGetAudioSpec(audioFile, 512, audioCallback);

    /* Open the Audio device */
    if( SDL_OpenAudio(&specs, 0) < 0 ) {
        printf("Couldn't open audio: %s\n", SDL_GetError());
        goto freeAndClose;
    }

    /* we start our decode thread, this always tries to buffer in some frames */
    /* so we can enjoy smooth playback */
    SDL_ffmpegStartDecoding(audioFile);

    /* we unpause the audio so our audiobuffer gets read */
    SDL_PauseAudio(0);

    /* we play the file with -1 as parameter, so audio plays forever */
    SDL_ffmpegPlay(audioFile, -1);

    done = 0;

    while( !done ) {

        /* just some standard SDL event stuff */
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                done = 1;
                break;
            }
        }

        /* we wish not to kill our poor cpu, so we give it some timeoff */
        SDL_Delay(5);
    }

    freeAndClose:

    /* after all is said and done, we should call this */
    SDL_ffmpegFree(audioFile);

    /* the SDL_Quit function offcourse... */
    SDL_Quit();

    return 0;
}
