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

#ifndef MAX_STREAMS
#define MAX_STREAMS 20
#endif

#define SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES      25
#define SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES     256

typedef void (*SDL_ffmpegCallback)(void *userdata, Uint8 *stream, int len);

/** Struct to hold audio data */
typedef struct SDL_ffmpegAudioFrame {
    /** Presentation timestamp, time at which this data should be used. */
    int64_t pts;
    /** Pointer to audio buffer, user adjustable. */
    uint8_t *buffer;
    /** Pointer at which this buffer was allocated, internal use only! */
    uint8_t *source;
    /** Size of this audio frame. */
    uint32_t size;
    /** Pointer to next SDL_ffmpegAudioFrame */
	struct SDL_ffmpegAudioFrame *next;
	/** Value indicating wheter or not this is the last frame before EOF */
	int last;
} SDL_ffmpegAudioFrame;


/** Struct to hold audio data */
typedef struct SDL_ffmpegVideoFrame {
    /** Presentation timestamp, time at which this data should be used. */
    int64_t pts;
    /** Pointer to audio buffer, user adjustable. */
    SDL_Surface *buffer;
    /** Value indicating if this frame holds data, or that it can be overwritten. */
    int filled;
    /** Pointer to next SDL_ffmpegVideoFrame */
	struct SDL_ffmpegVideoFrame *next;
	/** Value indicating wheter or not this is the last frame before EOF */
	int last;
} SDL_ffmpegVideoFrame;

/** This is the basic stream for SDL_ffmpeg */
typedef struct SDL_ffmpegStream {

    /** Value indicating when the file has reached EOF */
    int endReached,
    /** Value indicating when the file was flushed */
        flushed;

    /** Pointer to ffmpeg data, internal use only! */
    struct AVStream *_ffmpeg;

    /** Audio buffers */
    SDL_ffmpegAudioFrame audioBuffer[ SDL_FFMPEG_MAX_BUFFERED_AUDIOFRAMES ];

    /** Video buffers */
    SDL_ffmpegVideoFrame videoBuffer[ SDL_FFMPEG_MAX_BUFFERED_VIDEOFRAMES ];

    /** Framerate, duration of a single frame can be calculated using
        frameRate[0] / frameRate[1]. frameRate[1] can be zero, so watch out for
        devision by zero.
    */
    double frameRate[2];
    /** This holds a string representing the language, if available */
    char language[4];
    /** The samplerate of this stream */
    int sampleRate;
    /** The channel count of this stream */
    int channels;
    /** The name of the codec used to decode this stream */
    char codecName[32];
    /** The duration of a single frame */
    double timeBase;
    /** Width of video frame */
    uint16_t width;
    /** Height of video frame */
    uint16_t height;

    /** Id of the stream */
    int id;
    /** This holds the lastTimeStamp calculated, usefull when frames don't provide
        a usefull dts/pts
    */
    int64_t lastTimeStamp;

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

SDL_ffmpegStream* SDL_ffmpegGetAudioStream(SDL_ffmpegFile *file, uint32_t audioID);

int SDL_ffmpegSelectAudioStream(SDL_ffmpegFile* file, int audioID);

SDL_ffmpegStream* SDL_ffmpegGetVideoStream(SDL_ffmpegFile *file, uint32_t audioID);

int SDL_ffmpegSelectVideoStream(SDL_ffmpegFile* file, int videoID);

SDL_ffmpegFile* SDL_ffmpegCreateFile();

void SDL_ffmpegFree(SDL_ffmpegFile* file);

SDL_ffmpegFile* SDL_ffmpegOpen(const char* filename);

int SDL_ffmpegDecodeThread(void* data);

int SDL_ffmpegSeek(SDL_ffmpegFile* file, uint64_t timestamp);

int SDL_ffmpegSeekRelative(SDL_ffmpegFile* file, int64_t timestamp);

int SDL_ffmpegFlush(SDL_ffmpegFile *file);

SDL_ffmpegAudioFrame* SDL_ffmpegGetAudioFrame(SDL_ffmpegFile *file);

int64_t SDL_ffmpegGetPosition(SDL_ffmpegFile *file);

SDL_AudioSpec SDL_ffmpegGetAudioSpec(SDL_ffmpegFile *file, int samples, SDL_ffmpegCallback callback);

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
