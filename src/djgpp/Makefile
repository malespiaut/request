all: $(notdir $(QUEST_DJGPP_O))

CPPFLAGS += -I../

clean: Makefile
	rm -r -f *.o .depend

dep depend .depend:
	$(CC) $(CPPFLAGS) -I../ -MM *.c >.depend


include .depend
