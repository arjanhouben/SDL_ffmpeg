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
LDFLAGS=-L$(LIBDIR) -L$(SDLDIR)lib/
BASECFLAGS=-I$(INCDIR) -I$(SDLDIR)include/ -I$(FFMPEGDIR) -I$(FFMPEGDIR)libavutil -I$(FFMPEGDIR)libavformat -I$(FFMPEGDIR)libavcodec $(ECFLAGS)
CFLAGS=$(BASECFLAGS)
DLLFLAGS=-fPIC -shared
RCFLAGS=-O2
DCFLAGS=-g
NAME=SDL_ffmpeg
FFMPEGOBJECTS=$(OBJDIR)libavformat/*.o $(OBJDIR)libavcodec/*.o $(OBJDIR)libavutil/*.o

ifeq ($(strip $(FFMPEGDIR)),)
	FFMPEGDIR=\ FFMPEGDIR\ NOT\ SET!\ 
endif

ifeq ($(OS),Linux)
	LDFLAGS+=-lSDL -lSDLmain -lz
	EXTEXE=
	EXTDLL=.so
	PREDLL=lib
endif

ifeq ($(findstring MINGW,$(OS)),MINGW)
	LDFLAGS+=-lmingw32 -lSDLmain -lSDL -lwsock32  -Wl,--output-def,$(LIBDIR)$(NAME).def,--out-implib,$(LIBDIR)$(PREDLL)$(NAME).a
	CFLAGS=$(BASECFLAGS)
	EXTEXE=.exe
	EXTDLL=.dll
	PREDLL=
	ifeq ($(strip $(SDLDIR)),)
		SDLDIR=\ SDLDIR\ NOT\ SET!\ 
	endif
endif

ifeq ($(OS),Darwin)
	LDFLAGS=-framework Cocoa -framework SDL -L$(LIBDIR)  -lbz2 -lz
	CFLAGS=-I/Library/Frameworks/SDL.framework/Headers/ $(BASECFLAGS)
	DLLFLAGS=-dynamiclib -mdynamic-no-pic
	ESOURCE=include/SDL/SDLMain.m
	EXTEXE=
	EXTDLL=.dylib
	PREDLL=lib
	STATIC=true
endif

ifeq ($(strip $(STATIC)),)
  LIBDEBUG=dynamiclibrarydebug
  LIBRELEASE=dynamiclibrary
else
  LIBDEBUG=staticlibrarydebug
  LIBRELEASE=staticlibrary
endif

all: $(LIBRELEASE) $(LIBDEBUG) example audioexample multichannelexample
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
	-rm -f $(OBJDIR)libavformat/*.o
	-rm -f $(OBJDIR)libavformat/__.SYMDEF\ SORTED
	-rm -f $(OBJDIR)libavcodec/*.o
	-rm -f $(OBJDIR)libavcodec/__.SYMDEF\ SORTED
	-rm -f $(OBJDIR)libavutil/*.o
	-rm -f $(OBJDIR)libavutil/__.SYMDEF\ SORTED
ifeq ($(shell ls -1 $(OBJDIR)libavformat | wc -l),0)
	-rm -r $(OBJDIR)libavformat
endif
ifeq ($(shell ls -1 $(OBJDIR)libavcodec | wc -l),0)
	-rm -r $(OBJDIR)libavcodec
endif
ifeq ($(shell ls -1 $(OBJDIR)libavutil | wc -l),0)
	-rm -r $(OBJDIR)libavutil
endif
ifeq ($(shell ls -1 $(OBJDIR) | wc -l),0)
	-rm -r $(OBJDIR)
endif

.PHONY: cleanffmpegfiles

# SDL_ffmpeg object

$(OBJDIR)$(NAME).o: $(SRCDIR)$(NAME).c $(INCDIR)SDL/$(NAME).h
	@echo
	@echo [ $@ ]
	@-mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $(RCFLAGS) -DSDL_FFMPEG_LIBRARY -c $< -o $(OBJDIR)$(NAME).o

$(OBJDIR)$(NAME)d.o: $(SRCDIR)$(NAME).c $(INCDIR)SDL/$(NAME).h
	@echo
	@echo [ $@ ]
	@-mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) -DSDL_FFMPEG_LIBRARY -c $< -o $(OBJDIR)$(NAME)d.o




# static library

staticlibrary: $(LIBDIR)$(PREDLL)$(NAME).a

$(LIBDIR)$(PREDLL)$(NAME).a: $(FFMPEGOBJECTS) $(OBJDIR)$(NAME).o
	@echo
	@echo [ $@ ]
	@-mkdir $(LIBDIR)
	$(AR) -rcs $(LIBDIR)$(PREDLL)$(NAME).a $(OBJDIR)$(NAME).o $(FFMPEGOBJECTS)

.PHONY: staticlibrary

cleanstaticlibrary:
	-rm -f $(LIBDIR)$(PREDLL)$(NAME).a
ifeq ($(shell ls -1 $(LIBDIR) | wc -l),0)
	-rm -r $(LIBDIR)
endif

.PHONY: cleanstaticlibrary

staticlibrarydebug: $(LIBDIR)$(PREDLL)$(NAME)d.a

$(LIBDIR)$(PREDLL)$(NAME)d.a: $(FFMPEGOBJECTS) $(OBJDIR)$(NAME)d.o
	@echo
	@echo [ $@ ]
	@-mkdir $(LIBDIR)
	$(AR) -rcs $(LIBDIR)$(PREDLL)$(NAME)d.a $(OBJDIR)$(NAME)d.o $(FFMPEGOBJECTS)

cleanstaticlibrarydebug:
	-rm -f $(LIBDIR)$(PREDLL)$(NAME)d.a
ifeq ($(shell ls -1 $(LIBDIR) | wc -l),0)
	-rm -r $(LIBDIR)
endif

.PHONY: cleanstaticlibrarydebug




# dynamic library

dynamiclibrary: $(LIBDIR)$(PREDLL)$(NAME)$(EXTDLL)

$(LIBDIR)$(PREDLL)$(NAME)$(EXTDLL): $(OBJDIR)$(NAME).o $(FFMPEGOBJECTS)
	@echo
	@echo [ $@ ]
	@-mkdir $(LIBDIR)
	$(CC) $(DLLFLAGS) -o $(LIBDIR)$(PREDLL)$(NAME)$(EXTDLL) $(OBJDIR)$(NAME).o $(FFMPEGOBJECTS) $(CFLAGS) $(RCFLAGS) -DSDL_FFMPEG_LIBRARY $(LDFLAGS)

.PHONY: dynamiclibrary

cleandynamiclibrary:
	-rm -f $(LIBDIR)$(PREDLL)$(NAME)$(EXTDLL)
	-rm -f $(LIBDIR)$(NAME).def
ifeq ($(shell ls -1 $(LIBDIR) | wc -l),0)
	-rm -r $(LIBDIR)
endif

.PHONY: cleandynamiclibrary

dynamiclibrarydebug: $(LIBDIR)$(PREDLL)$(NAME)d$(EXTDLL) $(OBJDIR)$(NAME)d.o

$(LIBDIR)$(PREDLL)$(NAME)d$(EXTDLL): $(OBJDIR)$(NAME)d.o $(FFMPEGOBJECTS)
	@echo
	@echo [ $@ ]
	@-mkdir $(LIBDIR)
	$(CC) $(DLLFLAGS) -o $(LIBDIR)$(PREDLL)$(NAME)d$(EXTDLL) $(OBJDIR)$(NAME)d.o $(FFMPEGOBJECTS) $(CFLAGS) $(DCFLAGS) -DSDL_FFMPEG_LIBRARY $(LDFLAGS) -Wl,--output-def,$(LIBDIR)$(NAME)d.def,--out-implib,$(LIBDIR)$(PREDLL)$(NAME)d.a

.PHONY: dynamiclibrarydebug

cleandynamiclibrarydebug:
	-rm -f $(LIBDIR)$(PREDLL)$(NAME)d$(EXTDLL)
	-rm -f $(LIBDIR)$(NAME)d.def
ifeq ($(shell ls -1 $(LIBDIR) | wc -l),0)
	-rm -r $(LIBDIR)
endif

.PHONY: cleandynamiclibrarydebug




# example

example: $(BINDIR)example$(EXTEXE)

$(BINDIR)example$(EXTEXE): $(LIBDEBUG)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) $(ESOURCE) $(SRCDIR)example.c -l$(NAME)d $(LDFLAGS) -o $(BINDIR)example$(EXTEXE)

.PHONY: example

cleanexample:
	-rm -f $(BINDIR)example$(EXTEXE)
	-rm -rf $(BINDIR)example.dSYM
ifeq ($(shell ls -1 $(BINDIR) | wc -l),0)
	-rm -r $(BINDIR)
endif

.PHONY: cleanexample

# audio example

audioexample: $(BINDIR)audioexample$(EXTEXE)

$(BINDIR)audioexample$(EXTEXE): $(LIBDEBUG)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) $(ESOURCE) $(SRCDIR)audioexample.c -l$(NAME)d $(LDFLAGS) -o $(BINDIR)audioexample$(EXTEXE)

.PHONY: audioexample

cleanaudioexample:
	-rm -f $(BINDIR)audioexample$(EXTEXE)
	-rm -rf $(BINDIR)audioexample.dSYM
ifeq ($(shell ls -1 $(BINDIR) | wc -l),0)
	-rm -r $(BINDIR)
endif

.PHONY: cleanaudioexample

# multichannel example

multichannelexample: $(BINDIR)multichannelexample$(EXTEXE)

$(BINDIR)multichannelexample$(EXTEXE): $(LIBDEBUG)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) $(ESOURCE) $(SRCDIR)multichannelexample.c -l$(NAME)d $(LDFLAGS) -o $(BINDIR)multichannelexample$(EXTEXE)

.PHONY: multichannelexample

cleanmultichannelexample:
	-rm -f $(BINDIR)multichannelexample$(EXTEXE)
	-rm -rf $(BINDIR)multichannelexample.dSYM
ifeq ($(shell ls -1 $(BINDIR) | wc -l),0)
	-rm -r $(BINDIR)
endif

.PHONY: cleanmultichannelexample

# clean

.PHONY: clean

clean: cleanexample cleanaudioexample cleanmultichannelexample cleanstaticlibrary cleandynamiclibrary cleanstaticlibrarydebug cleandynamiclibrarydebug cleanffmpegfiles
	-rm -f $(OBJDIR)$(NAME).o
	-rm -f $(OBJDIR)$(NAME)d.o
ifeq ($(shell ls -1 $(OBJDIR) | wc -l),0)
	-rm -r $(OBJDIR)
endif
ifeq ($(shell ls -1 $(LIBDIR) | wc -l),0)
	-rm -r $(LIBDIR)
endif
ifeq ($(shell ls -1 $(BINDIR) | wc -l),0)
	-rm -r $(BINDIR)
endif
	@echo
	@echo clean done
