# Unified Makefile for fMSX
# Works on Linux (including Raspberry Pi) and macOS (Darwin)

PROJECT = fmsx

# Standard installation paths
PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share/fmsx

# Detect architecture and OS
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Toolchain
CC      = gcc
CXX     = g++
RM      = rm -f
INSTALL = install

# Directories
EMULIB  = EMULib
LIBZ80  = Z80
FMSX    = fMSX
UNIX    = $(FMSX)/Unix
EMUUNIX = $(EMULIB)/Unix

# Compiler Flags
CFLAGS  = -O2 -std=c99 -pthread -Wall
CFLAGS += -I$(EMULIB) -I$(LIBZ80) -I$(FMSX) -I$(EMUUNIX) -I$(UNIX)

# Base Definitions
DEFINES = -D_GNU_SOURCE -D_DEFAULT_SOURCE -DFMSX -DUNIX -DBPS16 -DZLIB -DCONDEBUG -DDEBUG -DSDL2
DEFINES += -DFMSX_DATA_DIR=\"$(DATADIR)\"

# Endianness Detection (Default to LSB for ARM/x86)
ifeq ($(filter $(UNAME_M), x86_64 i386 i686 arm% aarch64), $(UNAME_M))
    DEFINES += -DLSB_FIRST
else
    DEFINES += -DMSB_FIRST
endif

# Source Files (Base)
SOURCES = \
    $(EMULIB)/EMULib.c \
    $(EMULIB)/Sound.c \
    $(EMULIB)/Image.c \
    $(EMULIB)/Console.c \
    $(EMULIB)/Record.c \
    $(EMULIB)/NetPlay.c \
    $(EMULIB)/Touch.c \
    $(EMULIB)/SHA1.c \
    $(EMULIB)/IPS.c \
    $(EMULIB)/MCF.c \
    $(EMULIB)/Hunt.c \
    $(EMULIB)/I8255.c \
    $(EMULIB)/YM2413.c \
    $(EMULIB)/AY8910.c \
    $(EMULIB)/SCC.c \
    $(EMULIB)/WD1793.c \
    $(EMULIB)/Floppy.c \
    $(EMULIB)/FDIDisk.c \
    $(EMUUNIX)/NetUnix.c \
    $(EMUUNIX)/LibSDL2.c \
    $(EMUUNIX)/SndSDL2.c \
    $(LIBZ80)/Z80.c \
    $(LIBZ80)/ConDebug.c \
    $(FMSX)/fMSX.c \
    $(FMSX)/MSX.c \
    $(FMSX)/V9938.c \
    $(FMSX)/I8251.c \
    $(FMSX)/Patch.c \
    $(FMSX)/Menu.c \
    $(UNIX)/Unix.c

CFLAGS  += $(shell pkg-config --cflags sdl2)
LIBS    += $(shell pkg-config --libs sdl2) -lz

OBJECTS = $(SOURCES:.c=.o)

all: $(PROJECT)

$(PROJECT): $(OBJECTS)
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

install: $(PROJECT)
	$(INSTALL) -d $(BINDIR)
	$(INSTALL) -m 755 $(PROJECT) $(BINDIR)
	$(INSTALL) -d $(DATADIR)
	$(INSTALL) -m 644 BIOS/*.ROM $(DATADIR) || true
	$(INSTALL) -m 644 BIOS/CARTS.* $(DATADIR) || true

uninstall:
	$(RM) $(BINDIR)/$(PROJECT)
	$(RM) -r $(DATADIR)

clean:
	$(RM) $(OBJECTS) $(PROJECT)

.PHONY: all clean install uninstall
