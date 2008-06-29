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

#ifndef SDL_FFMPEG_INCLUDED
#define SDL_FFMPEG_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL/SDL_thread.h"
#include "SDL/SDL.h"

#include "stdint.h"

#ifndef MAX_STREAMS
#define MAX_STREAMS 20
#endif

#define SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES      25
#define SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES     256

typedef void (*SDL_ffmpegCallback)(void *userdata, Uint8 *stream, int len);

typedef struct SDL_ffmpegAudioFrame {
    int64_t pts,
            dts;
    uint8_t *buffer;
    uint8_t *source;
    uint32_t size;
	struct SDL_ffmpegAudioFrame *next;
	int last;
} SDL_ffmpegAudioFrame;

typedef struct SDL_ffmpegVideoFrame {
    int64_t pts,
            dts;
    SDL_Surface *buffer;
    int filled;
	struct SDL_ffmpegVideoFrame *next;
	int last;
} SDL_ffmpegVideoFrame;

/* this is the basic stream for SDL_ffmpeg */
typedef struct SDL_ffmpegStream {

    int pixFmt,
        endReached,
        flushed;
    /* pointer to ffmpeg data, internal use only! */
    struct AVStream *_ffmpeg;

    /* audio/video buffers */
    SDL_ffmpegAudioFrame audioBuffer[ SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES ];

    SDL_ffmpegVideoFrame videoBuffer[ SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES ];

    /* userinfo */
    double frameRate[2];
    char language[4];
    int sampleRate;
    int channels;
    char codecName[32];
    double timeBase;
    uint16_t width;
    uint16_t height;

    /* extra data for audio */
    int id;
    int64_t lastTimeStamp, dts;

} SDL_ffmpegStream;

typedef struct SDL_ffmpegFile {

    /* pointer to ffmpeg data, internal use only! */
    struct AVFormatContext *_ffmpeg;

    /* our streams */
    SDL_ffmpegStream    *vs[MAX_STREAMS],
                        *as[MAX_STREAMS];

    SDL_ffmpegStream    *videoStream,
                        *audioStream;

    /* data used for syncing/searching */
    int64_t     offset,
                loopCount,
                startTime,
                seekTo;

    int			mustSeek;

    /* streams and data about threads */
    int     VStreams,
            AStreams,
            threadActive;

    SDL_ffmpegVideoFrame	*videoFrameInUse,
							*pendingVideoFrame;

    SDL_ffmpegAudioFrame	*audioFrameInUse,
							*pendingAudioFrame;

    SDL_Thread              *threadID;

} SDL_ffmpegFile;


int SDL_ffmpegStartDecoding(SDL_ffmpegFile* file);

int SDL_ffmpegStopDecoding(SDL_ffmpegFile* file);

SDL_ffmpegVideoFrame* SDL_ffmpegGetVideoFrame(SDL_ffmpegFile* file);

SDL_ffmpegStream* SDL_ffmpegGetAudioStream(SDL_ffmpegFile *file, int audioID);

int SDL_ffmpegSelectAudioStream(SDL_ffmpegFile* file, int audioID);

SDL_ffmpegStream* SDL_ffmpegGetVideoStream(SDL_ffmpegFile *file, int audioID);

int SDL_ffmpegSelectVideoStream(SDL_ffmpegFile* file, int videoID);

SDL_ffmpegFile* SDL_ffmpegCreateFile();

void SDL_ffmpegFree(SDL_ffmpegFile* file);

SDL_ffmpegFile* SDL_ffmpegOpen(const char* filename);

int SDL_ffmpegDecodeThread(void* data);

int SDL_ffmpegSeek(SDL_ffmpegFile* file, int64_t timestamp);

int SDL_ffmpegSeekRelative(SDL_ffmpegFile* file, int64_t timestamp);

int SDL_ffmpegFlush(SDL_ffmpegFile *file);

SDL_ffmpegAudioFrame* SDL_ffmpegGetAudioFrame(SDL_ffmpegFile *file);

int64_t SDL_ffmpegGetPosition(SDL_ffmpegFile *file);

SDL_AudioSpec* SDL_ffmpegGetAudioSpec(SDL_ffmpegFile *file, int samples, SDL_ffmpegCallback callback);

int SDL_ffmpegGetVideoSize(SDL_ffmpegFile *file, int *w, int *h);

int64_t SDL_ffmpegGetDuration(SDL_ffmpegFile *file);

int SDL_ffmpegValidAudio(SDL_ffmpegFile *file);

int SDL_ffmpegValidVideo(SDL_ffmpegFile *file);

int SDL_ffmpegPlay(SDL_ffmpegFile *file, int64_t count);

int SDL_ffmpegGetState(SDL_ffmpegFile *file);

#ifdef __cplusplus
}
#endif

#endif /* SDL_FFMPEG_INCLUDED */
