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

# Set USE_SDL2=1 to use SDL2 instead of X11
USE_SDL2 ?= 1

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
DEFINES = -D_GNU_SOURCE -D_DEFAULT_SOURCE -DFMSX -DUNIX -DBPS16 -DZLIB -DCONDEBUG -DDEBUG
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
    $(LIBZ80)/Z80.c \
    $(LIBZ80)/ConDebug.c \
    $(FMSX)/fMSX.c \
    $(FMSX)/MSX.c \
    $(FMSX)/V9938.c \
    $(FMSX)/I8251.c \
    $(FMSX)/Patch.c \
    $(FMSX)/Menu.c \
    $(UNIX)/Unix.c

ifeq ($(USE_SDL2), 1)
    DEFINES += -DSDL2
    SOURCES += $(EMUUNIX)/LibSDL2.c $(EMUUNIX)/SndSDL2.c
    
    ifeq ($(UNAME_S), Darwin)
        CFLAGS  += $(shell pkg-config --cflags sdl2)
        LIBS    += $(shell pkg-config --libs sdl2) -lz
    else
        CFLAGS  += $(shell pkg-config --cflags sdl2)
        LIBS    += $(shell pkg-config --libs sdl2) -lz
    endif
else
    DEFINES += -DMITSHM -DPULSE_AUDIO
    SOURCES += $(EMUUNIX)/LibUnix.c $(EMUUNIX)/SndUnix.c
    
    ifeq ($(UNAME_S), Darwin)
        # macOS with Homebrew/XQuartz
        CFLAGS  += -I/opt/homebrew/include -I/opt/X11/include
        LDFLAGS += -L/opt/homebrew/lib -L/opt/X11/lib
        LIBS    += -lX11 -lXext -lpulse-simple -lpulse -lz
    else
        # Linux (Arch, Debian, etc.)
        PKG_CONFIG_DEPS = x11 xext libpulse-simple zlib
        PKG_EXISTS := $(shell pkg-config --exists $(PKG_CONFIG_DEPS) && echo yes)
        
        ifeq ($(PKG_EXISTS), yes)
            CFLAGS  += $(shell pkg-config --cflags $(PKG_CONFIG_DEPS))
            LIBS    += $(shell pkg-config --libs $(PKG_CONFIG_DEPS))
        else
            CFLAGS  += -I/usr/include
            LIBS    += -lX11 -lXext -lpulse-simple -lz
        endif
    endif
endif

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
