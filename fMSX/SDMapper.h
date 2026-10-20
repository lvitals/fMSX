/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                        SDMapper.h                       **/
/**                                                         **/
/** The SD Mapper cartridge of Fabio Belavenuto (msxsdmapper) **/
/** with its mapper switched off: a flash in ASCII16 pages  **/
/** and an SPI port to two SD cards, both in the cartridge  **/
/** slot SDMSlot:                                           **/
/**   6000h     (write) flash page of 4000h-7FFFh           **/
/**   6001h     (write) bit 0: 1 = SPI port on              **/
/**   4000h-47FFh       SPI data: a read gives the byte of   **/
/**                     the previous transfer and starts one **/
/**                     sending FFh; a write sends the byte  **/
/**   4800h     (read)  b0,b1: 0 = card 0/1 inserted;        **/
/**                     b2,b3: 0 = card 0/1 write protected  **/
/**             (write) b0,b1: chip select of card 0/1 (0 on) **/
/** The cards are SDHC (block addressing) on image files.   **/
/** With SDMRam (its RAM switch on, in Mapper mode) the slot **/
/** is expanded: the flash and the SPI port in subslot 1,   **/
/** a 512kB memory mapper (ports FCh-FFh, page n = segment n **/
/** after a reset) in subslot 3, as in its CPLD.            **/
/** With SDMV2 it is the SD Mapper v2 (msxsdmapperv2):      **/
/**   6000h-67FFh (write) flash page of 4000h-7FFFh, 0-7    **/
/**   7B00h-7EFFh SPI data, with page 7 in 4000h-7FFFh only **/
/**   7FF0h     (write) b0,b1: 1 = card 1/2 selected        **/
/**             (read)  no card selected: b0 = RAM switch;  **/
/**                     one: b1 = 0 inserted, b2 = 1 write  **/
/**                     protected, b0 = 1 changed           **/
/**   7FF1h     timer (here always 0, expired)              **/
/** and with SDMRam the flash is in subslot 0, the mapper   **/
/** in subslot 1.                                           **/
/** The flash takes the JEDEC commands (AAh at 5555h, 55h   **/
/** at 2AAAh): ID (90h), program (A0h), chip (80h, 10h) and **/
/** sector erase (80h, 30h), reset (F0h); written, it goes  **/
/** back to its file on exit.  v1 (AM29F010, 16kB sectors): **/
/** port 5Fh b7 = 1 sends the reads and writes of pages 1-2 **/
/** to the flash page b2-b0.  v2 (SST39SF020, 4kB sectors): **/
/** 7000h-77FFh is the page of 8000h-BFFFh, b3 = 1 the flash **/
/** there, for reads and writes.                            **/
/** With SDMMega (and SDMRam), as clones of the v1 sold as  **/
/** "1 MB": a MegaRAM of 512kB in subslot 2 as well, 8kB    **/
/** banks at 4000h-BFFFh; after OUT (8Eh) the writes go to  **/
/** its RAM, after IN (8Eh) they choose the bank of their   **/
/** 8kB.                                                    **/
/*************************************************************/
#ifndef SDMAPPER_H
#define SDMAPPER_H

#include "Z80.h"

extern int SDMSlot;         /* Primary slot, -1 for none     */
extern int SDMSpi;          /* 1: the SPI port is on         */
extern int SDMRam;          /* 1: the mapper and expander on */
extern int SDMV2;           /* 1: the SD Mapper v2 (-sdmv2)  */
extern int SDMMega;         /* 1: a MegaRAM too (-sdmmega)   */
extern int SDMLate;         /* 1: its RAM shows up only once */
                            /* its header is read (-sdmlate) */

#define SDMSUB     (SDMRam&&!SDMV2? 1:0) /* Subslot of the flash */
#define SDMRAMSUB  (SDMV2? 1:3)  /* Subslot of the mapper     */
#define SDMRAMSEGS 32            /* 16kB segments of it       */
#define SDMMEGASUB 2             /* Subslot of the MegaRAM    */
#define SDMMEGABANKS 64          /* 8kB banks of it           */

int  SDMCard(int N,const char *FileName); /* Card N, 0 = next */
void SDMReset(void);                /* Reset of the machine  */
int  SDMPort(word A);               /* 1: A reads the SPI port   */
int  SDMFlashAt(word A);            /* 1: A reads the flash chip */
byte SDMFlashRd(word A);            /* Read of the flash chip    */
int  SDMFlashWr(word A,byte V);     /* Write, pages 1-2: 1 taken */
void SDMOut5F(byte V);              /* v1: flash write port 5Fh  */
void SDMTrash(void);                /* Save a written flash      */
byte SDMRead(word A);               /* Read of the SPI port      */
int  SDMWrite(word A,byte V);       /* Write in page 1: 1 if taken */

#endif /* SDMAPPER_H */
