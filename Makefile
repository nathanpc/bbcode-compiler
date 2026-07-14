# Environment
PLATFORM := $(shell uname -s)

# Paths
BUILDDIR := build
SOURCES  := utils.c atom.c
OBJECTS  := $(patsubst %.c, $(BUILDDIR)/%.o, $(SOURCES))
TARGETS  := $(OBJECTS) $(BUILDDIR)/bbcode

# Tools
CC    = gcc
GDB   = gdb --args
RM    = rm -f
MKDIR = mkdir -p
TOUCH = touch

# Handle OS X-specific tools.
ifeq ($(PLATFORM), Darwin)
	CC  = clang
	GDB = lldb
endif

# Flags
CFLAGS  = -Wall -Wno-psabi --std=gnu89

.PHONY: all compile debug memcheck clean

all: compile

$(BUILDDIR)/stamp:
	$(MKDIR) $(@D)
	$(TOUCH) $@

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/bbcode: $(OBJECTS) $(BUILDDIR)/bbcode.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

compile: $(BUILDDIR)/stamp $(TARGETS)

debug: CFLAGS += -g3 -D_DEBUG
debug: compile

memcheck: CFLAGS += -g3 -D_DEBUG -DMEMCHECK
memcheck: compile

clean:
	$(RM) -r $(BUILDDIR)
