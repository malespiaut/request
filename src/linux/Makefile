all: $(notdir $(QUEST_SVGA_O) $(QUEST_X11_O))

CPPFLAGS += -I../

clean: Makefile
	rm -r -f *.o .depend

dep depend .depend:
	$(CC) $(CPPFLAGS) -I../ -MM *.c >.depend


include .depend
