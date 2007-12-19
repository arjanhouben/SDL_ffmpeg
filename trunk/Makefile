# SDL_ffmpeg - Arjan Houben - 2007

CC=/usr/bin/gcc
AR=/usr/bin/ar
CFLAGS=-I$(INCDIR)
LDFLAGS=-lSDL -lz -L$(LIBDIR)
PROG=$(NAME)_example
NAME=SDL_ffmpeg

SRCDIR=src/
OBJDIR=obj/
LIBDIR=lib/
INCDIR=include/

FFMPEGDIR=../ffmpeg/

all: $(PROG)

$(LIBDIR)lib$(NAME).a: $(SRCDIR)$(NAME).c $(INCDIR)SDL/$(NAME).h
	-mkdir lib
	-mkdir obj
	$(CC) $(CFLAGS) -DSDL_FFMPEG_LIBRARY -c $< -o $(OBJDIR)$(NAME).o
	$(AR) -rc lib/lib$(NAME).a $(OBJDIR)$(NAME).o `find $(FFMPEGDIR)lib* -name \*.o`
	
$(PROG): $(LIBDIR)lib$(NAME).a
	-mkdir bin
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCDIR)example.c -l$(NAME) -o bin/$(PROG)

.PHONY: clean

clean:
	echo $(FFMPEGDIR)
	-rm bin/$(PROG)
	-rm $(OBJDIR)*.o
	-rm $(LIBDIR)/lib$(NAME).a
