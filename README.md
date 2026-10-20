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
    and the ROMs fMSX adds)

```bash
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./fmsx -msx1 -nosound -nosync \
    -frames 1200 -keys '600:p,610:r,620:i,630:n,640:t,650:SPACE,660:1,670:RETURN' \
    -dumpscreen screen.txt
```

Debugging an alefc program (its `.elf` from `alefc -g`, beside the ROM) with
GDB:

```bash
./fmsx -msx1 -debugger gdb program.rom
```

In an MSXgl-alef project built with `DEBUG=1`, `make debug` does it.

### Disk interfaces (SD Mapper, Sunrise IDE)

Two cartridges with hard disks on image files, in a cartridge slot (1 or
2):

*   `-sdmapper <slot>` - The SD Mapper of Fabio Belavenuto (msxsdmapper,
    version 1): the SPI port at 4000h-48FFh (switched on by bit 0 of
    6001h) and two SD cards.  Its flash is a cartridge in that slot, loaded
    as usual (`-rom 5 file`, ASCII16); without one it reads FFh.
*   `-sdmram` - Its RAM switch on, in Mapper mode: as in its CPLD, the slot
    is expanded, the flash and the SPI port in subslot 1 and a memory
    mapper of 512 KB (ports FCh-FFh, page n = segment n after a reset) in
    subslot 3; 4800h reads both switches on.
*   `-sdmlate` - (with `-sdmram`) The mapper shows up only when the BIOS
    reads the header of the flash, after it has chosen the RAM of the
    machine: as on an MSX1 whose own RAM comes first in the slot order.
*   `-sdcard <image>` - The next free SD card, 1 then 2 (SDHC, block
    addressed; the size is rounded down to 512 KB).
*   `-sdcard1 <image>`, `-sdcard2 <image>` - The SD card of that slot of
    the cartridge; the other one may stay empty.  `SDM_TRACE=1` prints the commands
    the cards get on the standard error.
*   `-sunrise <slot>` - The Sunrise IDE, as in openMSX: the flash in banks
    of 16 KB at 4000h-7FFFh (bits 7-3 of 4104h, in reverse order, choose
    the bank; after a reset, the last), and with bit 0 of 4104h the ATA
    registers at 7C00h-7FFFh.
*   `-sunriserom <image>` - Its flash, up to 128 KB (a smaller one repeats);
    without one it reads FFh.
*   `-hd <image>` - The next ATA disk, master then slave (LBA and CHS, PIO;
    READ/WRITE SECTORS, IDENTIFY, DIAGNOSTIC).

`-nodiskrom` leaves the machine with no disk interface (no Disk ROM in
slot 3-1), as an MSX1 of 64 KB that starts only from a cartridge.

The images are opened for writing and may be sparse and larger than 4 GB:

```bash
./fmsx -msx2 -ram 256k -rom 5 sdmapper.rom -sdmapper 1 -sdcard card1.img -sdcard card2.img
./fmsx -msx2 -ram 256k -rom 5 sdmapper.rom -sdmapper 1 -sdcard1 system.img -sdcard2 data.img
./fmsx -msx2 -ram 256k -sunrise 2 -sunriserom sunrise.rom -hd master.img -hd slave.img
./fmsx -msx1 -ram 64k -nodiskrom -rom 5 sdmapper.rom -sdmapper 1 -sdmram -sdcard card1.img
```

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
