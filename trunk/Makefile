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
RCFLAGS=-s -O2
DCFLAGS=-g
NAME=SDL_ffmpeg
FFMPEGOBJECTS=$(OBJDIR)libavformat/*.o $(OBJDIR)libavcodec/*.o $(OBJDIR)libavutil/*.o

ifeq ($(strip $(STATIC)),)
  LIBDEBUG=dynamiclibrarydebug
  LIBRELEASE=dynamiclibrary
else
  LIBDEBUG=staticlibrarydebug
  LIBRELEASE=staticlibrary
endif

ifeq ($(strip $(FFMPEGDIR)),)
	FFMPEGDIR=\ FFMPEGDIR\ NOT\ SET!\ 
endif

ifeq ($(OS),Linux)
	LDFLAGS+=-lSDL -lSDLmain -lz
	EXTEXE=
	EXTDLL=.so
endif

ifeq ($(findstring MINGW,$(OS)),MINGW)
	LDFLAGS+=-lmingw32 -lSDLmain -lSDL -lwsock32
	CFLAGS=$(BASECFLAGS)
	EXTDLL=.dll
	EXTEXE=.exe
	ifeq ($(strip $(SDLDIR)),)
		SDLDIR=\ SDLDIR\ NOT\ SET!\ 
	endif
endif

ifeq ($(OS),Darwin)
	LDFLAGS=-framework Cocoa -framework SDL -L$(LIBDIR) include/SDL/SDLMain.m -lbz2 -lz
	CFLAGS=-I/Library/Frameworks/SDL.framework/Headers/ $(BASECFLAGS)
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
	-rm -f $(OBJDIR)libavformat/*.o 2>/dev/null
	-rm -f $(OBJDIR)libavformat/__.SYMDEF\ SORTED 2>/dev/null
	-rm -f $(OBJDIR)libavcodec/*.o 2>/dev/null
	-rm -f $(OBJDIR)libavcodec/__.SYMDEF\ SORTED 2>/dev/null
	-rm -f $(OBJDIR)libavutil/*.o 2>/dev/null
	-rm -f $(OBJDIR)libavutil/__.SYMDEF\ SORTED 2>/dev/null
ifeq ($(shell ls -1 $(OBJDIR)libavformat 2>/dev/null | wc -l),0)
	-rm -r $(OBJDIR)libavformat 2>/dev/null
endif
ifeq ($(shell ls -1 $(OBJDIR)libavcodec 2>/dev/null | wc -l),0)
	-rm -r $(OBJDIR)libavcodec 2>/dev/null
endif
ifeq ($(shell ls -1 $(OBJDIR)libavutil 2>/dev/null | wc -l),0)
	-rm -r $(OBJDIR)libavutil 2>/dev/null
endif
ifeq ($(shell ls -1 $(OBJDIR) 2>/dev/null | wc -l),0)
	-rm -r $(OBJDIR) 2>/dev/null
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

staticlibrary: $(LIBDIR)lib$(NAME).a

$(LIBDIR)lib$(NAME).a: $(FFMPEGOBJECTS) $(OBJDIR)$(NAME).o
	@echo
	@echo [ $@ ]
	@-mkdir $(LIBDIR)
	$(AR) -rcs $(LIBDIR)lib$(NAME).a $(OBJDIR)$(NAME).o $(FFMPEGOBJECTS)

.PHONY: staticlibrary

cleanstaticlibrary:
	-rm -f $(LIBDIR)lib$(NAME).a 2>/dev/null
ifeq ($(shell ls -1 $(LIBDIR) 2>/dev/null | wc -l),0)
	-rm -r $(LIBDIR) 2>/dev/null
endif

.PHONY: cleanstaticlibrary

staticlibrarydebug: $(LIBDIR)lib$(NAME)d.a

$(LIBDIR)lib$(NAME)d.a: $(FFMPEGOBJECTS) $(OBJDIR)$(NAME)d.o
	@echo
	@echo [ $@ ]
	@-mkdir $(LIBDIR)
	$(AR) -rcs $(LIBDIR)lib$(NAME)d.a $(OBJDIR)$(NAME)d.o $(FFMPEGOBJECTS)

cleanstaticlibrarydebug:
	-rm -f $(LIBDIR)lib$(NAME)d.a 2>/dev/null
ifeq ($(shell ls -1 $(LIBDIR) 2>/dev/null | wc -l),0)
	-rm -r $(LIBDIR) 2>/dev/null
endif

.PHONY: cleanstaticlibrarydebug

# dynamic library

dynamiclibrary: $(BINDIR)$(NAME)$(EXTDLL)

$(BINDIR)$(NAME)$(EXTDLL): $(OBJDIR)$(NAME).o $(FFMPEGOBJECTS)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	@-mkdir $(LIBDIR)
	$(CC) -shared -o $(BINDIR)$(NAME)$(EXTDLL) $(OBJDIR)$(NAME).o $(FFMPEGOBJECTS) $(CFLAGS) $(RCFLAGS) -DSDL_FFMPEG_LIBRARY $(LDFLAGS) -Wl,--output-def,$(LIBDIR)$(NAME).def,--out-implib,$(LIBDIR)lib$(NAME).a

.PHONY: dynamiclibrary

cleandynamiclibrary:
	-rm -f $(BINDIR)$(NAME)$(EXTDLL) 2>/dev/null
	-rm -f $(LIBDIR)lib$(NAME).a 2>/dev/null
	-rm -f $(LIBDIR)$(NAME).def 2>/dev/null
ifeq ($(shell ls -1 $(BINDIR) 2>/dev/null | wc -l),0)
	-rm -r $(BINDIR) 2>/dev/null
endif
ifeq ($(shell ls -1 $(LIBDIR) 2>/dev/null | wc -l),0)
	-rm -r $(LIBDIR) 2>/dev/null
endif

.PHONY: cleandynamiclibrary

dynamiclibrarydebug: $(BINDIR)$(NAME)d$(EXTDLL) $(OBJDIR)$(NAME)d.o

$(BINDIR)$(NAME)d$(EXTDLL): $(OBJDIR)$(NAME)d.o $(FFMPEGOBJECTS)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	@-mkdir $(LIBDIR)
	$(CC) -shared -o $(BINDIR)$(NAME)d$(EXTDLL) $(OBJDIR)$(NAME)d.o $(FFMPEGOBJECTS) $(CFLAGS) $(DCFLAGS) -DSDL_FFMPEG_LIBRARY $(LDFLAGS) -Wl,--output-def,$(LIBDIR)$(NAME)d.def,--out-implib,$(LIBDIR)lib$(NAME)d.a

.PHONY: dynamiclibrarydebug

cleandynamiclibrarydebug:
	-rm -f $(BINDIR)$(NAME)d$(EXTDLL) 2>/dev/null
	-rm -f $(LIBDIR)lib$(NAME)d.a 2>/dev/null
	-rm -f $(LIBDIR)$(NAME)d.def 2>/dev/null
ifeq ($(shell ls -1 $(LIBDIR) 2>/dev/null | wc -l),0)
	-rm -r $(LIBDIR) 2>/dev/null
endif
ifeq ($(shell ls -1 $(BINDIR) 2>/dev/null | wc -l),0)
	-rm -r $(BINDIR) 2>/dev/null
endif

.PHONY: cleandynamiclibrarydebug

# example

example: $(BINDIR)example$(EXTEXE)

$(BINDIR)example$(EXTEXE): $(LIBDEBUG)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) $(SRCDIR)example.c -l$(NAME)d $(LDFLAGS) -o $(BINDIR)example$(EXTEXE)

.PHONY: example

cleanexample:
	-rm -f $(BINDIR)example$(EXTEXE) 2>/dev/null
ifeq ($(shell ls -1 $(BINDIR) 2>/dev/null | wc -l),0)
	-rm -r $(BINDIR) 2>/dev/null
endif

.PHONY: cleanexample

# audio example

audioexample: $(BINDIR)audioexample$(EXTEXE)

$(BINDIR)audioexample$(EXTEXE): $(LIBDEBUG)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) $(SRCDIR)audioexample.c -l$(NAME)d $(LDFLAGS) -o $(BINDIR)audioexample$(EXTEXE)

.PHONY: audioexample

cleanaudioexample:
	-rm -f $(BINDIR)audioexample$(EXTEXE) 2>/dev/null
ifeq ($(shell ls -1 $(BINDIR) 2>/dev/null | wc -l),0)
	-rm -r $(BINDIR) 2>/dev/null
endif

.PHONY: cleanaudioexample

# multichannel example

multichannelexample: $(BINDIR)multichannelexample$(EXTEXE)

$(BINDIR)multichannelexample$(EXTEXE): $(LIBDEBUG)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(DCFLAGS) $(SRCDIR)multichannelexample.c -l$(NAME)d $(LDFLAGS) -o $(BINDIR)multichannelexample$(EXTEXE)

.PHONY: multichannelexample

cleanmultichannelexample:
	-rm -f $(BINDIR)multichannelexample$(EXTEXE) 2>/dev/null
ifeq ($(shell ls -1 $(BINDIR) 2>/dev/null | wc -l),0)
	-rm -r $(BINDIR) 2>/dev/null
endif

.PHONY: cleanmultichannelexample

# clean

.PHONY: clean

clean: cleanexample cleanaudioexample cleanmultichannelexample cleanstaticlibrary cleandynamiclibrary cleanstaticlibrarydebug cleandynamiclibrarydebug cleanffmpegfiles
	-rm -f $(OBJDIR)$(NAME).o 2>/dev/null
	-rm -f $(OBJDIR)$(NAME)d.o 2>/dev/null
ifeq ($(shell ls -1 $(OBJDIR) 2>/dev/null | wc -l),0)
	-rm -r $(OBJDIR) 2>/dev/null
endif
ifeq ($(shell ls -1 $(LIBDIR) 2>/dev/null | wc -l),0)
	-rm -r $(LIBDIR) 2>/dev/null
endif
ifeq ($(shell ls -1 $(BINDIR) 2>/dev/null | wc -l),0)
	-rm -r $(BINDIR) 2>/dev/null
endif
	@echo
	@echo clean done
