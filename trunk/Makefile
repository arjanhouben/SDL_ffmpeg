# SDL_ffmpeg - Arjan Houben - 2007

SRCDIR=src/
OBJDIR=obj/
LIBDIR=lib/
INCDIR=include/
BINDIR=bin/

OS=$(shell uname)

CC=gcc
AR=ar
LD=ld
LDFLAGS=-lSDL -L$(LIBDIR)
BASECFLAGS= -I$(INCDIR) -I$(SDLDIR)include/ -L$(SDLDIR)lib/ -I$(FFMPEGDIR) -I$(FFMPEGDIR)libavutil -I$(FFMPEGDIR)libavformat -I$(FFMPEGDIR)libavcodec $(ECFLAGS)
RCFLAGS=-s -O2
DCFLAGS=-g
NAME=SDL_ffmpeg
FFMPEGOBJECTS=$(OBJDIR)libavformat/*.o $(OBJDIR)libavcodec/*.o $(OBJDIR)libavutil/*.o
LINKLIBRARY=staticlibrary
LDLIBRARY=$(NAME)

ifeq ($(strip $(FFMPEGDIR)),)
	FFMPEGDIR=\ FFMPEGDIR\ NOT\ SET!\ 
endif


ifeq ($(OS),Linux)
	LDFLAGS+=-lz
	CFLAGS=$(BASECFLAGS)
	EXTEXE=
	EXTDLL=.so
endif


ifeq ($(findstring MINGW,$(OS)),MINGW)
	LDFLAGS+=-lwsock32
	CFLAGS=-lmingw32 $(BASECFLAGS) -lSDLmain
	EXTDLL=.dll
	EXTEXE=.exe
	ifeq ($(strip $(SDLDIR)),)
		SDLDIR=\ SDLDIR\ NOT\ SET!\ 
	endif
endif


ifeq ($(OS),Darwin)
	LDFLAGS=-framework Cocoa -framework SDL -L$(LIBDIR) include/SDL/SDLMain.m -lbz2 -lz
endif


ifeq ($(strip $(DEBUG)),)
	CFLAGS+=$(RCFLAGS)
else
	CFLAGS+=$(DCFLAGS)
	LINKLIBRARY=staticlibrarydebug
	LDLIBRARY=$(NAME)d
endif


all: dynamiclibrary staticlibrary dynamiclibrarydebug staticlibrarydebug example audioexample multichannelexample
	@echo 
	@echo all done!

.PHONY: all

# ffmpeg objects

ffmpegfiles: $(FFMPEGOBJECTS)

$(FFMPEGOBJECTS): $(FFMPEGDIR)libavformat/libavformat.a $(FFMPEGDIR)libavcodec/libavcodec.a $(FFMPEGDIR)libavutil/libavutil.a
	@echo
	@echo [ $@ ]
	@-mkdir $(OBJDIR)
	@-mkdir $(OBJDIR)libavformat
	cd $(OBJDIR)libavformat;$(AR) x $(FFMPEGDIR)libavformat/libavformat.a
	@-mkdir $(OBJDIR)libavcodec
	cd $(OBJDIR)libavcodec;$(AR) x $(FFMPEGDIR)libavcodec/libavcodec.a
	@-mkdir $(OBJDIR)libavutil
	cd $(OBJDIR)libavutil;$(AR) x $(FFMPEGDIR)libavutil/libavutil.a

.PHONY: ffmpegfiles

cleanffmpegfiles:
	-rm $(OBJDIR)libavformat/*.o
	-rm $(OBJDIR)libavformat/__.SYMDEF\ SORTED
	-rm $(OBJDIR)libavcodec/*.o
	-rm $(OBJDIR)libavcodec/__.SYMDEF\ SORTED
	-rm $(OBJDIR)libavutil/*.o
	-rm $(OBJDIR)libavutil/__.SYMDEF\ SORTED
	-rm -r $(OBJDIR)libavformat
	-rm -r $(OBJDIR)libavcodec
	-rm -r $(OBJDIR)libavutil
	-rm -r $(OBJDIR)

.PHONY: cleanffmpegfiles

# SDL_ffmpeg object

$(OBJDIR)$(NAME).o: $(SRCDIR)$(NAME).c $(INCDIR)SDL/$(NAME).h
	@echo
	@echo [ $@ ]
	@-mkdir $(OBJDIR)
	$(CC) $(CFLAGS) -DSDL_FFMPEG_LIBRARY -c $< -o $(OBJDIR)$(NAME).o

$(OBJDIR)$(NAME)d.o: $(SRCDIR)$(NAME).c $(INCDIR)SDL/$(NAME).h
	@echo
	@echo [ $@ ]
	@-mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) -DSDL_FFMPEG_LIBRARY -c $< -o $(OBJDIR)$(NAME)d.o

# static library

staticlibrary: $(LIBDIR)lib$(NAME).a

$(LIBDIR)lib$(NAME).a: $(FFMPEGOBJECTS) $(OBJDIR)$(NAME).o
	@echo
	@echo [ $@ ]
	@-mkdir $(LIBDIR)
	$(AR) -rcs $(LIBDIR)lib$(NAME).a $(OBJDIR)$(NAME).o $(FFMPEGOBJECTS)

.PHONY: staticlibrary

cleanstaticlibrary:
	-rm -f $(LIBDIR)lib$(NAME).a
	-rm -r $(LIBDIR)

.PHONY: cleanstaticlibrary

staticlibrarydebug: $(LIBDIR)lib$(NAME)d.a

$(LIBDIR)lib$(NAME)d.a: $(FFMPEGOBJECTS) $(OBJDIR)$(NAME)d.o
	@echo
	@echo [ $@ ]
	@-mkdir $(LIBDIR)
	$(AR) -rcs $(LIBDIR)lib$(NAME)d.a $(OBJDIR)$(NAME)d.o $(FFMPEGOBJECTS)

cleanstaticlibrarydebug:
	-rm -f $(LIBDIR)lib$(NAME)d.a
	-rm -r $(LIBDIR)

.PHONY: cleanstaticlibrarydebug

# dynamic library

dynamiclibrary: $(BINDIR)$(NAME)$(EXTDLL)

$(BINDIR)$(NAME)$(EXTDLL):
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) -DSDL_FFMPEG_LIBRARY $(LDFLAGS) -shared -o $(BINDIR)$(NAME)$(EXTDLL)

.PHONY: dynamiclibrary

cleandynamiclibrary:
	-rm -f $(BINDIR)$(NAME)$(EXTDLL)
	-rm -r $(BINDIR)

.PHONY: cleandynamiclibrary

dynamiclibrarydebug: $(BINDIR)$(NAME)d$(EXTDLL)

$(BINDIR)$(NAME)d$(EXTDLL):
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) -DSDL_FFMPEG_LIBRARY $(LDFLAGS) -shared -o $(BINDIR)$(NAME)d$(EXTDLL)

.PHONY: dynamiclibrarydebug

cleandynamiclibrarydebug:
	-rm -f $(BINDIR)$(NAME)d$(EXTDLL)
	-rm -r $(BINDIR)

.PHONY: cleandynamiclibrarydebug

# example

example: $(BINDIR)example$(EXTEXE)

$(BINDIR)example$(EXTEXE): $(LINKLIBRARY)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(SRCDIR)example.c -l$(LDLIBRARY) $(LDFLAGS) -o $(BINDIR)example$(EXTEXE)

.PHONY: example

cleanexample:
	-rm -f $(BINDIR)example$(EXTEXE)
	-rm -r $(BINDIR)

.PHONY: cleanexample

# audio example

audioexample: $(BINDIR)audioexample$(EXTEXE)

$(BINDIR)audioexample$(EXTEXE): $(LINKLIBRARY)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(SRCDIR)audioexample.c -l$(LDLIBRARY) $(LDFLAGS) -o $(BINDIR)audioexample$(EXTEXE)

.PHONY: audioexample

cleanaudioexample:
	-rm -f $(BINDIR)audioexample$(EXTEXE)
	-rm -r $(BINDIR)

.PHONY: cleanaudioexample

# multichannel example

multichannelexample: $(BINDIR)multichannelexample$(EXTEXE)

$(BINDIR)multichannelexample$(EXTEXE): $(LINKLIBRARY)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(SRCDIR)multichannelexample.c -l$(LDLIBRARY) $(LDFLAGS) -o $(BINDIR)multichannelexample$(EXTEXE)

.PHONY: multichannelexample

cleanmultichannelexample:
	-rm -f $(BINDIR)multichannelexample$(EXTEXE)
	-rm -r $(BINDIR)

.PHONY: cleanmultichannelexample

# clean

.PHONY: clean

clean: cleanexample cleanaudioexample cleanmultichannelexample cleanstaticlibrary cleandynamiclibrary cleanstaticlibrarydebug cleandynamiclibrarydebug cleanffmpegfiles
	-rm -f $(OBJDIR)$(NAME).o
	-rm -f $(OBJDIR)$(NAME)d.o
	-rm -r $(OBJDIR)
	-rm -r $(LIBDIR)
	-rm -r $(BINDIR)
	@echo
	@echo clean done
