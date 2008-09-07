# SDL_ffmpeg - Arjan Houben - 2007

include Makefile.include

all: library dynamiclibrary example audioexample multichannelexample
	@echo 
	@echo all done!
	@echo 

.PHONY: all


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

cleandynamiclibrary:
	-rm -f $(LIBDIR)$(DYNLIBPREFIX)$(NAME)$(DEBUGNAME)$(DYNLIBEXTENSION)
ifeq ($(shell find $(LIBDIR) -type f | wc -l),0)
	-rm -r $(LIBDIR)
endif

.PHONY: cleandynamiclibrary


# example

example: $(BINDIR)example$(BINEXTENSION)

$(BINDIR)example$(BINEXTENSION): $(LIBRARY)
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(ESOURCE) $(SRCDIR)example.c -l$(NAME)$(DEBUGNAME) $(LDFLAGS) -o $@

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

$(BINDIR)audioexample$(BINEXTENSION): $(LIBRARY)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(ESOURCE) $(SRCDIR)audioexample.c -l$(NAME)$(DEBUGNAME) $(LDFLAGS) -o $@

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

$(BINDIR)multichannelexample$(BINEXTENSION): $(LIBRARY)
	@echo
	@echo [ $@ ]
	@-mkdir $(BINDIR)
	$(CC) $(CFLAGS) $(ESOURCE) $(SRCDIR)multichannelexample.c -l$(NAME)$(DEBUGNAME) $(LDFLAGS) -o $@

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

distclean: clean
	-rm -rf obj/

.PHONY: distclean
