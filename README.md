# fMSX - MSX Home Computer Emulator

![fMSX Logo](https://fms.komkon.org/fMSX/fMSX.png)

## Description

fMSX is a portable MSX, MSX2, and MSX2+ home computer emulator developed by Marat Fayzullin since 1993. It runs most MSX software and emulates popular hardware extensions like SCC and OPLL.

**Key Features:**
- Supports MSX, MSX2, and MSX2+ systems
- Cross-platform (Windows, Unix, Linux, Mac, Android, etc.)
- Disk and tape emulation
- Multiple display scaling options
- Save/load emulation states
- Built-in debugger

## Installation

### macOS Requirements

1. Install PulseAudio for audio support:

   ```bash
   brew install pulseaudio
   brew services start pulseaudio
   ```

2.  Install build dependencies:

    ```bash
    brew install cmake
    ```
    

### Building from Source

```bash
git clone https://github.com/lvitals/fMSX.git
cd fMSX
mkdir build
cd build
cmake ..
make
```

## Running fMSX

Basic usage:

```bash
./fmsx [-options] [cartridge1] [cartridge2]
```

Example:

```bash
./fmsx -msx2 -diska DSK/MSXDOS23.DSK -home ROMS
```
## System Requirements

*   ROM files (must be placed in ROMS directory):
    *   MSX.ROM (required)
    *   MSX2.ROM (for MSX2 support)
    *   DISK.ROM (for disk support)
    *   Other optional ROMs (see below)
        

## Directory Structure


```
.
├── CMakeLists.txt         # Build configuration
├── DSK/                   # Disk images
├── EMULib/                # Emulation library core
├── fMSX/                  # Main emulator code
├── ROMS/                  # System ROM files (must provide)
└── Z80/                   # Z80 CPU emulation
```

## Required ROM Files

Place these in the `ROMS/` directory:

*   `MSX.ROM` - Standard MSX BIOS (required)
*   `MSX2.ROM` - For MSX2 support
*   `DISK.ROM` - For disk support
*   `MSX2EXT.ROM` - MSX2 extensions
*   `CMOS.ROM` - Non-volatile memory
    

## Keyboard Controls

| Key Combination | Function |
| --- | --- |
| F6  | Load state |
| F7  | Save state |
| F8  | Rewind emulation |
| F9  | Fast-forward |
| F10 | Configuration menu |
| F11 | Reset |
| F12 | Quit |
| Ctrl+F8 | Toggle scanlines |
| Alt+F8 | Toggle screen softening |
| Ctrl+F10 | Debugger |

## Command Line Options

Common options:

*   `-msx1`/`-msx2`/`-msx2+` - Select MSX model
*   `-diska <file>` - Set disk image for drive A:
*   `-diskb <file>` - Set disk image for drive B:
*   `-tape <file>` - Set tape image
*   `-sound <Hz>` - Set sound quality (44100 recommended)
*   `-nosound` - Disable sound
    

Full list available by running `fmsx -help`

### Headless runs

For automated tests, with SDL's dummy drivers no window opens:

*   `-frames <count>` - Quit after `<count>` frames
*   `-keys <script>` - Press keys at given frames: `frame:KEY[*hold]`,
    comma-separated. `KEY` is a character or a name (`ESC`, `SPACE`,
    `RETURN`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `SHIFT`, `CTRL`, `F1`..`F5`,
    ...); it is held 4 frames unless `*hold` says otherwise
*   `-dumpscreen <file>` - At exit, write the screen mode, then the text
    of SCREEN 0/1 (printable ASCII; any other byte as `.`)
*   `-dumpvram <file>` - At exit, write the VRAM
*   `-debugger <command>` - Start a debugger on the program: fMSX runs
    `<command> -ex 'target remote /dev/pts/N' [options] program.elf` on
    the terminal, the protocol on a pseudo-terminal of its own (gdb's
    serial line; no network), the `.elf` beside the ROM when there is one.
    Ctrl-C stops the program through the debugger; fMSX waits for the
    debugger to quit before it exits
*   `-gdb -` - Serve the protocol on the standard input and output, for a
    debugger that starts fMSX (`target remote | fmsx ... -gdb -
    program.rom`); fMSX's messages go to stderr, and it quits when the
    debugger closes the pipe. Both serve gdb's remote protocol (target
    `z80`) from the first instruction: registers, the memory the CPU sees,
    breakpoints (kept by the emulator, the code may be in ROM), step,
    continue, Ctrl-C; the end of the emulation (`-frames`) reaches the
    debugger as the program's exit. A breakpoint
    stops only the program's code: a cartridge in slot 1 or 2, or the RAM
    (the same address is other code in the BIOS, the SUB-ROM, the disk ROM
    and the ROMs fMSX adds). An extension a gdb never asks for,
    `qAlefMap` (announced as `qAlefMap+`), answers which part of the ROM
    image is at each 8kB page: alef-db's segments of a mapped ROM

```bash
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./fmsx -msx1 -nosound -nosync \
    -frames 1200 -keys '600:p,610:r,620:i,630:n,640:t,650:SPACE,660:1,670:RETURN' \
    -dumpscreen screen.txt
```

Debugging an alefc program (its `.elf` from `alefc -g`, beside the ROM) with
alef-db:

```bash
./fmsx -msx1 -debugger alef-db program.rom
```

In an MSXgl-alef project built with `DEBUG=1`, `make debug` does it.

## Disk Usage

1.  Create disk images using included tools:
    
    ```bash
    ./wrdsk image.DSK file1 file2...
    ```
2.  Mount in emulator:
    
    ```bash
    ./fmsx -diska image.DSK
    ```

## Troubleshooting

**No sound on macOS:**

```bash
brew services restart pulseaudio
```

**ROM loading issues:**

*   Verify required ROM files are in ROMS/ directory    
*   Try different mapper types with `-rom <N>`
    

**BASIC programs not working:**

1.  Press Ctrl+Del at boot to disable second disk drive
2.  In BASIC: `POKE &hFFFF,&hAA`
    

## License

fMSX source code is open for viewing but not public domain. Commercial use requires permission from the author (Marat Fayzullin).

## Resources

*   Official site: [http://fms.komkon.org/fMSX/](http://fms.komkon.org/fMSX/)
*   MSX software archive: [http://fms.komkon.org/MSX/](http://fms.komkon.org/MSX/)
