.EXPORT_ALL_VARIABLES:

include ./Makefile.cfg


SUB_DIRS = linux djgpp gui games/qengine


SOBJECTS = $(QUEST_O) $(QUEST_SVGA_O) $(QUEST_GAMES_O) $(QUEST_GUI_O) texfull.o
XOBJECTS = $(QUEST_O) $(QUEST_X11_O) $(QUEST_GAMES_O) $(QUEST_GUI_O) texfull_wm.o
DJOBJECTS = $(QUEST_O) $(QUEST_DJ_O) $(QUEST_GAMES_O) $(QUEST_GUI_O) texfull.o


all clean depend dep .depend::
	@for I in $(SUB_DIRS); do \
		echo "Making \`$@' in $$I"; \
		$(MAKE) -C $$I $@; \
	done

sub_dirs::
	@for I in $(SUB_DIRS); do \
		echo "Making \`all' in $$I"; \
		$(MAKE) -C $$I all; \
	done

all:: $(TARGETS)

include ./Makefile.objects

ifeq (yes,$(DO_SVGA))
squest: sub_dirs $(SOBJECTS)
	$(CXX) $(SOBJECTS) -o squest $(SVGA_LDFLAGS) -lm
endif

ifeq (yes,$(DO_X))
xquest: sub_dirs $(XOBJECTS)
	$(CXX) $(XOBJECTS) -o xquest $(X_LDFLAGS) -lm
endif

ifeq (yes,$(DO_DJGPP))
quest.exe: sub_dirs $(DJOBJECTS)
	$(CXX) -v $(DJOBJECTS) -o quest.exe
endif


# Currently not very useful
install:
	cp $(TARGETS) ..


clean:: Makefile
	rm -r -f *.o squest xquest quest.exe .depend

clobber mrproper distclean:: Makefile clean
	rm -r -f *.bak *~ $(TARGETS)

dep depend .depend::
	$(CC) $(CPPFLAGS) -MM *.c >.depend

.PHONY: dep depend clean dummy all sub_dirs

include .depend
