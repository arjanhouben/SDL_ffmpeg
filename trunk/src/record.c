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

#include <math.h>

int main(int argc, char** argv) {

    SDL_ffmpegFile  *file = 0;
    int             x, y;
    float           xf, yf,
                    a = 0.0,
                    percentage;

    /* check if we got an argument */
    if(argc < 2) {
        printf( "usage: \"%s\" \"filename\"\n", argv[0] );
        return -1;
    }

    /* open file from arg[1] */
    file = SDL_ffmpegCreate( argv[1] );
    if( !file ) {
        fprintf( stderr, "error creating file\n" );
        SDL_Quit();
        return -1;
    }

    SDL_ffmpegAddVideoStream( file );
    SDL_ffmpegAddAudioStream( file );

    SDL_ffmpegSelectVideoStream( file, 0 );
    SDL_ffmpegSelectAudioStream( file, 0 );

    SDL_ffmpegVideoFrame *videoFrame = SDL_ffmpegCreateVideoFrame( file, 0, 0 );
    SDL_ffmpegAudioFrame *audioFrame = SDL_ffmpegCreateAudioFrame( file, 0 );

    if( SDL_ffmpegValidAudio(file) || SDL_ffmpegValidVideo(file) ) {

        while( SDL_ffmpegDuration(file) < 5000  ) {

            percentage = (float)SDL_ffmpegDuration(file) / 5000.0;

            printf("%3.0f%%\r", percentage*100);
            fflush(stdout);

            if( SDL_ffmpegValidVideo(file) && ( !SDL_ffmpegValidAudio(file) ||
                                                SDL_ffmpegAudioDuration(file) > SDL_ffmpegVideoDuration(file) ) ) {

                /* draw moving line */
                if( videoFrame ) {
                    int *c = videoFrame->surface->pixels;
                    for(y=0; y<videoFrame->surface->h; y++) {
                        for(x=0; x<videoFrame->surface->w; x++) {
                            if( (int)(percentage*videoFrame->surface->w) == x ) {
                                *c = 0xFFFFFFFF;
                            } else {
                                *c = 0xFF000000;
                            }
                            c++;
                        }
                    }
                }

                SDL_ffmpegAddVideoFrame( file, videoFrame );

            } else {

                /* generate tone */
                int16_t *s = (int16_t*)audioFrame->buffer;
                int i;
                for(i=0; i<audioFrame->capacity/4; i++) {
                    *s = sin(a) * 0x7FFF;
                    s++;
                    *s = *(s-1);
                    s++;
                    a+= 0.03;
                }

                SDL_ffmpegAddAudioFrame( file, audioFrame );
            }
        }
    }

    /* after all is said and done, we should call this */
    SDL_ffmpegFree( file );

    /* the SDL_Quit function offcourse... */
    SDL_Quit();

    return 0;
}
