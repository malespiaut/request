all: $(notdir $(QUEST_GAMES_O))

CPPFLAGS += -I../../

clean: Makefile
	rm -r -f *.o .depend

dep depend .depend:
	$(CC) $(CPPFLAGS) -MM *.c *.cc >.depend


include .depend
