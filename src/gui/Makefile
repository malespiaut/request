all: $(notdir $(QUEST_GUI_O))

CPPFLAGS += -I../

clean: Makefile
	rm -r -f *.o .depend

dep depend .depend:
	$(CC) $(CPPFLAGS) -MM *.cc >.depend


include .depend
