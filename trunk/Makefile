# SDL_ffmpeg - Arjan Houben - 2007

CC=/usr/bin/gcc
AR=/usr/bin/ar
CFLAGS=-I$(INCDIR)
LDFLAGS=-lavformat -lSDL
PROG=example
NAME=SDL_ffmpeg

SRCDIR=src/
SRC=$(NAME).c

OBJDIR=obj/
OBJ=$(NAME).o

INCDIR=include/
INC=$(NAME).h

all: $(PROG)

lib: $(addprefix $(OBJDIR),$(OBJ))
	-mkdir lib
	$(AR) -rc lib/lib$(NAME).a $<
	
$(PROG): lib $(SRCDIR)example.c
	-mkdir bin
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCDIR)example.c -l$(NAME) -Llib -o bin/$(NAME)

%.o: $(SRCDIR)$(SRC) $(INCDIR)SDL/$(INC)
	-mkdir obj
	$(CC) $(CFLAGS) -DSDL_FFMPEG_LIBRARY -c $< -o $@

.PHONY: clean

clean:
	-rm bin/$(NAME)
	-rm $(OBJDIR)*.o
	-rm lib/lib$(NAME).a
