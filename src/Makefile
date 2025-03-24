# Makefile for Quest X11 version on Linux

# Compiler settings
CC = gcc
CXX = g++
CFLAGS = -w -DWINDOW_MODE
CXXFLAGS = $(CFLAGS)

# Directory structure
SRCDIR = .
OBJDIR = obj
BINDIR = bin

# Platform-specific directories
LINUX_DIR = linux
GUI_DIR = gui
GAMES_DIR = games/qengine

# Include directories
INCLUDES = -I/usr/X11R6/include -I$(SRCDIR)

# Preprocessor flags
CPPFLAGS = -D_UNIX $(INCLUDES)

# Libraries
LDFLAGS = -L/usr/X11R6/lib -lXext -lX11 -lXpm -ljpeg -lm

# Target executable
TARGET = $(BINDIR)/xquest

# Create directory structure
$(shell mkdir -p $(OBJDIR) $(BINDIR) $(OBJDIR)/$(LINUX_DIR) $(OBJDIR)/$(GUI_DIR) $(OBJDIR)/$(GAMES_DIR))

# Source files
SOURCES = $(wildcard *.c)
CPPSOURCES = $(wildcard *.cc *.cpp)
LINUX_SOURCES = $(wildcard $(LINUX_DIR)/hw_x11.c $(LINUX_DIR)/key_x11.c $(LINUX_DIR)/m_x11.c $(LINUX_DIR)/unixext.c)
GUI_SOURCES = $(wildcard $(GUI_DIR)/*.cc)
GAMES_SOURCES = $(wildcard $(GAMES_DIR)/*.c $(GAMES_DIR)/*.cc)

# Object files
OBJECTS = $(patsubst %.c,$(OBJDIR)/%.o,$(SOURCES))
CPPOBJECTS = $(patsubst %.cc,$(OBJDIR)/%.o,$(CPPSOURCES))
LINUX_OBJECTS = $(patsubst $(LINUX_DIR)/%.c,$(OBJDIR)/$(LINUX_DIR)/%.o,$(LINUX_SOURCES))
GUI_OBJECTS = $(patsubst $(GUI_DIR)/%.cc,$(OBJDIR)/$(GUI_DIR)/%.o,$(GUI_SOURCES))
GAMES_OBJECTS = $(patsubst $(GAMES_DIR)/%.c,$(OBJDIR)/$(GAMES_DIR)/%.o,$(wildcard $(GAMES_DIR)/*.c))
GAMES_CPPOBJECTS = $(patsubst $(GAMES_DIR)/%.cc,$(OBJDIR)/$(GAMES_DIR)/%.o,$(wildcard $(GAMES_DIR)/*.cc))

# All objects
ALL_OBJECTS = $(OBJECTS) $(CPPOBJECTS) $(LINUX_OBJECTS) $(GUI_OBJECTS) $(GAMES_OBJECTS) $(GAMES_CPPOBJECTS)
#$(TEXFULL_WM)

# Default target
all: $(TARGET)

# Rule to build the target
$(TARGET): $(ALL_OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Rule for C source files
$(OBJDIR)/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Rule for C++ source files
$(OBJDIR)/%.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Rule for Linux-specific files
$(OBJDIR)/$(LINUX_DIR)/%.o: $(LINUX_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Rule for GUI files
$(OBJDIR)/$(GUI_DIR)/%.o: $(GUI_DIR)/%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Rule for Games files
$(OBJDIR)/$(GAMES_DIR)/%.o: $(GAMES_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/$(GAMES_DIR)/%.o: $(GAMES_DIR)/%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Install target
install: $(TARGET)
	mkdir -p $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/

# Phony targets
.PHONY: all clean install

# Dependencies
DEPFILE = .depend
$(DEPFILE): $(SOURCES) $(CPPSOURCES) $(LINUX_SOURCES) $(GUI_SOURCES) $(GAMES_SOURCES)
	$(CC) $(CPPFLAGS) -MM $(SOURCES) > $(DEPFILE)
	$(CC) $(CPPFLAGS) -MM $(LINUX_SOURCES) | sed 's/^/$(OBJDIR)\//' >> $(DEPFILE)
	$(CXX) $(CPPFLAGS) -MM $(GUI_SOURCES) | sed 's/^/$(OBJDIR)\//' >> $(DEPFILE)
	$(CC) $(CPPFLAGS) -MM $(GAMES_SOURCES) | sed 's/^/$(OBJDIR)\//' >> $(DEPFILE)

dep: $(DEPFILE)

-include $(DEPFILE)
