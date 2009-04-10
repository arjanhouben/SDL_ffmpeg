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

#include "SDL/SDL_thread.h"
#include "SDL/SDL.h"

#ifdef __cplusplus
extern "C" {
#endif

enum SDL_ffmpegStreamType {
    SDL_ffmpegUninitialized = 0,
    SDL_ffmpegOutputStream,
    SDL_ffmpegInputStream
};

typedef void (*SDL_ffmpegCallback)(void *userdata, Uint8 *stream, int len);

/** Struct to hold packet buffers */
typedef struct SDL_ffmpegPacket {
    struct AVPacket *data;
    struct SDL_ffmpegPacket *next;
} SDL_ffmpegPacket;

/** Struct to hold audio data */
typedef struct SDL_ffmpegAudioFrame {
    /** Presentation timestamp, time at which this data should be used. */
    int64_t pts;
    /** Pointer to audio buffer, user adjustable. */
    uint8_t *buffer;
    /** Size of this audio frame. */
    uint32_t size;
    /** Size of the complete audio frame. */
    uint32_t capacity;
	/** Value indicating wheter or not this is the last frame before EOF */
	int last;
} SDL_ffmpegAudioFrame;


/** Struct to hold audio data */
typedef struct SDL_ffmpegVideoFrame {
    /** Presentation timestamp, time at which this data should be used. */
    int64_t pts;
    /** Pointer to video buffer, user adjustable. */
    SDL_Surface *surface;
    /** Pointer to overlay buffer, user adjustable. */
    SDL_Overlay *overlay;
    /** Value indicating if this frame holds data, or that it can be overwritten. */
    int ready;
	/** Value indicating wheter or not this is the last frame before EOF */
	int last;
} SDL_ffmpegVideoFrame;

/** This is the basic stream for SDL_ffmpeg */
typedef struct SDL_ffmpegStream {

    /** Pointer to ffmpeg data, internal use only! */
    struct AVStream *_ffmpeg;
    /** Intermediate frame which will be used when decoding */
    struct AVFrame *decodeFrame;
    /** Intermediate frame which will be used when encoding */
    struct AVFrame *encodeFrame;

    int encodeFrameBufferSize;
    uint8_t *encodeFrameBuffer;

    int encodeAudioInputSize;
    uint64_t frameCount;

    /** buffer for decoded audio data */
    int8_t *sampleBuffer;
    /** amount of data in samplebuffer */
    int sampleBufferSize;
    /** position of data in samplebuffer */
    int sampleBufferOffset;
    /** timestamp which fits the data in samplebuffer */
    int64_t sampleBufferTime;

    /** packet buffer */
    SDL_ffmpegPacket *buffer;
    /** mutex for multi threaded acces to buffer */
    SDL_mutex *mutex;

    /** Id of the stream */
    int id;
    /** This holds the lastTimeStamp calculated, usefull when frames don't provide
        a usefull dts/pts, also used for determining at what point we are in the file */
    int64_t lastTimeStamp;

    /** pointer to the next stream, or NULL if current stream is the last one */
    struct SDL_ffmpegStream *next;

} SDL_ffmpegStream;

/** Struct to hold information about file */
typedef struct SDL_ffmpegFile {

    /** type of file */
    enum SDL_ffmpegStreamType type;

    /** Pointer to ffmpeg data, internal use only! */
    struct AVFormatContext *_ffmpeg;

    /** Video streams */
    SDL_ffmpegStream    *vs,
    /** Audio streams */
                        *as;

    /** stream mutex */
    SDL_mutex           *streamMutex;

    /** Amount of video streams in file */
    int                 videoStreams,
    /** Amount of audio streams in file */
                        audioStreams;

    /** Pointer to active videoStream, NULL if no video stream is active */
    SDL_ffmpegStream    *videoStream,
    /** Pointer to active audioStream, NULL if no audio stream is active */
                        *audioStream;

    /** Holds the lowest timestamp which will be decoded */
    int64_t             minimalTimestamp;

} SDL_ffmpegFile;

int SDL_ffmpegGetVideoFrame( SDL_ffmpegFile *file, SDL_ffmpegVideoFrame *frame );

SDL_ffmpegStream* SDL_ffmpegGetAudioStream(SDL_ffmpegFile *file, uint32_t audioID);

int SDL_ffmpegSelectAudioStream(SDL_ffmpegFile* file, int audioID);

SDL_ffmpegStream* SDL_ffmpegGetVideoStream(SDL_ffmpegFile *file, uint32_t audioID);

int SDL_ffmpegSelectVideoStream(SDL_ffmpegFile* file, int videoID);

SDL_ffmpegFile* SDL_ffmpegCreateFile();

SDL_ffmpegVideoFrame* SDL_ffmpegCreateVideoFrame( const SDL_ffmpegFile *file, const uint32_t format, SDL_Surface *screen );

void SDL_ffmpegFree(SDL_ffmpegFile* file);

void SDL_ffmpegFreeFrame(SDL_ffmpegAudioFrame* frame);

SDL_ffmpegFile* SDL_ffmpegOpen(const char* filename);

SDL_ffmpegFile* SDL_ffmpegCreate( const char* filename);

SDL_ffmpegStream* SDL_ffmpegAddAudioStream( SDL_ffmpegFile *file );

int SDL_ffmpegAddAudioFrame( SDL_ffmpegFile *file, SDL_ffmpegAudioFrame *frame );

SDL_ffmpegStream* SDL_ffmpegAddVideoStream( SDL_ffmpegFile *file );

int SDL_ffmpegAddVideoFrame( SDL_ffmpegFile *file, SDL_ffmpegVideoFrame *frame );

int SDL_ffmpegSeek(SDL_ffmpegFile* file, uint64_t timestamp);

int SDL_ffmpegSeekRelative(SDL_ffmpegFile* file, int64_t timestamp);

SDL_ffmpegAudioFrame* SDL_ffmpegCreateAudioFrame( SDL_ffmpegFile *file, uint32_t bytes );

int SDL_ffmpegGetAudioFrame( SDL_ffmpegFile *file, SDL_ffmpegAudioFrame *frame );

int64_t SDL_ffmpegGetPosition(SDL_ffmpegFile *file);

SDL_AudioSpec SDL_ffmpegGetAudioSpec(SDL_ffmpegFile *file, int samples, SDL_ffmpegCallback callback);

int SDL_ffmpegGetVideoSize(SDL_ffmpegFile *file, int *w, int *h);

uint64_t SDL_ffmpegDuration(SDL_ffmpegFile *file);

uint64_t SDL_ffmpegAudioDuration( SDL_ffmpegFile *file );

uint64_t SDL_ffmpegVideoDuration( SDL_ffmpegFile *file );

int SDL_ffmpegValidAudio(SDL_ffmpegFile *file);

int SDL_ffmpegValidVideo(SDL_ffmpegFile *file);

int SDL_ffmpegPreloaded(SDL_ffmpegFile *file);

/** \cond */
int SDL_ffmpegFlush(SDL_ffmpegFile *file);
/** \endcond */

#ifdef __cplusplus
}
#endif

#endif /* SDL_FFMPEG_INCLUDED */
