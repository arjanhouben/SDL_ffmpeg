# SDL_ffmpeg - Arjan Houben - 2007

SRCDIR=src/
OBJDIR=obj/
LIBDIR=lib/
INCDIR=include/
BINDIR=bin/

CC=gcc
AR=ar
LD=ld
LDFLAGS=-lSDL -L$(LIBDIR)
BASECFLAGS=-I$(INCDIR) -I$(SDLDIR)include/ -L$(SDLDIR)lib/ -I$(FFMPEGDIR) -I$(FFMPEGDIR)libswscale -I$(FFMPEGDIR)libavutil -I$(FFMPEGDIR)libavformat -I$(FFMPEGDIR)libavcodec $(ECFLAGS)
RCFLAGS=-s -O2
DCFLAGS=-g
PROG=$(NAME)_example
NAME=SDL_ffmpeg
FFMPEGOBJECTS=$(OBJDIR)libavformat/*.o $(OBJDIR)libavcodec/*.o $(OBJDIR)libavutil/*.o

ifeq ($(strip $(FFMPEGDIR)),)
	FFMPEGDIR=\ FFMPEGDIR\ NOT\ SET!\ 
endif

ifeq ($(strip $(WIN)),)
	LDFLAGS+=-lz
	CFLAGS=$(BASECFLAGS)
	EXTEXE=
	EXTDLL=.so
else
	LDFLAGS+=-lwsock32
	CFLAGS=-lmingw32 $(BASECFLAGS)
	EXTDLL=.dll
	EXTEXE=.exe
	ifeq ($(strip $(SDLDIR)),)
		SDLDIR=\ SDLDIR\ NOT\ SET!\ 
	endif
endif

all: dynamiclibrary staticlibrary dynamiclibrarydebug staticlibrarydebug audio_example example
	@echo 
	@echo all done!

# ffmpeg objects

ffmpegfiles: $(FFMPEGDIR)libavformat/libavformat.a $(FFMPEGDIR)libavcodec/libavcodec.a $(FFMPEGDIR)libavutil/libavutil.a
	@echo
	@echo [ $@ ]
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
	@echo
	@echo [ $@ ]
	@-mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(RCFLAGS) -DSDL_FFMPEG_LIBRARY -c $< -o $(OBJDIR)$(NAME).o

cleansdl_ffmpeg:
	-rm -f $(OBJDIR)$(NAME).o

$(OBJDIR)$(NAME)d.o: sdl_ffmpegdebug

sdl_ffmpegdebug: $(SRCDIR)$(NAME).c $(INCDIR)SDL/$(NAME).h
	@echo
	@echo [ $@ ]
	@-mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) -DSDL_FFMPEG_LIBRARY -c $< -o $(OBJDIR)$(NAME)d.o

cleansdl_ffmpegdebug:
	-rm -f $(OBJDIR)$(NAME)d.o

# static library

$(LIBDIR)lib$(NAME).a: staticlibrary

staticlibrary: ffmpegfiles sdl_ffmpeg
	@echo
	@echo [ $@ ]
	@-mkdir -p $(LIBDIR)
	$(AR) -rcs $(LIBDIR)lib$(NAME).a $(OBJDIR)$(NAME).o $(FFMPEGOBJECTS)

cleanstaticlibrary:
	-rm -f $(LIBDIR)lib$(NAME).a

$(LIBDIR)lib$(NAME)d.a: staticlibrarydebug

staticlibrarydebug: ffmpegfiles sdl_ffmpegdebug
	@echo
	@echo [ $@ ]
	@-mkdir -p $(LIBDIR)
	$(AR) -rcs $(LIBDIR)lib$(NAME)d.a $(OBJDIR)$(NAME)d.o $(FFMPEGOBJECTS)

cleanstaticlibrarydebug:
	-rm -f $(LIBDIR)lib$(NAME)d.a

# dynamic library

$(BINDIR)$(NAME)$(EXTDLL): dynamiclibrary

dynamiclibrary: ffmpegfiles sdl_ffmpeg
	@echo
	@echo [ $@ ]
	@-mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(RCFLAGS) -DSDL_FFMPEG_LIBRARY $(FFMPEGOBJECTS) $(LDFLAGS) -shared -o $(BINDIR)$(NAME)$(EXTDLL)

cleandynamiclibrary:
	-rm -f $(BINDIR)$(NAME)$(EXTDLL)

$(BINDIR)$(NAME)d$(EXTDLL): dynamiclibrarydebug

dynamiclibrarydebug: ffmpegfiles sdl_ffmpegdebug
	@echo
	@echo [ $@ ]
	@-mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) -DSDL_FFMPEG_LIBRARY $(FFMPEGOBJECTS) $(LDFLAGS) -shared -o $(BINDIR)$(NAME)d$(EXTDLL)

cleandynamiclibrarydebug:
	-rm -f $(BINDIR)$(NAME)d$(EXTDLL)

# example

example: staticlibrary
	@echo
	@echo [ $@ ]
	@-mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) $(SRCDIR)example.c -lSDLmain -l$(NAME) $(LDFLAGS) -o $(BINDIR)example$(EXTEXE)

cleanexample:
	-rm -f $(BINDIR)example$(EXTEXE)

# audio example

audio_example: staticlibrary
	@echo
	@echo [ $@ ]
	@-mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(RCFLAGS) $(SRCDIR)audio_example.c -lSDLmain -l$(NAME) $(LDFLAGS) -o $(BINDIR)audio_example$(EXTEXE)

cleanaudio_example:
	-rm -f $(BINDIR)audio_example$(EXTEXE)

# clean

.PHONY: clean

clean: cleanexample cleanaudio_example cleanstaticlibrary cleandynamiclibrary cleanstaticlibrarydebug cleandynamiclibrarydebug cleansdl_ffmpeg cleansdl_ffmpegdebug
	-rm -Rf $(OBJDIR)
	-rm -Rf $(LIBDIR)
	-rm -Rf $(BINDIR)

