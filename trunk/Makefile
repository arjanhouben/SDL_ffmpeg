# SDL_ffmpeg - Arjan Houben - 2007

include Makefile.include

all: $(LIBRARY) example audioexample multichannelexample
	@echo 
	@echo all done!
	@echo 

.PHONY: all

Makefile.include:
	@echo
	@echo "Please run './configure' script before trying to build the library"
	@echo
	@exit -1


# SDL_ffmpeg object

$(OBJDIR)$(NAME)$(DEBUGNAME).o: $(SRCDIR)$(NAME).c $(INCDIR)SDL/$(NAME).h
	@echo 
	@echo [ $@ ]
	@-mkdir $(OBJDIR)
	$(CC) $(CFLAGS) -DSDL_FFMPEG_LIBRARY -c $(SRCDIR)$(NAME).c -o $@


# staticlibrary

library: $(LIBDIR)$(LIBPREFIX)$(NAME)$(DEBUGNAME)$(LIBEXTENSION)

$(LIBDIR)$(LIBPREFIX)$(NAME)$(DEBUGNAME)$(LIBEXTENSION): $(FFMPEGOBJECTS) $(OBJDIR)$(NAME)$(DEBUGNAME).o
	@echo
	@echo [ $@ ]
	@-mkdir $(LIBDIR)
	$(AR) -rcs $@ $(OBJDIR)$(NAME)$(DEBUGNAME).o $(FFMPEGOBJECTS)

.PHONY: library

cleanlibrary:
	-rm -f $(LIBDIR)$(LIBPREFIX)$(NAME)$(DEBUGNAME)$(LIBEXTENSION)
ifeq ($(shell find $(LIBDIR) -type f | wc -l),0)
	-rm -r $(LIBDIR)
endif

.PHONY: cleanlibrary


# dynamiclibrary

dynamiclibrary: $(LIBDIR)$(DYNLIBPREFIX)$(NAME)$(DEBUGNAME)$(DYNLIBEXTENSION)

$(LIBDIR)$(DYNLIBPREFIX)$(NAME)$(DEBUGNAME)$(DYNLIBEXTENSION): $(FFMPEGOBJECTS) $(OBJDIR)$(NAME)$(DEBUGNAME).o
	@echo
	@echo [ $@ ]
	@-mkdir $(LIBDIR)
	$(CC) $(LIBFLAGS) -o $@ $(OBJDIR)$(NAME)$(DEBUGNAME).o $(FFMPEGOBJECTS) $(CFLAGS) -DSDL_FFMPEG_LIBRARY $(LDFLAGS)
	# dll's should be moved to the bin dir
ifeq ($(DYNLIBEXTENSION),.dll)
	@-mkdir $(BINDIR)
	mv $@ $(BINDIR)
endif

cleandynamiclibrary:
	-rm -f $(LIBDIR)$(DYNLIBPREFIX)$(NAME)$(DEBUGNAME)$(DYNLIBEXTENSION)
	-rm -f $(LIBDIR)$(DYNLIBPREFIX)$(NAME)$(DEBUGNAME).def
ifeq ($(DYNLIBEXTENSION),.dll)
	-rm -f $(BINDIR)$(DYNLIBPREFIX)$(NAME)$(DEBUGNAME)$(DYNLIBEXTENSION)
endif
ifeq ($(shell find $(LIBDIR) -type f | wc -l),0)
	-rm -r $(LIBDIR)
endif

.PHONY: cleandynamiclibrary


# example

example: $(BINDIR)example$(BINEXTENSION)

$(BINDIR)example$(BINEXTENSION): $(LIBRARY) $(SRCDIR)example.c
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(ESOURCE) $(SRCDIR)example.c -l$(LIBNAME) $(LDFLAGS) -o $@

.PHONY: example

cleanexample:
	-rm -f $(BINDIR)example$(BINEXTENSION)
	-rm -rf $(BINDIR)example.dSYM
ifeq ($(shell find $(BINDIR) -type f | wc -l),0)
	-rm -r $(BINDIR)
endif

.PHONY: cleanexample




# audio example

audioexample: $(BINDIR)audioexample$(BINEXTENSION)

$(BINDIR)audioexample$(BINEXTENSION): $(LIBRARY) $(SRCDIR)audioexample.c
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(ESOURCE) $(SRCDIR)audioexample.c -l$(LIBNAME) $(LDFLAGS) -o $@

.PHONY: audioexample

cleanaudioexample:
	-rm -f $(BINDIR)audioexample$(BINEXTENSION)
	-rm -rf $(BINDIR)audioexample.dSYM
ifeq ($(shell find $(BINDIR) -type f | wc -l),0)
	-rm -r $(BINDIR)
endif

.PHONY: cleanaudioexample




# multichannel example

multichannelexample: $(BINDIR)multichannelexample$(BINEXTENSION)

$(BINDIR)multichannelexample$(BINEXTENSION): $(LIBRARY) $(SRCDIR)multichannelexample.c
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(ESOURCE) $(SRCDIR)multichannelexample.c -l$(LIBNAME) $(LDFLAGS) -o $@

.PHONY: multichannelexample

cleanmultichannelexample:
	-rm -f $(BINDIR)multichannelexample$(BINEXTENSION)
	-rm -rf $(BINDIR)multichannelexample.dSYM
ifeq ($(shell find $(BINDIR) -type f | wc -l),0)
	-rm -r $(BINDIR)
endif

.PHONY: cleanmultichannelexample



# install

install: $(LIBRARY)
	@echo aap
	cp $(LIBRARY) /usr/lib/
	cp $(INCDIR) /usr/lib/

.PHONY: install


# clean

clean: cleanexample cleanaudioexample cleanmultichannelexample cleanlibrary cleandynamiclibrary
	-rm -f $(OBJDIR)$(NAME)$(DEBUGNAME).o
ifeq ($(shell find $(OBJDIR) -type f | wc -l),0)
	-rm -r $(OBJDIR)
endif
ifeq ($(shell find $(LIBDIR) -type f | wc -l),0)
	-rm -r $(LIBDIR)
endif
ifeq ($(shell find $(BINDIR) -type f | wc -l),0)
	-rm -r $(BINDIR)
endif
	@echo 
	@echo clean done
	@echo

.PHONY: clean

# distclean

distclean:
	-rm -rf obj/
	-rm Makefile.include

.PHONY: distclean
