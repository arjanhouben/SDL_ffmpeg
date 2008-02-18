# SDL_ffmpeg - Arjan Houben - 2007

SRCDIR=src/
OBJDIR=obj/
LIBDIR=lib/
INCDIR=include/
BINDIR=bin/

FFMPEGDIR=
SDLDIR=
WIN=

CC=gcc
AR=ar
LD=ld
LDFLAGS=-lSDL -L$(LIBDIR)
BASECFLAGS=-I$(INCDIR) -I$(SDLDIR)include/ -L$(SDLDIR)lib/ -I$(FFMPEGDIR)libswscale -I$(FFMPEGDIR)libavutil -I$(FFMPEGDIR)libavformat -I$(FFMPEGDIR)libavcodec $(ECFLAGS)
PROG=$(NAME)_example
NAME=SDL_ffmpeg
FFMPEGOBJECTS=$(OBJDIR)libavformat/*.o $(OBJDIR)libavcodec/*.o $(OBJDIR)libavutil/*.o

ifeq ($(strip $(FFMPEGDIR)),)
	FFMPEGDIR=\ FFMPEG\ PATH\ NOT\ SET!\ 
endif

ifeq ($(strip $(WIN)),)
	LDFLAGS+=-lz
	CFLAGS=$(BASECFLAGS)
	EXTEXE=
	EXTDLL=.so
	ifeq ($(strip $(SDLDIR)),)
		SDLDIR=\ SDL\ PATH\ NOT\ SET!\ 
	endif
else
	LDFLAGS+=-lwsock32
	CFLAGS=-lmingw32 $(BASECFLAGS)
	EXTDLL=.dll
	EXTEXE=.exe
endif

all: example audio_example dynamiclibrary

# ffmpeg objects

ffmpegfiles: $(FFMPEGDIR)libavformat/libavformat.a $(FFMPEGDIR)libavcodec/libavcodec.a $(FFMPEGDIR)libavutil/libavutil.a
	@-mkdir -p $(OBJDIR)
	@-mkdir -p $(OBJDIR)libavformat
	cd $(OBJDIR)libavformat;$(AR) x $(FFMPEGDIR)libavformat/libavformat.a
	@-mkdir -p $(OBJDIR)libavcodec
	cd $(OBJDIR)libavcodec;$(AR) x $(FFMPEGDIR)libavcodec/libavcodec.a
	@-mkdir -p $(OBJDIR)libavutil
	cd $(OBJDIR)libavutil;$(AR) x $(FFMPEGDIR)libavutil/libavutil.a

# SDL_ffmpeg object

$(OBJDIR)$(NAME).o: sdl_ffmpeg

sdl_ffmpeg: $(SRCDIR)$(NAME).c $(INCDIR)SDL/$(NAME).h
	@-mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -DSDL_FFMPEG_LIBRARY -c $< -o $(OBJDIR)$(NAME).o

cleansdl_ffmpeg:
	-rm -f $(OBJDIR)$(NAME).o

# static library

$(LIBDIR)lib$(NAME).a: staticlibrary

staticlibrary: ffmpegfiles sdl_ffmpeg
	@-mkdir -p $(LIBDIR)
	$(AR) -rcs $(LIBDIR)lib$(NAME).a $(OBJDIR)$(NAME).o $(FFMPEGOBJECTS)

cleanstaticlibrary:
	-rm -f $(LIBDIR)lib$(NAME).a

# dynamic library

$(BINDIR)$(NAME)$(EXTDLL): dynamiclibrary

dynamiclibrary: ffmpegfiles sdl_ffmpeg
	@-mkdir -p $(BINDIR)
	$(CC) $(CFLAGS)-DSDL_FFMPEG_LIBRARY $(FFMPEGOBJECTS) $(LDFLAGS) -shared -o $(BINDIR)$(NAME)$(EXTDLL)

cleandynamiclibrary:
	-rm -f $(BINDIR)$(NAME)$(EXTDLL)

# example

example: staticlibrary
	@-mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(SRCDIR)example.c -lSDLmain -l$(NAME) $(LDFLAGS) -o $(BINDIR)example$(EXTEXE)

cleanexample:
	-rm -f $(BINDIR)example$(EXTEXE)

# audio example

audio_example: staticlibrary
	@-mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(SRCDIR)audio_example.c -lSDLmain -l$(NAME) $(LDFLAGS) -o $(BINDIR)audio_example$(EXTEXE)

cleanaudio_example:
	-rm -f $(BINDIR)audio_example$(EXTEXE)

# clean

.PHONY: clean

clean: cleanexample cleanaudio_example cleanstaticlibrary cleandynamiclibrary cleansdl_ffmpeg
	-rm -Rf $(OBJDIR)
	-rm -Rf $(LIBDIR)
	-rm -Rf $(BINDIR)

