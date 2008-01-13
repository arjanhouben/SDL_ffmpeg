# SDL_ffmpeg - Arjan Houben - 2007

SRCDIR=src/
OBJDIR=obj/
LIBDIR=lib/
INCDIR=include/

FFMPEGDIR=/home/arjan/ffmpeg/

CC=gcc
AR=ar
LD=ld
LDFLAGS=-lSDL -L$(LIBDIR) -lz
CFLAGS=-I$(INCDIR) -I$(FFMPEGDIR)libswscale -I$(FFMPEGDIR)libavutil -I$(FFMPEGDIR)libavformat -I$(FFMPEGDIR)libavcodec
CFLAGS+=$(ECFLAGS)
PROG=$(NAME)_example
NAME=SDL_ffmpeg

all: $(PROG)

$(LIBDIR)lib$(NAME).a: $(SRCDIR)$(NAME).c $(INCDIR)SDL/$(NAME).h
	-mkdir obj
	$(CC) $(CFLAGS) -DSDL_FFMPEG_LIBRARY -c $< -o $(OBJDIR)$(NAME).o
	-mkdir $(OBJDIR)libavformat
	cd $(OBJDIR)libavformat;$(AR) x $(FFMPEGDIR)libavformat/libavformat.a
	-mkdir $(OBJDIR)libavcodec
	cd $(OBJDIR)libavcodec;$(AR) x $(FFMPEGDIR)libavcodec/libavcodec.a
	-mkdir $(OBJDIR)libavutil
	cd $(OBJDIR)libavutil;$(AR) x $(FFMPEGDIR)libavutil/libavutil.a
	-mkdir lib
	$(AR) -rcs $(LIBDIR)lib$(NAME).a $(OBJDIR)$(NAME).o $(OBJDIR)libavformat/*.o $(OBJDIR)libavcodec/*.o $(OBJDIR)libavutil/*.o
	
$(PROG): $(LIBDIR)lib$(NAME).a
	-mkdir bin
	$(CC) $(CFLAGS) $(SRCDIR)example.c -lSDLmain -l$(NAME) $(LDFLAGS) -o bin/$(PROG)

.PHONY: clean

clean:
	-rm bin/$(PROG)
	-rm -R $(OBJDIR)
	-rm -R $(LIBDIR)
