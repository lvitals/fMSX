cmake_minimum_required(VERSION 3.10)
project(fMSX C)

# Set C standard and compiler flags
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pthread")

# Base directory definitions
set(BASEDIR ${CMAKE_SOURCE_DIR})
set(EMULIB ${BASEDIR}/EMULib)
set(LIBZ80 ${BASEDIR}/Z80)

# Include directories
include_directories(
    ${BASEDIR}
    ${EMULIB}
    ${EMULIB}/Unix
    ${LIBZ80}
    ${BASEDIR}/fMSX
    ${BASEDIR}/fMSX/Unix
)

# Detect platform and set includes/libs accordingly
if(APPLE)
    include_directories(/opt/homebrew/include /opt/homebrew/include/pulse /opt/X11/include)
    link_directories(/opt/homebrew/lib /opt/X11/lib)
    set(PLATFORM_LIBS X11 Xext pulse pulse-simple)
else()
    include_directories(/usr/X11R6/include)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PULSE REQUIRED libpulse-simple)
    include_directories(${PULSE_INCLUDE_DIRS})
    link_directories(${PULSE_LIBRARY_DIRS})
    set(PLATFORM_LIBS ${PULSE_LIBRARIES} X11 Xext)
endif()

# Definitions
add_definitions(
    -DFMSX
    -DLSB_FIRST
    -DCONDEBUG
    -DDEBUG
    -DUNIX
    -DMITSHM
    -DBPS16
    -DPULSE_AUDIO
    -DZLIB
)

# Sources from EMULib
set(EMULIB_SOURCES
    ${EMULIB}/EMULib.c
    ${EMULIB}/Sound.c
    ${EMULIB}/Image.c
    ${EMULIB}/Console.c
    ${EMULIB}/Record.c
    ${EMULIB}/NetPlay.c
    ${EMULIB}/Touch.c
    ${EMULIB}/SHA1.c
    ${EMULIB}/IPS.c
    ${EMULIB}/MCF.c
    ${EMULIB}/Hunt.c
    ${EMULIB}/I8255.c
    ${EMULIB}/YM2413.c
    ${EMULIB}/AY8910.c
    ${EMULIB}/SCC.c
    ${EMULIB}/WD1793.c
    ${EMULIB}/Floppy.c
    ${EMULIB}/FDIDisk.c
    ${EMULIB}/Unix/LibUnix.c
    ${EMULIB}/Unix/SndUnix.c
    ${EMULIB}/Unix/NetUnix.c
)

# CPU Z80
set(Z80_SOURCES
    ${LIBZ80}/Z80.c
    ${LIBZ80}/ConDebug.c
)

# Core fMSX emulator sources
set(FMSX_SOURCES
    ${BASEDIR}/fMSX/fMSX.c
    ${BASEDIR}/fMSX/MSX.c
    ${BASEDIR}/fMSX/V9938.c
    ${BASEDIR}/fMSX/I8251.c
    ${BASEDIR}/fMSX/Patch.c
    ${BASEDIR}/fMSX/Menu.c
    ${BASEDIR}/fMSX/Unix/Unix.c
)

# Find and link ZLIB
find_package(ZLIB REQUIRED)

# Executable
add_executable(fmsx 
    ${EMULIB_SOURCES}
    ${Z80_SOURCES}
    ${FMSX_SOURCES}
)

# Link with required libraries
target_link_libraries(fmsx 
    ${ZLIB_LIBRARIES}
    ${PLATFORM_LIBS} 
    pthread
)